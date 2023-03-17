// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <types.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <varmapper.h>
#include <iostream>
#include <sstream>


/** Class VarMapperBase */
void VarMapperBase::insert(uint64_t key) {
  assert(next_var < MAX_UINT);
  assert(!sealed);
  uint32_t idx = next_var++;
  map[key] = idx;
}

bool VarMapperBase::exists(uint64_t key) const { return map.count(key) > 0; }

uint32_t VarMapperBase::get(uint64_t key) { return map[key]; }

uint32_t VarMapperBase::var_for_key(uint64_t key) {
  if (!exists(key)) insert(key);
  return get(key);
}

bool VarMapperBase::search_key_for_var(uint32_t var, uint64_t key) const {
  foreach(map, it) {
    if (it->second == var) {
      key = it->first;
      return true;
    }
  }
  return false;
}

VarMapperBase::VarMapperBase(uint32_t start_var)
    : next_var(start_var), sealed(false) {}

// stop new IDs from being generated
void VarMapperBase::seal() { sealed = true; }

uint32_t VarMapperBase::get_num_vars() const { return map.size(); }

uint32_t VarMapperBase::get_next_var() const { return next_var; }

// debugging support
string VarMapperBase::var2str(uint32_t var) const {
  uint64_t key;

  if (search_key_for_var(var, key)) {
    return key2str(key, var);
  } else {
    return "<?>";
  }
}

// should be overridden by children
string VarMapperBase::key2str(uint64_t key, uint32_t var) const {
  std::ostringstream buf;
  buf << "X" << var;
  return buf.str();
}

unordered_map<uint32_t, string> VarMapperBase::get_translation_table()
    const {
  unordered_map<uint32_t, string> table;

  foreach(map, kv) { table[kv->second] = key2str(kv->first, kv->second); }

  return table;
}
