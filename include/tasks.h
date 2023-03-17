// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_TASKS_H_
#define INCLUDE_TASKS_H_

#include <types.h>
#include <algorithm>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::vector;

class Processor;
class ProcessorSet;
class Resource;
class ResourceSet;
class Param;
class Random_Gen;


class Request {
 private:
  uint resource_id;
  uint num_requests;
  ulong max_length;
  ulong total_length;
  ulong fg_total_length;
  uint locality;
  vector<uint64_t> requests_length;  // find-graind total length
  struct sort_decrease {
    bool operator() (uint64_t i,uint64_t j) { return (i>j);}
  } sort_decrease;

 public:
  Request(uint resource_id, uint num_requests, ulong max_length,
          ulong total_length, uint locality = MAX_INT, bool FG = false, double min_ratio = 0);

  uint get_resource_id() const;
  uint get_num_requests() const;
  void set_num_requests(uint64_t num);
  ulong get_max_length() const;
  ulong get_total_length() const;
  ulong set_total_length(uint64_t length);
  ulong get_fg_total_length() const;  // find-graind total length
  ulong set_fg_total_length(uint64_t length);
  ulong get_locality() const;
  void set_locality(ulong partition);
  const vector<uint64_t> &get_requests_length() const;

};

class GPURequest {
 private:
  uint gpu_id;
  uint num_requests;
  uint32_t g_block_num;
  uint32_t g_thread_num;
  uint64_t g_wcet;
  uint64_t total_workload;

 public:
  GPURequest(uint gpu_id, uint num_requests, uint32_t g_b_num, uint32_t g_t_num, uint64_t g_wcet);

  uint get_gpu_id() const;
  uint get_num_requests() const;
  uint32_t get_g_block_num() const;
  uint32_t get_g_thread_num() const;
  uint64_t get_g_wcet() const;
  uint64_t get_total_workload() const;
};


typedef vector<Request> Resource_Requests;
typedef vector<GPURequest> GPU_Requests;
typedef vector<uint> CPU_Set;

typedef struct ArcNode {
  uint tail;  // i
  uint head;  // j
  // ArcPtr headlink;
  // ArcPtr taillink;
} ArcNode, *ArcPtr;

typedef struct VNode {
  uint job_id;
  uint type;
  uint pair;
  uint64_t wcet = 0;
  uint64_t wcet_non_critical_section = 0;
  uint64_t wcet_critical_section = 0;
  uint64_t deadline;
  uint64_t response_time;
  uint64_t RCT = 0; // relative completion time in unrestricted carry-in scenario
  uint partition;  // 0XFFFFFFFF
  uint level;
  Resource_Requests requests;
  vector<ArcPtr> precedences;
  vector<ArcPtr> follow_ups;
} VNode, *VNodePtr;

typedef struct {
  vector<VNode> v;
  vector<ArcNode> a;
} Graph;

typedef struct {
  vector<VNode> vnodes;
  uint64_t wcet = 0;
  uint64_t wcet_non_critical_section = 0;
  uint64_t wcet_critical_section = 0;
  Resource_Requests requests;
} Path;

typedef vector<Path> Paths;

class Task {
 private:
  uint model = TPS_TASK_MODEL;
  uint id;
  uint index;
  ulong wcet;
  ulong wcet_critical_sections;
  ulong wcet_non_critical_sections;
  ulong spin;
  ulong self_suspension;
  ulong local_blocking;
  ulong remote_blocking;
  ulong total_blocking;
  ulong jitter;
  ulong response_time;  // initialization as WCET
  ulong deadline;
  ulong period;
  uint priority;
  uint partition;  // 0XFFFFFFFF
  // uint cluster;
  Cluster cluster;
  CPU_Set* affinity;
  bool independent;
  bool carry_in;
  double utilization;
  double density;
  Ratio ratio;  // for heterogeneous platform
  double speedup;
  Resource_Requests requests;
  GPU_Requests g_requests;
  ulong other_attr;

