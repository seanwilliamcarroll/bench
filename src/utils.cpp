#include "utils.hpp"
#include <filesystem>
#include <iostream>
#include <sys/types.h>
#include <time.h>
#include <unordered_set>

timespec subtract(timespec start_time, timespec end_time) {
  timespec output;
  output.tv_sec = end_time.tv_sec - start_time.tv_sec;
  output.tv_nsec = end_time.tv_nsec - start_time.tv_nsec;
  if (output.tv_nsec < 0) {
    output.tv_sec -= 1;
    output.tv_nsec += 1'000'000'000;
  }
  return output;
}

void print_rusage(const struct rusage &ru) {
  double user_ms = ru.ru_utime.tv_sec * 1000.0 + ru.ru_utime.tv_usec / 1000.0;
  double sys_ms = ru.ru_stime.tv_sec * 1000.0 + ru.ru_stime.tv_usec / 1000.0;
  std::cout << "user time:                    " << user_ms << " ms\n";
  std::cout << "system time:                  " << sys_ms << " ms\n";
  std::cout << "peak memory:                  " << ru.ru_maxrss << " KB\n";
  std::cout << "minor page faults:            " << ru.ru_minflt << "\n";
  std::cout << "major page faults:            " << ru.ru_majflt << "\n";
  std::cout << "voluntary context switches:   " << ru.ru_nvcsw << "\n";
  std::cout << "involuntary context switches: " << ru.ru_nivcsw << "\n";
}

void wait_n_usec(int n) {
  timespec interval = {0, 1'000 * n};
  nanosleep(&interval, nullptr);
}

void wait_n_msec(int n) { wait_n_usec(n * 1'000); }

std::unordered_set<pid_t> discover_tids(pid_t pid) {
  std::unordered_set<pid_t> output;

  const std::string task_path = "/proc/" + std::to_string(pid) + "/task";
  for (const auto &entry : std::filesystem::directory_iterator(task_path)) {
    output.insert(std::stoi(entry.path().filename()));
  }

  return output;
}
