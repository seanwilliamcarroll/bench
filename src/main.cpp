#include "record.hpp"
#include <iostream>
#include <string_view>

static int cmd_report(int argc, char* argv[]) {
  std::cerr << "UNIMPLEMENTED: report\n";
  return 1;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: bench <command> [args...]\n";
    std::cerr << "Commands: record, report\n";
    return 1;
  }

  std::string_view command = argv[1];

  if (command == "record") {
    return cmd_record(argc - 2, argv + 2);
  } else if (command == "report") {
    return cmd_report(argc - 2, argv + 2);
  } else {
    std::cerr << "Unknown command: " << command << "\n";
    std::cerr << "Commands: record, report\n";
    return 1;
  }
}