 public:
/**
 * Function: Task(uint id, ulong wcet, ulong period, ulong deadline = 0,
 *                uint priority = MAX_INT);
 * Description: Create a task instance.
 * Input:
 *   id: Unique identification for a task, must not change after created.
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   wcet: The worst-case execution time of the task.
 *   period: The minimum recurrent time of the task;
 *   deadline: The relative deadline of the task, if the input equals 0, then it is identical to the peiord.
 *   priority: The priority of the task in the system.
 * Output: NULL
 * Return: NULL
 */
  Task(uint id, ulong wcet, ulong period, ulong deadline = 0,
       uint priority = MAX_INT);

/**
 * Function: Task(uint id, ResourceSet* resourceset, Param param, ulong wcet, ulong period,
 *                ulong deadline = 0, uint priority = MAX_INT);
 * Description: Create a task instance.
 * Input:
 *   id: Unique identification for a task, must not change after created.
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   param: A configuration for generating the task instance.
 *   wcet: The worst-case execution time of the task.
 *   period: The minimum recurrent time of the task;
 *   deadline: The relative deadline of the task, if the input equals 0, then it is identical to the peiord.
 *   priority: The priority of the task in the system.
 */
  Task(uint id, ResourceSet* resourceset, Param param, ulong wcet, ulong period,
       ulong deadline = 0, uint priority = MAX_INT);

/**
 * Function: Task(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
 *                ulong cs_wcet, ulong period, ulong deadline = 0,
 *                uint priority = MAX_INT);
 * Description: Create a task instance
 * Input:
 *   id: Unique identification for a task, must not change after created.
 *   r_id: A resource id that the task instance will access.
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   ncs_wcet: The worst-case execution time of the task's non-ciritcal sections.
 *   cs_wcet: The worst-case execution time of the task's ciritcal sections.
 *   period: The minimum recurrent time of the task;
 *   deadline: The relative deadline of the task, if the input equals 0, then it is identical to the peiord.
 *   priority: The priority of the task in the system.
 */
  Task(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
       ulong cs_wcet, ulong period, ulong deadline = 0,
       uint priority = MAX_INT);

/**
 * Function: ~Task();
 * Description: Destructor of a task instance.
 */
  ~Task();

/**
 * Function: init();
 * Description: Reset attributes of a task instance.
 * Return: void
 */
  void init();

/**
 * Function: task_model();
 * Description: To identify the task model.
 * Return: task model indicator
 */
  uint task_model();

/**
 * Function: get_id() const;
 * Description: Get the task id.
 * Return: task id
 */
  uint get_id() const;

/**
 * Function: set_id(uint id);
 * Description: Set the task id.
 * Input:
 *   id: A new task id.
 * Return: void
  This function should never be used.
 */
  void set_id(uint id);

/**
 * Function: get_index() const;
 * Description: Get the task index in a task set.
 * Return: task index
 */
  uint get_index() const;

/**
 * Function:set_index(uint index);
 * Description: Set the task index in a task set.
 * Input:
 *   id: A new task index.
 * Return: void
 */
  void set_index(uint index);

/**
 * Function: get_wcet() const;
 * Description: Get the worst-case execution time of the task.
 * Return: WCET
 */
  ulong get_wcet() const;

/**
 * Function: get_wcet_heterogeneous() const;
 * Description: Get the worst-case execution time of the task with regard to the processor speed.
 * Return: WCET
 */
  ulong get_wcet_heterogeneous() const;

/**
 * Function: get_deadline() const;
 * Description: Get the relative deadline of the task.
 * Return: relative deadline
 */
  ulong get_deadline() const;

/**
 * Function: get_period() const;
 * Description: Get the period (minimum recurrent time) of the task.
 * Return: period
 */
  ulong get_period() const;

/**
 * Function: get_slack() const;
 * Description: Get the slack of the task.
 * Return: slack
 */
  ulong get_slack() const;

/**
 * Function: is_feasible() const;
 * Description: To check the feasibility of the task.
 * Return:
 *   true: feasible
 *   false: unfeasible
 */
  bool is_feasible() const;


/**
 * Function: get_requests() const;
 * Description: Get a set of request descriptions to all shared resources that the task will access.
 * Return: A set of request descriptions
 */
  const Resource_Requests& get_requests() const;


/**
 * Function: get_g_requests() const;
 * Description: Get a set of request descriptions to all GPUs that the task will access.
 * Return: A set of GPU request descriptions
 */
  const GPU_Requests& get_g_requests() const;


/**
 * Function: get_request_by_id(uint id) const;
 * Description: Get a request description to a specific shared resource.
 * Input:
 *   id: A shared resource id.
 * Return: A request description
 */
  const Request& get_request_by_id(uint id) const;


/**
 * Function: get_g_request_by_id(uint id) const;
 * Description: Get a request description to a specific GPU.
 * Input:
 *   id: A GPU id.
 * Return: A GPU request description
 */
  const GPURequest& get_g_request_by_id(uint id) const;

/**
 * Function: is_request_exist(uint resource_id) const;
 * Description: To check if the task will access a specific shared resource.
 * Input:
 *   resource_id: A shared resource id.
 * Return:
 *   true: The task will access the shared resource.
 *   false: The task will not access the shared resource.
 */
  bool is_request_exist(uint resource_id) const;

/**
 * Function: is_g_request_exist(uint gpu_id) const;
 * Description: To check if the task will access a specific GPU.
 * Input:
 *   resource_id: A GPU id.
 * Return:
 *   true: The task will access the GPU.
 *   false: The task will not access the GPU.
 */
  bool is_g_request_exist(uint gpu_id) const;

/**
 * Function: update_requests(ResourceSet resources);
 * Description: To update attributes of a shared resource that the task will access, i.e. resource locality.
 * Input:
 *   resources: A set of all shared resources of the system.
 * Return: void
 */
  void update_requests(ResourceSet resources);


/**
 * Function: get_wcet_critical_sections() const;
 * Description: Get the worst-case execution time of the task's critical sections.
 * Return: WCET
 */
  ulong get_wcet_critical_sections() const;

/**
 * Function: get_wcet_critical_sections_heterogeneous() const;
 * Description: Get the worst-case execution time of the task's critical sections with regards to the processor speed.
 * Return: WCET
 */
  ulong get_wcet_critical_sections_heterogeneous() const;

/**
 * Function: set_wcet_critical_sections(ulong csl);
 * Description: Set the worst-case execution time of the task's critical sections.
 * Input:
 *   csl: A new critical section length.
 * Return: void
 */
  void set_wcet_critical_sections(ulong csl);

/**
 * Function: get_wcet_non_critical_sections() const;
 * Description: Get the worst-case execution time of the task's non-critical sections.
 * Return: WCET
 */
  ulong get_wcet_non_critical_sections() const;

/**
 * Function: get_wcet_non_critical_sections_heterogeneous() const;
 * Description: Get the worst-case execution time of the task's non-critical sections with regards to the processor speed.
 * Return: WCET
 */
  ulong get_wcet_non_critical_sections_heterogeneous() const;

/**
 * Function: set_wcet_non_critical_sections(ulong csl);
 * Description: Set the worst-case execution time of the task's non-critical sections.
 * Input:
 *   csl: A new critical section length.
 * Return: void
 */
  void set_wcet_non_critical_sections(ulong ncsl);

/**
 * Function: get_spin() const;
 * Description: Get the total blocking time with spin locks.
 * Return: total blocking time with spin locks
 * Note: This function is specialized for the analysis of RTA_PFP_WF_spinlock & RTA_PFP_WF_spinlock_heterogeneous.
 */
  ulong get_spin() const;

/**
 * Function: set_spin(ulong spining);
 * Description: Set the total blocking time with spin locks.
 * Input:
 *   spining: A total blocking time with spin locks.
 * Return: void
 * Note: This function is specialized for the analysis of RTA_PFP_WF_spinlock & RTA_PFP_WF_spinlock_heterogeneous.
 */
  void set_spin(ulong spining);

/**
 * Function: get_local_blocking() const;
 * Description: Get the local blocking time.
 * Return: local blocking time
 */
  ulong get_local_blocking() const;

/**
 * Function: set_local_blocking(ulong lb);
 * Description: Set the local blocking time.
 * Input:
 *   lb: A local blocking time.
 * Return: void
 */
  void set_local_blocking(ulong lb);

/**
 * Function: get_remote_blocking() const;
 * Description: Get the remote blocking time.
 * Return: remote blocking time
 */
  ulong get_remote_blocking() const;

/**
 * Function: set_remote_blocking(ulong rb);
 * Description: Set the remote blocking time.
 * Input:
 *   rb: A remote blocking time.
 * Return: void
 */
  void set_remote_blocking(ulong rb);

/**
 * Function: get_total_blocking() const;
 * Description: Get the total blocking time.
 * Return: total blocking time
 */
  ulong get_total_blocking() const;

/**
 * Function: set_total_blocking(ulong tb);
 * Description: Set the total blocking time.
 * Input:
 * tb: A total blocking time.
 * Return: void
 */
  void set_total_blocking(ulong tb);

/**
 * Function: get_self_suspension() const;
 * Description: Get the self suspension time.
 * Return: self suspension time
 */
  ulong get_self_suspension() const;

/**
 * Function: set_self_suspension(ulong ss);
 * Description: Set the self suspension time.
 * Input:
 *   ss: A self suspension time.
 * Return: void
 */
  void set_self_suspension(ulong ss);

/**
 * Function: get_jitter() const;
 * Description: Get the jitter of the task.
 * Return: task jitter
 */
  ulong get_jitter() const;

/**
 * Function: set_jitter(ulong jit);
 * Description: Set the jitter of the task.
 * Input:
 *   jit: The length of a jitter.
 * Return: void
 */
  void set_jitter(ulong jit);

/**
 * Function: get_response_time() const;
 * Description: Get the worst-case response time of the task.
 * Return: worst-case response time
 */
  ulong get_response_time() const;

/**
 * Function: set_response_time(ulong response);
 * Description: Set the worst-case response time of the task.
 * Input:
 *   response: The worst-case response time
 * Return: void
 */
  void set_response_time(ulong response);

/**
 * Function: get_priority() const;
 * Description: Get the priority of the task.
 * Return: task prioirty
 */
  uint get_priority() const;

/**
 * Function: set_priority(uint prio);
 * Description: Set the priority of the task.
 * Input:
 *   prio: A task prioirty.
 * Return: void
 */
  void set_priority(uint prio);

/**
 * Function: get_partition() const;
 * Description: Get the partition (processor assignment) of the task.
 * Return: processor id
 */
  uint get_partition() const;

/**
 * Function: set_partition(uint cpu, double speedup = 1);
 * Description: Set the partition (processor assignment) of the task and the speedup factor of the assigned processor.
 * Input:
 *   p_id: A local blocking time.
 *   speedup: A speedup factor of the processor, speedup factor = 1 as default.
 * Return: void
 */
  void set_partition(uint p_id, double speedup = 1);

/**
 * Function: get_cluster() const;
 * Description: Get the partition (processor assignment) of the task.
 * Return: a set of processors
 */
  Cluster get_cluster() const;

/**
 * Function: set_cluster(Cluster cluster, double speedup = 1);
 * Description: Set the partition (processor assignment) of the task and the speedup factor of the assigned processor.
 * Input:
 *   p_id: A local blocking time.
 *   speedup: A speedup factor of the processors, speedup factor = 1 as default.
 * Return: void
 */
  void set_cluster(Cluster cluster, double speedup = 1);

  void add2cluster(uint32_t p_id, double speedup = 1);

  bool is_in_cluster(uint32_t p_id) const;

/**
 * Function: get_ratio() const;
 * Description: Get the speed of a specific processor.
 * Input:
 *   p_id: A specific processor id. 
 * Return: speedup factor
 */
  double get_ratio(uint p_id) const;

/**
 * Function: set_ratio(uint p_id, double speed);
 * Description: Set the speed of a specific processor.
 * Input:
 *   p_id: A specific processor id. 
 *   speed: A speedup factor.
 * Return: void
 */
  void set_ratio(uint p_id, double speed);

/**
 * Function: get_cluster() const;
 * Description: Get the cluster of the task.
 * Return: clustur index
 */
  // uint get_cluster() const;

/**
 * Function: set_cluster(uint clu);
 * Description: Set the cluster of the task.
 * Input:
 *   clu: A clustur index.
 * Return: void
 */
  // void set_cluster(uint clu);

/**
 * Function: get_affinity() const;
 * Description: Get the affinity of the task.
 * Return: affinities to processors
 */
  CPU_Set* get_affinity() const;

/**
 * Function: set_affinity(CPU_Set* affi);
 * Description: Set the affinity of the task.
 * Input:
 *   affi: Affinities to processors
 * Return: void
 */
  void set_affinity(CPU_Set* affi);

/**
 * Function: is_independent() const;
 * Description: To check if the task is independent.
 * Return:
 *   true: Independent.
 *   false: Dependent.
 */
  bool is_independent() const;

/**
 * Function: set_dependent();
 * Description: Set the task as a dependent task.
 * Return: void
 */
  void set_dependent();

/**
 * Function: is_carry_in() const;
 * Description: To check if an instance of the task should be counted as a carry in job.
 * Return:
 *   true: carry in job
 *   false: not carry in job
 */
  bool is_carry_in() const;

/**
 * Function: set_carry_in();
 * Description: Set as a carry in job.
 * Return: void
 */
  void set_carry_in();

/**
 * Function: clear_carry_in();
 * Description: Set as a non-carry in job.
 * Return: void
 */
  void clear_carry_in();

/**
 * Function: get_other_attr() const;
 * Description: Get the reserved attributes.
 * Return: reserved attributes
 */
  ulong get_other_attr() const;

/**
 * Function: set_local_blocking(ulong lb);
 * Description: Set the local blocking time.
 * Input:
 *   spining: A local blocking time.
 * Return: void
 */
  void set_other_attr(ulong attr);


/**
 * Function: add_request(uint r_id, uint num, ulong max_len, ulong total_len,
 * uint locality = MAX_INT);
 * Description: To add a request description to a specific shared resource.
 * Input:
 *   r_id: A shared resource id.
 *   num: The number of request to the shared resource.
 *   max_len: The worst-case execution length of a shared resource request.
 *   total_len: The cummulative length of requests to the shared resource.
 *   locality: The partition (assignment, if exists) of the shared resource, set to 0xffffffff as default.
 * Return: void
 */
  void add_request(uint r_id, uint num, ulong max_len, ulong total_len,
                   uint locality = MAX_INT);

/**
 * Function: add_request(ResourceSet* resources, uint r_id, uint num,
 *                       ulong max_len);
 * Description: To add a request description to a specific shared resource.
 * Input:
 *   resources: A set of shared resources.
 *   r_id: A shared resource id.
 *   num: The number of request to the shared resource.
 *   max_len: The worst-case execution length of a shared resource request.
 * Return: void
 */
  void add_request(ResourceSet* resources, uint r_id, uint num,
                   ulong max_len);

