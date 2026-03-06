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
  return config;
}

ReportConfig parse_report_args(int argc, char *argv[]) {
  ReportConfig config;
  int opt;
  while ((opt = getopt(argc, argv, "i:")) != -1) {
    switch (opt) {
    case 'i':
      config.input_path = optarg;
      break;
    default:
      std::cerr << "Usage: bench report [-i input]\n";
      std::exit(1);
    }
  }
  return config;
}
