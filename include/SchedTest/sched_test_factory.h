// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_SCHED_TEST_FACTORY_H_
#define INCLUDE_SCHEDTEST_SCHED_TEST_FACTORY_H_

#include <types.h>
#include <string>
// #include "sched_test_base.h"

using std::string;

class TaskSet;
class ProcessorSet;
class ResourceSet;
class SchedTestBase;

class SchedTestFactory {
 public:
  SchedTestBase *createSchedTest(string test_name, TaskSet *tasks,
                                 DAG_TaskSet *dag_tasks,
                                 ProcessorSet *processors,
                                 ResourceSet *resources);
};

#endif  // INCLUDE_SCHEDTEST_SCHED_TEST_FACTORY_H_