  /**
   * Function: get_max_job_num(ulong interval) const;
   *   Description: Get the maxmum number of the task instance during a
   * consecutive interval. Input: interval: The length of a consecutive
   * interval. Return: instance number
   */
  uint get_max_job_num(ulong interval) const;

/**
 * Function: get_max_request_num(uint resource_id, ulong interval) const;
 * Description: Get the maxmum number of requests to a specific shared resource during a consecutive interval.
 * Input:
 *   resource_id: A shared resource id.
 *   interval: The length of a consecutive interval.
 * Return: request number
 */
  uint get_max_request_num(uint resource_id, ulong interval) const;



/**
 * Function: get_max_request_num_np(uint resource_id, ulong interval) const;
 * Description: Get the maxmum number of requests to a specific shared resource during a consecutive interval (use deadline instead of response time).
 * Input:
 *   resource_id: A shared resource id.
 *   interval: The length of a consecutive interval.
 * Return: request number
 */
  uint get_max_request_num_nu(uint resource_id, ulong interval) const;

/**
 * Function: get_utilization() const;
 * Description: Get the utilization of the task.
 * Return: utilization
 */
  double get_utilization() const;

/**
 * Function: get_density() const;
 * Description: Get the density of the task.
 * Return: density
 */
  double get_density() const;

/**
 * Function: get_NCS_utilization() const;
 * Description: Get the utilization of the task's non-critical sections.
 * Return: utilization (non-critical sections)
 */
  double get_NCS_utilization() const;
};

typedef vector<Task> Tasks;

class TaskSet {
 private:
  Tasks tasks;
  double utilization_sum;
  double utilization_max;
  double density_sum;
  double density_max;

 public:
/**
 * Function: TaskSet();
 * Description: Create an empty task set.
 */
  TaskSet();

/**
 * Function: ~TaskSet();
 * Description: Destructor of a task set.
 */
  ~TaskSet();


/**
 * Function: init();
 * Description: Reset attributes of all tasks in the task set.
 * Return: void
 */
  void init();

/**
 * Function: add_task(Task task);
 * Description: To add a task to the task set.
 * Input:
 *   task: A task instance.
 * Return: void
 */
  void add_task(Task task);

/**
 * Function: add_task(ulong wcet, ulong period, ulong deadline = 0);
 * Description: To add a task to the task set.
 * Input:
 *   wcet: The worst-case execution time of the added task.
 *   peiord: The period of the added task.
 *   deadline: The relative deadline of the added task, if set to 0, then it is identical to the period.
 * Return: void
 */
  void add_task(ulong wcet, ulong period, ulong deadline = 0);

/**
 * Function: add_task(ResourceSet* resourceset, Param param, ulong wcet, ulong period,
            ulong deadline = 0);
 * Description: To add a task to the task set.
 * Input:
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   param: A configuration for generating a task instance.
 *   wcet: The worst-case execution time of the added task.
 *   peiord: The period of the added task.
 *   deadline: The relative deadline of the added task, if set to 0, then it is identical to the period.
 * Return: void
 */
  void add_task(ResourceSet* resourceset, Param param, ulong wcet, ulong period,
                ulong deadline = 0);

/**
 * Function: add_task(uint r_id, ResourceSet* resourceset, Param param, long ncs_wcet,
            long cs_wcet, long period, long deadline = 0);
 * Description: To add a task to the task set.
 * Input:
 *   r_id:
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   param: A configuration for generating a task instance.
 *   ncs_wcet: The worst-case execution time of the task's non-ciritcal sections.
 *   cs_wcet: The worst-case execution time of the task's ciritcal sections.
 *   peiord: The period of the added task.
 *   deadline: The relative deadline of the added task, if set to 0, then it is identical to the period.
 * Return: void
 */
  void add_task(uint r_id, ResourceSet* resourceset, Param param,
                uint64_t ncs_wcet, uint64_t cs_wcet, uint64_t period,
                uint64_t deadline = 0);

/**
 * Function: add_task(ResourceSet* resourceset, string bufline);
 * Description: To add a task from a bufline.
 * Input:
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   bufline: Bufline with task parameters.
 * Return: void
 */
  void add_task(ResourceSet* resourceset, string bufline);


/**
 * Function: get_tasks();
 * Description: Get a vector of tasks.
 * Return: tasks
 */
  Tasks& get_tasks();

/**
 * Function: get_task_by_id(uint id);
 * Description: Get a task instance by its id.
 * Input:
 *   id: Task id.
 * Return: task instance
 */
  Task& get_task_by_id(uint id);

/**
 * Function: get_task_by_index(uint index);
 * Description: Get a task instance by its index.
 * Input:
 *   index: Task index in the vector.
 * Return: task instance
 */
  Task& get_task_by_index(uint index);

/**
 * Function: get_task_by_priority(uint pi);
 * Description: Get a task instance by its priority.
 * Input:
 *   pi: Task priority.
 * Return: task instance
 */
  Task& get_task_by_priority(uint pi);

/**
 * Function: is_implicit_deadline();
 * Description: To check if it is implicit deadline task set.
 * Return:
 *   true: It is implicit deadline task set.
 *   false: It is not implicit deadline task set.
 */
  bool is_implicit_deadline();

/**
 * Function: is_constrained_deadline();
 * Description: To check if it is constrained deadline task set.
 * Return:
 *   true: It is constrained deadline task set.
 *   false: It is not constrained deadline task set.
 */
  bool is_constrained_deadline();

/**
 * Function: is_arbitary_deadline();
 * Description: To check if it is arbitary deadline task set.
 * Return:
 *   true: It is arbitary deadline task set.
 *   false: It is not arbitary deadline task set.
 */
  bool is_arbitary_deadline();

/**
 * Function: get_taskset_size() const;
 * Description: Get the number of tasks in the task set.
 * Return: task number
 */
  uint get_taskset_size() const;


/**
 * Function: get_task_utilization(uint index) const;
 * Description: Get the utilization of a specific task.
 * Input:
 * index: Task index in the vector.
 * Return: task utilization
 */
  double get_task_utilization(uint index) const;

/**
 * Function: get_task_density(uint index) const;
 * Description: Get the density of a specific task.
 * Input:
 * index: Task index in the vector.
 * Return: task density
 */
  double get_task_density(uint index) const;

/**
 * Function: get_task_wcet(index) const;
 * Description: Get the worst-case execution time of a specific task.
 * Input:
 * index: Task index in the vector.
 * Return: WCET
 */
  ulong get_task_wcet(uint index) const;

/**
 * Function: get_task_deadline(index) const;
 * Description: Get the relative deadline of a specific task.
 * Input:
 *   index: Task index in the vector.
 * Return: relative deadline
 */
  ulong get_task_deadline(uint index) const;

/**
 * Function: get_task_period(index) const;
 * Description: Get the period of a specific task.
 * Input:
 *   index: Task index in the vector.
 * Return: period
 */
  ulong get_task_period(uint index) const;

/**
 * Function: get_utilization_sum() const;
 * Description: Get the total utilization of the task set.
 * Return: total utilization
 */
  double get_utilization_sum() const;

/**
 * Function: get_utilization_max() const;
 * Description: Get the maximum utilization of the task set.
 * Return: maximum utilization
 */
  double get_utilization_max() const;

/**
 * Function: get_density_sum() const;
 * Description: Get the total density of the task set.
 * Return: total density
 */
  double get_density_sum() const;

/**
 * Function: get_density_max() const;
 * Description: Get the maximum density of the task set.
 * Return: maximum density
 */
  double get_density_max() const;


// Following functions sort the task set vector in specific orders
/**
 * Function: sort_by_id();
 * Description: Sort the task set in the order of increasing task id.
 * Return: void
 */
  void sort_by_id();
  void sort_by_index();
  void sort_by_period();       // increase
  void sort_by_deadline();     // increase
  void sort_by_utilization();  // decrease
  void sort_by_density();      // decrease
  void sort_by_DC();
  void sort_by_DCC();
  void sort_by_DDC();
  void sort_by_UDC();
  void RM_Order();
  void DM_Order();
  void Density_Decrease_Order();
  void DC_Order();
  void DCC_Order();
  void DDC_Order();
  void UDC_Order();
  void SM_PLUS_Order();
  void SM_PLUS_2_Order();
  void SM_PLUS_3_Order();
  void Leisure_Order();
  void SM_PLUS_4_Order(uint p_num);
  ulong leisure(uint index);

/**
 * Function: display();
 * Description: Output selected attributes of all tasks to the console.
 * Return: void
 */
  void display();

/**
 * Function: update_requests(const ResourceSet& resoruces);
 * Description: To update all tasks' requests.
 * Input:
 *   resources: A set of all shared resources of the system.
 * Return: void
 */
  void update_requests(const ResourceSet& resoruces);

/**
 * Function: export_taskset(string path);
 * Description: Export the task set to a file.
 * Input:
 * path: The path of the output file.
 * Return: void
 */
  void export_taskset(string path);

/**
 * Function: task_gen(ResourceSet* resourceset, Param param, double utilization,
 *                    uint task_number);
 * Description: Task generation.
 * Input:
 *   resources: A set of all shared resources of the system.
 *   param: A configuration for generating a task instance.
 *   utilization: The designated total utilization.
 *   task_number: The designated task number.
 * Return: void
 */
  void task_gen(ResourceSet* resourceset, Param param, double utilization,
                uint task_number);

/**
 * Function: task_load(ResourceSet* resourceset, string file_name);
 * Description: Load task set from a file.
 * Input:
 *   resources: A set of all shared resources of the system.
 *   file_name: The path of the input file.
 * Return: void
 */
  void task_load(ResourceSet* resourceset, string file_name);
};


