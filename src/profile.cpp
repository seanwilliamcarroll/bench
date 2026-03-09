#include "profile.hpp"
#include <cassert>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <unordered_map>

void Profile::write(const std::string &path) const {
  std::ofstream out(path);

  for (const auto &sample : tid_samples_map.at(pid)) {
    out << "tid=" << pid << "\n";
    for (const auto &frame : sample.frames) {
      out << frame.name << "\n";
    }
    out << "\n";
  }

  for (const auto &[tid, samples] : tid_samples_map) {
    for (const auto &sample : samples) {
      out << "tid=" << tid << "\n";
      for (const auto &frame : sample.frames) {
        out << frame.name << "\n";
      }
      out << "\n";
    }
  }
}

Profile Profile::read(const std::string &path) {
  // Profile profile;
  std::ifstream in(path);
  // Sample current;
  std::string line;
  bool beginning_of_sample = true;
  bool first_sample = true;
  pid_t pid;
  pid_t tid;
  std::unordered_map<pid_t, Samples> tid_samples_map;
  while (std::getline(in, line)) {
    if (beginning_of_sample) {
      assert(line.substr(0, 4) == "tid=" && "Bad format!");
      int value = std::stoi(line.substr(4, line.size()));
      if (first_sample) {
        pid = value;
        tid = pid;
      } else {
        tid = value;
      }
      tid_samples_map[tid].push_back(Sample(tid, {}));
      beginning_of_sample = false;
      first_sample = false;
    } else if (line.empty()) {
      beginning_of_sample = true;
      // if (!current.frames.empty()) {
      //   profile.samples.push_back(std::move(current));
      //   current = {};
      // }
    } else {
      // current.frames.push_back({0, line});
      tid_samples_map[tid].back().frames.push_back({0, line});
    }
  }
  // Handle last sample if file doesn't end with a blank line
  // if (!current.frames.empty()) {
  //   profile.samples.push_back(std::move(current));
  // }
  Profile profile(pid);
  profile.tid_samples_map = tid_samples_map;
  return profile;
}
