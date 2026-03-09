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

  Profile(pid_t pid) : pid(pid) {}

  size_t sample_count() const {
    size_t count = 0;
    for (const auto &[tid, samples] : tid_samples_map) {
      count += samples.size();
    }
    return count;
  }

  void write(const std::string &path) const;
  static Profile read(const std::string &path);
};

struct RecordingProfile {
  Profile profile;
  SymbolTable symbol_table;

  RecordingProfile(pid_t pid) : profile(pid), symbol_table(pid) {}

  void sample(pid_t tid, CallStack frames);
  void write(const std::string &path) const { profile.write(path); }
  size_t sample_count() const { return profile.sample_count(); }
};
