#pragma once
#include <string>
#include <vector>

struct RecordConfig {
  std::string output_path = "bench.out";
  int interval_ms = 10;
  std::vector<std::string> command;
};

enum class ReportFormat {
  flat,
  folded,
};

struct ReportConfig {
  std::string input_path = "bench.out";
  ReportFormat format = ReportFormat::flat;
};

RecordConfig parse_record_args(int argc, char *argv[]);
ReportConfig parse_report_args(int argc, char *argv[]);