// class DAG_Task : public Task {
class DAG_Task {
 private:
  uint model = DAG_TASK_MODEL;
  uint id;
  uint index;
  vector<VNode> vnodes;
  vector<ArcNode> arcnodes;
  ulong wcet;  // total wcet of the jobs in graph
  ulong wcet_non_critical_sections;
  ulong wcet_critical_sections;
  ulong len;
  ulong CPL;
  ulong deadline;
  ulong period;
  uint parallel_degree;
  double utilization;
  double density;
  uint vexnum;
  uint arcnum;
  ulong spin;
  ulong self_suspension;
  ulong local_blocking;
  ulong remote_blocking;
  ulong total_blocking;
  ulong jitter;
  ulong response_time;  // initialization as WCET
  uint priority;
  uint partition;  // 0XFFFFFFFF
  Cluster cluster;
  Ratio ratio;     // for heterogeneous platform
  uint dcores;
  double speedup;
  Resource_Requests requests;
  ulong other_attr;
  vector<int> visited;
  vector<int> match;

 public:
  DAG_Task(const DAG_Task& dt);
  DAG_Task(uint task_id, ulong period, ulong deadline = 0, uint priority = 0);
  DAG_Task(uint task_id, ResourceSet* resourceset, Param param,
           uint priority = 0);
  DAG_Task(uint task_id, ResourceSet* resourceset, Param param,
           double utilization, uint priority = 0);
  DAG_Task(uint task_id, ResourceSet* resourceset, Param param, ulong wcet,
           ulong period, ulong deadline = 0, uint priority = 0);
  DAG_Task(uint task_id, ResourceSet* resourceset, Param param, double cpp,
           ulong wcet, ulong period, ulong deadline = 0, uint priority = 0);
  void graph_gen(vector<VNode>* v, vector<ArcNode>* a, Param param, uint n_num,
                 double arc_density = 0.6);
  void sub_graph_gen(vector<VNode>* v, vector<ArcNode>* a, uint n_num,
                     int G_TYPE = G_TYPE_P);
  void sequential_graph_gen(vector<VNode>* v, vector<ArcNode>* a, uint n_num);
  void graph_insert(vector<VNode>* v, vector<ArcNode>* a, uint replace_node);
  void Erdos_Renyi(uint v_num, double e_prob);
  void Erdos_Renyi_v2(uint v_num, double e_prob);

  void init();

  uint task_model();

  uint get_id() const;
  void set_id(uint id);
  uint get_index() const;
  void set_index(uint index);
  uint get_vnode_num() const;
  uint get_arcnode_num() const;


  const Resource_Requests& get_requests() const;
  const Request& get_request_by_id(uint id) const;
  bool is_request_exist(uint resource_id) const;
  void add_request(uint res_id, uint num, ulong max_len, ulong total_len,
                   uint locality = MAX_INT, bool FG = false, double mean = 0.5);
  uint get_max_job_num(ulong interval) const;  // for sequential tasks
  uint get_max_request_num(uint resource_id,
                           ulong interval) const;  // for sequential tasks
  uint get_max_request_num_nu(uint resource_id, ulong interval) const;

  const vector<VNode>& get_vnodes() const;
  const VNode& get_vnode_by_id(uint job_id) const;
  const vector<ArcNode>& get_arcnodes() const;
  ulong get_deadline() const;
  ulong get_period() const;
  ulong set_period(uint64_t period);
  ulong get_wcet() const;
  ulong get_wcet_critical_sections() const;
  void set_wcet_critical_sections(ulong csl);
  ulong get_wcet_non_critical_sections() const;
  void set_wcet_non_critical_sections(ulong ncsl);
  ulong get_len() const;
  bool is_connected(VNode i, VNode j) const;
  void update_parallel_degree();
  uint get_parallel_degree() const;
  double get_utilization() const;
  double get_density() const;
  uint get_priority() const;
  void set_priority(uint prio);
  uint get_partition() const;
  void set_partition(uint cpu, double speedup = 1);
  Cluster get_cluster() const;
  void set_cluster(Cluster cluster, double speedup = 1);
  void add2cluster(uint32_t p_id, double speedup = 1);
  bool is_in_cluster(uint32_t p_id) const;
  ulong get_response_time() const;
  void set_response_time(ulong response);
  uint get_dcores() const;
  void set_dcores(uint n);
  ulong get_other_attr() const;
  void set_other_attr(ulong attr);
  void add_job(ulong wcet, ulong deadline = 0);
  void add_job(vector<VNode>* v, ulong wcet, ulong deadline = 0);
  void add_arc(uint tail, uint head);
  void add_arc(vector<ArcNode>* a, uint tail, uint head);
  void delete_arc(uint tail, uint head);
  void refresh_relationship();
  void refresh_relationship(vector<VNode>* v, vector<ArcNode>* a);
  void update_wcet();
  void update_len();
  void update_len_2();
  bool is_acyclic();
  double get_NCS_utilization() const;

  ulong DFS(VNode vnode) const;  // Depth First Search
  ulong BFS(VNode vnode) const;  // Breath First Search

  bool is_arc_exist(uint tail, uint head) const;
  bool is_arc_exist(const vector<ArcNode>& a, uint tail, uint head);

  void display();
  void display_vertices();
  void display_vertices(const vector<VNode>& v);
  void display_arcs();
  void display_arcs(const vector<ArcNode>& a);
  void display_follow_ups(uint job_id);
  void display_precedences(uint job_id);
  uint get_indegrees(uint job_id) const;
  uint get_outdegrees(uint job_id) const;
  void display_in_dot();

  vector<VNode> get_critical_path();
  uint64_t get_path_length(vector<VNode> path);
  uint64_t get_critical_path_length() const;

  bool MMDFS(Graph *graph, VNode *vnode);  // Maximum Match DFS

  int maximumMatch(Graph *graph);

  Graph toBPGraph();

  void Floyd();
  void unFloyd();
  Paths EP_DFS(VNode vnode) const;
  // void EP_DFS(VNode vnode, Paths *paths, uint64_t wcet, Resource_Requests requests);
  Paths get_paths() const;
/**
 * Function: update_requests(ResourceSet resources);
 * Description: To update attributes of a shared resource that the task will access, i.e. resource locality.
 * Input:
 *   resources: A set of all shared resources of the system.
 * Return: void
 */
  void update_requests(ResourceSet resources);

 /**
 * Function: calculate_RCT();
 * Description: To calculate the relative completion time for each node.
 * Input:
 * Return: void
 */ 
  void calculate_RCT();
  uint64_t calculate_RCT(uint32_t job_id);
  uint64_t get_RCT(uint32_t job_id) const;
};



typedef vector<DAG_Task> DAG_Tasks;

class DAG_TaskSet {
 private:
  DAG_Tasks dag_tasks;
  double utilization_sum;
  double utilization_max;
  double density_sum;
  double density_max;

 public:
  DAG_TaskSet();
  ~DAG_TaskSet();

  void init();



  void add_task(ResourceSet* resourceset, Param param);
  void add_task(ResourceSet* resourceset, Param param, double utilization);
  void add_task(ResourceSet* resourceset, Param param, ulong wcet, ulong period,
                ulong deadline = 0);
  void add_task(ResourceSet* resourceset, Param param, double cpp, ulong wcet,
                ulong period, ulong deadline = 0);
  void add_task(DAG_Task dag_task);

