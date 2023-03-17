// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_OUTPUT_H_
#define INCLUDE_OUTPUT_H_

#include <types.h>
#include <mgl_chart.h>
#include <param.h>
#include <sched_result.h>
#include <unistd.h>
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>

using std::string;
using std::to_string;
using std::stringstream;
using std::ofstream;
using std::setprecision;

class Param;
class Chart;

class Output {
 private:
  string path;
  Param param;
  // Result_Sets result_sets;
  SchedResultSet srs;
  Chart chart;

 public:
  explicit Output(const char* path);
  explicit Output(Param param);

  string get_path();
  // void add_set();
  uint get_sets_num() const;
  // void add_result(uint index, double x, double y, uint e_num, uint s_num);
  SchedResultSet& get_results();
  bool add_result(string test_name, string line_style, double x,
                  uint e_num, uint s_num, double runtime = 0);
  uint get_exp_time_by_utilization(double utilization);
  uint get_exp_time_by_x(double x);
  bool no_success_at_x(double x);
  uint get_results_num(uint index) const;
  string output_filename();
  string get_method_name(Test_Attribute ta);

  // output to console
  void proceeding();
  void proceeding(string test_name, double utilization, uint e_num, uint s_num);
  void finish();

  // export to csv
  void export_param();
  void export_csv();
  void export_table_head();
  void export_result_append(double utilization);
  void append2file(string flie_name, string buffer);
  void export_runtime();

  // export to graph
  void SetGraphSize(int width, int height);
  void SetGraphQual(int quality);
  void Export(int format = 0);
  void Export(double from, double to, double step, int format = 0, string x_label = "", string y_label = "", uint pos = 0);
};

#endif  // INCLUDE_OUTPUT_H_
