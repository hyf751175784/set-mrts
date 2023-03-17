// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_PROCESSORS_H_
#define INCLUDE_PROCESSORS_H_

#include <types.h>
#include <set>
#include <vector>

// using namespace std;
using std::set;
using std::vector;

class Task;
class TaskSet;
class DAG_Task;
class DAG_TaskSet;
class Resource;
class ResourceSet;
class Param;
class BPTask;
class BPTaskSet;

typedef struct GPU {
  uint32_t gpu_id;
  uint32_t sm_num;
  uint32_t se_sum;  // per SM
} GPU;

class Processor {
 private:
  uint cluster_id;
  uint processor_id;
  double speedfactor;
  double utilization;
  double resource_utilization;
  double density;
  bool tryed_assign;
  // TaskQueue tQueue;
  // ResourceQueue rQueue;
  set<uint> tQueue;
  set<uint> rQueue;
  BPTaskSet* bptasks;
  TaskSet* tasks;
  DAG_TaskSet* dag_tasks;
  ResourceSet* resources;
  double power;

 public:
  explicit Processor(uint id, double speedfactor = (1.0));
  ~Processor();
  uint get_processor_id() const;
  uint get_cluster_id() const;
  void set_cluster_id(uint cluster_id);
  double get_speedfactor() const;
  void set_speedfactor(double sf);
  double get_utilization();
  double get_NCS_utilization();
  double get_density();
  double get_resource_utilization();
  bool get_tryed_assign() const;
  const set<uint>& get_taskqueue();
  bool add_task(uint t_id, uint u_check = 1);
  bool remove_task(uint t_id);
  const set<uint>& get_resourcequeue();
  bool add_resource(uint r_id);
  bool remove_resource(uint r_id);
  void init();
  void update(TaskSet* tasks, ResourceSet* resources);
  void update(BPTaskSet* tasks, ResourceSet* resources);
  void update(DAG_TaskSet* dag_tasks, ResourceSet* resources);
  double get_power();
  double get_power_heterogeneous();
};

typedef vector<Processor> Processors;
typedef vector<GPU> GPUs;

class ProcessorSet {
 private:
  Processors processors;
  // GPUs
  GPUs gpus;

 public:
  ProcessorSet();
  // for identical multiprocessor platform
  ProcessorSet(uint32_t p_num);
  explicit ProcessorSet(Param param);
  uint get_processor_num() const;
  Processors& get_processors();
  GPUs& get_gpus();
  void init();
  void sort_by_task_utilization(uint dir);
  void sort_by_resource_utilization(uint dir);
  void sort_by_speedfactor(uint dir);
  void update(TaskSet* tasks, ResourceSet* resources);
  void update(BPTaskSet* tasks, ResourceSet* resources);
  void update(DAG_TaskSet* dag_tasks, ResourceSet* resources);
  double get_total_power();
  double get_total_power_heterogeneous();
};

#endif  // INCLUDE_PROCESSORS_H_
