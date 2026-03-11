#include "report.hpp"
#include "config.hpp"
#include "profile.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

std::vector<pid_t> get_thread_order(const Profile &profile) {
  // Print main thread first, then other threads
  std::vector<pid_t> thread_order;
  thread_order.push_back(profile.pid);
  for (const auto &[tid, samples] : profile.tid_samples_map) {
    if (tid != profile.pid) {
      thread_order.push_back(tid);
    }
  }
  return thread_order;
}

void flat_report(const ReportConfig &config, const Profile &profile) {
  const auto thread_order = get_thread_order(profile);

  for (const auto &tid : thread_order) {
    auto it = profile.tid_samples_map.find(tid);
    if (it == profile.tid_samples_map.end() || it->second.empty()) {
      continue;
    }
    const auto &samples = it->second;

    std::cout << "--- Thread " << tid;
    if (tid == profile.pid) {
      std::cout << " (main)";
    }
    std::cout << " [" << samples.size() << " samples] ---\n";

    std::unordered_map<std::string, int> exclusive_counts;
    std::unordered_map<std::string, int> inclusive_counts;
    std::unordered_set<std::string> already_seen;
    for (const auto &sample : samples) {
      if (!sample.frames.empty()) {
        exclusive_counts[sample.frames[0].name]++;
      }
      already_seen.clear();
      for (const auto &frame : sample.frames) {
        if (already_seen.count(frame.name) > 0) {
          continue;
        }
        inclusive_counts[frame.name]++;
        already_seen.insert(frame.name);
      }
    }

    std::vector<std::pair<std::string, int>> sorted(exclusive_counts.begin(),
                                                    exclusive_counts.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    int total = samples.size();
    std::cout << std::fixed << std::setprecision(1);
    std::cout << std::setw(7) << "excl%"
              << "  " << std::setw(4) << "excl"
              << "    " << std::setw(7) << "incl%"
              << "  " << std::setw(4) << "incl"
              << "  name\n";
    for (const auto &[name, excl] : sorted) {
      int incl = 0;
      auto it = inclusive_counts.find(name);
      if (it != inclusive_counts.end()) {
        incl = it->second;
      }
      double excl_pct = 100.0 * excl / total;
      double incl_pct = 100.0 * incl / total;
      std::cout << std::setw(6) << excl_pct << "%"
                << "  " << std::setw(4) << excl << "    " << std::setw(6)
                << incl_pct << "%"
                << "  " << std::setw(4) << incl << "  " << name << "\n";
    }
    std::cout << "\n";
  }
}

std::string folded_frame_name(pid_t tid, const Sample &sample) {
  std::string output = std::to_string(tid);

  for (const auto &frame : sample.frames | std::views::reverse) {
    output += ";" + frame.name;
  }

  return output;
}

void folded_report(const ReportConfig &config, const Profile &profile) {
  const auto thread_order = get_thread_order(profile);

  for (const auto &tid : thread_order) {
    auto it = profile.tid_samples_map.find(tid);
    if (it == profile.tid_samples_map.end() || it->second.empty()) {
      continue;
    }
    const auto &samples = it->second;

    std::unordered_map<std::string, size_t> unique_frame_counts;

    for (const auto &sample : samples) {
      unique_frame_counts[folded_frame_name(tid, sample)]++;
    }

    std::vector<std::pair<std::string, int>> sorted(unique_frame_counts.begin(),
                                                    unique_frame_counts.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    for (const auto &[unique_frame_name, count] : sorted) {
      std::cout << unique_frame_name << " " << count << "\n";
    }
  }
}

int cmd_report(int argc, char *argv[]) {
  ReportConfig config = parse_report_args(argc, argv);
  Profile profile = Profile::read(config.input_path);

  if (profile.tid_samples_map.empty()) {
    std::cerr << "No samples found in " << config.input_path << "\n";
    return 1;
  }

  if (config.format == ReportFormat::flat) {
    flat_report(config, profile);
  } else if (config.format == ReportFormat::folded) {
    folded_report(config, profile);
  } else {
    std::cerr << "Unknown ReportFormat::" << static_cast<int>(config.format)
              << "\n";
    return 1;
  }

  return 0;
}
