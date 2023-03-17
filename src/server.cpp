// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <network.h>
#include <output.h>
#include <param.h>
#include <processors.h>
#include <pthread.h>
#include <random_gen.h>
#include <resources.h>
#include <tasks.h>
#include <toolkit.h>
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
#define MAXBUFFER 100
#define BACKLOG 64

// #define TIME_LIMIT_INIT 600000         // 10 min

#define typeof(x) __typeof(x)

#define BUFFER_SIZE 1024

using std::cout;
using std::endl;
using std::ifstream;
using std::string;

int main(int argc, char** argv) {
  uint init_param;
  if (1 >= argc)
    init_param = 0;
  else
    init_param = atoi(argv[1]);
  // parse parameters from config.xml
  vector<Param> parameters = get_parameters();

  // Network parameters
  int maxi, maxfd;
  int nready;
  int listenfd, connectfd;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  int recvlen;
  ssize_t n;
  fd_set rset, allset;
  string sendbuffer;
  struct sockaddr_in server, client_addr;
  list<NetWork> clients;
  list<NetWork*> idle, busy;
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    cout << "Create socket failed." << endl;
    exit(EXIT_SUCCESS);
  }
  // int flags = fcntl(listenfd, F_GETFL, 0);
  // fcntl(listenfd, F_SETFL, flags|O_NONBLOCK);
  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(parameters[0].port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(listenfd, (struct sockaddr*)&server, sizeof(struct sockaddr)) ==
      -1) {
    cout << "Bind error." << endl;
    exit(EXIT_SUCCESS);
  }

  if (listen(listenfd, BACKLOG) == -1) {
    printf("Listen error.");
    exit(EXIT_SUCCESS);
  }
  maxfd = listenfd;
  maxi = -1;

  FD_ZERO(&allset);
  FD_SET(listenfd, &allset);

  // vector<Param> specific_parameters;
  // specific_parameters.push_back(parameters[4]);
  // specific_parameters.push_back(parameters[44]);


  // foreach(parameters, param) {
  // foreach(specific_parameters, param) {
  // for (uint id = 118; id < parameters.size(); id++) {
  for (uint id = init_param; id < parameters.size(); id++) {
  Param* param = &(parameters[id]);
    // Param* param = &(parameters[0]);
    Result_Set results[MAX_METHOD];
    Output output(*param);
    SchedResultSet srs;
    XML::SaveConfig((output.get_path() + "config.xml").data());
    output.export_param();

    double utilization = param->u_range.min;

    vector<uint> success_num;
    for (uint i = 0; i < param->test_attributes.size(); i++) {
      success_num.push_back(0);
    }

    time_t start, end;
    char time_buf[40];
    start = time(NULL);
    cout << endl
         << "Configuration " << param->id
         << " start at:" << ctime_r(&start, time_buf) << endl;

    do {
      // cout<<"In the circle."<<endl;
      cout << "Utilization:" << utilization << endl;
      uint exp_time;
      // Network
      rset = allset;
      nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
      if (FD_ISSET(listenfd, &rset)) {
        cout << "waiting for connect" << endl;
        if ((connectfd = accept(listenfd, (struct sockaddr*)&client_addr,
                                &sin_size)) == -1) {
          printf("Accept error.");
          continue;
        }

        if (clients.size() < FD_SETSIZE) {
          NetWork client(connectfd, client_addr);
          clients.push_back(client);

          cout << "Connection from [ip:" << inet_ntoa(client_addr.sin_addr)
               << "] has already established." << endl;
        }

        if (clients.size() == FD_SETSIZE) cout << "Too many clients." << endl;

        FD_SET(connectfd, &allset);

        if (connectfd > maxfd) maxfd = connectfd;

        if (clients.size() > maxi) maxi = clients.size();

        if (nready <= 0) continue;
      }

      foreach(clients, client) {
        if (FD_ISSET(client->get_socket(), &rset)) {
          // cout<<"waiting for
          // client:"<<client->get_socket()<<endl;
          string recvbuf = client->recvbuf();

          if (client->get_status()) {
            cout << "Disconnected." << endl;
            FD_CLR(client->get_socket(), &allset);
            clients.erase(client);
            client = clients.begin();
            continue;
          } else {
            cout << recvbuf << endl;
          }

          vector<string> elements;

          extract_element(&elements, recvbuf);

          // foreach(elements, element)
          //   cout<<"element:"<<*element<<endl;

          if (0 == strcmp(elements[0].data(), "3")) {  // heartbeat
            // do nothing
          } else if (0 == strcmp(elements[0].data(), "0")) {  // connection
            sendbuffer = "1,";
            sendbuffer += to_string(param->id) + ",";
            sendbuffer += to_string(utilization);
            client->sendbuf(sendbuffer);
          } else if (0 == strcmp(elements[0].data(), "2")) {  // result
            if (param->id != atoi(elements[1].data())) {
              sendbuffer = "1,";
              sendbuffer += to_string(param->id) + ",";
              sendbuffer += to_string(utilization);
              client->sendbuf(sendbuffer);
              continue;
            }

            // result checking
            bool abandon = false;
            for (uint i = 0; i < param->test_attributes.size(); i++) {
              if (1 < atoi(elements[i + 3].data())) {
                abandon = true;
                sendbuffer = "1,";
                sendbuffer += to_string(param->id) + ",";
                sendbuffer += to_string(utilization);
                client->sendbuf(sendbuffer);
                break;
              }
            }
            if (abandon)
              continue;

            floating_t u(elements[2]);

            // if(!(fabs(u - utilization)<_EPS))
            //   continue;
            // cout<<"Extract result..."<<endl;
            for (uint i = 0; i < param->test_attributes.size(); i++) {
              string test_name;
              if (!param->test_attributes[i].rename.empty()) {
                test_name = param->test_attributes[i].rename;
              } else {
                test_name = param->test_attributes[i].test_name;
              }

              if (output.add_result(test_name, param->test_attributes[i].style,
                                    u, 1,
                                    atoi(elements[i + 3].data()))) {
                stringstream buf;
                buf << test_name;
                buf << "\t" << u << "\t" << 1 << "\t"
                    << elements[i + 3];
                cout << buf.str() << endl;
                output.append2file("result-logs.csv", buf.str());
                success_num[i] += atoi(elements[i + 3].data());
              }
            }

            cout << exp_time << " " << param->exp_times << endl;

            sendbuffer = "1,";
            sendbuffer += to_string(param->id) + ",";
            sendbuffer += to_string(utilization);
            client->sendbuf(sendbuffer);
          } else {  // send work
            sendbuffer = "1,";
            sendbuffer += to_string(param->id) + ",";
            sendbuffer += to_string(utilization);
            client->sendbuf(sendbuffer);
          }

          if (nready <= 0) break;
        }
      }

      exp_time = output.get_exp_time_by_utilization(utilization);

      cout << exp_time << endl;

      if (exp_time == param->exp_times) {
        utilization += param->step;
        bool quick_pass = true;
        for (uint i = 0; i < param->test_attributes.size(); i++) {
          if (0 < success_num[i]) {
            quick_pass = false;
            break;
          }
        }

        if (quick_pass) {
          while (utilization < param->u_range.max ||
                 fabs(param->u_range.max - utilization) < _EPS) {
              for (uint i = 0; i < param->test_attributes.size(); i++) {
              string qp_test_name;
              if (!param->test_attributes[i].rename.empty()) {
                qp_test_name = param->test_attributes[i].rename;
              } else {
                qp_test_name = param->test_attributes[i].test_name;
              }
              output.add_result(qp_test_name, param->test_attributes[i].style,
                                utilization, param->exp_times, 0);
              stringstream qp_buf;
              qp_buf << qp_test_name;
              qp_buf << "\t" << utilization << "\t" << param->exp_times << "\t"
                  << 0;
              output.append2file("result-logs.csv", qp_buf.str());
            }
            utilization += param->step;
          }
          break;
        } else {
          for (uint i = 0; i < param->test_attributes.size(); i++) {
            success_num[i] = 0;
          }
        }
        output.Export(param->u_range.min, param->u_range.max,
                      param->step, PNG, param->graph_x_label,
                      param->graph_y_label, param->graph_legend_pos);
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
    output.Export(PNG | EPS);
  }

  foreach(clients, client) { client->sendbuf("-1"); }

  return 0;
}
