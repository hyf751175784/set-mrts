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
vector<Param> get_parameters();

int main(int argc, char** argv) {
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
    double utilization = param->u_range.min;

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
      cout << "Utilization:" << utilization << endl;
      vector<int> success;
      vector<int> exp;
      vector<int> exc;
      for (uint i = 0; i < param->test_attributes.size(); i++) {
        exp.push_back(0);
        success.push_back(0);
        exc.push_back(0);
      }
      for (uint i = 0; i < param->exp_times; i++) {
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
        ProcessorSet processorset = ProcessorSet(*param);
        ResourceSet resourceset = ResourceSet();
        resource_gen(&resourceset, *param);
        //tast_gen(&taskset, &resourceset, *param, utilization);
        dag_task_gen(&dag_tasks, &resourceset, *param, utilization);

        // foreach(dag_tasks.get_tasks(), task) {
        //   task->display_vertices();
        //   task->display_arcs();
        // }
        
        resourceset.update(&dag_tasks);

        for (uint j = 0; j < param->get_method_num(); j++) {
          taskset.init();
          processorset.init();
          resourceset.init();
          exp[j]++;

          if (0 != last_success[j]) {
            SchedTestBase* schedTest =
                STFactory.createSchedTest(param->test_attributes[j].test_name,
                                          &taskset, &dag_tasks, &processorset, &resourceset);
            if (NULL == schedTest) {
              cout << "Incorrect test name." << endl;
              return -1;
            }

            // cout<<""<<endl;
            if (!param->test_attributes[j].rename.empty()) {
              cout << param->test_attributes[j].rename << ":";
            } else {
              cout << param->test_attributes[j].test_name << ":";
            }

            time_t s, e;
            s = time(NULL);
            if (schedTest->is_schedulable()) {
              time(&e);
              ulong gap = difftime(e, s);
              uint hour = gap / 3600;
              uint min = (gap % 3600) / 60;
              uint sec = (gap % 3600) % 60;
              cout << hour << "hour " << min << "min " << sec << "sec. "
                   << "success!" << endl;
              temp_success[j]++;
              // success[j]++;
              s_n++;
              s_i = j;
#if SORT_DEBUG
              cout << param->test_attributes[j].test_name << " success!"
                   << endl;
#endif
            } else {
              time(&e);

              ulong gap = difftime(e, s);
              uint hour = gap / 3600;
              uint min = (gap % 3600) / 60;
              uint sec = (gap % 3600) % 60;

              cout << hour << "hour " << min << "min " << sec << "sec. "
                   << "failed!" << endl;
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
          // sleep(1);
        }

        result.utilization = utilization;
      }  // exp_times
      cout << endl;

      for (uint j = 0; j < param->test_attributes.size(); j++) {
        fraction_t ratio(success[j], exp[j]);

        if (!param->test_attributes[j].rename.empty()) {
          output.add_result(param->test_attributes[j].rename,
                            param->test_attributes[j].style, utilization,
                            exp[j], success[j]);
        } else {
          output.add_result(param->test_attributes[j].test_name,
                            param->test_attributes[j].style, utilization,
                            exp[j], success[j]);
        }

        stringstream buf;
        if (0 == strcmp(param->test_attributes[j].rename.data(), ""))
          buf << param->test_attributes[j].test_name;
        else
          buf << param->test_attributes[j].rename;
        buf << "\t" << utilization << "\t" << exp[j] << "\t" << success[j];
        output.append2file("result-logs.csv", buf.str());

        cout << "Method " << j << ": exp_times(" << exp[j] << ") success times("
             << success[j] << ") success ratio:" << ratio
             << " exc_s:" << exc[j] << endl;
        cout << "======================================================"
             << endl;
      }
      output.export_result_append(utilization);
      output.Export(PNG);
      utilization += param->step;

      for (uint j = 0; j < param->test_attributes.size(); j++) {
        last_success[j] = success[j];
      }
    } while (utilization < param->u_range.max ||
             fabs(param->u_range.max - utilization) < _EPS);

    time(&end);
    cout << endl << "Finish at:" << ctime_r(&end, time_buf) << endl;

    ulong gap = difftime(end, start);
    uint hour = gap / 3600;
    uint min = (gap % 3600) / 60;
    uint sec = (gap % 3600) % 60;

    cout << "Duration:" << hour << " hour " << min << " min " << sec << " sec."
         << endl;

    output.export_csv();

    output.Export(PNG | EPS | SVG | TGA | JSON);
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

vector<Param> get_parameters() {
  vector<Param> parameters;

  XML::LoadFile("config.xml");

  if (0 == access(string("results").data(), 0)) {
    printf("results folder exsists.\n");
  } else {
    printf("results folder does not exsist.\n");
    if (0 == mkdir(string("results").data(), S_IRWXU))
      printf("results folder has been created.\n");
    else
      return parameters;
  }

  // Server info
  const char* ip = XML::get_server_ip();
  uint port = XML::get_server_port();

  Int_Set lambdas, p_nums;
  Double_Set steps, ratio;
  Range_Set p_ranges, u_ranges, d_ranges;
  Test_Attribute_Set test_attributes;
  uint exp_times;

  // scheduling parameter
  XML::get_method(&test_attributes);
  exp_times = XML::get_experiment_times();
  XML::get_lambda(&lambdas);
  XML::get_processor_num(&p_nums);
  XML::get_period_range(&p_ranges);
  XML::get_deadline_propotion(&d_ranges);
  XML::get_utilization_range(&u_ranges);
  XML::get_step(&steps);
  XML::get_doubles(&ratio, "processor_ratio");

  // resource parameter
  Int_Set resource_nums, rrns, mcsns;
  Double_Set rrps, tlfs;
  Range_Set rrrs;
  XML::get_resource_num(&resource_nums);
  XML::get_resource_request_probability(&rrps);
  XML::get_resource_request_num(&rrns);
  XML::get_resource_request_range(&rrrs);
  XML::get_total_len_factor(&tlfs);
  XML::get_integers(&mcsns, "mcsn");

  // graph parameters
  Range_Set job_num_ranges;
  Range_Set arc_num_ranges;
  Int_Set is_cyclics;
  Int_Set max_indegrees;
  Int_Set max_outdegrees;
  Double_Set para_probs, cond_probs, arc_densities;
  Int_Set max_para_jobs, max_cond_branches;

  XML::get_ranges(&job_num_ranges, "dag_job_num_range");
  XML::get_ranges(&arc_num_ranges, "dag_arc_num_range");
  XML::get_integers(&is_cyclics, "is_cyclic");
  XML::get_integers(&max_indegrees, "max_indegree");
  XML::get_integers(&max_outdegrees, "max_outdegree");
  XML::get_doubles(&para_probs, "paralleled_probability");
  XML::get_doubles(&cond_probs, "conditional_probability");
  XML::get_doubles(&arc_densities, "dag_arc_density");
  XML::get_integers(&max_para_jobs, "max_paralleled_job");
  XML::get_integers(&max_cond_branches, "max_conditional_branch");

  foreach(lambdas, lambda)
    foreach(p_nums, p_num)
      foreach(steps, step)
        foreach(p_ranges, p_range)
          foreach(u_ranges, u_range)
            foreach(d_ranges, d_range)
              foreach(resource_nums, resource_num)
                foreach(rrns, rrn)
                  foreach(mcsns, mcsn)
                    foreach(rrps, rrp)
                      foreach(tlfs, tlf)
                        foreach(rrrs, rrr)
                          foreach(job_num_ranges, job_num_range)
                            foreach(arc_num_ranges, arc_num_range)
                              foreach(max_indegrees, max_indegree)
                                foreach(max_outdegrees, max_outdegree)
                                  foreach(para_probs, para_prob)
                                    foreach(cond_probs, cond_prob)
                                      foreach(arc_densities, arc_density)
                                        foreach(max_para_jobs, max_para_job)
                                          foreach(max_cond_branches,
                                                   max_cond_branch) {
                                            Param param;
                                            // set parameters
                                            param.id = parameters.size();
                                            param.server_ip = ip;
                                            param.port = port;
                                            param.lambda = *lambda;
                                            param.p_num = *p_num;
                                            param.ratio = ratio;
                                            param.step = *step;
                                            param.p_range = *p_range;
                                            param.u_range = *u_range;
                                            param.d_range = *d_range;
                                            param.test_attributes =
                                                test_attributes;
                                            param.exp_times = exp_times;
                                            param.resource_num = *resource_num;
                                            param.mcsn = *mcsn;
                                            param.rrn = *rrn;
                                            param.rrp = *rrp;
                                            param.tlf = *tlf;
                                            param.rrr = *rrr;

                                            param.job_num_range =
                                                *job_num_range;
                                            param.arc_num_range =
                                                *arc_num_range;

                                            if (0 == is_cyclics[0])
                                              param.is_cyclic = false;
                                            else
                                              param.is_cyclic = true;

                                            param.max_indegree = *max_indegree;
                                            param.max_outdegree =
                                                *max_outdegree;
                                            param.para_prob = *para_prob;
                                            param.cond_prob = *cond_prob;
                                            param.arc_density = *arc_density;
                                            param.max_para_job = *max_para_job;
                                            param.max_cond_branch =
                                                *max_cond_branch;
                                            parameters.push_back(param);
                                          }
  cout << "param num:" << parameters.size() << endl;
  return parameters;
}
