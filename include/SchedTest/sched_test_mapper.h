// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_SCHED_TEST_MAPPER_H_
#define INCLUDE_SCHEDTEST_SCHED_TEST_MAPPER_H_

#include <stdint.h>
#include <types.h>
#include <string>
#include <unordered_map>

using std::unordered_map;

class SchedTestMapper {
 protected:
  unordered_map<uint64_t, uint> map;

 public:
  SchedTestMapper();
  uint get_method_num() const;
  uint get_next_idx() const;

  unordered_map<uint, string> get_translation_table() const;

  void insert(uint64_t key);
  bool exists(uint64_t key) const;
  uint get(uint64_t key);
  uint get_methord_id(uint64_t key);
  string key2str(uint64_t key, uint var) const;
};

#endif  // INCLUDE_SCHEDTEST_SCHED_TEST_MAPPER_H_
