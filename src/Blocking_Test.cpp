// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <glpk.h>
#include <iteration-helper.h>
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

  // vector<double> test = Random_Gen::RandFixedSum(5, 20, 1, 10);

  // double sum = 0;
  // foreach(test, t) {
  //   cout << (*t) << endl;
  //   sum += *t;
  // }
  // cout << "Sum:" << sum << endl;

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
      Result result;
      cout << "X:" << x << endl;
      param->reserve_double_2 = x;
      vector<int> success;
      vector<int> exp;
      vector<int> exc;
      for (uint i = 0; i < param->test_attributes.size(); i++) {
        exp.push_back(0);
        success.push_back(0);
        exc.push_back(0);
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

        TaskSet taskset = TaskSet();
        DAG_TaskSet dag_tasks = DAG_TaskSet();
        DAG_TaskSet test = DAG_TaskSet();
        ProcessorSet processorset = ProcessorSet(*param);
        ResourceSet resourceset = ResourceSet();
        resource_gen(&resourceset, *param);
        // task_gen_UUnifast_Discard(&taskset, &resourceset, *param, 0.75 *
        // (double)param->p_num); dag_task_gen_UUnifast_Discard(&dag_tasks,
        // &resourceset, *param, param->reserve_int_2, param->reserve_double_1 *
        // (double)param->p_num);
        dag_task_gen_UUnifast_Discard(&dag_tasks, &resourceset, *param, x);
        // dag_task_gen_RandFixedSum(
        //     &dag_tasks, &resourceset, *param, param->reserve_int_2,
        //     param->reserve_double_1 * static_cast<double>(param->p_num));

        for (uint j = 0; j < param->get_method_num(); j++) {
          taskset.init();
          processorset.init();
          resourceset.init();
          exp[j]++;

          if (0 != last_success[j]) {
            SchedTestBase* schedTest = STFactory.createSchedTest(
                param->test_attributes[j].test_name, &taskset, &dag_tasks,
                &processorset, &resourceset);

            if (NULL == schedTest) {
              cout << "Incorrect test name." << endl;
              return -1;
            }

            if (!param->test_attributes[j].rename.empty()) {
              cout << param->test_attributes[j].rename << ":";
            } else {
              cout << param->test_attributes[j].test_name << ":";
            }
            cout << endl;

            if (schedTest->is_schedulable()) {
              temp_success[j]++;
              s_n++;
              s_i = j;
            }

            if (GLP_UNDEF == schedTest->get_status()) {
              cout << "Abandon cause GLP_UNDEF" << endl;
              abandon = true;
              for (uint k = 0; k <= j; k++) {
                temp_success[k] = 0;
                exp[k]--;
              }
              s_n = 0;
              delete (schedTest);
              break;
            } else {
              delete (schedTest);
            }
          } else {
            if (!param->test_attributes[i].rename.empty()) {
              cout << param->test_attributes[j].rename << ": Abandoned!"
                   << endl;
            } else {
              cout << param->test_attributes[j].test_name << ": Abandoned!"
                   << endl;
            }
          }
        }

        for (uint t = 0; t < param->test_attributes.size(); t++) {
          success[t] += temp_success[t];
        }

        if (1 == s_n) {
          exc[s_i]++;
        }

        result.x = x;
      }  // exp_times
      cout << endl;

      for (uint j = 0; j < param->test_attributes.size(); j++) {
        fraction_t ratio(success[j], exp[j]);

        if (!param->test_attributes[j].rename.empty()) {
          output.add_result(param->test_attributes[j].rename,
                            param->test_attributes[j].style, x, exp[j],
                            success[j]);
        } else {
          output.add_result(param->test_attributes[j].test_name,
                            param->test_attributes[j].style, x, exp[j],
                            success[j]);
        }

        stringstream buf;
        if (0 == strcmp(param->test_attributes[j].rename.data(), ""))
          buf << param->test_attributes[j].test_name;
        else
          buf << param->test_attributes[j].rename;
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
                    param->step, PNG, param->graph_x_label,
                    param->graph_y_label, param->graph_legend_pos);
      x += param->step;
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
                  param->step, PNG | EPS | SVG | TGA | JSON,
                  param->graph_x_label, param->graph_y_label,
                  param->graph_legend_pos);
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