  void update_requests(const ResourceSet& resoruces);

  DAG_Tasks& get_tasks();
  DAG_Task& get_task_by_id(uint id);
  uint get_taskset_size() const;

  double get_utilization_sum() const;
  double get_utilization_max() const;
  double get_density_sum() const;
  double get_density_max() const;

  void sort_by_period();
  void RM_Order();
  void DM_Order();

  void task_gen(ResourceSet* resourceset, Param param, double utilization,
                uint task_number);
  void task_gen_v2(ResourceSet* resourceset, Param param, double utilization,
                uint task_number);
  void task_gen(ResourceSet* resourceset, Param param, uint task_number);
  void task_load(ResourceSet* resourceset, string file_name);

  void display();
};

// The task generation functions beneath are deprecated.

void task_gen(TaskSet* taskset, ResourceSet* resourceset, Param param,
              double utilization);

void task_gen_UUnifast_Discard(TaskSet* taskset, ResourceSet* resourceset,
                               Param param, double utilization);

void task_gen_UUnifast_Discard(TaskSet* taskset, ResourceSet* resourceset,
                               Param param, uint taskset_size,
                               double utilization);

void task_load(TaskSet* taskset, ResourceSet* resourceset, string file_name);

void dag_task_gen(DAG_TaskSet* dag_taskset, ResourceSet* resourceset,
                  Param param, double utilization);

void dag_task_gen_UUnifast_Discard(DAG_TaskSet* dag_taskset,
                                   ResourceSet* resourceset, Param param,
                                   double utilization);

void dag_task_gen_UUnifast_Discard(DAG_TaskSet* dag_taskset,
                                   ResourceSet* resourceset, Param param,
                                   uint taskset_size, double utilization);

void dag_task_gen_RandFixedSum(DAG_TaskSet *dag_taskset,
                                   ResourceSet *resourceset, Param param,
                                   uint taskset_size, double utilization);

uint32_t gcd(uint32_t a, uint32_t b);

template <typename Format> Format GCD(Format a, Format b);
template <typename Format> Format LCM(Format a, Format b);



/*2022 by wyg BPtasks*/


class BPTask {
 private:
  uint model = TPS_TASK_MODEL;
  uint id;
  uint index;
  ulong wcet;
  ulong wcet_critical_sections;
  ulong wcet_non_critical_sections;
  ulong spin;
  ulong self_suspension;
  ulong local_blocking;
  ulong remote_blocking;
  ulong total_blocking;
  ulong jitter;
  ulong response_time;  // initialization as WCET
  ulong deadline;
  ulong period;
  uint priority;
  uint partition;  // 0XFFFFFFFF
  // uint cluster;
  Cluster cluster;
  CPU_Set* affinity;
  bool independent;
  bool status;                 //1 for Primary   0 for  backpack 2022/1
  bool carry_in;
  double utilization;
  double density;
  Ratio ratio;  // for heterogeneous platform
  double speedup;
  Resource_Requests requests;
  GPU_Requests g_requests;
  ulong other_attr;
  uint linkID;              //2022/1
 public:
 
 BPTask::BPTask(uint id, ulong wcet, ulong period, ulong deadline,bool status=true);
/**
 * Function: BPTask(uint id, ulong wcet, ulong period, ulong deadline = 0,
 *                uint priority = MAX_INT);
 * Description: Create a task instance.
 * Input:
 *   id: Unique identification for a task, must not change after created.
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   wcet: The worst-case execution time of the task.
 *   period: The minimum recurrent time of the task;
 *   deadline: The relative deadline of the task, if the input equals 0, then it is identical to the peiord.
 *   priority: The priority of the task in the system.
 * Output: NULL
 * Return: NULL
 */
 BPTask::BPTask(uint id, ulong wcet, ulong period, ulong deadline, uint priority,bool status=true);
 
