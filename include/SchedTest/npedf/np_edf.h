#ifndef INCLUDE_SCHEDTEST_NP_EDF_H_
#define INCLUDE_SCHEDTEST_NP_EDF_H_

#include <vector>
#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

class NP_EDF : public PartitionedSched {
 protected:
  NPTaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  uint64_t  hyperperiod;

 public:
  NP_EDF();
  NP_EDF(NPTaskSet tasks, ProcessorSet processors, ResourceSet resources);
  ~NP_EDF();
  ulong currentTime;

  void Scheduler();
  void scheduler();
  bool alloc_schedulable();
  bool is_schedulable();
  void add_task(NPTask task);
  void add_period_task(NPTask task);
  void judge(NPTask task);
};
#endif  // INCLUDE_SCHEDTEST_NP_EDF_H_
