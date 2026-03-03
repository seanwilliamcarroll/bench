#include "record.hpp"
#include <asm/ptrace.h>
#include <elf.h>
#include <iostream>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>

constexpr uint64_t FRAME_POINTER_REGISTER = 29;
constexpr uint64_t LINK_REGISTER = 30;

struct Frame {
  // Struct to store relevant information from a particular frame in the call
  // stack
  uint64_t address;
};

using CallStack = std::vector<Frame>;

struct Sample {
  CallStack frames;

  void print() const {
    int frame_index = 0;
    for (const auto &frame : frames) {
      std::cout << "Frame: " << frame_index++ << std::endl;
      std::cout << "\t" << std::hex << frame.address << std::dec << std::endl;
    }
  }
};

using Samples = std::vector<Sample>;

struct Profile {
  Samples samples;

  void print() const {
    int index = 0;
    for (const auto &sample : samples) {
      std::cout << "Sample: " << index++ << std::endl;
      sample.print();
    }
  }
};

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
  std::cout << "user time:                 " << user_ms << " ms\n";
  std::cout << "system time:               " << sys_ms << " ms\n";
  std::cout << "peak memory:               " << ru.ru_maxrss << " KB\n";
  std::cout << "minor page faults:         " << ru.ru_minflt << "\n";
  std::cout << "major page faults:         " << ru.ru_majflt << "\n";
  std::cout << "voluntary context switches:   " << ru.ru_nvcsw << "\n";
  std::cout << "involuntary context switches: " << ru.ru_nivcsw << "\n";
}

void wait_n_msec(int n) {
  timespec interval = {0, 1'000'000 * n};
  nanosleep(&interval, nullptr);
}

CallStack record_frames(pid_t pid) {
  CallStack frames;
  user_pt_regs regs;
  iovec iov = {&regs, sizeof(regs)};
  ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov);
  frames.push_back({regs.pc});
  uint64_t return_address = regs.regs[LINK_REGISTER];
  auto frame_pointer = regs.regs[FRAME_POINTER_REGISTER];
  while (frame_pointer != 0) {
    frames.push_back({return_address - 4});
    errno = 0;
    return_address =
        ptrace(PTRACE_PEEKDATA, pid, (void *)(frame_pointer + 8), 0);
    if (errno) {
      break;
    }
    errno = 0;
    uint64_t prev_frame_pointer =
        ptrace(PTRACE_PEEKDATA, pid, (void *)frame_pointer, 0);
    if (errno) {
      break;
    }
    frame_pointer = prev_frame_pointer;
  }
  return frames;
}

int fork_exec(char *argv[]) {
  timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  pid_t pid = fork();

  if (pid == -1) {
    std::cerr << "Failed to fork!" << std::endl;
    return -1;
  }
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    return execvp(argv[0], argv);
  }
  std::cout << "Parent of pid: " << pid << std::endl;

  int status;
  waitpid(pid, &status, 0);

  Profile profile;

  // Want to poll basically
  while (true) {
    ptrace(PTRACE_CONT, pid, 0, 0);
    wait_n_msec(10);
    if (kill(pid, SIGSTOP) < 0) {
      std::cout << "child gone" << std::endl;
      break;
    }
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      break;
    } else if (WIFSTOPPED(status)) {
      profile.samples.push_back({record_frames(pid)});
    } else {
      std::cout << "Can this happen?" << std::endl;
      break;
    }
  }

  timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  auto difference = subtract(start_time, end_time);
  std::cout << "I'm the parent who waited on " << pid << std::endl;
  std::cout << "Waited for " << difference.tv_sec << " second(s) and "
            << difference.tv_nsec << " nanosecond(s)" << std::endl;

  rusage child_stats;
  int ret = getrusage(RUSAGE_CHILDREN, &child_stats);
  if (ret) {
    std::cerr << "Failed to call getrusage!" << std::endl;
    return ret;
  }
  print_rusage(child_stats);

  std::cout << "Recorded " << profile.samples.size() << " sample(s)"
            << std::endl;

  return 0;
}

int cmd_record(int argc, char *argv[]) {
  if (argc < 1) {
    std::cerr << "Must pass a command to record!" << std::endl;
    return -1;
  }
  return fork_exec(argv);
}