 uint get_linkID() const;
 void set_linkID(uint linkID) const;
 bool get_status() const;
 void set_status(bool status) const;



/**
 * Function: BPTask(uint id, ResourceSet* resourceset, Param param, ulong wcet, ulong period,
 *                ulong deadline = 0, uint priority = MAX_INT);
 * Description: Create a task instance.
 * Input:
 *   id: Unique identification for a task, must not change after created.
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   param: A configuration for generating the task instance.
 *   wcet: The worst-case execution time of the task.
 *   period: The minimum recurrent time of the task;
 *   deadline: The relative deadline of the task, if the input equals 0, then it is identical to the peiord.
 *   priority: The priority of the task in the system.
 */
  BPTask(uint id, ResourceSet* resourceset, Param param, ulong wcet, ulong period,
       ulong deadline = 0, uint priority = MAX_INT,bool status=true);

/**
 * Function: BPTask(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
 *                ulong cs_wcet, ulong period, ulong deadline = 0,
 *                uint priority = MAX_INT);
 * Description: Create a task instance
 * Input:
 *   id: Unique identification for a task, must not change after created.
 *   r_id: A resource id that the task instance will access.
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   ncs_wcet: The worst-case execution time of the task's non-ciritcal sections.
 *   cs_wcet: The worst-case execution time of the task's ciritcal sections.
 *   period: The minimum recurrent time of the task;
 *   deadline: The relative deadline of the task, if the input equals 0, then it is identical to the peiord.
 *   priority: The priority of the task in the system.
 */
  BPTask(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
       ulong cs_wcet, ulong period, ulong deadline = 0,
       uint priority = MAX_INT,bool status=true);

/**
 * Function: ~BPTask();
 * Description: Destructor of a task instance.
 */
  ~BPTask();

/**
 * Function: init();
 * Description: Reset attributes of a task instance.
 * Return: void
 */
  void init();

/**
 * Function: task_model();
 * Description: To identify the task model.
 * Return: task model indicator
 */
  uint task_model();

/**
 * Function: get_id() const;
 * Description: Get the task id.
 * Return: task id
 */
  uint get_id() const;

/**
 * Function: set_id(uint id);
 * Description: Set the task id.
 * Input:
 *   id: A new task id.
 * Return: void
  This function should never be used.
 */
  void set_id(uint id);

/**
 * Function: get_index() const;
 * Description: Get the task index in a task set.
 * Return: task index
 */
  uint get_index() const;

/**
 * Function:set_index(uint index);
 * Description: Set the task index in a task set.
 * Input:
 *   id: A new task index.
 * Return: void
 */
  void set_index(uint index);

/**
 * Function: get_wcet() const;
 * Description: Get the worst-case execution time of the task.
 * Return: WCET
 */
  ulong get_wcet() const;

/**
 * Function: get_wcet_heterogeneous() const;
 * Description: Get the worst-case execution time of the task with regard to the processor speed.
 * Return: WCET
 */
  ulong get_wcet_heterogeneous() const;

/**
 * Function: get_deadline() const;
 * Description: Get the relative deadline of the task.
 * Return: relative deadline
 */
  ulong get_deadline() const;

/**
 * Function: get_period() const;
 * Description: Get the period (minimum recurrent time) of the task.
 * Return: period
 */
  ulong get_period() const;

/**
 * Function: get_slack() const;
 * Description: Get the slack of the task.
 * Return: slack
 */
  ulong get_slack() const;

/**
 * Function: is_feasible() const;
 * Description: To check the feasibility of the task.
 * Return:
 *   true: feasible
 *   false: unfeasible
 */
  bool is_feasible() const;


/**
 * Function: get_requests() const;
 * Description: Get a set of request descriptions to all shared resources that the task will access.
 * Return: A set of request descriptions
 */
  const Resource_Requests& get_requests() const;


/**
 * Function: get_g_requests() const;
 * Description: Get a set of request descriptions to all GPUs that the task will access.
 * Return: A set of GPU request descriptions
 */
  const GPU_Requests& get_g_requests() const;


/**
 * Function: get_request_by_id(uint id) const;
 * Description: Get a request description to a specific shared resource.
 * Input:
 *   id: A shared resource id.
 * Return: A request description
 */
  const Request& get_request_by_id(uint id) const;


/**
 * Function: get_g_request_by_id(uint id) const;
 * Description: Get a request description to a specific GPU.
 * Input:
 *   id: A GPU id.
 * Return: A GPU request description
 */
  const GPURequest& get_g_request_by_id(uint id) const;

/**
 * Function: is_request_exist(uint resource_id) const;
 * Description: To check if the task will access a specific shared resource.
 * Input:
 *   resource_id: A shared resource id.
 * Return:
 *   true: The task will access the shared resource.
 *   false: The task will not access the shared resource.
 */
  bool is_request_exist(uint resource_id) const;

/**
 * Function: is_g_request_exist(uint gpu_id) const;
 * Description: To check if the task will access a specific GPU.
 * Input:
 *   resource_id: A GPU id.
 * Return:
 *   true: The task will access the GPU.
 *   false: The task will not access the GPU.
 */
  bool is_g_request_exist(uint gpu_id) const;

/**
 * Function: update_requests(ResourceSet resources);
 * Description: To update attributes of a shared resource that the task will access, i.e. resource locality.
 * Input:
 *   resources: A set of all shared resources of the system.
 * Return: void
 */
  void update_requests(ResourceSet resources);


/**
 * Function: get_wcet_critical_sections() const;
 * Description: Get the worst-case execution time of the task's critical sections.
 * Return: WCET
 */
  ulong get_wcet_critical_sections() const;

/**
 * Function: get_wcet_critical_sections_heterogeneous() const;
 * Description: Get the worst-case execution time of the task's critical sections with regards to the processor speed.
 * Return: WCET
 */
  ulong get_wcet_critical_sections_heterogeneous() const;

/**
 * Function: set_wcet_critical_sections(ulong csl);
 * Description: Set the worst-case execution time of the task's critical sections.
 * Input:
 *   csl: A new critical section length.
 * Return: void
 */
  void set_wcet_critical_sections(ulong csl);

/**
 * Function: get_wcet_non_critical_sections() const;
 * Description: Get the worst-case execution time of the task's non-critical sections.
 * Return: WCET
 */
  ulong get_wcet_non_critical_sections() const;

/**
 * Function: get_wcet_non_critical_sections_heterogeneous() const;
 * Description: Get the worst-case execution time of the task's non-critical sections with regards to the processor speed.
 * Return: WCET
 */
  ulong get_wcet_non_critical_sections_heterogeneous() const;

/**
 * Function: set_wcet_non_critical_sections(ulong csl);
 * Description: Set the worst-case execution time of the task's non-critical sections.
 * Input:
 *   csl: A new critical section length.
 * Return: void
 */
  void set_wcet_non_critical_sections(ulong ncsl);

/**
 * Function: get_spin() const;
 * Description: Get the total blocking time with spin locks.
 * Return: total blocking time with spin locks
 * Note: This function is specialized for the analysis of RTA_PFP_WF_spinlock & RTA_PFP_WF_spinlock_heterogeneous.
 */
  ulong get_spin() const;

/**
 * Function: set_spin(ulong spining);
 * Description: Set the total blocking time with spin locks.
 * Input:
 *   spining: A total blocking time with spin locks.
 * Return: void
 * Note: This function is specialized for the analysis of RTA_PFP_WF_spinlock & RTA_PFP_WF_spinlock_heterogeneous.
 */
  void set_spin(ulong spining);

/**
 * Function: get_local_blocking() const;
 * Description: Get the local blocking time.
 * Return: local blocking time
 */
  ulong get_local_blocking() const;

/**
 * Function: set_local_blocking(ulong lb);
 * Description: Set the local blocking time.
 * Input:
 *   lb: A local blocking time.
 * Return: void
 */
  void set_local_blocking(ulong lb);

/**
 * Function: get_remote_blocking() const;
 * Description: Get the remote blocking time.
 * Return: remote blocking time
 */
  ulong get_remote_blocking() const;

/**
 * Function: set_remote_blocking(ulong rb);
 * Description: Set the remote blocking time.
 * Input:
 *   rb: A remote blocking time.
 * Return: void
 */
  void set_remote_blocking(ulong rb);

/**
 * Function: get_total_blocking() const;
 * Description: Get the total blocking time.
 * Return: total blocking time
 */
  ulong get_total_blocking() const;

/**
 * Function: set_total_blocking(ulong tb);
 * Description: Set the total blocking time.
 * Input:
 * tb: A total blocking time.
 * Return: void
 */
  void set_total_blocking(ulong tb);

/**
 * Function: get_self_suspension() const;
 * Description: Get the self suspension time.
 * Return: self suspension time
 */
  ulong get_self_suspension() const;

/**
 * Function: set_self_suspension(ulong ss);
 * Description: Set the self suspension time.
 * Input:
 *   ss: A self suspension time.
 * Return: void
 */
  void set_self_suspension(ulong ss);

/**
 * Function: get_jitter() const;
 * Description: Get the jitter of the task.
 * Return: task jitter
 */
  ulong get_jitter() const;

/**
 * Function: set_jitter(ulong jit);
 * Description: Set the jitter of the task.
 * Input:
 *   jit: The length of a jitter.
 * Return: void
 */
  void set_jitter(ulong jit);

/**
 * Function: get_response_time() const;
 * Description: Get the worst-case response time of the task.
 * Return: worst-case response time
 */
  ulong get_response_time() const;

/**
 * Function: set_response_time(ulong response);
 * Description: Set the worst-case response time of the task.
 * Input:
 *   response: The worst-case response time
 * Return: void
 */
  void set_response_time(ulong response);

/**
 * Function: get_priority() const;
 * Description: Get the priority of the task.
 * Return: task prioirty
 */
  uint get_priority() const;

/**
 * Function: set_priority(uint prio);
 * Description: Set the priority of the task.
 * Input:
 *   prio: A task prioirty.
 * Return: void
 */
  void set_priority(uint prio);

/**
 * Function: get_partition() const;
 * Description: Get the partition (processor assignment) of the task.
 * Return: processor id
 */
  uint get_partition() const;

/**
 * Function: set_partition(uint cpu, double speedup = 1);
 * Description: Set the partition (processor assignment) of the task and the speedup factor of the assigned processor.
 * Input:
 *   p_id: A local blocking time.
 *   speedup: A speedup factor of the processor, speedup factor = 1 as default.
 * Return: void
 */
  void set_partition(uint p_id, double speedup = 1);

/**
 * Function: get_cluster() const;
 * Description: Get the partition (processor assignment) of the task.
 * Return: a set of processors
 */
  Cluster get_cluster() const;

/**
 * Function: set_cluster(Cluster cluster, double speedup = 1);
 * Description: Set the partition (processor assignment) of the task and the speedup factor of the assigned processor.
 * Input:
 *   p_id: A local blocking time.
 *   speedup: A speedup factor of the processors, speedup factor = 1 as default.
 * Return: void
 */
  void set_cluster(Cluster cluster, double speedup = 1);

  void add2cluster(uint32_t p_id, double speedup = 1);

  bool is_in_cluster(uint32_t p_id) const;

/**
 * Function: get_ratio() const;
 * Description: Get the speed of a specific processor.
 * Input:
 *   p_id: A specific processor id. 
 * Return: speedup factor
 */
  double get_ratio(uint p_id) const;

/**
 * Function: set_ratio(uint p_id, double speed);
 * Description: Set the speed of a specific processor.
 * Input:
 *   p_id: A specific processor id. 
 *   speed: A speedup factor.
 * Return: void
 */
  void set_ratio(uint p_id, double speed);

/**
 * Function: get_cluster() const;
 * Description: Get the cluster of the task.
 * Return: clustur index
 */
  // uint get_cluster() const;

/**
 * Function: set_cluster(uint clu);
 * Description: Set the cluster of the task.
 * Input:
 *   clu: A clustur index.
 * Return: void
 */
  // void set_cluster(uint clu);

/**
 * Function: get_affinity() const;
 * Description: Get the affinity of the task.
 * Return: affinities to processors
 */
  CPU_Set* get_affinity() const;

/**
 * Function: set_affinity(CPU_Set* affi);
 * Description: Set the affinity of the task.
 * Input:
 *   affi: Affinities to processors
 * Return: void
 */
  void set_affinity(CPU_Set* affi);

/**
 * Function: is_independent() const;
 * Description: To check if the task is independent.
 * Return:
 *   true: Independent.
 *   false: Dependent.
 */
  bool is_independent() const;

/**
 * Function: set_dependent();
 * Description: Set the task as a dependent task.
 * Return: void
 */
  void set_dependent();

/**
 * Function: is_carry_in() const;
 * Description: To check if an instance of the task should be counted as a carry in job.
 * Return:
 *   true: carry in job
 *   false: not carry in job
 */
  bool is_carry_in() const;

/**
 * Function: set_carry_in();
 * Description: Set as a carry in job.
 * Return: void
 */
  void set_carry_in();

/**
 * Function: clear_carry_in();
 * Description: Set as a non-carry in job.
 * Return: void
 */
  void clear_carry_in();

/**
 * Function: get_other_attr() const;
 * Description: Get the reserved attributes.
 * Return: reserved attributes
 */
  ulong get_other_attr() const;

/**
 * Function: set_local_blocking(ulong lb);
 * Description: Set the local blocking time.
 * Input:
 *   spining: A local blocking time.
 * Return: void
 */
  void set_other_attr(ulong attr);


/**
 * Function: add_request(uint r_id, uint num, ulong max_len, ulong total_len,
 * uint locality = MAX_INT);
 * Description: To add a request description to a specific shared resource.
 * Input:
 *   r_id: A shared resource id.
 *   num: The number of request to the shared resource.
 *   max_len: The worst-case execution length of a shared resource request.
 *   total_len: The cummulative length of requests to the shared resource.
 *   locality: The partition (assignment, if exists) of the shared resource, set to 0xffffffff as default.
 * Return: void
 */
  void add_request(uint r_id, uint num, ulong max_len, ulong total_len,
                   uint locality = MAX_INT);

/**
 * Function: add_request(ResourceSet* resources, uint r_id, uint num,
 *                       ulong max_len);
 * Description: To add a request description to a specific shared resource.
 * Input:
 *   resources: A set of shared resources.
 *   r_id: A shared resource id.
 *   num: The number of request to the shared resource.
 *   max_len: The worst-case execution length of a shared resource request.
 * Return: void
 */
  void add_request(ResourceSet* resources, uint r_id, uint num,
                   ulong max_len);

