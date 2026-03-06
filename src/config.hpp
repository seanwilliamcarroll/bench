#pragma once
#include <string>

struct RecordConfig {
  std::string output_path = "bench.out";
  int interval_ms = 10;
};

struct ReportConfig {
  std::string input_path = "bench.out";
};

RecordConfig parse_record_args(int argc, char *argv[]);
ReportConfig parse_report_args(int argc, char *argv[]);
