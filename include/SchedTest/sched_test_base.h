// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_SCHED_TEST_BASE_H_
#define INCLUDE_SCHEDTEST_SCHED_TEST_BASE_H_

#include <types.h>
#include <fstream>
#include <sstream>
#include <string>

using std::string;
using std::stringstream;

// Test Method
#define UTI_BOUND 0
#define RTA 1
#define NC 2  // Necessary Condition
#define SB 3  // Speedup Bound
#define CAB 4  // Capacity Augmentation Bound

// Scheduling Method
#define GLOBAL 0
#define PARTITIONED 1

// Priority Assignment
#define FIX_PRIORITY 0
#define EDF 1

// Locking Protocols
#define NONE 0
#define SPIN 1
#define PIP 2
#define DPCP 3
#define MPCP 4
#define FMLP 5
#define OTHER 6

// Status
#define INIT -1
#define SUCCESS 0
#define UNDEF 1
#define UNSUCCESS 2

class Task;
class TaskSet;
class Request;
class Resource;
class ResourceSet;
class Processor;
class ProcessorSet;
class LinearExpression;
class LinearProgram;

class SchedTestBase {
 protected:
  bool LinearProgramming;
  uint TestMethod;
  uint SchedMethod;
  uint PriorityAssignment;
  uint LockingProtocol;
  string name;
  string remark;
  int status;

 public:
  SchedTestBase(bool LinearProgramming, uint TestMethod, uint SchedMethod,
                uint PriorityAssignment, uint LockingProtocol = NONE,
                string name = "", string remark = "");
  virtual ~SchedTestBase();
  string get_test_name();
  virtual bool is_schedulable() = 0;
  void set_status(int status);
  int get_status();
  // virtual TaskSet get_TaskSet();
  // virtual ResourceSet get_ResourceSet();
  // virtual ProcessorSet get_ProcessorSet();
};

#endif  // INCLUDE_SCHEDTEST_SCHED_TEST_BASE_H_