  /**
   * Function: get_max_job_num(ulong interval) const;
   *   Description: Get the maxmum number of the task instance during a
   * consecutive interval. Input: interval: The length of a consecutive
   * interval. Return: instance number
   */
  uint get_max_job_num(ulong interval) const;

/**
 * Function: get_max_request_num(uint resource_id, ulong interval) const;
 * Description: Get the maxmum number of requests to a specific shared resource during a consecutive interval.
 * Input:
 *   resource_id: A shared resource id.
 *   interval: The length of a consecutive interval.
 * Return: request number
 */
  uint get_max_request_num(uint resource_id, ulong interval) const;



/**
 * Function: get_max_request_num_np(uint resource_id, ulong interval) const;
 * Description: Get the maxmum number of requests to a specific shared resource during a consecutive interval (use deadline instead of response time).
 * Input:
 *   resource_id: A shared resource id.
 *   interval: The length of a consecutive interval.
 * Return: request number
 */
  uint get_max_request_num_nu(uint resource_id, ulong interval) const;

/**
 * Function: get_utilization() const;
 * Description: Get the utilization of the task.
 * Return: utilization
 */
  double get_utilization() const;

/**
 * Function: get_density() const;
 * Description: Get the density of the task.
 * Return: density
 */
  double get_density() const;

/**
 * Function: get_NCS_utilization() const;
 * Description: Get the utilization of the task's non-critical sections.
 * Return: utilization (non-critical sections)
 */
  double get_NCS_utilization() const;
};

typedef vector<BPTask> BPTasks;

class BPTaskSet {
 private:
  BPTasks tasks;
  double utilization_sum;
  double utilization_max;
  double density_sum;
  double density_max;

 public:
/**
 * Function: BPTaskSet();
 * Description: Create an empty task set.
 */
  BPTaskSet();

/**
 * Function: ~BPTaskSet();
 * Description: Destructor of a task set.
 */
  ~BPTaskSet();

/*make backpack for primary*/
  void task_copy(int size); 

/**
 * Function: init();
 * Description: Reset attributes of all tasks in the task set.
 * Return: void
 */ 
  void init();

/**
 * Function: add_task(BPTask task);
 * Description: To add a task to the task set.
 * Input:
 *   task: A task instance.
 * Return: void
 */
  void add_task(BPTask task);

/**
 * Function: add_task(ulong wcet, ulong period, ulong deadline = 0);
 * Description: To add a task to the task set.
 * Input:
 *   wcet: The worst-case execution time of the added task.
 *   peiord: The period of the added task.
 *   deadline: The relative deadline of the added task, if set to 0, then it is identical to the period.
 * Return: void
 */
  void add_task(ulong wcet, ulong period, ulong deadline = 0,bool status=true);

/**
 * Function: add_task(ResourceSet* resourceset, Param param, ulong wcet, ulong period,
            ulong deadline = 0);
 * Description: To add a task to the task set.
 * Input:
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   param: A configuration for generating a task instance.
 *   wcet: The worst-case execution time of the added task.
 *   peiord: The period of the added task.
 *   deadline: The relative deadline of the added task, if set to 0, then it is identical to the period.
 * Return: void
 */
  void add_task(ResourceSet* resourceset, Param param, ulong wcet, ulong period,
                ulong deadline = 0,bool status=true);

/**
 * Function: add_task(uint r_id, ResourceSet* resourceset, Param param, long ncs_wcet,
            long cs_wcet, long period, long deadline = 0);
 * Description: To add a task to the task set.
 * Input:
 *   r_id:
 *   resourceset: A set of shared resources, for the generation of shared resource requests.
 *   param: A configuration for generating a task instance.
 *   ncs_wcet: The worst-case execution time of the task's non-ciritcal sections.
 *   cs_wcet: The worst-case execution time of the task's ciritcal sections.
 *   peiord: The period of the added task.
 *   deadline: The relative deadline of the added task, if set to 0, then it is identical to the period.
 * Return: void
 */
  void add_task(uint r_id, ResourceSet* resourceset, Param param,
                uint64_t ncs_wcet, uint64_t cs_wcet, uint64_t period,
                uint64_t deadline = 0,bool status=true);



/**
 * Function: get_tasks();
 * Description: Get a vector of tasks.
 * Return: tasks
 */
  BPTasks& get_tasks();

/**
 * Function: get_task_by_id(uint id);
 * Description: Get a task instance by its id.
 * Input:
 *   id: BPTask id.
 * Return: task instance
 */
  BPTask& get_task_by_id(uint id);

/**
 * Function: get_task_by_index(uint index);
 * Description: Get a task instance by its index.
 * Input:
 *   index: BPTask index in the vector.
 * Return: task instance
 */
  BPTask& get_task_by_index(uint index);

/**
 * Function: get_task_by_priority(uint pi);
 * Description: Get a task instance by its priority.
 * Input:
 *   pi: BPTask priority.
 * Return: task instance
 */
  BPTask& get_task_by_priority(uint pi);

/**
 * Function: is_implicit_deadline();
 * Description: To check if it is implicit deadline task set.
 * Return:
 *   true: It is implicit deadline task set.
 *   false: It is not implicit deadline task set.
 */
  bool is_implicit_deadline();

/**
 * Function: is_constrained_deadline();
 * Description: To check if it is constrained deadline task set.
 * Return:
 *   true: It is constrained deadline task set.
 *   false: It is not constrained deadline task set.
 */
  bool is_constrained_deadline();

/**
 * Function: is_arbitary_deadline();
 * Description: To check if it is arbitary deadline task set.
 * Return:
 *   true: It is arbitary deadline task set.
 *   false: It is not arbitary deadline task set.
 */
  bool is_arbitary_deadline();

/**
 * Function: get_taskset_size() const;
 * Description: Get the number of tasks in the task set.
 * Return: task number
 */
  uint get_taskset_size() const;


/**
 * Function: get_task_utilization(uint index) const;
 * Description: Get the utilization of a specific task.
 * Input:
 * index: BPTask index in the vector.
 * Return: task utilization
 */
  double get_task_utilization(uint index) const;

/**
 * Function: get_task_density(uint index) const;
 * Description: Get the density of a specific task.
 * Input:
 * index: BPTask index in the vector.
 * Return: task density
 */
  double get_task_density(uint index) const;

/**
 * Function: get_task_wcet(index) const;
 * Description: Get the worst-case execution time of a specific task.
 * Input:
 * index: BPTask index in the vector.
 * Return: WCET
 */
  ulong get_task_wcet(uint index) const;

/**
 * Function: get_task_deadline(index) const;
 * Description: Get the relative deadline of a specific task.
 * Input:
 *   index: BPTask index in the vector.
 * Return: relative deadline
 */
  ulong get_task_deadline(uint index) const;

/**
 * Function: get_task_period(index) const;
 * Description: Get the period of a specific task.
 * Input:
 *   index: BPTask index in the vector.
 * Return: period
 */
  ulong get_task_period(uint index) const;

/**
 * Function: get_utilization_sum() const;
 * Description: Get the total utilization of the task set.
 * Return: total utilization
 */
  double get_utilization_sum() const;

/**
 * Function: get_utilization_max() const;
 * Description: Get the maximum utilization of the task set.
 * Return: maximum utilization
 */
  double get_utilization_max() const;

/**
 * Function: get_density_sum() const;
 * Description: Get the total density of the task set.
 * Return: total density
 */
  double get_density_sum() const;

/**
 * Function: get_density_max() const;
 * Description: Get the maximum density of the task set.
 * Return: maximum density
 */
  double get_density_max() const;


// Following functions sort the task set vector in specific orders
/**
 * Function: sort_by_id();
 * Description: Sort the task set in the order of increasing task id.
 * Return: void
 */
  void sort_by_id();
  void sort_by_index();
  void sort_by_period();       // increase
  void sort_by_deadline();     // increase
  void sort_by_utilization();  // decrease
  void sort_by_density();      // decrease
  void sort_by_DC();
  void sort_by_DCC();
  void sort_by_DDC();
  void sort_by_UDC();
  void RM_Order();
  void DM_Order();
  void Density_Decrease_Order();
  void DC_Order();
  void DCC_Order();
  void DDC_Order();
  void UDC_Order();
  void SM_PLUS_Order();
  void SM_PLUS_2_Order();
  void SM_PLUS_3_Order();
  void Leisure_Order();
  void SM_PLUS_4_Order(uint p_num);
  ulong leisure(uint index);

/**
 * Function: display();
 * Description: Output selected attributes of all tasks to the console.
 * Return: void
 */
  void display();

/**
 * Function: update_requests(const ResourceSet& resoruces);
 * Description: To update all tasks' requests.
 * Input:
 *   resources: A set of all shared resources of the system.
 * Return: void
 */
  void update_requests(const ResourceSet& resoruces);

/**
 * Function: export_taskset(string path);
 * Description: Export the task set to a file.
 * Input:
 * path: The path of the output file.
 * Return: void
 */
  void export_taskset(string path);

/**
 * Function: task_gen(ResourceSet* resourceset, Param param, double utilization,
 *                    uint task_number);
 * Description: BPTask generation.
 * Input:
 *   resources: A set of all shared resources of the system.
 *   param: A configuration for generating a task instance.
 *   utilization: The designated total utilization.
 *   task_number: The designated task number.
 * Return: void
 */
  void task_gen(ResourceSet* resourceset, Param param, double utilization,
                uint task_number);

/**
 * Function: task_load(ResourceSet* resourceset, string file_name);
 * Description: Load task set from a file.
 * Input:
 *   resources: A set of all shared resources of the system.
 *   file_name: The path of the input file.
 * Return: void
 */
  void task_load(ResourceSet* resourceset, string file_name,bool status);
};



