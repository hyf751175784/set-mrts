// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <glpk.h>
#include <iteration-helper.h>
#include <param.h>
#include <processors.h>
#include <pthread.h>
#include <random_gen.h>
#include <resources.h>
#include <sched_test_base.h>
#include <sched_test_factory.h>
#include <solution.h>
#include <tasks.h>
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
using std::ifstream;


struct ARG {
  int connectfd;
  // struct sockaddr_in server;
  // double utilization;
  // Param param;
} typedef ARG;

void getFiles(string path, string dir);
void read_line(string path, vector<string> *files);
unsigned int alarm(unsigned int seconds);
void *func(void *arg);

int main(int argc, char **argv) {
  /*
          if(2 != argc)
          {
                  printf("Usage: %s [port]\n",argv[0]);
                  exit(EXIT_SUCCESS);
          }
          int port = atoi(argv[1]);
  */

  pthread_t tid;
  vector<Param> parameters = get_parameters();
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
    cout << "Connect failed! Reconnect in 10 second..." << endl;
    sleep(10);
  }

  ARG *arg = reinterpret_cast<ARG *>(malloc(sizeof(ARG)));
  arg->connectfd = socketfd;

  if (send(socketfd, "0", sizeof("0"), 0) < 0) {
    cout << "Send failed!" << endl;
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
      floating_t utilization(elements[2]);
      Param *param = &(parameters[param_index]);

      cout << "param id:" << param->id << endl;

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
        buf << utilization;

        TaskSet taskset = TaskSet();
        // DAG_TaskSet dag_tasks = DAG_TaskSet();
        ProcessorSet processorset = ProcessorSet(*param);
        ResourceSet resourceset = ResourceSet();
        resource_gen(&resourceset, *param);
        task_gen(&taskset, &resourceset, *param, utilization);
        // task_gen_UUnifast_Discard(&taskset, &resourceset, *param, utilization);
        // dag_task_gen(&dag_tasks, &resourceset, *param, utilization);
        // dag_task_gen_UUnifast_Discard(&dag_tasks, &resourceset, *param, 8, utilization);

        resourceset.update(&taskset);

        // For debug
        // TaskSet taskset_bkp;
        // ResourceSet resourceset_bkp;
        // ProcessorSet processorset_bkp;         
        // bool is_ROP_success = false;


        for (uint j = 0; j < param->get_method_num(); j++) {
          taskset.init();
          //dag_tasks.init();
          processorset.init();
          resourceset.init();
          SchedTestBase *schedTest;
          schedTest = STFactory.createSchedTest(param->test_attributes[j].test_name,
                                        &taskset, NULL, &processorset, &resourceset);

          // For debug
          // if(strcmp(param->test_attributes[j].test_name.data(),"LP-RTA-PFP-ROP-DPCP-PLUS") && is_ROP_success) {
          //   schedTest =
          //       STFactory.createSchedTest(param->test_attributes[j].test_name,
          //                               &taskset_bkp, NULL, &processorset_bkp, &resourceset_bkp);
          // } else {
          //   schedTest =
          //       STFactory.createSchedTest(param->test_attributes[j].test_name,
          //                               &taskset, NULL, &processorset, &resourceset);
          // }


          if (NULL == schedTest) {
            cout << "Incorrect test name." << endl;
            return -1;
          }

          if (schedTest->is_schedulable()) {
            success[j]++;
            buf << "," << 1;
            // For debug
            // if(strcmp(param->test_attributes[j].test_name.data(),"RTA-PFP-ROP")) {
            //   is_ROP_success = true;
            // }

          } else {
            buf << "," << 0;
          }

          // For debug
          // if(strcmp(param->test_attributes[j].test_name.data(),"RTA-PFP-ROP")) {
          //   taskset_bkp = ((RTA_PFP_ROP*)schedTest)->get_TaskSet();
          //   resourceset_bkp = ((RTA_PFP_ROP*)schedTest)->get_ResourceSet();
          //   processorset_bkp = ((RTA_PFP_ROP*)schedTest)->get_ProcessorSet();
          // }

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

      if (abandon) {
        if (send(socketfd, "0", sizeof("0"), 0) < 0) {
          cout << "Send failed!" << endl;
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
  }
}

void *func(void *arg) {
  struct ARG *info = reinterpret_cast<ARG *>(arg);
  string sendbuf = "3";
  while (1) {
    if (send(info->connectfd, sendbuf.data(), strlen(sendbuf.data()), 0) < 0) {
      cout << "Send failed!" << endl;
    }
    sleep(30);
  }
}
