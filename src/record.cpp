#include "record.hpp"
#include "config.hpp"
#include "profile.hpp"
#include "symbols.hpp"
#include "utils.hpp"
#include <asm/ptrace.h>
#include <cerrno>
#include <elf.h>
#include <iostream>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vector>

constexpr uint64_t FRAME_POINTER_REGISTER = 29;
constexpr uint64_t LINK_REGISTER = 30;

CallStack record_frames(pid_t pid) {
  CallStack frames;
  user_pt_regs regs;
  iovec iov = {&regs, sizeof(regs)};
  ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov);
  frames.push_back({regs.pc});
  uint64_t return_address = regs.regs[LINK_REGISTER];
  auto frame_pointer = regs.regs[FRAME_POINTER_REGISTER];
  auto is_valid_address = [](uint64_t address) {
    return address != 0 && address < 0x0001000000000000ULL;
  };
  while (is_valid_address(frame_pointer) && is_valid_address(return_address)) {
    frames.push_back({return_address - 4});
    errno = 0;
    return_address =
        ptrace(PTRACE_PEEKDATA, pid, (void *)(frame_pointer + 8), 0);
    if (errno)
      break;
    errno = 0;
    uint64_t prev_frame_pointer =
        ptrace(PTRACE_PEEKDATA, pid, (void *)frame_pointer, 0);
    if (errno)
      break;
    frame_pointer = prev_frame_pointer;
  }
  return frames;
}

int fork_exec(const RecordConfig &config) {
  std::vector<char *> argv;
  for (const auto &s : config.command) {
    argv.push_back(const_cast<char *>(s.c_str()));
  }
  argv.push_back(nullptr);

  timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  pid_t pid = fork();

  if (pid == -1) {
    std::cerr << "Failed to fork!\n";
    return -1;
  }
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    return execvp(argv[0], argv.data());
  }

  std::cout << "Parent of pid: " << pid << "\n";

  int status;
  waitpid(pid, &status, 0);

  Profile profile;
  auto symbol_table = SymbolTable(pid);

  while (true) {
    ptrace(PTRACE_CONT, pid, 0, 0);
    wait_n_msec(config.interval_ms);
    if (kill(pid, SIGSTOP) < 0) {
      break;
    }
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
      break;
    } else if (WIFSTOPPED(status)) {
      profile.samples.push_back({record_frames(pid)});
      for (auto &frame : profile.samples.back().frames) {
        frame.name = symbol_table.get_symbol(frame.address).name;        
      }
    } else {
      break;
    }
  }

  timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  auto difference = subtract(start_time, end_time);
  std::cout << "wall time: " << difference.tv_sec << "s "
            << difference.tv_nsec / 1'000'000 << "ms\n";

  rusage child_stats;
  if (getrusage(RUSAGE_CHILDREN, &child_stats) == 0) {
    print_rusage(child_stats);
  }

  std::cout << "Recorded " << profile.samples.size() << " sample(s)\n";

  profile.write(config.output_path);
  
  return 0;
}

int cmd_record(int argc, char *argv[]) {
  RecordConfig config = parse_record_args(argc, argv);
  return fork_exec(config);
}
