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
#include <stdlib.h>
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

typedef struct WS{
  double x;
  uint32_t received;
  uint32_t dispatched;
  uint32_t total;
  WS(double x, uint32_t r, uint32_t d, uint32_t t) : x(x) , received(r) , dispatched(d), total(t) {}
} WS;


class WorkStatus {
  protected:
    vector<WS> works;
  public:
    WorkStatus() {}
    WorkStatus(Param* param) {
      double x = param->reserve_range_1.min;

      do {
        works.push_back(WS(x, 0, 0, param->exp_times));
        x += param->reserve_double_1;
      } while (x < param->reserve_range_1.max ||
              fabs(param->reserve_range_1.max - x) < _EPS);
    }
    vector<WS>& get_work_status() { return works;}
    WS& get_work_status_by_x(double x) {
      WS empty(0, 0, 0, 0);

      foreach(works, ws) {
        if (fabs(ws->x - x) <= _EPS) {
          return *ws;
        }
      }
      return empty;
    }

    void receive(double x) {
      foreach(works, ws) {
        if (fabs(ws->x - x) <= _EPS) {
          ws->received++;
        }
      }
    }

    void dispatch(double x) {
      foreach(works, ws) {
        if (fabs(ws->x - x) <= _EPS) {
          ws->dispatched++;
        }
      }
    }

    void disconnected(double x) {
      foreach(works, ws) {
        if (fabs(ws->x - x) <= _EPS) {
          ws->dispatched--;
        }
      }
    }

    void work_status_panel() {
      foreach(works, ws) {
        string prefix_color_1, prefix_color_2;
        if (ws->received < ws->total) {
          prefix_color_1 = "\033[31m";
        } else {
          prefix_color_1 = "\033[32m";
        }
        if (ws->dispatched < ws->total) {
          prefix_color_2 = "\033[31m";
        } else {
          prefix_color_2 = "\033[32m";
        }

        cout << "X[" << std::setw(4) << ws->x << "]:\t"
             << "RECV:" << prefix_color_1 << ws->received << "/" << ws->total << "\033[0m\t"
             << "DISP:" << prefix_color_2 << ws->dispatched << "/" << ws->total << "\033[0m" << endl;
      }
    }

    double get_job() {
      if (is_finished())
        return works.end()->x; 
      foreach(works, ws) {
        if (ws->dispatched < ws->total) {
          return ws->x;
        }
      }
      foreach(works, ws) {
        if (ws->received < ws->total)
          return ws->x;
      }
      return works.end()->x; 
    }

    void calibrate(list<NetWork> clients) {
      foreach(works, ws) {
        ws->dispatched = ws->received;
      }
      foreach(clients, client) {
        dispatch(atof(client->get_assigned_job().data()));
      }
    }

    bool is_finished() {
      foreach(works, ws) {
        if (ws->received < ws->total)
          return false;
      }
      return true;
    }

};

