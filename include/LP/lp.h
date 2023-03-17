// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_LP_LP_H_
#define INCLUDE_LP_LP_H_

#include <types.h>
#include <iostream>
#include <set>
#include <utility>
#include <vector>

using std::pair;
using std::set;
using std::vector;

typedef pair<double, uint> Term;
typedef vector<Term> Terms;

class LinearExpression {
 private:
  Terms terms;

 public:
  Terms& get_terms();
  bool has_terms() const;
  bool term_exist(uint var_index) const;
  int get_terms_size() const;
  double get_term(uint var_index) const;
  void set_term(uint var_index, double coefficient);
  void add_term(uint var_index, double coefficient = 1);
  void sub_term(uint var_index, double pos_coefficient = 1);
  void add_var(uint var_index);
  void sub_var(uint var_index);
};

typedef struct {
  uint var_index;
  bool has_upper;  // upper bound
  bool has_lower;  // lower bound
  double upper_bound, lower_bound;
} VariableRange;

typedef pair<LinearExpression*, double> Constraint;
typedef vector<Constraint> Constraints;
typedef vector<VariableRange> VariableRanges;

class LinearProgram {
  // the function to be maximized
  LinearExpression* objective;

  // linear expressions constrained to an exact value
  Constraints equalities;

  // linear expressions constrained by an upper bound (exp <= bound)
  Constraints inequalities;

  // set of integer variables
  std::set<uint> variables_integer;

  // set of binary variables
  std::set<uint> variables_binary;

  // By default all variables have a lower bound of zero and an upper bound of
  // one. Exceptional cases are stored in this (unsorted) vector.
  VariableRanges non_default_bounds;

 public:
  LinearProgram();
  ~LinearProgram();
  void declare_variable_integer(uint variable_index);
  void declare_variable_binary(uint variable_index);
  void declare_variable_bounds(uint variable_id, bool has_lower, double lower,
                               bool has_upper, double upper);
  void set_objective(LinearExpression* obj);
  void add_inequality(LinearExpression* exp, double upper_bound);
  void add_equality(LinearExpression* exp, double equal_to);
  LinearExpression* get_objective() const;
  const set<uint>& get_integer_variables() const;
  bool has_binary_variables() const;
  bool has_integer_variables() const;
  bool is_integer_variable(unsigned int variable_id) const;
  bool is_binary_variable(unsigned int variable_id) const;
  const set<uint>& get_binary_variables() const;
  const Constraints& get_equalities() const;
  const Constraints& get_inequalities() const;
  const VariableRanges& get_non_default_variable_ranges() const;
};

#endif  // INCLUDE_LP_LP_H_
