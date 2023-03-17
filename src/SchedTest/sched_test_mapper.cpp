#include <sched_test_mapper.h>

SchedTestMapper::SchedTestMapper() {}

uint SchedTestMapper::get_method_num() const { return map.size(); }

uint SchedTestMapper::get_next_idx() const { return map.size(); }

void SchedTestMapper::insert(uint64_t key) {
  unsigned int idx = get_next_idx();
  map[key] = idx;
}

bool SchedTestMapper::exists(uint64_t key) const { return map.count(key) > 0; }

uint SchedTestMapper::get(uint64_t key) { return map[key]; }

uint SchedTestMapper::get_methord_id(uint64_t key) {}

string SchedTestMapper::key2str(uint64_t key, uint var) const {}
