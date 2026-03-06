#include "report.hpp"
#include "profile.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

int cmd_report(int argc, char *argv[]) {
  Profile profile = Profile::read("bench.out");

  if (profile.samples.empty()) {
    std::cerr << "No samples found in bench.out\n";
    return 1;
  }

  std::unordered_map<std::string, int> counts;
  for (const auto &sample : profile.samples) {
    if (!sample.frames.empty()) {
      counts[sample.frames[0].name]++;
    }
  }

  std::vector<std::pair<std::string, int>> sorted(counts.begin(), counts.end());
  std::sort(sorted.begin(), sorted.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  int total = profile.samples.size();
  for (const auto &[name, count] : sorted) {
    double pct = 100.0 * count / total;
    std::cout << std::setw(6) << std::fixed << std::setprecision(1) << pct
              << "%  " << count << "\t" << name << "\n";
  }

  return 0;
}
