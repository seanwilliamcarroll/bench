#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct Frame {
  uint64_t address;
  std::string name;
};

using CallStack = std::vector<Frame>;

struct Sample {
  CallStack frames;

  void print() const {
    int frame_index = 0;
    for (const auto &frame : frames) {
      std::cout << "  Frame " << frame_index++ << ": " << std::hex
                << frame.address << std::dec << "\n";
    }
  }
};

using Samples = std::vector<Sample>;

struct Profile {
  Samples samples;

  void write(const std::string &path) const;
  static Profile read(const std::string &path);

  void print() const {
    int index = 0;
    for (const auto &sample : samples) {
      std::cout << "Sample " << index++ << ":\n";
      sample.print();
    }
  }
};
