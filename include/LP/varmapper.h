// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_LP_VARMAPPER_H_
#define INCLUDE_LP_VARMAPPER_H_

#include <stdint.h>
#include <types.h>
#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

class VarMapperBase {
 private:
  unordered_map<uint64_t, uint32_t> map;
  uint32_t next_var;
  bool sealed;

 protected:
  void insert(uint64_t key);
  bool exists(uint64_t key) const;
  uint32_t get(uint64_t key);
  uint32_t var_for_key(uint64_t key);
  bool search_key_for_var(uint32_t var, uint64_t key) const;

 public:
  explicit VarMapperBase(uint32_t start_var = 0);
  // stop new IDs from being generated
  void seal();
  uint32_t get_num_vars() const;
  uint32_t get_next_var() const;
  unordered_map<uint32_t, string> get_translation_table() const;

  // debugging support
  string var2str(uint32_t var) const;

  // should be overridden by children
  virtual string key2str(uint64_t key, uint32_t var) const;
};

#endif  // INCLUDE_LP_VARMAPPER_H_
