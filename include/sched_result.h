// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHED_RESULT_H_
#define INCLUDE_SCHED_RESULT_H_

#include <types.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

template <typename TaskModel>
int increase_order(TaskModel t1, TaskModel t2);

class SchedResult {
 private:
  string test_name;
  string line_style;
  vector<Result> results;

 public:
  explicit SchedResult(string name, string line_style = "");
  string get_test_name();
	void set_test_name(string name);
  string get_line_style();
  void insert_result(double x, uint e_time, uint s_time, double runtime = 0);
  vector<Result>& get_results();
  Result get_result_by_utilization(double utilization);
  Result get_result_by_x(double x);
  void display_result();
};

class SchedResultSet {
 private:
  vector<SchedResult> sched_result_set;

 public:
  SchedResultSet();

  uint size();
  vector<SchedResult>& get_sched_result_set();
  SchedResult& get_sched_result(string test_name, string line_style = "");
};

#endif  // INCLUDE_SCHED_RESULT_H_
