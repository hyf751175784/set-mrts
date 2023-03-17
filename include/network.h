// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_NETWORK_H_
#define INCLUDE_NETWORK_H_

#include <types.h>
// Socket
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

using std::string;

#define MAX_BUFFER 1024

#define CONNECTED 0
#define DISCONNECTED 1
#define IDLE 2
#define BUSY 3

class NetWork {
 private:
  int connectfd;
  struct sockaddr_in addr;
  int status;
  string assigned_job;


 public:
  NetWork(int socketfd, struct sockaddr_in addr) {
    connectfd = socketfd;
    this->addr = addr;
    status = CONNECTED;
  }

  NetWork(int socketfd, string ip, int port) {
    connectfd = socketfd;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    status = CONNECTED;
  }

  ~NetWork() {}

  int get_socket() { return connectfd; }

  void set_status(int stat) { status = stat; }
  void set_assigned_job(string job) { assigned_job = job; }

  int get_status() { return status; }
  string get_assigned_job() { return assigned_job; }

  string recvbuf() {
    char recvbuf[MAX_BUFFER];
    memset(recvbuf, 0, MAX_BUFFER);
    int recv_len;
    if ((recv_len = recv(connectfd, recvbuf, sizeof(recvbuf), 0)) <= 0) {
      status = DISCONNECTED;
      // cout << "receive:" << recv_len << " " << recvbuf << endl;
      return "";
    }

    // cout << "receive:" << recv_len << " " << recvbuf << endl;
    return recvbuf;
  }

  int sendbuf(string buffer) {
    return send(connectfd, buffer.c_str(), strlen(buffer.c_str()), 0);
  }
};

#endif  // INCLUDE_NETWORK_H_
