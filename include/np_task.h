#ifndef INCLUDE_NP_TASK_H_
#define INCLUDE_NP_TASK_H_
#include <algorithm>
#include <string>
#include <vector>
#include <types.h>
#include <tasks.h>

using std::cout;
using std::endl;
using std::vector;


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


class NPTask{
  private:
    //任务
    uint model = TPS_TASK_MODEL;
    uint id;
    uint index;

    ulong wcet;//最坏执行时间
    ulong wcet_critical_sections;
    ulong wcet_non_critical_sections
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
    NPTask(uint id, ulong wcet, ulong period, ulong deadline = 0,
       uint priority = MAX_INT);
    NPTask(uint id, ResourceSet* resourceset, Param param, ulong wcet, ulong period,
       ulong deadline = 0, uint priority = MAX_INT);
    NPTask(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
       ulong cs_wcet, ulong period, ulong deadline = 0,
       uint priority = MAX_INT);
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
    ulong get_arrive_time() const;
    ulong get_start_time() const;
    void set_start_time(ulong startTime);
    ulong get_finish_time() const;
    void set_finish_time(ulong finishTime);
    ulong get_remaining_execution_time() const;
    void set_remaining_execution_time(ulong remainingExecutionTime);
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
   
   void add_task(NPTask task);
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

#endif //INCLUDE_NP_TASK_H_