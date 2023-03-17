// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>

/** Class LinearExpression */

Terms& LinearExpression::get_terms() { return terms; }

bool LinearExpression::has_terms() const { return !terms.empty(); }

bool LinearExpression::term_exist(uint var_index) const {
  foreach(terms, term) {
    if (var_index == term->second) return true;
  }
  return false;
}

int LinearExpression::get_terms_size() const { return terms.size(); }

double LinearExpression::get_term(uint var_index) const {
  foreach(terms, term) {
    if (var_index == term->second) {
      return term->first;
    }
  }
  return 0;
}

void LinearExpression::set_term(uint var_index, double coefficient) {
  foreach(terms, term) {
    if (var_index == term->second) {
      term->first = coefficient;
    }
  }
}

void LinearExpression::add_term(uint var_index, double coefficient) {
  // cout<<"Term:"<<coefficient<<endl;
  if (term_exist(var_index)) {
    double coef = get_term(var_index);
    set_term(var_index, coef + coefficient);
  } else {
    terms.push_back(Term(coefficient, var_index));
  }
}
void LinearExpression::sub_term(uint var_index, double pos_coefficient) {
  add_term(var_index, -pos_coefficient);
}

void LinearExpression::add_var(uint var_index) { add_term(var_index, 1); }
void LinearExpression::sub_var(uint var_index) { sub_term(var_index, 1); }

/** Class Linearprogram */

LinearProgram::LinearProgram() : objective(new LinearExpression()) {}

LinearProgram::~LinearProgram() {
  delete objective;
  foreach(equalities, eq)
    delete eq->first;
  foreach(inequalities, ineq)
    delete ineq->first;
  equalities.clear();
  inequalities.clear();
  variables_integer.clear();
  variables_binary.clear();
  non_default_bounds.clear();
}

void LinearProgram::declare_variable_integer(uint variable_index) {
  variables_integer.insert(variable_index);
}

void LinearProgram::declare_variable_binary(uint variable_index) {
  variables_binary.insert(variable_index);
}

void LinearProgram::declare_variable_bounds(uint variable_id, bool has_lower,
                                            double lower, bool has_upper,
                                            double upper) {
  VariableRange b;
  b.var_index = variable_id;
  b.has_lower = has_lower;
  b.has_upper = has_upper;
  b.lower_bound = lower;
  b.upper_bound = upper;
  non_default_bounds.push_back(b);
}

void LinearProgram::set_objective(LinearExpression* obj) {
  delete objective;
  objective = obj;
}

void LinearProgram::add_inequality(LinearExpression* exp, double upper_bound) {
  if (exp->has_terms()) {
    // cout<<"has term"<<endl;
    inequalities.push_back(Constraint(exp, upper_bound));
  } else {
    // cout<<"no term"<<endl;
    delete exp;
  }
}

void LinearProgram::add_equality(LinearExpression* exp, double equal_to) {
  if (exp->has_terms())
    equalities.push_back(Constraint(exp, equal_to));
  else
    delete exp;
}

LinearExpression* LinearProgram::get_objective() const {
  return objective;
}

const set<uint>& LinearProgram::get_integer_variables() const {
  return variables_integer;
}

bool LinearProgram::has_binary_variables() const {
  return !variables_binary.empty();
}

bool LinearProgram::has_integer_variables() const {
  return !variables_integer.empty();
}

bool LinearProgram::is_integer_variable(unsigned int variable_id) const {
  return variables_integer.find(variable_id) != variables_integer.end();
}

bool LinearProgram::is_binary_variable(unsigned int variable_id) const {
  return variables_binary.find(variable_id) != variables_binary.end();
}

const set<uint>& LinearProgram::get_binary_variables() const {
  return variables_binary;
}

const Constraints& LinearProgram::get_equalities() const { return equalities; }

const Constraints& LinearProgram::get_inequalities() const {
  return inequalities;
}

const VariableRanges& LinearProgram::get_non_default_variable_ranges() const {
  return non_default_bounds;
}
