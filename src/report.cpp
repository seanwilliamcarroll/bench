#include "report.hpp"
#include "config.hpp"
#include "profile.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

int cmd_report(int argc, char *argv[]) {
  ReportConfig config = parse_report_args(argc, argv);
  Profile profile = Profile::read(config.input_path);

  if (profile.tid_samples_map.empty()) {
    std::cerr << "No samples found in " << config.input_path << "\n";
    return 1;
  }

  // Print main thread first, then other threads
  std::vector<pid_t> thread_order;
  thread_order.push_back(profile.pid);
  for (const auto &[tid, samples] : profile.tid_samples_map) {
    if (tid != profile.pid) {
      thread_order.push_back(tid);
    }
  }

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

    std::unordered_map<std::string, int> counts;
    for (const auto &sample : samples) {
      if (!sample.frames.empty()) {
        counts[sample.frames[0].name]++;
      }
    }

    std::vector<std::pair<std::string, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    int total = samples.size();
    for (const auto &[name, count] : sorted) {
      double pct = 100.0 * count / total;
      std::cout << std::setw(6) << std::fixed << std::setprecision(1) << pct
                << "%  " << count << "\t" << name << "\n";
    }
    std::cout << "\n";
  }

  return 0;
}
