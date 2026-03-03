#include "symbols.hpp"
#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>

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

    std::cout << "Valid: ";
    output.push_back({.start = start,
                      .end = end,
                      .load_bias = (start - offset),
                      .path = path});
    output.back().print();
  }

  return output;
}
