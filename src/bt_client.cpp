// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <glpk.h>
#include <iteration-helper.h>
#include <math-helper.h>
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
#include <toolkit.h>
#include <unistd.h>
#include <xml.h>
// Socket
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define MAX_LEN 100
#define MAX_METHOD 8
#define MAXBUFFER 100

#define typeof(x) __typeof(x)

using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::ifstream;


struct ARG {
  int connectfd;
  // struct sockaddr_in server;
  // double utilization;
  // Param param;
} typedef ARG;

void getFiles(string path, string dir);
void read_line(string path, vector<string> *files);
// unsigned int alarm(unsigned int seconds);
void *func(void *arg);

int main(int argc, char **argv) {
  string config_file = "config.xml";
  if (1 != argc) {
    for (uint arg = 1; arg < argc; arg++) {
      if (0 == strcmp(argv[arg], "-c")) {
        config_file = argv[++arg];
      }
    }
  }
  cout << "Using configuration: " << config_file << endl;
  // Experiment parameters
  vector<Param> parameters = get_parameters(config_file);
  // Param* param = &(parameters[0]);

  pthread_t tid;
  SchedTestFactory STFactory;
  Random_Gen::uniform_integral_gen(0, 10);
  Random_Gen::exponential_gen(10);
#if UNDEF_ABANDON
  GLPKSolution::set_time_limit(TIME_LIMIT_INIT);
#endif
#if UNDEF_UNSUCCESS
  GLPKSolution::set_time_limit(TIME_LIMIT_INIT);
#endif

  int socketfd;
  int status, recvlen;
  int sin_size = sizeof(struct sockaddr_in);
  char recvbuffer[MAXBUFFER];
  memset(recvbuffer, 0, MAXBUFFER);
  struct sockaddr_in server;
  bool runtime_check = false;
  Time_Record timer;
  double ms_time = 0;

  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Create socket failed.");
    exit(EXIT_SUCCESS);
  }
  if (inet_aton(parameters[0].server_ip, &(server.sin_addr)) == 0) {
    printf("Server ip illegal.");
    exit(EXIT_SUCCESS);
  }

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(parameters[0].server_ip);
  server.sin_port = htons(parameters[0].port);

  while (connect(socketfd, (struct sockaddr *)&server,
                 sizeof(struct sockaddr)) == -1) {
    cout << "Connect failed! Reconnect in 1 second..." << endl;
    sleep(1);
  }

  ARG *arg = reinterpret_cast<ARG *>(malloc(sizeof(ARG)));
  arg->connectfd = socketfd;

  if (send(socketfd, "0", sizeof("0"), 0) < 0) {
    cout << "Send failed!" << endl;
    // sleep(1);
  }

  if (pthread_create(&tid, NULL, func, reinterpret_cast<void *>(arg))) {
    printf("Thread create failed.");
  }

  while (1) {
    memset(recvbuffer, 0, MAXBUFFER);
    if ((recvlen = recv(socketfd, recvbuffer, sizeof(recvbuffer), 0)) == -1) {
      printf("Recieve error.\n");
      pthread_cancel(tid);
      exit(EXIT_SUCCESS);
    } else if (0 == recvlen) {
      printf("Disconnected!\n");
      exit(EXIT_SUCCESS);
    }

    cout << recvbuffer << endl;

    vector<string> elements;

    extract_element(&elements, recvbuffer);

    if (0 == strcmp(elements[0].data(), "-1")) {  // termination
      exit(EXIT_SUCCESS);
    } else if (0 == strcmp(elements[0].data(), "3")) {  // heartbeat
      if (send(socketfd, "3", sizeof("3"), 0) < 0) {
        cout << "Send failed!" << endl;
      }
    } else if (0 == strcmp(elements[0].data(), "1")) {  // work
      int param_index = atoi(elements[1].data());
      floating_t x(elements[2]);
      Param *param = &(parameters[param_index]);
      cout << "param id:" << param->id << endl;

      param->reserve_double_2 = x.get_d();
      // param->p_num = x.get_d();
      vector<int> success;

      for (uint i = 0; i < param->test_attributes.size(); i++) {
        success.push_back(0);
      }

      bool abandon;
      string sendbuf;
      // do
      {
        abandon = false;
        stringstream buf;
        buf << "2,";
        buf << param->id << ",";
        buf << x;
        uint m;

        if (0 == strcmp(param->graph_x_label.c_str(), "Normalized Utilization")) {
          param->reserve_double_2 = x.get_d();
          // cout << "Normalized Utilization:" << param->reserve_double_2 << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Tasks")) {
          param->reserve_int_2 = x.get_d();
          // cout << "Number of Tasks:" << param->reserve_int_2 << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Resources")) {
          param->resource_num = x.get_d();
          // cout << "Number of Resources:" << param->resource_num << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Vertices")) {
          param->job_num_range.max = x.get_d();
          // cout << "Number of Vertices:" << param->job_num_range.max << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Number of Processors")) {
          param->p_num = x.get_d();
          // cout << "Number of Vertices:" << param->job_num_range.max << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Edge Probability")) {
          param->edge_prob = x.get_d();
          // cout << "EP:" << param->rrp << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Shared Resource Request Probability")) {
          param->rrp = x.get_d();
          // cout << "SRP:" << param->rrp << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Critical Section Length per Request")) {
          param->reserve_double_2 = x.get_d();
          // cout << "Critical Section Length per Request:" << param->reserve_double_2 << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Critical Section Number per Task")) {
          param->reserve_double_2 = x.get_d();
          // cout << "Critical Section Number per Task:" << param->reserve_double_2 << endl;
        } else if (0 == strcmp(param->graph_x_label.c_str(), "Minimum Ratio")) {
          param->reserve_double_2 = x.get_d();
          // cout << "Critical Section Number per Task:" << param->reserve_double_2 << endl;
        } else {
          // cout << "default" << endl;
          param->reserve_double_2 = x.get_d();
        }

        if (0 != param->runtime_check) {
          runtime_check = true;
        }
        // param->rrp = x.get_d();
        // param->reserve_int_2 = x.get_d();
        // param->resource_num = x.get_d();
        // param->max_para_job = x.get_d();
        // param->edge_prob = x.get_d();

        TaskSet tasks;
        DAG_TaskSet dag_tasks;
        ResourceSet resourceset = ResourceSet();
        resource_gen(&resourceset, *param);

        double utilization;
        int task_number;

        if (abs(param->reserve_double_3 + 1) >= _EPS)
          utilization = param->reserve_double_3;
        else 
          utilization = x.get_d();

        if (-1 == param->reserve_int_2)
          task_number = utilization * param->p_num / param->mean;
        else {
          task_number = param->reserve_int_2;
          // utilization = param->mean * task_number / param->p_num;
        }

        switch (param->model) {
          case TPS_TASK_MODEL:
            tasks = TaskSet();

            // task_number = 5 * param->p_num;
            // cout << "Utilization:" << utilization << " "
            //       << "Task_number:" << task_number << endl;
                  
            tasks.task_gen(&resourceset, *param, utilization * param->p_num,
                           task_number);
            tasks.display();
            break;
          case DAG_TASK_MODEL:
            dag_tasks = DAG_TaskSet();

            // if (abs(param->reserve_double_3 + 1) >= _EPS)
            //   utilization = param->reserve_double_3;
            // else 
            //   utilization = x.get_d();

            // if (-1 == param->reserve_int_2)
            //   task_number = utilization * param->p_num / param->mean;
            // else
            //   task_number = param->reserve_int_2;

            // task_number = 0.25 * param->p_num;
            // task_number = 0.5 * param->p_num;
            // task_number = param->p_num;

            // cout << "p_num:" << param->p_num << endl;
            // cout << "Utilization:" << utilization << " "
            //       << "Task_number:" << task_number << endl;

            if (DAG_GEN_ERDOS_RENYI == param->dag_gen) {
              // cout << "DAG_GEN_ERDOS_RENYI" << endl;
              
              dag_tasks.task_gen_v2(&resourceset, *param, utilization * param->p_num,
                                task_number); 
              // dag_tasks.display();

            } else if (DAG_GEN_ERDOS_RENYI_v2 == param->dag_gen) {
              // cout << "DAG_GEN_ERDOS_RENYI_v2" << endl;
              // cout << abs(utilization) << endl;
              if (utilization > _EPS) {
                // cout << "utilization>0" << endl;
                dag_tasks.task_gen(&resourceset, *param, task_number);
                // foreach(dag_tasks.get_tasks(), ti) {
                //   cout << "U[" << ti->get_id() << "]:" << ti->get_utilization() << endl;
                // }

                m = ceiling(dag_tasks.get_utilization_sum(), utilization);
                // cout << dag_tasks.get_utilization_sum() << " " << utilization << " " << m << endl; 
                param->p_num = m;
                // dag_tasks.display();
              }
            } else {  // DAG_GEN_DEFAULT

              dag_tasks.task_gen_v2(&resourceset, *param, utilization * param->p_num,
                                task_number);

              // dag_tasks.display();
            }

            break;
          default:
            tasks = TaskSet();
            tasks.task_gen(&resourceset, *param, x.get_d(),
                           x.get_d()/param->mean);
            break;
        }

        ProcessorSet processorset = ProcessorSet(*param);
        // cout << "processor num:" << processorset.get_processor_num() << endl;

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
              buf << "," << 1;
            } else {
              buf << "," << 0;
            }
            timer.Record_MS_B();
            ms_time = timer.Record_MS();
            // ms_time = avg_pd;
            buf << "," << to_string(ms_time);
          } else {
            if (schedTest->is_schedulable()) {
              // exit(0);
              success[j]++;
              buf << "," << 1;
            } else {
              buf << "," << 0;
            }
            buf << "," << to_string(ms_time);
          }
          

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
        sendbuf = buf.str();
      }  // while(abandon);

      // buf<<"\n";

      if (abandon) {
        if (send(socketfd, "0", sizeof("0"), 0) < 0) {
          cout << "Send failed!" << endl;
          // sleep(1);
        }
      } else {
        /*
        #if UNDEF_ABANDON
                GLPKSolution::set_time_limit(TIME_LIMIT_INIT);
        #endif
        */
        cout << sendbuf << endl;
        if (send(socketfd, sendbuf.data(), strlen(sendbuf.data()), 0) < 0) {
          cout << "Send failed!" << endl;
          // sleep(1);
        }
      }
    }
  }

  return 0;
}

void getFiles(string path, string dir) {
  string cmd = "ls " + path + " > " + path + dir;
  system(cmd.data());
}

void read_line(string path, vector<string> *files) {
  string buf;
  ifstream dir(path.data(), ifstream::in);
  getline(dir, buf);
  while (getline(dir, buf)) {
    files->push_back("config/" + buf);
    // cout<<"file name:"<<buf<<endl;
  }
}

/*
int connectfd;
struct sockaddr_in server;
double utilization;
Param param;
*/

void *func(void *arg) {
  struct ARG *info = reinterpret_cast<ARG *>(arg);
  string sendbuf = "3";
  while (1) {
    // cout << "Send heartbeat..." << endl;
    if (send(info->connectfd, sendbuf.data(), strlen(sendbuf.data()), 0) < 0) {
      cout << "Send failed!" << endl;
      // sleep(1);
    }
    sleep(30);
  }
}
