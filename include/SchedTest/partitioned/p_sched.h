// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_P_SCHED_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_P_SCHED_H_

#include <sched_test_base.h>
#include <types.h>
#include <string>

using std::string;

class Task;
class TaskSet;
class Request;
class ProcessorSet;
class ResourceSet;

class PartitionedSched : public SchedTestBase {
 private:
 public:
  enum { UNTEST, TEST };

  PartitionedSched(bool LinearProgramming, uint TestMethod,
                   uint PriorityAssignment, uint LockingProtocol = NONE,
                   string name = "", string remark = "");

  virtual ~PartitionedSched();

  virtual bool alloc_schedulable() = 0;

  // Bin packing
  bool BinPacking_WF(Task* ti, TaskSet* tasks, ProcessorSet* processors,
                     ResourceSet* resources, uint MODE);
  bool BinPacking_BF(Task* ti, TaskSet* tasks, ProcessorSet* processors,
                     ResourceSet* resources, uint MODE);
  bool BinPacking_FF(Task* ti, TaskSet* tasks, ProcessorSet* processors,
                     ResourceSet* resources, uint MODE);
  bool BinPacking_NF(Task* ti, TaskSet* tasks, ProcessorSet* processors,
                     ResourceSet* resources, uint MODE);
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_P_SCHED_H_
