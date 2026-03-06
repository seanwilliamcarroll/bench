#pragma once
#include "utils.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sched.h>
#include <sstream>
#include <string>
#include <unordered_map>

struct Symbol {
  uint64_t start;
  uint64_t size;
  std::string name;

  static Symbol null(uint64_t addr) {
    std::stringstream name_stream;
    name_stream << std::hex;
    name_stream << "<no symbol found at 0x" << addr << ">";
    return {.start = addr, .size = 0, .name = name_stream.str()};
  }
};

inline bool operator==(const Symbol &symbol_a, const Symbol &symbol_b) {
  return symbol_a.start == symbol_b.start && symbol_a.size == symbol_b.size &&
         symbol_a.name == symbol_b.name;
}

struct MappedRegion {
  uint64_t start;
  uint64_t end;
  uint64_t load_bias;
  std::string path;
  RangeMap<uint64_t, Symbol> cached_symbols;

  void print() const {
    std::cout << "[0x" << std::hex << start << ", 0x" << end << "]"
              << " bias=0x" << load_bias << " " << path << std::dec << "\n";
  }

  uint64_t contains_addr(uint64_t addr) const {
    return end > addr && start <= addr;
  }

  std::vector<Symbol> load_symbols();

  Symbol lookup_symbol(uint64_t addr) {
    // Do we have the symbol cached?
    auto maybe_symbol = cached_symbols.lookup(addr);
    if (maybe_symbol != nullptr) {
      return *maybe_symbol;
    }
    // Lookup the symbols again and then recheck
    auto symbols = load_symbols();
    for (auto &symbol : symbols) {
      if (cached_symbols.lookup(symbol.start) == nullptr) {
        cached_symbols.insert(symbol.start, symbol.start + symbol.size,
                              std::move(symbol));
      }
    }
    symbols.clear();
    maybe_symbol = cached_symbols.lookup(addr);
    if (maybe_symbol == nullptr) {
      return Symbol::null(addr);
    }
    return *maybe_symbol;
  }
};

struct SymbolTable {
  pid_t pid;
  RangeMap<uint64_t, MappedRegion> regions;

  SymbolTable(pid_t pid) : pid(pid) {}

  MappedRegion *lookup_region(uint64_t addr);

  Symbol lookup_symbol(uint64_t addr);
};
