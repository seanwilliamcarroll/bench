#pragma once
#include <cstdint>
#include <iostream>
#include <string>
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
};

struct Symbol {
  uint64_t start;
  uint64_t size;
  std::string name;
};

std::vector<MappedRegion> read_maps(pid_t pid);
