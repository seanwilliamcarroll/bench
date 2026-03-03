#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct MappedRegion {
  uint64_t start;
  uint64_t end;
  uint64_t load_bias;
  std::string path;
};

struct Symbol {
  uint64_t start;
  uint64_t size;
  std::string name;
};

std::vector<MappedRegion> read_maps(pid_t pid);
