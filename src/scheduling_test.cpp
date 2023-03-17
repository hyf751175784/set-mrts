// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <glpk.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <output.h>
#include <param.h>
#include <processors.h>
#include <pthread.h>
#include <random_gen.h>
#include <resources.h>
#include <sched_test_base.h>
#include <sched_test_factory.h>
#include <solution.h>
#include <tasks.h>
#include <time_record.h>
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

void getFiles(string path, string dir);
void read_line(string path, vector<string>* files);

int main(int argc, char** argv) {
  vector<Param> parameters = get_parameters();
  cout << parameters.size() << endl;
  bool runtime_check = false;
  Time_Record timer;
  double ms_time = 0;
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

    time_t start, end;
    char time_buf[40];
    start = time(NULL);
    cout << endl << "Strat at:" << ctime_r(&start, time_buf) << endl;

    vector<int> last_success;

    for (uint i = 0; i < param->get_method_num(); i++) {
      last_success.push_back(1);
    }

    do {
      uint m;
      Result result;
      double total_rt = 0;
      cout << "X:" << x << endl;
      vector<int> success;
      vector<int> exp;
      vector<int> exc;
      for (uint i = 0; i < param->test_attributes.size(); i++) {
        exp.push_back(0);
        success.push_back(0);
        exc.push_back(0);
      }

      if (0 == strcmp(param->graph_x_label.c_str(), "Normalized Utilization")) {
        param->reserve_double_2 = x;
        // cout << "Normalized Utilization:" << param->reserve_double_2 << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Tasks")) {
        param->reserve_int_2 = x;
        // cout << "Number of Tasks:" << param->reserve_int_2 << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Resources")) {
        param->resource_num = x;
        // cout << "Number of Resources:" << param->resource_num << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Vertices")) {
        param->job_num_range.max = x;
        // cout << "Number of Vertices:" << param->job_num_range.max << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Edge Probability")) {
        param->edge_prob = x;
        // cout << "EP:" << param->rrp << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Shared Resource Request Probability")) {
        param->rrp = x;
        // cout << "SRP:" << param->rrp << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Critical Section Length per Request")) {
        param->reserve_double_2 = x;
        // cout << "Critical Section Length per Request:" << param->reserve_double_2 << endl;
      } else if (0 == strcmp(param->graph_x_label.c_str(), "Critical Section Number per Task")) {
        param->reserve_double_2 = x;
        // cout << "Critical Section Number per Task:" << param->reserve_double_2 << endl;
      } else {
        // cout << "default" << endl;
        param->reserve_double_2 = x;
      }

      if (0 != param->runtime_check) {
        runtime_check = true;
      }

      for (int i = 0; i < param->exp_times; i++) {
        cout << "EXP: " << i + 1 << " of " << param->exp_times << endl;
        cout << std::flush;
        uint s_n = 0;
        uint s_i = 0;
        vector<int> temp_success;
        bool abandon = false;



        for (uint t = 0; t < param->test_attributes.size(); t++) {
          temp_success.push_back(0);
        }



        TaskSet tasks;
        DAG_TaskSet dag_tasks;
        ResourceSet resourceset = ResourceSet();
        resource_gen(&resourceset, *param);

        switch (param->model) {
          case TPS_TASK_MODEL:
            tasks = TaskSet();
            tasks.task_gen(&resourceset, *param, x * param->p_num,
                           (param->p_num * x)/param->mean);
            break;
          case DAG_TASK_MODEL:
            dag_tasks = DAG_TaskSet();
            double utilization;
            int task_number;

            if (abs(param->reserve_double_3 + 1) >= _EPS)
              utilization = param->reserve_double_3;
            else 
              utilization = x;

            if (-1 == param->reserve_int_2)
              task_number = utilization * param->p_num / param->mean;
            else
              task_number = param->reserve_int_2;

            // task_number = 0.5 * param->p_num;

            cout << "Utilization:" << utilization << " "
                  << "Task_number:" << task_number << endl;

            if (DAG_GEN_ERDOS_RENYI == param->dag_gen) {
              cout << "DAG_GEN_ERDOS_RENYI" << endl;
              
              dag_tasks.task_gen_v2(&resourceset, *param, utilization * param->p_num,
                                task_number); 

            } else if (DAG_GEN_ERDOS_RENYI_v2 == param->dag_gen) {
              cout << "DAG_GEN_ERDOS_RENYI_v2" << endl;

              if (abs(utilization) > _EPS) {

                dag_tasks.task_gen(&resourceset, *param, task_number);
                foreach(dag_tasks.get_tasks(), ti) {
                  cout << "U[" << ti->get_id() << "]:" << ti->get_utilization() << endl;
                }

                m = ceiling(dag_tasks.get_utilization_sum(), utilization);
                cout << dag_tasks.get_utilization_sum() << " " << utilization << " " << m << endl; 
                param->p_num = m;
                // dag_tasks.display();
              }
            } else {  // DAG_GEN_DEFAULT

              dag_tasks.task_gen_v2(&resourceset, *param, utilization * param->p_num,
                                task_number);
            }

            // foreach(dag_tasks.get_tasks(), task) {
            //   task->display();
            //   task->display_in_dot();
            // }

            break;
          default:
            tasks = TaskSet();
            tasks.task_gen(&resourceset, *param, x,
                           x/param->mean);
            break;
        }

        

        ProcessorSet processorset = ProcessorSet(*param);
        cout << "processor num:" << processorset.get_processor_num() << endl;

        for (uint j = 0; j < param->get_method_num(); j++) {
          switch (param->model) {
            case TPS_TASK_MODEL:
              tasks.init();
              break;
            case DAG_TASK_MODEL:
              dag_tasks.init();
              break;
            default:
              tasks.init();
              break;
          }
          processorset.init();
          resourceset.init();
          

          if (!param->test_attributes[j].rename.empty()) {
            cout << param->test_attributes[j].rename << ":";
          } else {
            cout << param->test_attributes[j].test_name << ":";
          }
          cout << endl;

          SchedTestBase* schedTest;
          switch (param->model) {
            case TPS_TASK_MODEL:
              schedTest = STFactory.createSchedTest(
                  param->test_attributes[j].test_name, &tasks, NULL,
                  &processorset, &resourceset);
              break;
            case DAG_TASK_MODEL:
              schedTest = STFactory.createSchedTest(
                  param->test_attributes[j].test_name, NULL, &dag_tasks,
                  &processorset, &resourceset);
              break;
            default:
              schedTest = STFactory.createSchedTest(
                  param->test_attributes[j].test_name, &tasks, NULL,
                  &processorset, &resourceset);
              break;
          }
          if (NULL == schedTest) {
            cout << "Incorrect test name." << endl;
            return -1;
          }

          uint32_t avg_pd = 0;
          foreach(dag_tasks.get_tasks(), task) {
            avg_pd += task->get_parallel_degree();
          }
          if (0 < dag_tasks.get_taskset_size())
            avg_pd /= dag_tasks.get_taskset_size();

          ms_time = 0;
          if (runtime_check) {
            timer.Record_MS_A();
            if (schedTest->is_schedulable()) {
              // exit(0);
              success[j]++;
            }
            timer.Record_MS_B();
            ms_time = timer.Record_MS();
            total_rt += ms_time;
          } else {
            if (schedTest->is_schedulable()) {
              // exit(0);
              success[j]++;
            }
          }
          exp[j]++;
          

#if UNDEF_ABANDON
          if (GLP_UNDEF == schedTest->get_status()) {
            cout << "Abandon cause GLP_UNDEF" << endl;
            int64_t current_lmt = GLPKSolution::get_time_limit();
            int64_t new_lmt = (current_lmt * 2 <= TIME_LIMIT_UPPER_BOUND)
                               ? current_lmt * 2
                               : TIME_LIMIT_UPPER_BOUND;
            cout << "Set GLPK time limit to:" << new_lmt / 1000 << " s" << endl;
            GLPKSolution::set_time_limit(new_lmt);
            abandon = true;
            delete (schedTest);
            break;
          }
#endif

#if UNDEF_UNSUCCESS
          if (GLP_UNDEF == schedTest->get_status()) {
            cout << "Unsuccess due to GLP_UNDEF." << endl;
          }
#endif
          delete (schedTest);
        }

#if UNDEF_ABANDON
        if (abandon) {
          int64_t current_lmt = GLPKSolution::get_time_limit();
          int64_t new_lmt =
              (current_lmt + TIME_LIMIT_GAP <= TIME_LIMIT_UPPER_BOUND)
                  ? current_lmt + TIME_LIMIT_GAP
                  : TIME_LIMIT_UPPER_BOUND;
          cout << "Set GLPK time limit to:" << new_lmt / 1000 << " s" << endl;
          GLPKSolution::set_time_limit(new_lmt);
          i--;
          continue;
        }
#endif
        for (uint t = 0; t < param->test_attributes.size(); t++) {
          success[t] += temp_success[t];
        }
        /*
                                        if(temp_success[0]==1&&temp_success[1]==0)
                                        {

                                                exit(0);
                                        }
        */
        if (1 == s_n) {
          exc[s_i]++;
          // cout << "Exclusive." << endl;
          // exit(0);
          // cout << "======================================================"
          //      << endl;
#if SORT_DEBUG
          cout << "Exclusive Success TaskSet:" << endl;
          cout << "/////////////////" << param->test_attributes[s_i].test_name
               << "////////////////" << endl;
          foreach(taskset.get_tasks(), task) {
            cout << "Task " << task->get_id() << ":" << endl;
            cout << "WCET:" << task->get_wcet()
                 << " Deadline:" << task->get_deadline()
                 << " Period:" << task->get_period()
                 << " Gap:" << task->get_deadline() - task->get_wcet()
                 << " Leisure:" << taskset.leisure(task->get_id()) << endl;
            cout << "-----------------------" << endl;
          }
#endif
        }

        result.x = x;
      }  // exp_times
      cout << endl;

      for (uint j = 0; j < param->test_attributes.size(); j++) {
        string test_name;
        if (!param->test_attributes[j].rename.empty()) {
          test_name = param->test_attributes[j].rename;
        } else {
          test_name = param->test_attributes[j].test_name;
        }
        fraction_t ratio(success[j], exp[j]);
        output.add_result(test_name, param->test_attributes[j].style, x,
                            exp[j], success[j], total_rt/param->exp_times);

        stringstream buf;
        buf << test_name;
        buf << "\t" << x << "\t" << exp[j] << "\t" << success[j];
        output.append2file("result-logs.csv", buf.str());
        cout << "Method " << j << ": exp_times(" << exp[j] << ") success times("
             << success[j] << ") success ratio:" << ratio
             << " exc_s:" << exc[j] << endl;
        cout << "======================================================"
             << endl;
      }
      output.export_result_append(x);
      output.Export(param->reserve_range_1.min, param->reserve_range_1.max,
                      param->reserve_double_1, PNG, param->graph_x_label,
                      param->graph_y_label, param->graph_legend_pos);
      x += param->reserve_double_1;

      for (uint j = 0; j < param->test_attributes.size(); j++) {
        last_success[j] = success[j];
      }
    } while (x < param->reserve_range_1.max ||
             fabs(param->reserve_range_1.max - x) < _EPS);

    time(&end);
    cout << endl << "Finish at:" << ctime_r(&end, time_buf) << endl;
    ulong gap = difftime(end, start);
    uint hour = gap / 3600;
    uint min = (gap % 3600) / 60;
    uint sec = (gap % 3600) % 60;
    cout << "Duration:" << hour << " hour " << min << " min " << sec << " sec."
         << endl;

    output.export_csv();
    output.Export(param->reserve_range_1.min, param->reserve_range_1.max,
                  param->reserve_double_1, PNG | EPS | SVG | TGA | JSON, param->graph_x_label,
                  param->graph_y_label, param->graph_legend_pos);
    if (runtime_check) {
      output.export_runtime();
    }
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
  }
}