/*NPTask*/


class NPTask{
  private:
    //任务
    uint model = TPS_TASK_MODEL;
    uint id;
    uint index;

    ulong wcet;//最坏执行时间
    ulong wcet_critical_sections;
    ulong wcet_non_critical_sections;
    ulong spin;             //自旋(x)
    ulong self_suspension;  //(X)
    ulong local_blocking;   //(X)
    ulong remote_blocking;  //(X)
    ulong total_blocking;   //(X)
    ulong jitter;           //(X)
    ulong response_time;    //(=WCET)
    ulong deadline;         
    ulong period;
    ulong arrivalTime;
    ulong remainingExecutionTime;
    ulong startTime;
    ulong finishTime;
    double speedup;

    uint priority;
    uint partition;         //(?)
    //处理器
    Cluster cluster;        
    CPU_Set* affinity;      //(?)
    bool independent;       //(?)
    bool carry_in;          //(?)
    double utilization;
    double density;
    Ratio ratio;            //(heterogeneous用,X)
    Resource_Requests requests; //(?)
    GPU_Requests g_requests;    //(?)
    ulong other_attr;           //(?)

  public:
   //NPTask(uint id, ulong wcet, ulong period, ulong deadline = 0,
   //    uint priority = MAX_INT);
    NPTask(uint id, ResourceSet* resourceset, Param param, ulong wcet, ulong period,
       ulong deadline = 0, uint priority = MAX_INT);
    NPTask(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
       ulong cs_wcet, ulong period, ulong deadline = 0,
       uint priority = MAX_INT);

    NPTask(uint id, ulong wcet, ulong period, ulong deadline);

    NPTask(uint id, ulong wcet, ulong period, ulong deadline,
       ulong arrivalTime);
    ~NPTask();

    void init();
    uint task_model();
    uint get_id() const;
    void set_id(uint id);
    uint get_index() const;
    void set_index(uint index);
    ulong get_wcet() const;
    ulong get_wcet_heterogeneous() const;//(X)
    ulong get_deadline() const;
    ulong set_arrive_time(ulong aTime);
    ulong set_deadline(ulong dTime);
    ulong get_arrive_time() const;
    ulong get_start_time() const;
    void set_start_time(ulong sTime);
    ulong get_finish_time() const;
    void set_finish_time(ulong fTime);
    ulong get_remaining_execution_time() const;
    void set_remaining_execution_time(ulong rETime);
    ulong get_period() const;
    ulong get_slack() const;
    bool is_feasible() const;

    const Resource_Requests& get_requests() const;
    const GPU_Requests& get_g_requests() const;
    const Request& get_request_by_id(uint id) const;
    const GPURequest& get_g_request_by_id(uint id) const;
    bool is_request_exist(uint resource_id) const;
    bool is_g_request_exist(uint gpu_id) const;
    void update_requests(ResourceSet resources);

    ulong get_wcet_critical_sections() const;
    ulong get_wcet_critical_sections_heterogeneous() const;
    void set_wcet_critical_sections(ulong csl);
    ulong get_wcet_non_critical_sections() const;
    ulong get_wcet_non_critical_sections_heterogeneous() const;
    void set_wcet_non_critical_sections(ulong ncsl);

    ulong get_spin() const;
    void set_spin(ulong spining);
    ulong get_local_blocking() const;
    void set_local_blocking(ulong lb);
    ulong get_remote_blocking() const;
    void set_remote_blocking(ulong rb);
    ulong get_total_blocking() const;
    void set_total_blocking(ulong tb);
    ulong get_self_suspension() const;
    void set_self_suspension(ulong ss);

    ulong get_jitter() const;
    void set_jitter(ulong jit);
    ulong get_response_time() const;
    void set_response_time(ulong response);
    uint get_priority() const;
    void set_priority(uint prio);
    uint get_partition() const;
    void set_partition(uint p_id, double speedup = 1);

    Cluster get_cluster() const;
    void set_cluster(Cluster cluster, double speedup = 1);
    void add2cluster(uint32_t p_id, double speedup = 1);
    bool is_in_cluster(uint32_t p_id) const;
    double get_ratio(uint p_id) const;
    void set_ratio(uint p_id, double speed);
    CPU_Set* get_affinity() const;
    void set_affinity(CPU_Set* affi);
    bool is_independent() const;
    void set_dependent();
    bool is_carry_in() const;
    void set_carry_in();
    void clear_carry_in();
    ulong get_other_attr() const;
    void set_other_attr(ulong attr);

    void add_request(uint r_id, uint num, ulong max_len, ulong total_len,
                   uint locality = MAX_INT);
    void add_request(ResourceSet* resources, uint r_id, uint num,
                   ulong max_len);
    uint get_max_job_num(ulong interval) const;
    uint get_max_request_num(uint resource_id, ulong interval) const;
    uint get_max_request_num_nu(uint resource_id, ulong interval) const;
    double get_utilization() const;
    double get_density() const;
    double get_NCS_utilization() const;
    void display_task();

};

typedef vector<NPTask> NPTasks;

class NPTaskSet {
  private:
   NPTasks nptasks;
   NPTasks pending_nptasks;
   double utilization_sum;
   double utilization_max;
   double density_sum;
   double density_max;   

  public:
   NPTaskSet();
   ~NPTaskSet();

   void init();
   
  //  void add_task(NPTask task);
   void add_task(ulong wcet, ulong period, ulong deadline = 0);
   void add_task(ResourceSet* resourceset, Param param, ulong wcet, ulong period,
                ulong deadline = 0);
   void add_task(uint r_id, ResourceSet* resourceset, Param param,
                uint64_t ncs_wcet, uint64_t cs_wcet, uint64_t period,
                uint64_t deadline = 0);
   void add_task(ResourceSet* resourceset, string bufline);

   NPTasks& get_np_tasks();
   NPTasks& get_pending_tasks();
   NPTask& get_task_by_id(uint id);
  //  NPTask& get_task_by_index(uint index);
   NPTask& get_task_by_priority(uint pi);
  //  bool is_implicit_deadline();
  //  bool is_constrained_deadline();
  //  bool is_arbitary_deadline();

   uint get_NPTaskSet_size() const;
  //  double get_task_utilization(uint index) const;
  //  double get_task_density(uint index) const;
   ulong get_task_wcet(uint index) const;
   ulong get_task_deadline(uint index) const;
   ulong get_task_period(uint index) const;

   double get_utilization_sum() const;
   double get_utilization_max() const;
   double get_density_sum() const;
   double get_density_max() const;

   void sort_by_id();
   void sort_by_index();
   void sort_by_period();       // increase
   void sort_by_period_pendingTasks();
   void sort_by_deadline();     // increase
   void sort_by_deadline_pendingTasks();
   void sort_by_utilization();  // decrease
   void sort_by_density();      // decrease
   void sort_by_DC();
   void sort_by_DCC();
   void sort_by_DDC();
   void sort_by_UDC();

   void RM_Order();
   void DM_Order();
   void Density_Decrease_Order();
   void DC_Order();
   void DCC_Order();
   void DDC_Order();
   void UDC_Order();
   void SM_PLUS_Order();
   void SM_PLUS_2_Order();
   void SM_PLUS_3_Order();
   void Leisure_Order();
   void SM_PLUS_4_Order(uint p_num);
   ulong leisure(uint index);

   void display();
   void update_requests(const ResourceSet& resoruces);
   void export_taskset(string path);
   void task_gen(ResourceSet* resourceset, Param param, double utilization,
                uint task_number);
   void task_load(ResourceSet* resourceset, string file_name);

};





#endif  // INCLUDE_TASKS_H_
