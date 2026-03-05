#include "profile.hpp"
#include <fstream>

void Profile::write(const std::string &path) const {
  std::ofstream out(path);
  for (const auto &sample : samples) {
    for (const auto &frame : sample.frames) {
      out << frame.name << "\n";
    }
    out << "\n";
  }
}

Profile Profile::read(const std::string &path) {
  Profile profile;
  std::ifstream in(path);
  Sample current;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      if (!current.frames.empty()) {
        profile.samples.push_back(std::move(current));
        current = {};
      }
    } else {
      current.frames.push_back({0, line});
    }
  }
  // Handle last sample if file doesn't end with a blank line
  if (!current.frames.empty()) {
    profile.samples.push_back(std::move(current));
  }
  return profile;
}
