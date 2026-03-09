#include "profile.hpp"
#include <cassert>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <unordered_map>

void Profile::write(const std::string &path) const {
  std::ofstream out(path);
  out << "pid=" << pid << "\n";

  auto it = tid_samples_map.find(pid);
  assert(it != tid_samples_map.end() && "main thread must have samples");
  for (const auto &sample : it->second) {
    out << "tid=" << pid << "\n";
    for (const auto &frame : sample.frames) {
      out << frame.name << "\n";
    }
    out << "\n";
  }

  for (const auto &[tid, samples] : tid_samples_map) {
    if (tid == pid) {
      continue;
    }
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
  std::ifstream in(path);
  std::string line;

  assert(std::getline(in, line) && "Bad format: empty file");
  assert(line.substr(0, 4) == "pid=" && "Bad format: expected pid= header");
  pid_t pid = std::stoi(line.substr(4));

  bool beginning_of_sample = true;
  pid_t tid = 0;
  std::unordered_map<pid_t, Samples> tid_samples_map;

  while (std::getline(in, line)) {
    if (beginning_of_sample) {
      assert(line.substr(0, 4) == "tid=" && "Bad format: expected tid= header");
      tid = std::stoi(line.substr(4));
      tid_samples_map[tid].push_back(Sample(tid, {}));
      beginning_of_sample = false;
    } else if (line.empty()) {
      beginning_of_sample = true;
    } else {
      tid_samples_map[tid].back().frames.push_back({0, line});
    }
  }

  Profile profile(pid);
  profile.tid_samples_map = std::move(tid_samples_map);
  return profile;
}

void RecordingProfile::sample(pid_t tid, CallStack frames) {
  for (auto &frame : frames) {
    frame.name = symbol_table.lookup_symbol(frame.address).name;
  }
  profile.tid_samples_map[tid].emplace_back(tid, std::move(frames));
}