int main(int argc, char** argv) {
  string config_file = "config.xml";
  bool runtime_check = false;
  if (1 != argc) {
    for (uint arg = 1; arg < argc; arg++) {
      if (0 == strcmp(argv[arg], "-c")) {
        config_file = argv[++arg];
      }
    }
  }
  // cout << "Using configuration: " << config_file << endl;
  // Experiment parameters
  
  vector<Param> parameters = get_parameters(config_file);

  uint index_s = 0, index_e = parameters.size();
  if (1 != argc) {
    for (uint arg = 1; arg < argc; arg++) {
      if (0 == strcmp(argv[arg], "-s")) {
        index_s = atoi(argv[++arg]);
      } else if (0 == strcmp(argv[arg], "-i")) {
        index_s = atoi(argv[++arg]);
        index_e = index_s + 1;
      } else if (0 == strcmp(argv[arg], "-r")) {
        index_s = atoi(argv[++arg]);
        index_e = atoi(argv[++arg]) + 1;
      } else if (0 == strcmp(argv[arg], "-c")) {
        config_file = argv[++arg];
      } else {
        cout << "Unrecognized parameter." << endl;
        exit(0);
      }
    }
  }

  /*
          Param* param = &(parameters[0]);
          Result_Set results[MAX_METHOD];
          Output output(*param);
          SchedResultSet srs;
          XML::SaveConfig((output.get_path() + "config.xml").data());
          output.export_param();
  */

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

  // foreach(parameters, param) {
  for (uint id = index_s; id < index_e; id++) {
    Param* param = &(parameters[id]);
    Result_Set results[MAX_METHOD];
    Output output(*param);
    SchedResultSet srs;
    XML::SaveConfig((output.get_path() + "config.xml").data());
    output.export_param();

    if (0 != param->runtime_check) {
      runtime_check = true;
    }

    double x = param->reserve_range_1.min;
    WorkStatus works(param);

    param->param_panel();
    works.work_status_panel();

    // vector<work_status> works;
    // works.push_back(work_status{.x = x, .received = 0, .dispatched = 0,});

    // vector<uint> success_num;
    // for (uint i = 0; i < param->test_attributes.size(); i++) {
    //   success_num.push_back(0);
    // }

    time_t start, end;
    char time_buf[40];
    start = time(NULL);
    // cout << endl
    //      << "Configuration " << param->id
    //      << " start at:" << ctime_r(&start, time_buf) << endl;

    uint exp_time = 0;
    do {
      bool err = false;
      x = works.get_job();
      // cout<<"In the circle."<<endl;
      // cout << "X:" << x << endl;
      // Network
      rset = allset;
      nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
      if (FD_ISSET(listenfd, &rset)) {
        // cout << "waiting for connect" << endl;
        if ((connectfd = accept(listenfd, (struct sockaddr*)&client_addr,
                                &sin_size)) == -1) {
          printf("Accept error.");
          continue;
        }

        if (clients.size() < FD_SETSIZE) {
          NetWork client(connectfd, client_addr);
          clients.push_back(client);

          // cout << "Connection from [ip:" << inet_ntoa(client_addr.sin_addr)
          //      << "] has already established." << endl;
        }

        if (clients.size() == FD_SETSIZE) cout << "Too many clients." << endl;

        FD_SET(connectfd, &allset);

        if (connectfd > maxfd) maxfd = connectfd;

        if (clients.size() > maxi) maxi = clients.size();

        if (nready <= 0) continue;
      }

      foreach(clients, client) {
        x = works.get_job();
        if (FD_ISSET(client->get_socket(), &rset)) {
          // cout<<"waiting for
          // client:"<<client->get_socket()<<endl;
          string recvbuf = client->recvbuf();

          if (client->get_status()) {
            // cout << "Disconnected." << endl;
            works.disconnected(atof(client->get_assigned_job().data()));
            FD_CLR(client->get_socket(), &allset);
            clients.erase(client);
            client = clients.begin();
            continue;
          } else {
            // cout << recvbuf << endl;
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
            sendbuffer += to_string(x);
            client->sendbuf(sendbuffer);
            // exp_time++;
            works.dispatch(x);
            client->set_assigned_job(to_string(x));
            // cout << "Dispatch to client[" << client->get_socket() << "," 
            //     << client->get_assigned_job() << "] for EXP x = " << x 
            //     << ": " << exp_time << "/" << param->exp_times << endl;
          } else if (0 == strcmp(elements[0].data(), "2")) {  // result
            if (param->id != atoi(elements[1].data())) {  // result out of date, assign new job 
              cout << "ERR[1]:" << recvbuf << endl;
              err = true;
              sendbuffer = "1,";
              sendbuffer += to_string(param->id) + ",";
              sendbuffer += to_string(x);
              // sendbuffer += client->get_assigned_job();
              client->sendbuf(sendbuffer);
              // exp_time++;
              works.dispatch(x);
              client->set_assigned_job(to_string(x));
              // cout << "Dispatch to client[" << client->get_socket() << "," 
              //     << client->get_assigned_job() << "] for EXP x = " << x 
              //     << ": " << exp_time << "/" << param->exp_times << endl;
              continue;
            }

            // result checking
            bool abandon = false;
            for (uint i = 0; i < param->test_attributes.size(); i++) {
              if (1 < atoi(elements[i*2 + 3].data())) {  // probably wrong result, re-do
                cout << "ERR[2]:" << recvbuf << endl;
                err = true;
                abandon = true;
                sendbuffer = "1,";
                sendbuffer += to_string(param->id) + ",";
                // sendbuffer += to_string(x);
                sendbuffer += client->get_assigned_job();
                client->sendbuf(sendbuffer);
                // cout << "Re-dispatch to client[" << client->get_socket() << "," 
                //      << client->get_assigned_job() << "] for EXP x = " << client->get_assigned_job() 
                //      << ": " << exp_time << "/" << param->exp_times << endl;
                // exp_time++;
                break;
              }
            }
            if (abandon)
              continue;

            floating_t u(elements[2]);

            // if(!(fabs(u.get_d() - x)<_EPS))
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
                                    u.get_d(), 1,
                                    atoi(elements[i*2 + 3].data()), atof(elements[i*2 + 4].data()))) {
                stringstream buf;
                buf << test_name;
                buf << "\t" << u.get_d() << "\t" << 1 << "\t"
                    << elements[i*2 + 3] << "\t" << elements[i*2 + 4] << " ms";
                // cout << buf.str() << endl;
                output.append2file("result-logs.csv", buf.str());
                // success_num[i] += atoi(elements[i*2 + 3].data());
              }
            }

            works.receive(u.get_d());
            // cout << "Receive from client[" << client->get_socket() << "," 
            //      << client->get_assigned_job() << "] for EXP x = " << u.get_d() 
            //      << ": " << output.get_exp_time_by_x(u.get_d()) << "/" 
            //      << param->exp_times << endl;

            // cout << exp_time << " " << param->exp_times << endl;

            sendbuffer = "1,";
            sendbuffer += to_string(param->id) + ",";
            sendbuffer += to_string(x);
            client->sendbuf(sendbuffer);
            // exp_time++;
            works.dispatch(x);
            client->set_assigned_job(to_string(x));
            // cout << "Dispatch to client[" << client->get_socket() << "," 
            //      << client->get_assigned_job() << "] for EXP x = " << x 
            //      << ": " << exp_time << "/" << param->exp_times << endl;
          } else {  // send work
            sendbuffer = "1,";
            sendbuffer += to_string(param->id) + ",";
            sendbuffer += to_string(x);
            client->sendbuf(sendbuffer);
            // exp_time++;
            works.dispatch(x);
            client->set_assigned_job(to_string(x));
            // cout << "Dispatch to client[" << client->get_socket() << "," 
            //      << client->get_assigned_job() << "] for EXP x = " << x 
            //      << ": " << exp_time << "/" << param->exp_times << endl;
          }

          if (nready <= 0) break;
        }
      }

      // output.export_result_append(utilization);
      // output.Export(PNG);
      // exp_time = output.get_exp_time_by_x(x);

      if (!err) {
        system("clear");
        cout << "\e[3J";
      }
      

      param->param_panel();

      int c = 0;
      foreach(clients, client) {
        c++;
        string status;
        if (0 == client->get_status())
          status = "\033[32mONLINE\033[0m";
        else
          status = "\033[31mOFFLINE\033[0m";
        
        // cout.precision(4);
        cout << "C[" << std::setw(3) << c << "]:\tX:" << setprecision(2) 
             << atof(client->get_assigned_job().data()) << "\tS:"
             << status << "\t";
        if (0 == c%4) {
          cout << endl;
        } else if (clients.size() == c) {
          cout << endl;
        }
      }

      works.work_status_panel();
      works.calibrate(clients);



      // QUICK PASS
      bool quick_pass = true;
      double x_reverse = param->reserve_range_1.max;
      do {  // find the last finished X
        if (param->exp_times == output.get_exp_time_by_x(x_reverse))
          break;
        x_reverse -= param->reserve_double_1;
      } while (x_reverse > param->reserve_range_1.min || 
              fabs(param->reserve_range_1.min - x_reverse) < _EPS);

      quick_pass = output.no_success_at_x(x_reverse) && (param->exp_times == output.get_exp_time_by_x(x_reverse));

      bool is_finished_before = true;
      foreach(works.get_work_status(), ws) {
        if (ws->received < ws->dispatched)
          is_finished_before = false;
        if (fabs(ws->x - x_reverse) < _EPS)
          break;
      }

      if (quick_pass && is_finished_before) {
        cout << "Start quick pass from " << x_reverse << endl;
        x = x_reverse ;
        while (x < param->reserve_range_1.max ||
              fabs(param->reserve_range_1.max - x) < _EPS) {
            for (uint i = 0; i < param->test_attributes.size(); i++) {
            string qp_test_name;
            if (!param->test_attributes[i].rename.empty()) {
              qp_test_name = param->test_attributes[i].rename;
            } else {
              qp_test_name = param->test_attributes[i].test_name;
            }
            output.add_result(qp_test_name, param->test_attributes[i].style,
                              x, param->exp_times, 0);
            stringstream qp_buf;
            qp_buf << qp_test_name;
            qp_buf << "\t\t" << x << "\t" << param->exp_times << "\t"
                << 0;
            output.append2file("result-logs.csv", qp_buf.str());
          }
          x += param->reserve_double_1;
        }
        break;
      }


      // if (exp_time >= param->exp_times) {
      //   // cout << "Move forward to next x." << endl;
      //   x += param->reserve_double_1;
      //   exp_time = 0;
      // }

      output.Export(param->reserve_range_1.min, param->reserve_range_1.max,
                    param->reserve_double_1, PNG, param->graph_x_label,
                    param->graph_y_label, param->graph_legend_pos);
      if (works.is_finished())
        break;
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
                  param->reserve_double_1, PNG | EPS, param->graph_x_label,
                  param->graph_y_label, param->graph_legend_pos);
    if (runtime_check) {
      output.export_runtime();
    }
  }

  foreach(clients, client) { client->sendbuf("-1"); }

  /*
          output.export_csv();
          output.Export(PNG|EPS|SVG|TGA|JSON);
  */

  return 0;
}
