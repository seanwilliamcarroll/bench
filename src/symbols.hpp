#pragma once
#include <cstdint>
#include <iostream>
#include <sched.h>
#include <string>
#include <unordered_map>
#include <vector>

struct MappedRegion {
  uint64_t start;
  uint64_t end;
  uint64_t load_bias;
  std::string path;

  void print() const {
    std::cout << "[0x" << std::hex << start << ", 0x" << end << "]"
              << " bias=0x" << load_bias << " " << path << std::dec << "\n";
  }

  uint64_t contains_addr(uint64_t addr) const {
    return end > addr && start <= addr;
  }
};

struct Symbol {
  uint64_t start;
  uint64_t size;
  std::string name;
};

struct SymbolTable {
  pid_t pid;
  std::vector<MappedRegion> regions;
  std::unordered_map<uint64_t, Symbol> symbols;

  SymbolTable(pid_t pid) : pid(pid) {}

  // Symbol is cheap, just return a copy, could do refs in the future?
  Symbol get_symbol(uint64_t addr);

  Symbol lookup_symbol(uint64_t addr);

  const MappedRegion &lookup_region(uint64_t addr) const;
};
