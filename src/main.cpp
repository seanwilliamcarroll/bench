#include "record.hpp"
#include "report.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: bench <command> [args...]\n";
    std::cerr << "Commands: record, report\n";
    return 1;
  }

  std::string_view command = argv[1];

  if (command == "record") {
    return cmd_record(argc - 1, argv + 1);
  } else if (command == "report") {
    return cmd_report(argc - 1, argv + 1);
  } else {
    std::cerr << "Unknown command: " << command << "\n";
    std::cerr << "Commands: record, report\n";
    return 1;
  }
}
