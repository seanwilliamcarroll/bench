#include "record.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include <chrono>


int fork_test() {
  pid_t pid = fork();

  if (pid == -1) {
    std::cerr << "Failed to fork!" << std::endl;
    return -1;
  }
  if (pid == 0) {
    std::cout << "I'm the child!" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    return 0;
  }
  wait(NULL);
  std::cout << "I'm the parent who waited on " << pid << std::endl;
  return 0;
}

int cmd_record(int argc, char* argv[]) {
  return fork_test();
}
