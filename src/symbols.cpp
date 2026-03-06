#include "symbols.hpp"
#include <cstdint>
#include <elf.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <istream>
#include <sched.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

std::vector<MappedRegion> read_maps(pid_t pid) {
  // Perhaps a bit messy right now, but good enough

  const std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";

  std::ifstream maps_file(maps_path);

  std::vector<MappedRegion> output;

  std::string line;
  while (std::getline(maps_file, line)) {
    std::istringstream iss(line);

    uint64_t start;
    uint64_t end;
    char dash;
    iss >> std::hex >> start >> dash >> end;

    std::string section;
    std::getline(iss >> std::ws, section, ' ');
    if (section.find('x') == std::string::npos) {
      continue;
    }

    uint64_t offset;
    iss >> std::hex >> offset;

    // Don't care about dev
    std::getline(iss >> std::ws, section, ' ');

    // Don't care about inode
    std::getline(iss >> std::ws, section, ' ');

    std::string path;
    std::getline(iss >> std::ws, path);
    if (path.empty()) {
      // Anonymous
      continue;
    }
    if (path[0] == '[') {
      // Special
      continue;
    }

    // std::cout << "Valid: ";
    output.push_back({.start = start,
                      .end = end,
                      .load_bias = (start - offset),
                      .path = path});
    // output.back().print();
  }

  return output;
}

std::vector<Symbol> MappedRegion::load_symbols() {
  int file_descriptor = open(path.c_str(), O_RDONLY);
  struct stat buf;
  fstat(file_descriptor, &buf);
  size_t file_size = buf.st_size;
  void *base_addr =
      mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
  Elf64_Ehdr *elf_header = static_cast<Elf64_Ehdr *>(base_addr);

  // Find symbol table and load it up
  Elf64_Shdr *sections =
      (Elf64_Shdr *)((uint8_t *)base_addr + elf_header->e_shoff);

  Elf64_Shdr *symbol_table_section = nullptr;
  for (int section_index = 0; section_index < elf_header->e_shnum;
       ++section_index) {
    if (sections[section_index].sh_type == SHT_SYMTAB) {
      symbol_table_section = &sections[section_index];
      break;
    }
    if (symbol_table_section == nullptr &&
        sections[section_index].sh_type == SHT_DYNSYM) {
      // This is the correct section for symbols
      symbol_table_section = &sections[section_index];
    }
  }
  Elf64_Shdr *string_table_section = &sections[symbol_table_section->sh_link];
  const char *string_table =
      (const char *)base_addr + string_table_section->sh_offset;

  std::vector<Symbol> output;

  Elf64_Sym *symbols =
      (Elf64_Sym *)((uint8_t *)base_addr + symbol_table_section->sh_offset);
  int symbol_count =
      symbol_table_section->sh_size / symbol_table_section->sh_entsize;
  for (int symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
    if (ELF64_ST_TYPE(symbols[symbol_index].st_info) != STT_FUNC) {
      continue;
    }
    if (symbols[symbol_index].st_size == 0) {
      continue;
    }
    uint64_t function_start_addr = symbols[symbol_index].st_value;
    uint64_t function_end_addr =
        function_start_addr + symbols[symbol_index].st_size;
    uint64_t possible_load_bias = elf_header->e_type == ET_DYN ? load_bias : 0;
    output.push_back({.start = function_start_addr + possible_load_bias,
                      .size = symbols[symbol_index].st_size,
                      .name = (string_table + symbols[symbol_index].st_name)});
  }

  munmap(base_addr, file_size);
  close(file_descriptor);

  return output;
}

MappedRegion *SymbolTable::lookup_region(uint64_t addr) {
  auto maybe_region = regions.lookup(addr);
  if (maybe_region != nullptr) {
    return maybe_region;
  }
  auto maps = read_maps(pid);
  for (auto &map : maps) {
    if (regions.lookup(map.start) == nullptr) {
      std::cout << "Add mapped region: ";
      map.print();
      regions.insert(map.start, map.end, std::move(map));
    }
  }
  maps.clear();
  return regions.lookup(addr);
}

Symbol SymbolTable::lookup_symbol(uint64_t addr) {
  // Check if we've seen this region before
  auto maybe_region = lookup_region(addr);
  if (maybe_region == nullptr) {
    return Symbol::null(addr);
  }
  auto &region = *maybe_region;
  return region.lookup_symbol(addr);
}
