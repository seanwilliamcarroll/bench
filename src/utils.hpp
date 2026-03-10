#pragma once
#include <algorithm>
#include <cassert>
#include <sys/resource.h>
#include <time.h>
#include <unordered_set>
#include <vector>

template <typename KeyType, typename ValueType> struct RangeMap {
private:
  struct Entry {
    KeyType start;
    KeyType end;
    ValueType value;

    bool contains(KeyType key) const { return start <= key && key < end; }
  };

  std::vector<Entry> data;

public:
  void insert(KeyType start, KeyType end, ValueType value) {
    assert(end > start && "Can't accept invalid range");

    auto iter = std::lower_bound(
        data.begin(), data.end(), start,
        [](const Entry &entry, KeyType key) { return entry.start < key; });
    data.insert(iter, {start, end, value});
  }

  ValueType *lookup(KeyType key) {
    auto iter = std::upper_bound(data.begin(), data.end(), key,
                                 [](KeyType key_instance, const Entry &entry) {
                                   return key_instance < entry.start;
                                 });
    // Should return an iterator pointing to one entry past the one we want
    if (iter == data.begin()) {
      return nullptr;
    }
    // Step back an entry
    iter--;
    auto &entry = *iter;
    if (entry.contains(key)) {
      return &entry.value;
    }
    return nullptr;
  }
};

timespec subtract(timespec start_time, timespec end_time);
void print_rusage(const struct rusage &ru);
void wait_n_usec(int n);
void wait_n_msec(int n);

std::unordered_set<pid_t> discover_tids(pid_t pid);
