#pragma once
#include "symbols.hpp"
#include <cstdint>
#include <iostream>
#include <sched.h>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

struct Frame {
  uint64_t address;
  std::string name;
};

using CallStack = std::vector<Frame>;

struct Sample {
  pid_t tid;
  CallStack frames;

  Sample(pid_t tid, CallStack frames) : tid(tid), frames(std::move(frames)) {}

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
  pid_t pid;
  std::unordered_map<pid_t, Samples> tid_samples_map;
  SymbolTable symbol_table;
  size_t total_samples;

  Profile(pid_t pid) : pid(pid), symbol_table(pid), total_samples(0) {}

  void write(const std::string &path) const;
  static Profile read(const std::string &path);

  void sample(pid_t tid, CallStack frames) {
    ++total_samples;
    for (auto &frame : frames) {
      frame.name = symbol_table.lookup_symbol(frame.address).name;
    }
    tid_samples_map[tid].emplace_back(tid, std::move(frames));
  }

  void print() const {
    int index = 0;
    std::cout << "PID: " << pid << "\n";
    for (const auto &sample : tid_samples_map.at(pid)) {
      std::cout << "Sample " << index++ << ":\n";
      sample.print();
    }
    std::cout << "============================================================="
                 "===================\n\n";

    for (const auto &[tid, samples] : tid_samples_map) {
      index = 0;
      std::cout << "TID: " << tid << "\n";
      for (const auto &sample : samples) {
        std::cout << "Sample " << index++ << ":\n";
        sample.print();
      }
    }
  }
};
