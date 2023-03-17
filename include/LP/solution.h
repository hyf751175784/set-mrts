// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_LP_SOLUTION_H_
#define INCLUDE_LP_SOLUTION_H_

#include <glpk.h>
#include <types.h>
#include <iostream>

class LinearExpression;
class LinearProgram;

class GLPKSolution {
 private:
  
  const LinearProgram &lp;
  const uint col_num;
  const uint row_num;
  uint coeff_num;
  const bool is_mip;  // mixed integer programing
  const uint dir;
  const uint aim;
  int simplex_code;
  bool solved;
  static int64_t t_limit;

  static void callback(glp_tree *T, void *info);
  void solve(double var_lb, double var_ub);
  void set_objective();
  void set_bounds(double col_lb, double col_ub);
  void set_coefficients();
  void set_column_types();

 public:
  glp_prob *glpk;
  GLPKSolution(const LinearProgram &lp, unsigned int max_var_num,
               double var_lb = 0.0, double var_ub = 1.0, uint dir = 0,
               uint aim = 0);
  ~GLPKSolution();
  int get_status() const;
  void show_error() const;
  double get_value(unsigned int var) const;
  double evaluate(LinearExpression exp) const;
  bool is_solved() const;
  static void set_time_limit(int64_t time);
  static int64_t get_time_limit();
};

#endif  // INCLUDE_LP_SOLUTION_H_
