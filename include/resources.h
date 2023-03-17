// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_RESOURCES_H_
#define INCLUDE_RESOURCES_H_

#include <types.h>
#include <set>
#include <vector>
#include <iostream>
using std::cout;
using std::endl;

class Task;
class TaskSet;
class DAG_Task;
class DAG_TaskSet;
class Processor;
class ProcessorSet;
class Param;
class Random_Gen;
class BPTask;
class BPTaskSet;
class Resource {
 private:
  uint resource_id;
  uint locality;
  Cluster cluster;
  double utilization;
  bool global_resource;
  bool processor_local_resource;

  // Request_Tasks tasks;
  // TaskQueue queue;
  set<uint> queue;
  TaskSet* tasks;
  BPTaskSet* bptasks;
  DAG_TaskSet* dag_tasks;

 public:
  explicit Resource(uint id, uint locality = MAX_INT,
                    bool global_resource = false,
                    bool processor_local_resource = false);
  ~Resource();
  void init();
  uint get_resource_id() const;
  void set_locality(uint locality);
  uint get_locality() const;
  Cluster get_cluster() const;
  void set_cluster(Cluster cluster);
  void add2cluster(uint32_t p_id);
  double get_utilization();
  bool is_global_resource();
  bool is_processor_local_resource() const;
  // Request_Tasks get_tasks() const;
  TaskSet* get_tasks();
  DAG_TaskSet* get_dag_tasks();
  set<uint>& get_taskqueue();
  void add_task(uint id);
  void remove_task(uint t_id);
  uint get_ceiling() const;
  void update(TaskSet* tasks);
  void update(BPTaskSet* tasks);
  void update(DAG_TaskSet* dag_tasks);
  /*
  TaskQueue& get_taskqueue();
  void add_task(void* taskptr);
  uint get_ceiling();
  */
  // for debug
  void display_task_queue();
};

typedef vector<Resource> Resources;

class ResourceSet {
 private:
  vector<Resource> resources;

 public:
  ResourceSet();
  void init();
  void add_resource();
  uint size() const;
  Resources& get_resources();
  Resource& get_resource_by_id(uint r_id);
  uint get_resourceset_size() const;
  void add_task(uint resource_id, uint id);
  void sort_by_utilization();
  void update(TaskSet* tasks);
  void update(BPTaskSet* tasks);
  void update(DAG_TaskSet* dag_tasks);
};

void resource_gen(ResourceSet* resourceset, Param param);

/*
 * Using next fit allocation
 */
void resource_alloc(ResourceSet *resources, uint p_num);

#endif  // INCLUDE_RESOURCES_H_
