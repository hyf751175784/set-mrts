// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <glpk.h>
#include <iteration-helper.h>
#include <output.h>
#include <param.h>
#include <processors.h>
#include <pthread.h>
#include <time_record.h>
#include <random_gen.h>
#include <resources.h>
#include <sched_test_base.h>
#include <sched_test_factory.h>
#include <solution.h>
#include <tasks.h>
#include <unistd.h>
#include <xml.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define MAX_LEN 100
#define MAX_METHOD 8

#define typeof(x) __typeof(x)

using std::cout;
using std::endl;
using std::ifstream;
using std::string;

typedef vector<double> RTC;

typedef struct{
  double x;
  RTC time;
}runtime_log;

void getFiles(string path, string dir);
void read_line(string path, vector<string>* files);

int main(int argc, char** argv) {
  // floating_t U;
  // string U_str;
  // if (1 >= argc)
  //   return 0;
  // else
  //   U_str = argv[1];
  // U = floating_t(U_str);
  vector<Param> parameters = get_parameters();
  cout << parameters.size() << endl;
  foreach(parameters, param) {
    Result_Set results[MAX_METHOD];
    SchedTestFactory STFactory;
    Output output(*param);
    XML::SaveConfig((output.get_path() + "config.xml").data());
    output.export_param();

#if UNDEF_ABANDON
    GLPKSolution::set_time_limit(TIME_LIMIT_INIT);
#endif

    Random_Gen::uniform_integral_gen(0, 10);
    double x = param->reserve_range_1.min;

    do {
      Result result;
      cout << "X:" << x << endl;
      // vector<int> success;
      // vector<int> exp;
      // vector<int> exc;
      // for (uint i = 0; i < param->test_attributes.size(); i++) {
      //   exp.push_back(0);
      //   success.push_back(0);
      //   exc.push_back(0);
      // }

      param->reserve_int_2 = x;
      param->reserve_double_3 = x;

      Time_Record timer;
      // timer.Record_MS_A();

      switch (param->model) {
          case TPS_TASK_MODEL:
            for (int i = 0; i < param->exp_times; i++) {
              TaskSet tasks = TaskSet();
              ProcessorSet processorset = ProcessorSet(*param);
              ResourceSet resourceset = ResourceSet();
              resource_gen(&resourceset, *param);
              tasks.task_gen(&resourceset, *param, param->reserve_double_3, param->reserve_int_2);

              SchedTestBase* schedTest =
                      STFactory.createSchedTest(param->test_attributes[0].test_name,
                                                &tasks, NULL, &processorset, &resourceset);
              if (NULL == schedTest) {
                return -1;
              }
              timer.Record_MS_A();
              if (schedTest->is_schedulable()) {
              } 
              timer.Record_MS_B();
            }
            break;
          case DAG_TASK_MODEL:
            for (int i = 0; i < param->exp_times; i++) {
              DAG_TaskSet dag_tasks = DAG_TaskSet();
              ProcessorSet processorset = ProcessorSet(*param);
              ResourceSet resourceset = ResourceSet();
              resource_gen(&resourceset, *param);
              dag_tasks.task_gen(&resourceset, *param, param->reserve_double_3, param->reserve_int_2);
              SchedTestBase* schedTest =
                      STFactory.createSchedTest(param->test_attributes[0].test_name,
                                                NULL, &dag_tasks, &processorset, &resourceset);
              if (NULL == schedTest) {
                return -1;
              }
              timer.Record_MS_A();
              if (schedTest->is_schedulable()) {
              }
              timer.Record_MS_B();
            }
            break;
          default:
            break;
      }

      // timer.Record_MS_B();
      // double ms_time = timer.Record_MS();

      stringstream buf;
      if (0 == strcmp(param->test_attributes[0].rename.data(), ""))
        buf << param->test_attributes[0].test_name;
      else
        buf << param->test_attributes[0].rename;
      buf << "," << x << ","  << ms_time/param->exp_times;
      output.append2file("runtime_complexity.csv", buf.str());

      // time(&e);
      // ulong gap = difftime(e, s);
      // uint hour = gap / 3600;
      // uint min = (gap % 3600) / 60;
      // uint sec = (gap % 3600) % 60;

      // cout << hour << "hour " << min << "min " << sec << "sec. " << endl;

      x += param->reserve_double_1;
    } while (x < param->reserve_range_1.max ||
            fabs(param->reserve_range_1.max - x) < _EPS);
  }

  return 0;
}

void getFiles(string path, string dir) {
  string cmd = "ls " + path + " > " + path + dir;
  system(cmd.data());
}

void read_line(string path, vector<string>* files) {
  string buf;
  ifstream dir(path.data(), ifstream::in);
  getline(dir, buf);
  while (getline(dir, buf)) {
    files->push_back("config/" + buf);
    // cout<<"file name:"<<buf<<endl;
  }
}
