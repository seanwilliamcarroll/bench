#include "symbols.hpp"
#include <cassert>
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

Symbol SymbolTable::get_symbol(uint64_t addr) {
  auto iter = symbols.find(addr);
  if (iter != symbols.end()) {
    return iter->second;
  }
  // Need to look it up from scratch
  auto output = lookup_symbol(addr);
  symbols[addr] = output;
  return output;
}

Symbol SymbolTable::lookup_symbol(uint64_t addr) {
  // std::cout << "Lookup symbol for 0x" << std::hex << addr << std::dec <<
  // std::endl;
  // FIXME: Inefficient for the moment, we'll return to this
  regions = read_maps(pid);
  // Find correct region
  const auto &region = lookup_region(addr);

  const auto &path = region.path;

  // mmap correct elf file
  int file_descriptor = open(path.c_str(), O_RDONLY);
  struct stat buf;
  fstat(file_descriptor, &buf);
  size_t file_size = buf.st_size;
  void *base_addr =
      mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
  Elf64_Ehdr *elf_header = static_cast<Elf64_Ehdr *>(base_addr);

  // std::cout << "Open file: " << path << " of size " << file_size <<
  // std::endl;

  uint64_t file_address =
      (elf_header->e_type == ET_EXEC) ? addr : addr - region.load_bias;

  // std::cout << std::hex << "Looking for 0x" << file_address << std::dec <<
  // std::endl;

  // Find symbol table and load it up
  Elf64_Shdr *sections =
      (Elf64_Shdr *)((uint8_t *)base_addr + elf_header->e_shoff);

  // std::cout << "Found " << elf_header->e_shnum << " sections" << std::endl;

  Symbol output{0, 0, "<no symbol found>"};

  Elf64_Shdr *symbol_table_section = nullptr;
  for (int section_index = 0; section_index < elf_header->e_shnum;
       ++section_index) {
    if (sections[section_index].sh_type == SHT_SYMTAB) {
      symbol_table_section = &sections[section_index];
      break;
    }
    if (symbol_table_section == nullptr &&
        sections[section_index].sh_type == SHT_DYNSYM) {
      // std::cout << "Found symbol table" << std::endl;
      // This is the correct section for symbols
      symbol_table_section = &sections[section_index];
    }
  }
  Elf64_Shdr *string_table_section = &sections[symbol_table_section->sh_link];
  const char *string_table =
      (const char *)base_addr + string_table_section->sh_offset;

  Elf64_Sym *symbols =
      (Elf64_Sym *)((uint8_t *)base_addr + symbol_table_section->sh_offset);
  int symbol_count =
      symbol_table_section->sh_size / symbol_table_section->sh_entsize;
  // std::cout << "Found " << symbol_count << " symbols" << std::endl;
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
    // std::cout << std::hex << "[0x" << function_start_addr << ", 0x" <<
    // function_end_addr << "]: " << (string_table +
    // symbols[symbol_index].st_name)
    //           << std::dec<<std::endl;
    if (file_address >= function_start_addr &&
        file_address < function_end_addr) {
      // std::cout << "Matched to function: "
      //           << (string_table + symbols[symbol_index].st_name)
      //           << std::endl;
      output.start = function_start_addr;
      output.size = symbols[symbol_index].st_size;
      output.name = (string_table + symbols[symbol_index].st_name);
      break;
    }
  }

  munmap(base_addr, file_size);
  close(file_descriptor);
  return output;
}

const MappedRegion &SymbolTable::lookup_region(uint64_t addr) const {
  // FIXME: Not efficient, but whatever, can always be optimized
  for (const auto &mapped_region : regions) {
    if (mapped_region.contains_addr(addr)) {
      return mapped_region;
    }
  }
  assert(false && "Shouldn't get here!");
  return regions.front();
}
