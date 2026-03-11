#include "config.hpp"
#include <cstdlib>
#include <iostream>
#include <unistd.h>

RecordConfig parse_record_args(int argc, char *argv[]) {
  RecordConfig config;
  int opt;
  while ((opt = getopt(argc, argv, "o:r:")) != -1) {
    switch (opt) {
    case 'o':
      config.output_path = optarg;
      break;
    case 'r':
      config.interval_ms = std::atoi(optarg);
      break;
    default:
      std::cerr << "Usage: bench record [-o output] [-r interval_ms] <cmd>\n";
      std::exit(1);
    }
  }
  if (optind >= argc) {
    std::cerr << "Must pass a command to record!\n";
    std::exit(1);
  }
  for (int i = optind; i < argc; ++i) {
    config.command.push_back(argv[i]);
  }
  return config;
}

ReportConfig parse_report_args(int argc, char *argv[]) {
  ReportConfig config;
  int opt;
  while ((opt = getopt(argc, argv, "i:f:")) != -1) {
    switch (opt) {
    case 'i':
      config.input_path = optarg;
      break;
    case 'f':
      if (std::string(optarg) == "folded") {
        config.format = ReportFormat::folded;
      } else if (std::string(optarg) == "flat") {
        config.format = ReportFormat::flat;
      } else {
        std::cerr << "Unknown format: " << optarg << " (flat, folded)\n";
        std::exit(1);
      }
      break;
    default:
      std::cerr << "Usage: bench report [-i input] [-f flat|folded]\n";
      std::exit(1);
    }
  }
  return config;
}
