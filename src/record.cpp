#include "record.hpp"
#include <asm/ptrace.h>
#include <chrono>
#include <elf.h>
#include <iostream>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>

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
      std::cout << "Exited" << std::endl;
      break;
    } else if (WIFSTOPPED(status)) {
      user_pt_regs regs;
      iovec iov = {&regs, sizeof(regs)};
      ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov);
      std::cout << "Did reg read!" << std::endl;
    } else {
      std::cout << "Can this happen?" << std::endl;
      break;
    }
  }

  // wait(NULL);
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

  return 0;
}

int cmd_record(int argc, char *argv[]) {
  if (argc < 1) {
    std::cerr << "Must pass a command to record!" << std::endl;
    return -1;
  }
  return fork_exec(argv);
}
