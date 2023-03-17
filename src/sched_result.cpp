// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <sched_result.h>
#include <iteration-helper.h>
#include <math-helper.h>

template <typename TaskModel>
int increase_order(TaskModel t1, TaskModel t2) {
  return t1.x < t2.x;
}

/** Class SchedResult */

SchedResult::SchedResult(string name, string style) {
  test_name = name;
  line_style = style;
}

string SchedResult::get_test_name() { return test_name; }


void SchedResult::set_test_name(string name) {
	test_name = name;
}

string SchedResult::get_line_style() { return line_style; }

void SchedResult::insert_result(double x, uint e_time, uint s_time, double runtime) {
  bool exist = false;

  foreach(results, result) {
    if (fabs(result->x - x) <= _EPS) {
      exist = true;
      result->exp_num += e_time;
      result->success_num += s_time;
      result->total_rt += runtime;
      break;
    }
  }

  if (!exist) {
    Result temp;
    temp.x = x;
    temp.exp_num = e_time;
    temp.success_num = s_time;
    temp.total_rt = runtime;

    results.push_back(temp);

    sort(results.begin(), results.end(), increase_order<Result>);
  }
}

vector<Result>& SchedResult::get_results() { return results; }

Result SchedResult::get_result_by_utilization(double utilization) {
  Result empty;
  empty.x = 0;
  empty.exp_num = 0;
  empty.success_num = 0;
  empty.total_rt = 0;

  foreach(results, result) {
    if (fabs(result->x - utilization) <= _EPS) {
      return *result;
    }
  }
  return empty;
}

Result SchedResult::get_result_by_x(double x) {
  Result empty;
  empty.x = 0;
  empty.exp_num = 0;
  empty.success_num = 0;
  empty.total_rt = 0;

  foreach(results, result) {
    if (fabs(result->x - x) <= _EPS) {
      return *result;
    }
  }
  return empty;
}

void SchedResult::display_result() {
  cout << "TestName:" << test_name << endl;
  foreach(results, r) {
    cout << "u:" << r->x << " exp:" << r->exp_num
         << " success:" << r->success_num << " total_runtime:" << r->total_rt << endl;
  }
}

/** Class SchedResultSet */

SchedResultSet::SchedResultSet() {}

uint SchedResultSet::size() { return sched_result_set.size(); }

vector<SchedResult>& SchedResultSet::get_sched_result_set() {
  return sched_result_set;
}

SchedResult& SchedResultSet::get_sched_result(string test_name,
                                              string line_style) {
  foreach(sched_result_set, sched_result) {
    if (0 == strcmp(sched_result->get_test_name().data(), test_name.data())) {
      return (*sched_result);
    }
  }

  SchedResult new_element(test_name, line_style);

  sched_result_set.push_back(new_element);

  return sched_result_set[sched_result_set.size() - 1];
}
