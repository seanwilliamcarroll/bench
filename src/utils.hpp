#pragma once
#include <sys/resource.h>
#include <time.h>

timespec subtract(timespec start_time, timespec end_time);
void print_rusage(const struct rusage &ru);
void wait_n_msec(int n);
