// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <resources.h>
#include <sort.h>
#include <stdio.h>
#include <tasks.h>
#include <time_record.h>
#include <toolkit.h>
#include <xml.h>
#include <fstream>
#include <sstream>
#include <string>

using std::exception;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::string;
using std::stringstream;
using std::to_string;

/** Class Request */

Request::Request(uint resource_id, uint num_requests, ulong max_length,
                 ulong total_length, uint locality, bool FG, double min_ratio) {
  this->resource_id = resource_id;
  this->num_requests = num_requests;
  if (!FG)
    this->max_length = max_length;
  else
    this->max_length = 0;
  this->total_length = total_length;
  this->locality = locality;

  if (FG) {  // fine-grained
    for (uint i = 0; i < num_requests; i++) {
      // double ratio = Random_Gen::exponential_gen(1.0/mean);
      // assert(0<=(2*mean-1));
      double ratio = Random_Gen::uniform_real_gen(min_ratio, 1);
      // cout << "ratio:" << ratio << endl;
      uint64_t length = ratio*max_length;
      // assert(length <= max_length);
      requests_length.push_back(length);
      this->fg_total_length += length;
      if (this->max_length < length)
        this->max_length = length;
    }
    sort(requests_length.begin(), requests_length.end(), sort_decrease);
  } else {
    requests_length.push_back(max_length);
    this->fg_total_length += max_length;
    for (uint i = 1; i < num_requests; i++) {
      // double ratio = Random_Gen::exponential_gen(1.0/mean);
      // assert(0<=(2*mean-1));
      double ratio = Random_Gen::uniform_real_gen(min_ratio, 1);
      // cout << "ratio:" << ratio << endl;
      uint64_t length = ratio*max_length;
      // assert(length <= max_length);
      requests_length.push_back(length);
      this->fg_total_length += length;
    }
    sort(requests_length.begin(), requests_length.end(), sort_decrease);
  }
  
}


uint Request::get_resource_id() const { return resource_id; }
uint Request::get_num_requests() const { return num_requests; }
void Request::set_num_requests(uint64_t num) { num_requests = num; }
ulong Request::get_max_length() const { return max_length; }
ulong Request::get_total_length() const { return total_length; }
ulong Request::set_total_length(uint64_t length) { total_length = length; }
ulong Request::get_fg_total_length() const { return fg_total_length; }  // find-graind total length
ulong Request::set_fg_total_length(uint64_t length) { fg_total_length = length; }
ulong Request::get_locality() const { return locality; }
void Request::set_locality(ulong partition) { locality = partition; }
const vector<uint64_t> &Request::get_requests_length() const { return requests_length; }


/** Class GPURequest */

GPURequest::GPURequest(uint gpu_id, uint num_requests, uint32_t g_b_num, uint32_t g_t_num, uint64_t g_wcet) {
  this->gpu_id = gpu_id;
  this->num_requests = num_requests;
  this->g_block_num = g_b_num;
  this->g_thread_num = g_t_num;
  this->g_wcet = g_wcet;
  this->total_workload = g_b_num * g_t_num * g_wcet;
}


uint GPURequest::get_gpu_id() const { return gpu_id; }
uint GPURequest::get_num_requests() const { return num_requests; }
uint32_t GPURequest::get_g_block_num() const { return g_block_num; }
uint32_t GPURequest::get_g_thread_num() const { return g_thread_num; }
uint64_t GPURequest::get_g_wcet() const { return g_wcet; }
uint64_t GPURequest::get_total_workload() const { return total_workload; }


/** Class Task */

Task::Task(uint id, ulong wcet, ulong period, ulong deadline, uint priority) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}

Task::Task(uint id, ResourceSet *resourceset, Param param, ulong wcet,
           ulong period, ulong deadline, uint priority) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;

  uint critical_section_num = 0;

  for (int i = 0; i < param.p_num; i++) {
    if (i < param.ratio.size())
      ratio.push_back(param.ratio[i]);
    else
      ratio.push_back(1);
  }

  for (int i = 0; i < resourceset->size(); i++) {
    if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
      if (Random_Gen::probability(param.rrp)) {
        uint num = Random_Gen::uniform_integral_gen(
            1, min(param.rrn,
                   static_cast<uint>(param.mcsn - critical_section_num)));
        // uint num = min(param.rrn,
        //            static_cast<uint>(param.mcsn - critical_section_num));
        uint max_len = Random_Gen::uniform_integral_gen(
            param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
            cout << "max_len:" << max_len << endl;
        ulong length = num * max_len;
        if (length >= wcet_non_critical_sections) continue;

        add_request(i, num, max_len, num * max_len,
                    resourceset->get_resource_by_id(i).get_locality());

        resourceset->add_task(i, id);
        critical_section_num += num;
      }
    }
  }

  // Experimental: model the requests for GPU
  // only one request per task
  /**
   * 
  double g_ratio;
  if (0 == strcmp(param.graph_x_label.c_str(), "GPU Segment Ratio")) {
    g_ratio = param.reserve_double_2;
  } else {
    g_ratio = Random_Gen::uniform_real_gen(param.reserve_range_2.min, param.reserve_range_2.max);
  }
  cout << "g_ratio:" << g_ratio << endl;
  int b_num = Random_Gen::uniform_integral_gen(1, 20);
  int t_num;
  if (0 == strcmp(param.graph_x_label.c_str(), "GPU Block Size (multiple of 32)")) {
    t_num = 32 * Random_Gen::uniform_integral_gen(1, param.reserve_double_2);
  } else {
    t_num = 128 * Random_Gen::uniform_integral_gen(1, 2);
  }
  int g = 2;
  int m_g = 2048;
  int h = gcd(2048, t_num);

  uint64_t g_wcet =  g_ratio * wcet / (ceiling(b_num, g*(m_g/t_num)));

  this->g_requests.push_back(GPURequest(0, 1, b_num, t_num, g_wcet));

  //  Model the GPU task as a shared resource
  // cout << "# resource:" << resourceset->size() << endl;
  if (0 != t_num && 0 != b_num && 0 != g_wcet) {
    add_request(0, 1, g_ratio * wcet, g_ratio * wcet, resourceset->get_resource_by_id(0).get_locality());
    resourceset->add_task(0, id);
  }
  **/

  // wcet_critical_sections += b_num * t_num * g_wcet;
  // wcet_non_critical_sections -= b_num * t_num * g_wcet;
  // wcet_critical_sections += (g_ratio * wcet);
  // wcet_non_critical_sections -= (g_ratio * wcet);


}

Task::Task(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
           ulong cs_wcet, ulong period, ulong deadline, uint priority) {
  this->id = id;
  this->index = id;
  this->wcet = ncs_wcet + cs_wcet;
  wcet_non_critical_sections = ncs_wcet;
  wcet_critical_sections = cs_wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  independent = true;
  carry_in = false;
  other_attr = 0;

  add_request(r_id, 1, cs_wcet, cs_wcet,
              resourceset->get_resource_by_id(r_id).get_locality());
  resourceset->add_task(r_id, id);
}

Task::~Task() {
  // if(affinity)
  // delete affinity;
  requests.clear();
}

void Task::init() {
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  // priority = MAX_INT;
  independent = true;
  // wcet_non_critical_sections = this->wcet;
  // wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}

uint Task::task_model() { return model; }

void Task::add_request(uint r_id, uint num, ulong max_len, ulong total_len,
                       uint locality) {
  wcet_non_critical_sections -= total_len;
  wcet_critical_sections += total_len;
  requests.push_back(Request(r_id, num, max_len, total_len, locality));
}

void Task::add_request(ResourceSet *resources, uint r_id, uint num,
                       ulong max_len) {
  uint64_t total_len = num * max_len;
  if (total_len < wcet_non_critical_sections) {
    resources->add_task(r_id, this->id);
    wcet_non_critical_sections -= total_len;
    wcet_critical_sections += total_len;
    uint locality = resources->get_resource_by_id(r_id).get_locality();
    requests.push_back(Request(r_id, num, max_len, total_len, locality));
  }
}

uint Task::get_max_job_num(ulong interval) const {
  uint num_jobs;
  // cout << "interval " << interval << endl;
  // cout << "get_response_time() " << get_response_time() << endl;
  // cout << "get_wcet() " << get_wcet() << endl;
  num_jobs = ceiling(interval + get_response_time() - get_wcet(), get_period());
  return num_jobs;
}

uint Task::get_max_request_num(uint resource_id, ulong interval) const {
  if (is_request_exist(resource_id)) {
    const Request &request = get_request_by_id(resource_id);
    // cout << get_max_job_num(interval) * request.get_num_requests() << endl;
    return get_max_job_num(interval) * request.get_num_requests();
  } else {
    return 0;
  }
}

double Task::get_utilization() const { return utilization; }

double Task::get_NCS_utilization() const {
  double temp = get_wcet_non_critical_sections();
  temp /= get_period();
  return temp;
}

double Task::get_density() const { return density; }

uint Task::get_id() const { return id; }
void Task::set_id(uint id) { this->id = id; }
uint Task::get_index() const { return index; }
void Task::set_index(uint index) { this->index = index; }
ulong Task::get_wcet() const { return wcet; }
ulong Task::get_wcet_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet, speedup);
  else
    return wcet;
}
ulong Task::get_deadline() const { return deadline; }
ulong Task::get_period() const { return period; }
ulong Task::get_slack() const { return deadline - wcet; }
bool Task::is_feasible() const {
  return deadline >= wcet && period >= wcet && wcet > 0;
}

const Resource_Requests &Task::get_requests() const { return requests; }

const GPU_Requests &Task::get_g_requests() const { return g_requests; }

const Request &Task::get_request_by_id(uint id) const {
  Request *result = NULL;
  for (uint i = 0; i < requests.size(); i++) {
    if (id == requests[i].get_resource_id()) return requests[i];
  }
  return *result;
}

const GPURequest &Task::get_g_request_by_id(uint id) const {
  GPURequest *result = NULL;
  for (uint i = 0; i < g_requests.size(); i++) {
    if (id == g_requests[i].get_gpu_id()) return g_requests[i];
  }
  return *result;
}

bool Task::is_request_exist(uint resource_id) const {
  for (uint i = 0; i < requests.size(); i++) {
    if (resource_id == requests[i].get_resource_id()) return true;
  }
  return false;
}

bool Task::is_g_request_exist(uint gpu_id) const {
  for (uint i = 0; i < g_requests.size(); i++) {
    if (gpu_id == g_requests[i].get_gpu_id()) return true;
  }
  return false;
}

void Task::update_requests(ResourceSet resources) {
  foreach(requests, request) {
    uint q = request->get_resource_id();
    request->set_locality(resources.get_resource_by_id(q).get_locality());
  }
}

ulong Task::get_wcet_critical_sections() const {
  return wcet_critical_sections;
}
ulong Task::get_wcet_critical_sections_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet_critical_sections, speedup);
  else
    return wcet_critical_sections;
}
void Task::set_wcet_critical_sections(ulong csl) {
  wcet_critical_sections = csl;
}
ulong Task::get_wcet_non_critical_sections() const {
  return wcet_non_critical_sections;
}
ulong Task::get_wcet_non_critical_sections_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet_non_critical_sections, speedup);
  else
    return wcet_non_critical_sections;
}
void Task::set_wcet_non_critical_sections(ulong ncsl) {
  wcet_non_critical_sections = ncsl;
}
ulong Task::get_spin() const { return spin; }
void Task::set_spin(ulong spining) { spin = spining; }
ulong Task::get_local_blocking() const { return local_blocking; }
void Task::set_local_blocking(ulong lb) { local_blocking = lb; }
ulong Task::get_remote_blocking() const { return remote_blocking; }
void Task::set_remote_blocking(ulong rb) { remote_blocking = rb; }
ulong Task::get_total_blocking() const { return total_blocking; }
void Task::set_total_blocking(ulong tb) { total_blocking = tb; }
ulong Task::get_self_suspension() const { return self_suspension; }
void Task::set_self_suspension(ulong ss) { self_suspension = ss; }
ulong Task::get_jitter() const { return jitter; }
void Task::set_jitter(ulong jit) { jitter = jit; }
ulong Task::get_response_time() const { return response_time; }
void Task::set_response_time(ulong response) { response_time = response; }
uint Task::get_priority() const { return priority; }
void Task::set_priority(uint prio) { priority = prio; }
uint Task::get_partition() const { return partition; }
void Task::set_partition(uint p_id, double speedup) {
  partition = p_id;
  this->speedup = speedup;
}
Cluster Task::get_cluster() const { return cluster; }
void Task::set_cluster(Cluster cluster, double speedup) {
  this->cluster = cluster;
  this->speedup = speedup;
}
void Task::add2cluster(uint32_t p_id, double speedup) {
  bool included = false;
  foreach(cluster, p_k) {
    if ((*p_k) == p_id)
      included = true;
  }
  if (included) {
    cluster.push_back(p_id);
    this->speedup = speedup;
  }
}
bool Task::is_in_cluster(uint32_t p_id) const {
  foreach(cluster, p_k) {
    if (p_id == (*p_k))
      return true;
  }
  return false;
}
double Task::get_ratio(uint p_id) const { return ratio[p_id]; }
void Task::set_ratio(uint p_id, double speed) { ratio[p_id] = speed; }
CPU_Set *Task::get_affinity() const { return affinity; }
void Task::set_affinity(CPU_Set *affi) { affinity = affi; }
bool Task::is_independent() const { return independent; }
void Task::set_dependent() { independent = false; }
bool Task::is_carry_in() const { return carry_in; }
void Task::set_carry_in() { carry_in = true; }
void Task::clear_carry_in() { carry_in = false; }
ulong Task::get_other_attr() const { return other_attr; }
void Task::set_other_attr(ulong attr) { other_attr = attr; }

/** Class TaskSet */

TaskSet::TaskSet() {
  utilization_sum = 0;
  utilization_max = 0;
  density_sum = 0;
  density_max = 0;
}

TaskSet::~TaskSet() { tasks.clear(); }

void TaskSet::init() {
  for (uint i = 0; i < tasks.size(); i++) tasks[i].init();
}

double TaskSet::get_utilization_sum() const { return utilization_sum; }

double TaskSet::get_utilization_max() const { return utilization_max; }

double TaskSet::get_density_sum() const { return density_sum; }

double TaskSet::get_density_max() const { return density_max; }

void TaskSet::add_task(Task task) { tasks.push_back(task); }

void TaskSet::add_task(ulong wcet, ulong period, ulong deadline) {
  double utilization_new = wcet, density_new = wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  tasks.push_back(Task(tasks.size(), wcet, period, deadline));
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}

void TaskSet::add_task(ResourceSet *resourceset, Param param, ulong wcet,
                       ulong period, ulong deadline) {
  double utilization_new = wcet, density_new = wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  tasks.push_back(
      Task(tasks.size(), resourceset, param, wcet, period, deadline));
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}

void TaskSet::add_task(uint r_id, ResourceSet* resourceset, Param param,
                       uint64_t ncs_wcet, uint64_t cs_wcet, uint64_t period,
                       uint64_t deadline) {
  double utilization_new = ncs_wcet + cs_wcet,
             density_new = ncs_wcet + cs_wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  tasks.push_back(Task(tasks.size(), r_id, resourceset, ncs_wcet, cs_wcet,
                       period, deadline));

  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}


void TaskSet::add_task(ResourceSet* resourceset, string bufline) {
  uint id = tasks.size();
  // if (NULL != strstr(bufline.data(), "Utilization")) continue;
  // if (0 == strcmp(buf.data(), "\r")) break;
  vector<uint64_t> elements;
  // cout << "bl:" << bufline << endl;
  double utilization_new, density_new;
  extract_element(&elements, bufline, 0, MAX_INT, ",");
  if (3 <= elements.size()) {
    Task task(id, elements[0], elements[1], elements[2]);
    uint n = (elements.size() - 3) / 3;
    for (uint i = 0; i < n; i++) {
      uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
      task.add_request(elements[3 + i * 3], elements[4 + i * 3],
                        elements[5 + i * 3], length);

      resourceset->add_task(elements[3 + i * 3], id);
    }
    add_task(task);
    utilization_new = task.get_utilization();
    density_new = task.get_density();
  }
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
  sort_by_period();
}

Tasks &TaskSet::get_tasks() { return tasks; }

Task &TaskSet::get_task_by_id(uint id) {
  foreach(tasks, task) {
    if (id == task->get_id()) return (*task);
  }
  return *reinterpret_cast<Task *>(0);
}

Task &TaskSet::get_task_by_index(uint index) { return tasks[index]; }

Task &TaskSet::get_task_by_priority(uint pi) {
  foreach(tasks, task) {
    if (pi == task->get_priority()) return (*task);
  }
  return *reinterpret_cast<Task *>(0);
}

bool TaskSet::is_implicit_deadline() {
  foreach_condition(tasks, tasks[i].get_deadline() != tasks[i].get_period());
  return true;
}
bool TaskSet::is_constrained_deadline() {
  foreach_condition(tasks, tasks[i].get_deadline() > tasks[i].get_period());
  return true;
}
bool TaskSet::is_arbitary_deadline() {
  return !(is_implicit_deadline()) && !(is_constrained_deadline());
}
uint TaskSet::get_taskset_size() const { return tasks.size(); }

double TaskSet::get_task_utilization(uint index) const {
  return tasks[index].get_utilization();
}
double TaskSet::get_task_density(uint index) const {
  return tasks[index].get_density();
}
ulong TaskSet::get_task_wcet(uint index) const {
  return tasks[index].get_wcet();
}
ulong TaskSet::get_task_deadline(uint index) const {
  return tasks[index].get_deadline();
}
ulong TaskSet::get_task_period(uint index) const {
  return tasks[index].get_period();
}

void TaskSet::sort_by_id() {
  sort(tasks.begin(), tasks.end(), id_increase<Task>);
}

void TaskSet::sort_by_index() {
  sort(tasks.begin(), tasks.end(), index_increase<Task>);
}

void TaskSet::sort_by_period() {
  sort(tasks.begin(), tasks.end(), period_increase<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_deadline() {
  sort(tasks.begin(), tasks.end(), deadline_increase<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_utilization() {
  sort(tasks.begin(), tasks.end(), utilization_decrease<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_density() {
  sort(tasks.begin(), tasks.end(), density_decrease<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_DC() {
  sort(tasks.begin(), tasks.end(), task_DC_increase<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_DCC() {
  sort(tasks.begin(), tasks.end(), task_DCC_increase<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_DDC() {
  sort(tasks.begin(), tasks.end(), task_DDC_increase<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::sort_by_UDC() {
  sort(tasks.begin(), tasks.end(), task_UDC_increase<Task>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void TaskSet::RM_Order() {
  sort_by_period();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "RM Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void TaskSet::DM_Order() {
  sort_by_deadline();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void TaskSet::Density_Decrease_Order() {
  sort_by_density();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void TaskSet::DC_Order() {
  sort_by_DC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "DC Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void TaskSet::DCC_Order() {
  sort_by_DCC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void TaskSet::DDC_Order() {
  sort_by_DDC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void TaskSet::UDC_Order() {
  sort_by_UDC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void TaskSet::SM_PLUS_Order() {
  sort_by_DC();

  if (1 < tasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < tasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < tasks.size() - 1; index++) {
      vector<Task>::iterator it = (tasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();

      for (int index2 = index + 1; index2 < tasks.size(); index2++) {
        vector<Task>::iterator it2 = (tasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();

        if (c > gap2 && d > d2) {
          accum[(it->get_id())] += c2;

          Task temp = (*it2);
          tasks.erase((it2));
          tasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void TaskSet::SM_PLUS_2_Order() {
  sort_by_DC();

  if (1 < tasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < tasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < tasks.size() - 1; index++) {
      vector<Task>::iterator it = (tasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();

      for (int index2 = index + 1; index2 < tasks.size(); index2++) {
        vector<Task>::iterator it2 = (tasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();

        if (c > gap2 && d > d2 && (accum[(it->get_id())] + c2) <= gap) {
          accum[(it->get_id())] += c2;

          Task temp = (*it2);
          tasks.erase((it2));
          tasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP2:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void TaskSet::SM_PLUS_3_Order() {
  sort_by_period();

  if (1 < tasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < tasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < tasks.size() - 1; index++) {
      vector<Task>::iterator it = (tasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();
      ulong p = (it)->get_period();

      for (int index2 = index + 1; index2 < tasks.size(); index2++) {
        vector<Task>::iterator it2 = (tasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();
        ulong p2 = (it2)->get_period();
        uint N = ceiling(p, p2);
        if (gap > gap2 && (accum[(it->get_id())] + N * c2) <= gap) {
          accum[(it->get_id())] += N * c2;

          Task temp = (*it2);
          tasks.erase((it2));
          tasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP3:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void TaskSet::Leisure_Order() {
  Tasks NewSet;
  sort_by_id();

  for (int i = tasks.size() - 1; i >= 0; i--) {
    int64_t l_max = 0xffffffffffffffff;
    uint index = i;
    for (int j = i; j >= 0; j--) {
      vector<Task>::iterator it = (tasks.begin() + j);
      Task temp = (*it);
      tasks.erase((it));
      tasks.push_back(temp);
      int64_t l = leisure(i);
      if (l > l_max) {
        l_max = l;
        index = j;
      }
    }
    sort_by_id();
    vector<Task>::iterator it2 = (tasks.begin() + index);
    Task temp2 = (*it2);
    tasks.erase(it2);
    NewSet.insert(NewSet.begin(), temp2);
  }

  tasks.clear();
  tasks = NewSet;

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_index(i);
    tasks[i].set_priority(i);
  }

  cout << "Leisure Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
}

void TaskSet::SM_PLUS_4_Order(uint p_num) {
  sort_by_period();
#if SORT_DEBUG
  cout << "////////////////////////////////////////////" << endl;
  cout << "RM:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif

  if (1 < tasks.size()) {
    uint min_id;
    ulong min_slack;
    vector<uint> id_stack;

    foreach(tasks, task) {
      task->set_other_attr(0);  // accumulative adjustment
    }

    for (uint index = 0; index < tasks.size(); index++) {
      bool is_continue = false;
      min_id = 0;
      min_slack = MAX_LONG;
      foreach(tasks, task) {  // find next minimum slack
        is_continue = false;
        uint temp_id = task->get_id();
        foreach(id_stack, element) {
          if (temp_id == (*element)) is_continue = true;
        }
        if (is_continue) continue;
        ulong temp_slack = task->get_slack();
        if (min_slack > task->get_slack()) {
          min_id = temp_id;
          min_slack = temp_slack;
        }
      }
      id_stack.push_back(min_id);
      is_continue = false;

      // locate minimum slack
      for (uint index2 = 0; index2 < tasks.size(); index2++) {
        if (min_id == tasks[index2].get_id()) {
          vector<Task>::iterator task1 = (tasks.begin() + index2);
          for (int index3 = index2 - 1; index3 >= 0; index3--) {
            vector<Task>::iterator task2 = (tasks.begin() + index3);
            if ((p_num - 1) <= task2->get_other_attr()) {
              is_continue = true;
              break;
            }
            if (task1->get_slack() < task2->get_slack()) {
              Task temp = (*task1);
              tasks.erase((task1));
              if (task2->get_deadline() < task1->get_wcet() + task2->get_wcet())
                task2->set_other_attr(task2->get_other_attr() + 1);
              tasks.insert(task2, temp);
              task1 = (tasks.begin() + index3);
            }
          }

          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP4:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

ulong TaskSet::leisure(uint index) {
  Task &task = get_task_by_id(index);
  ulong gap = task.get_deadline() - task.get_wcet();
  ulong period = task.get_period();
  ulong remain = gap;
  for (uint i = 0; i < index; i++) {
    Task &temp = get_task_by_id(i);
    remain -= temp.get_wcet() * ceiling(period, temp.get_period());
  }
  return remain;
}

void TaskSet::display() {
  foreach(tasks, task) {
    cout << "Task" << task->get_index() << ": id:"
         << task->get_id()
         << ": partition:"
         << task->get_partition()
         << ": priority:"
         << task->get_priority() << endl;
    cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
         << " cs-wcet:" << task->get_wcet_critical_sections()
         << " wcet:" << task->get_wcet()
         << " response time:" << task->get_response_time()
         << " deadline:" << task->get_deadline()
         << " period:" << task->get_period() << endl;
    foreach(task->get_requests(), request) {
      cout << "request " << request->get_resource_id() << ":"
           << " num:" << request->get_num_requests()
           << " length:" << request->get_max_length() << endl;
    }
    foreach(task->get_g_requests(), g_request) {
      cout << "g_request " << g_request->get_gpu_id() << ":"
           << " num:" << g_request->get_num_requests()
           << " b_num:" << g_request->get_g_block_num()
           << " t_num:" << g_request->get_g_thread_num()
           << " length:" << g_request->get_g_wcet()
           << " workload:" << g_request->get_total_workload() << endl;

    }
    cout << "-------------------------------------------" << endl;
    if (task->get_wcet() > task->get_response_time()) exit(0);
  }
  cout << get_utilization_sum() << endl;
  // for (int i = 0; i < tasks.size(); i++) {
  //   cout << "Task index:" << tasks[i].get_index()
  //        << " Task id:" << tasks[i].get_id()
  //        << " Task priority:" << tasks[i].get_priority() << endl;
  // }
}

void TaskSet::update_requests(const ResourceSet &resources) {
  foreach(tasks, task) { task->update_requests(resources); }
}

void TaskSet::export_taskset(string path) {
  ofstream file(path, ofstream::app);
  stringstream buf;
  buf << "Utilization:" << get_utilization_sum() << "\n";
  foreach(tasks, task) {
    buf << task->get_wcet() << "," << task->get_period() << ","
        << task->get_deadline();
    foreach(task->get_requests(), request) {
      buf << "," << request->get_resource_id() << ","
          << request->get_num_requests() << "," << request->get_max_length();
    }
    buf << "\n";
  }
  file << buf.str() << "\n";
  file.flush();
  file.close();
}

void TaskSet::task_gen(ResourceSet *resourceset, Param param,
                       double utilization, uint task_number) {
  // Generate task set utilization...
  double SumU;
  vector<double> u_set;
  bool re_gen = true;

  switch (param.u_gen) {
    case GEN_UNIFORM:
    cout << "GEN_UNIFORM" << endl;
      SumU = 0;
      while (SumU < utilization) {
        double u = Random_Gen::uniform_real_gen(0, 1);
        if (1 + _EPS <= u)
          continue;
        if (SumU + u > utilization) {
          u = utilization - SumU;
          u_set.push_back(u);
          SumU += u;
          break;
        }
        u_set.push_back(u);
        SumU += u;
      }
      break;

    case GEN_EXPONENTIAL:
    cout << "GEN_EXPONENTIAL" << endl;
      SumU = 0;
      while (SumU < utilization) {
        double u = Random_Gen::exponential_gen(1/param.mean);
        if (1 + _EPS <= u)
          continue;
        if (SumU + u > utilization) {
          u = utilization - SumU;
          u_set.push_back(u);
          SumU += u;
          break;
        }
        u_set.push_back(u);
        SumU += u;
      }
      break;

    case GEN_UUNIFAST:  // Same as the UUnifast_Discard
    cout << "GEN_UUNIFAST" << endl;
    case GEN_UUNIFAST_D:
    cout << "GEN_UUNIFAST_D" << endl;
      while (re_gen) {
        SumU = utilization;
        re_gen = false;
        for (uint i = 1; i < task_number; i++) {
          double temp = Random_Gen::uniform_real_gen(0, 1);
          double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
          double u = SumU - NextSumU;
          if ((u - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            // cout << "Regenerate taskset." << endl;
            break;
          } else {
            u_set.push_back(u);
            SumU = NextSumU;
          }
        }
        if (!re_gen) {
          if ((SumU - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            // cout << "Regenerate taskset." << endl;
          } else {
            u_set.push_back(SumU);
          }
        }
      }
      break;

    default:
      while (re_gen) {
        SumU = utilization;
        re_gen = false;
        for (uint i = 1; i < task_number; i++) {
          double temp = Random_Gen::uniform_real_gen(0, 1);
          double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
          double u = SumU - NextSumU;
          if ((u - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            cout << "Regenerate taskset." << endl;
            break;
          } else {
            u_set.push_back(u);
            SumU = NextSumU;
          }
        }
        if (!re_gen) {
          if ((SumU - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            cout << "Regenerate taskset." << endl;
          } else {
            u_set.push_back(SumU);
          }
        }
      }
      break;
  }
  cout<<"start generate"<<endl;
  uint i = 0;
  foreach(u_set, u) {
    // ulong period = param.p_range.min + Random_Gen::exponential_gen(2) * (param.p_range.max - param.p_range.min);
    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    ulong wcet = ceil((*u) * period);
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      if (deadline < wcet) deadline = wcet;
    }
    add_task(resourceset, param, wcet, period, deadline);
    cout<<"add scussess"<<endl;
  }
  sort_by_period();
}

void TaskSet::task_load(ResourceSet* resourceset, string file_name) {
  ifstream file(file_name, ifstream::in);
  string buf;
  uint id = 0;
  while (getline(file, buf)) {
    if (NULL != strstr(buf.data(), "Utilization")) continue;
    if (0 == strcmp(buf.data(), "\r")) break;

    vector<uint64_t> elements;
    extract_element(&elements, buf, 0, MAX_INT);
    if (3 <= elements.size()) {
      Task task(id, elements[0], elements[1], elements[2]);
      uint n = (elements.size() - 3) / 3;
      for (uint i = 0; i < n; i++) {
        uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
        task.add_request(elements[3 + i * 3], elements[4 + i * 3],
                         elements[5 + i * 3], length);

        resourceset->add_task(elements[3 + i * 3], id);
      }
      add_task(task);

      id++;
    }
  }
  sort_by_period();
}



uint32_t gcd(uint32_t a, uint32_t b) {
  uint32_t temp;
  while (b) {
    temp = a;
    a = b;
    b = temp % b;
  }
  return a;
}

template <typename Format> Format GCD(Format a, Format b) {
  Format temp;
  while (b) {
    temp = a;
    a = b;
    b = temp % b;
  }
  return a;
}

template <typename Format> Format LCM(Format a, Format b) { return a * b / GCD<Format>(a, b); }


/** Class BPTask */

BPTask::BPTask(uint id, ulong wcet, ulong period, ulong deadline,bool status) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  this->status = status;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}

BPTask::BPTask(uint id, ulong wcet, ulong period, ulong deadline, uint priority,bool status) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  this->status = status;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}

BPTask::BPTask(uint id, ResourceSet *resourceset, Param param, ulong wcet,
           ulong period, ulong deadline, uint priority,bool status) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  this->status = status;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;

  uint critical_section_num = 0;

  for (int i = 0; i < param.p_num; i++) {
    if (i < param.ratio.size())
      ratio.push_back(param.ratio[i]);
    else
      ratio.push_back(1);
  }

  for (int i = 0; i < resourceset->size(); i++) {
    if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
      if (Random_Gen::probability(param.rrp)) {
        uint num = Random_Gen::uniform_integral_gen(
            1, min(param.rrn,
                   static_cast<uint>(param.mcsn - critical_section_num)));
        // uint num = min(param.rrn,
        //            static_cast<uint>(param.mcsn - critical_section_num));
        uint max_len = Random_Gen::uniform_integral_gen(
            param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
            cout << "max_len:" << max_len << endl;
        ulong length = num * max_len;
        if (length >= wcet_non_critical_sections) continue;

        add_request(i, num, max_len, num * max_len,
                    resourceset->get_resource_by_id(i).get_locality());

        resourceset->add_task(i, id);
        critical_section_num += num;
      }
    }
  }

  // Experimental: model the requests for GPU
  // only one request per task
  /**
   * 
  double g_ratio;
  if (0 == strcmp(param.graph_x_label.c_str(), "GPU Segment Ratio")) {
    g_ratio = param.reserve_double_2;
  } else {
    g_ratio = Random_Gen::uniform_real_gen(param.reserve_range_2.min, param.reserve_range_2.max);
  }
  cout << "g_ratio:" << g_ratio << endl;
  int b_num = Random_Gen::uniform_integral_gen(1, 20);
  int t_num;
  if (0 == strcmp(param.graph_x_label.c_str(), "GPU Block Size (multiple of 32)")) {
    t_num = 32 * Random_Gen::uniform_integral_gen(1, param.reserve_double_2);
  } else {
    t_num = 128 * Random_Gen::uniform_integral_gen(1, 2);
  }
  int g = 2;
  int m_g = 2048;
  int h = gcd(2048, t_num);

  uint64_t g_wcet =  g_ratio * wcet / (ceiling(b_num, g*(m_g/t_num)));

  this->g_requests.push_back(GPURequest(0, 1, b_num, t_num, g_wcet));

  //  Model the GPU task as a shared resource
  // cout << "# resource:" << resourceset->size() << endl;
  if (0 != t_num && 0 != b_num && 0 != g_wcet) {
    add_request(0, 1, g_ratio * wcet, g_ratio * wcet, resourceset->get_resource_by_id(0).get_locality());
    resourceset->add_task(0, id);
  }
  **/

  // wcet_critical_sections += b_num * t_num * g_wcet;
  // wcet_non_critical_sections -= b_num * t_num * g_wcet;
  // wcet_critical_sections += (g_ratio * wcet);
  // wcet_non_critical_sections -= (g_ratio * wcet);


}

BPTask::BPTask(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
           ulong cs_wcet, ulong period, ulong deadline, uint priority,bool status) {
  this->id = id;
  this->index = id;
  this->wcet = ncs_wcet + cs_wcet;
  this->status = status;
  wcet_non_critical_sections = ncs_wcet;
  wcet_critical_sections = cs_wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  independent = true;
  carry_in = false;
  other_attr = 0;

  add_request(r_id, 1, cs_wcet, cs_wcet,
              resourceset->get_resource_by_id(r_id).get_locality());
  resourceset->add_task(r_id, id);
}

BPTask::~BPTask() {
  // if(affinity)
  // delete affinity;
  requests.clear();
}

void BPTask::init() {
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  // priority = MAX_INT;
  independent = true;
  // wcet_non_critical_sections = this->wcet;
  // wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}

uint BPTask::task_model() { return model; }

void BPTask::add_request(uint r_id, uint num, ulong max_len, ulong total_len,
                       uint locality) {
  wcet_non_critical_sections -= total_len;
  wcet_critical_sections += total_len;
  requests.push_back(Request(r_id, num, max_len, total_len, locality));
}

void BPTask::add_request(ResourceSet *resources, uint r_id, uint num,
                       ulong max_len) {
  uint64_t total_len = num * max_len;
  if (total_len < wcet_non_critical_sections) {
    resources->add_task(r_id, this->id);
    wcet_non_critical_sections -= total_len;
    wcet_critical_sections += total_len;
    uint locality = resources->get_resource_by_id(r_id).get_locality();
    requests.push_back(Request(r_id, num, max_len, total_len, locality));
  }
}

uint BPTask::get_max_job_num(ulong interval) const {
  uint num_jobs;
  // cout << "interval " << interval << endl;
  // cout << "get_response_time() " << get_response_time() << endl;
  // cout << "get_wcet() " << get_wcet() << endl;
  num_jobs = ceiling(interval + get_response_time() - get_wcet(), get_period());
  return num_jobs;
}

uint BPTask::get_max_request_num(uint resource_id, ulong interval) const {
  if (is_request_exist(resource_id)) {
    const Request &request = get_request_by_id(resource_id);
    // cout << get_max_job_num(interval) * request.get_num_requests() << endl;
    return get_max_job_num(interval) * request.get_num_requests();
  } else {
    return 0;
  }
}

double BPTask::get_utilization() const { return utilization; }

double BPTask::get_NCS_utilization() const {
  double temp = get_wcet_non_critical_sections();
  temp /= get_period();
  return temp;
}

double BPTask::get_density() const { return density; }

uint BPTask::get_id() const { return id; }
uint BPTask::get_linkID() const { return linkID; }
bool BPTask::get_status() const { return status; }
void BPTask::set_id(uint id) { this->id = id; }
uint BPTask::get_index() const { return index; }
void BPTask::set_index(uint index) { this->index = index; }
ulong BPTask::get_wcet() const { return wcet; }
ulong BPTask::get_wcet_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet, speedup);
  else
    return wcet;
}
ulong BPTask::get_deadline() const { return deadline; }
ulong BPTask::get_period() const { return period; }
ulong BPTask::get_slack() const { return deadline - wcet; }
bool BPTask::is_feasible() const {
  return deadline >= wcet && period >= wcet && wcet > 0;
}

const Resource_Requests &BPTask::get_requests() const { return requests; }

const GPU_Requests &BPTask::get_g_requests() const { return g_requests; }

const Request &BPTask::get_request_by_id(uint id) const {
  Request *result = NULL;
  for (uint i = 0; i < requests.size(); i++) {
    if (id == requests[i].get_resource_id()) return requests[i];
  }
  return *result;
}

const GPURequest &BPTask::get_g_request_by_id(uint id) const {
  GPURequest *result = NULL;
  for (uint i = 0; i < g_requests.size(); i++) {
    if (id == g_requests[i].get_gpu_id()) return g_requests[i];
  }
  return *result;
}

bool BPTask::is_request_exist(uint resource_id) const {
  for (uint i = 0; i < requests.size(); i++) {
    if (resource_id == requests[i].get_resource_id()) return true;
  }
  return false;
}

bool BPTask::is_g_request_exist(uint gpu_id) const {
  for (uint i = 0; i < g_requests.size(); i++) {
    if (gpu_id == g_requests[i].get_gpu_id()) return true;
  }
  return false;
}

void BPTask::update_requests(ResourceSet resources) {
  foreach(requests, request) {
    uint q = request->get_resource_id();
    request->set_locality(resources.get_resource_by_id(q).get_locality());
  }
}

ulong BPTask::get_wcet_critical_sections() const {
  return wcet_critical_sections;
}
ulong BPTask::get_wcet_critical_sections_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet_critical_sections, speedup);
  else
    return wcet_critical_sections;
}
void BPTask::set_wcet_critical_sections(ulong csl) {
  wcet_critical_sections = csl;
}
ulong BPTask::get_wcet_non_critical_sections() const {
  return wcet_non_critical_sections;
}
ulong BPTask::get_wcet_non_critical_sections_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet_non_critical_sections, speedup);
  else
    return wcet_non_critical_sections;
}
void BPTask::set_wcet_non_critical_sections(ulong ncsl) {
  wcet_non_critical_sections = ncsl;
}
ulong BPTask::get_spin() const { return spin; }
void BPTask::set_spin(ulong spining) { spin = spining; }
ulong BPTask::get_local_blocking() const { return local_blocking; }
void BPTask::set_local_blocking(ulong lb) { local_blocking = lb; }
ulong BPTask::get_remote_blocking() const { return remote_blocking; }
void BPTask::set_remote_blocking(ulong rb) { remote_blocking = rb; }
ulong BPTask::get_total_blocking() const { return total_blocking; }
void BPTask::set_total_blocking(ulong tb) { total_blocking = tb; }
ulong BPTask::get_self_suspension() const { return self_suspension; }
void BPTask::set_self_suspension(ulong ss) { self_suspension = ss; }
ulong BPTask::get_jitter() const { return jitter; }
void BPTask::set_jitter(ulong jit) { jitter = jit; }
ulong BPTask::get_response_time() const { return response_time; }
void BPTask::set_response_time(ulong response) { response_time = response; }
uint BPTask::get_priority() const { return priority; }
void BPTask::set_priority(uint prio) { priority = prio; }
uint BPTask::get_partition() const { return partition; }
void BPTask::set_partition(uint p_id, double speedup) {
  partition = p_id;
  this->speedup = speedup;
}
Cluster BPTask::get_cluster() const { return cluster; }
void BPTask::set_cluster(Cluster cluster, double speedup) {
  this->cluster = cluster;
  this->speedup = speedup;
}
void BPTask::add2cluster(uint32_t p_id, double speedup) {
  bool included = false;
  foreach(cluster, p_k) {
    if ((*p_k) == p_id)
      included = true;
  }
  if (included) {
    cluster.push_back(p_id);
    this->speedup = speedup;
  }
}
bool BPTask::is_in_cluster(uint32_t p_id) const {
  foreach(cluster, p_k) {
    if (p_id == (*p_k))
      return true;
  }
  return false;
}
double BPTask::get_ratio(uint p_id) const { return ratio[p_id]; }
void BPTask::set_ratio(uint p_id, double speed) { ratio[p_id] = speed; }
CPU_Set *BPTask::get_affinity() const { return affinity; }
void BPTask::set_affinity(CPU_Set *affi) { affinity = affi; }
bool BPTask::is_independent() const { return independent; }
void BPTask::set_dependent() { independent = false; }
bool BPTask::is_carry_in() const { return carry_in; }
void BPTask::set_carry_in() { carry_in = true; }
void BPTask::clear_carry_in() { carry_in = false; }
ulong BPTask::get_other_attr() const { return other_attr; }
void BPTask::set_other_attr(ulong attr) { other_attr = attr; }

/** Class BPTaskSet */

BPTaskSet::BPTaskSet() {
  utilization_sum = 0;
  utilization_max = 0;
  density_sum = 0;
  density_max = 0;
}

BPTaskSet::~BPTaskSet() { tasks.clear(); }

void BPTaskSet::init() {
  for (uint i = 0; i < tasks.size(); i++) tasks[i].init();
}

double BPTaskSet::get_utilization_sum() const { return utilization_sum; }

double BPTaskSet::get_utilization_max() const { return utilization_max; }

double BPTaskSet::get_density_sum() const { return density_sum; }

double BPTaskSet::get_density_max() const { return density_max; }

void BPTaskSet::add_task(BPTask task) { tasks.push_back(task); }

void BPTaskSet::add_task(ulong wcet, ulong period, ulong deadline,bool status) {
  double utilization_new = wcet, density_new = wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  tasks.push_back(BPTask(tasks.size(), wcet, period, deadline,status));
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}

void BPTaskSet::add_task(ResourceSet *resourceset, Param param, ulong wcet,
                       ulong period, ulong deadline,bool status) {
  double utilization_new = wcet, density_new = wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  tasks.push_back(
      BPTask(tasks.size(), resourceset, param, wcet, period, deadline,status));
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}

void BPTaskSet::add_task(uint r_id, ResourceSet* resourceset, Param param,
                       uint64_t ncs_wcet, uint64_t cs_wcet, uint64_t period,
                       uint64_t deadline,bool status) {
  double utilization_new = ncs_wcet + cs_wcet,
             density_new = ncs_wcet + cs_wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  tasks.push_back(BPTask(tasks.size(), r_id, resourceset, ncs_wcet, cs_wcet,
                       period, deadline,status));

  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}




BPTasks &BPTaskSet::get_tasks() { return tasks; }

BPTask &BPTaskSet::get_task_by_id(uint id) {
  foreach(tasks, task) {
    if (id == task->get_id()) return (*task);
  }
  return *reinterpret_cast<BPTask *>(0);
}

BPTask &BPTaskSet::get_task_by_index(uint index) { return tasks[index]; }

BPTask &BPTaskSet::get_task_by_priority(uint pi) {
  foreach(tasks, task) {
    if (pi == task->get_priority()) return (*task);
  }
  return *reinterpret_cast<BPTask *>(0);
}

bool BPTaskSet::is_implicit_deadline() {
  foreach_condition(tasks, tasks[i].get_deadline() != tasks[i].get_period());
  return true;
}
bool BPTaskSet::is_constrained_deadline() {
  foreach_condition(tasks, tasks[i].get_deadline() > tasks[i].get_period());
  return true;
}
bool BPTaskSet::is_arbitary_deadline() {
  return !(is_implicit_deadline()) && !(is_constrained_deadline());
}
uint BPTaskSet::get_taskset_size() const { return tasks.size(); }

double BPTaskSet::get_task_utilization(uint index) const {
  return tasks[index].get_utilization();
}
double BPTaskSet::get_task_density(uint index) const {
  return tasks[index].get_density();
}
ulong BPTaskSet::get_task_wcet(uint index) const {
  return tasks[index].get_wcet();
}
ulong BPTaskSet::get_task_deadline(uint index) const {
  return tasks[index].get_deadline();
}
ulong BPTaskSet::get_task_period(uint index) const {
  return tasks[index].get_period();
}

void BPTaskSet::sort_by_id() {
  sort(tasks.begin(), tasks.end(), id_increase<BPTask>);
}

void BPTaskSet::sort_by_index() {
  sort(tasks.begin(), tasks.end(), index_increase<BPTask>);
}

void BPTaskSet::sort_by_period() {
  sort(tasks.begin(), tasks.end(), period_increase<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_deadline() {
  sort(tasks.begin(), tasks.end(), deadline_increase<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_utilization() {
  sort(tasks.begin(), tasks.end(), utilization_decrease<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_density() {
  sort(tasks.begin(), tasks.end(), density_decrease<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_DC() {
  sort(tasks.begin(), tasks.end(), task_DC_increase<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_DCC() {
  sort(tasks.begin(), tasks.end(), task_DCC_increase<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_DDC() {
  sort(tasks.begin(), tasks.end(), task_DDC_increase<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::sort_by_UDC() {
  sort(tasks.begin(), tasks.end(), task_UDC_increase<BPTask>);
  for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
}

void BPTaskSet::RM_Order() {
  sort_by_period();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "RM Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void BPTaskSet::DM_Order() {
  sort_by_deadline();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void BPTaskSet::Density_Decrease_Order() {
  sort_by_density();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void BPTaskSet::DC_Order() {
  sort_by_DC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "DC Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void BPTaskSet::DCC_Order() {
  sort_by_DCC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void BPTaskSet::DDC_Order() {
  sort_by_DDC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void BPTaskSet::UDC_Order() {
  sort_by_UDC();
  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void BPTaskSet::SM_PLUS_Order() {
  sort_by_DC();

  if (1 < tasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < tasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < tasks.size() - 1; index++) {
      vector<BPTask>::iterator it = (tasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();

      for (int index2 = index + 1; index2 < tasks.size(); index2++) {
        vector<BPTask>::iterator it2 = (tasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();

        if (c > gap2 && d > d2) {
          accum[(it->get_id())] += c2;

          BPTask temp = (*it2);
          tasks.erase((it2));
          tasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }
}

void BPTaskSet::SM_PLUS_2_Order() {
  sort_by_DC();

  if (1 < tasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < tasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < tasks.size() - 1; index++) {
      vector<BPTask>::iterator it = (tasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();

      for (int index2 = index + 1; index2 < tasks.size(); index2++) {
        vector<BPTask>::iterator it2 = (tasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();

        if (c > gap2 && d > d2 && (accum[(it->get_id())] + c2) <= gap) {
          accum[(it->get_id())] += c2;

          BPTask temp = (*it2);
          tasks.erase((it2));
          tasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP2:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void BPTaskSet::SM_PLUS_3_Order() {
  sort_by_period();

  if (1 < tasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < tasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < tasks.size() - 1; index++) {
      vector<BPTask>::iterator it = (tasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();
      ulong p = (it)->get_period();

      for (int index2 = index + 1; index2 < tasks.size(); index2++) {
        vector<BPTask>::iterator it2 = (tasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();
        ulong p2 = (it2)->get_period();
        uint N = ceiling(p, p2);
        if (gap > gap2 && (accum[(it->get_id())] + N * c2) <= gap) {
          accum[(it->get_id())] += N * c2;

          BPTask temp = (*it2);
          tasks.erase((it2));
          tasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP3:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void BPTaskSet::Leisure_Order() {
  BPTasks NewSet;
  sort_by_id();

  for (int i = tasks.size() - 1; i >= 0; i--) {
    int64_t l_max = 0xffffffffffffffff;
    uint index = i;
    for (int j = i; j >= 0; j--) {
      vector<BPTask>::iterator it = (tasks.begin() + j);
      BPTask temp = (*it);
      tasks.erase((it));
      tasks.push_back(temp);
      int64_t l = leisure(i);
      if (l > l_max) {
        l_max = l;
        index = j;
      }
    }
    sort_by_id();
    vector<BPTask>::iterator it2 = (tasks.begin() + index);
    BPTask temp2 = (*it2);
    tasks.erase(it2);
    NewSet.insert(NewSet.begin(), temp2);
  }

  tasks.clear();
  tasks = NewSet;

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_index(i);
    tasks[i].set_priority(i);
  }

  cout << "Leisure Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
}

void BPTaskSet::SM_PLUS_4_Order(uint p_num) {
  sort_by_period();
#if SORT_DEBUG
  cout << "////////////////////////////////////////////" << endl;
  cout << "RM:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif

  if (1 < tasks.size()) {
    uint min_id;
    ulong min_slack;
    vector<uint> id_stack;

    foreach(tasks, task) {
      task->set_other_attr(0);  // accumulative adjustment
    }

    for (uint index = 0; index < tasks.size(); index++) {
      bool is_continue = false;
      min_id = 0;
      min_slack = MAX_LONG;
      foreach(tasks, task) {  // find next minimum slack
        is_continue = false;
        uint temp_id = task->get_id();
        foreach(id_stack, element) {
          if (temp_id == (*element)) is_continue = true;
        }
        if (is_continue) continue;
        ulong temp_slack = task->get_slack();
        if (min_slack > task->get_slack()) {
          min_id = temp_id;
          min_slack = temp_slack;
        }
      }
      id_stack.push_back(min_id);
      is_continue = false;

      // locate minimum slack
      for (uint index2 = 0; index2 < tasks.size(); index2++) {
        if (min_id == tasks[index2].get_id()) {
          vector<BPTask>::iterator task1 = (tasks.begin() + index2);
          for (int index3 = index2 - 1; index3 >= 0; index3--) {
            vector<BPTask>::iterator task2 = (tasks.begin() + index3);
            if ((p_num - 1) <= task2->get_other_attr()) {
              is_continue = true;
              break;
            }
            if (task1->get_slack() < task2->get_slack()) {
              BPTask temp = (*task1);
              tasks.erase((task1));
              if (task2->get_deadline() < task1->get_wcet() + task2->get_wcet())
                task2->set_other_attr(task2->get_other_attr() + 1);
              tasks.insert(task2, temp);
              task1 = (tasks.begin() + index3);
            }
          }

          break;
        }
      }
    }

    for (int i = 0; i < tasks.size(); i++) tasks[i].set_index(i);
  }

  for (uint i = 0; i < tasks.size(); i++) {
    tasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP4:" << endl;
  cout << "-----------------------" << endl;
  foreach(tasks, task) {
    cout << " BPTask " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Gap:" << task->get_deadline() - task->get_wcet()
         << " Leisure:" << leisure(task->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

ulong BPTaskSet::leisure(uint index) {
  BPTask &task = get_task_by_id(index);
  ulong gap = task.get_deadline() - task.get_wcet();
  ulong period = task.get_period();
  ulong remain = gap;
  for (uint i = 0; i < index; i++) {
    BPTask &temp = get_task_by_id(i);
    remain -= temp.get_wcet() * ceiling(period, temp.get_period());
  }
  return remain;
}

void BPTaskSet::display() {
  foreach(tasks, task) {
    cout << "BPTask" << task->get_index() << ": id:"
         << task->get_id()
         << ": partition:"
         << task->get_partition()
         << ": priority:"
         << task->get_priority() 
         <<"status"
         <<(int)task->get_status()
         <<"link id"
         <<task->get_linkID()
         << endl;
    cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
         << " cs-wcet:" << task->get_wcet_critical_sections()
         << " wcet:" << task->get_wcet()
         << " response time:" << task->get_response_time()
         << " deadline:" << task->get_deadline()
         << " period:" << task->get_period() << endl;
    foreach(task->get_requests(), request) {
      cout << "request " << request->get_resource_id() << ":"
           << " num:" << request->get_num_requests()
           << " length:" << request->get_max_length() << endl;
    }
    foreach(task->get_g_requests(), g_request) {
      cout << "g_request " << g_request->get_gpu_id() << ":"
           << " num:" << g_request->get_num_requests()
           << " b_num:" << g_request->get_g_block_num()
           << " t_num:" << g_request->get_g_thread_num()
           << " length:" << g_request->get_g_wcet()
           << " workload:" << g_request->get_total_workload() << endl;

    }
    cout << "-------------------------------------------" << endl;
    if (task->get_wcet() > task->get_response_time()) exit(0);
  }
  cout << get_utilization_sum() << endl;
  // for (int i = 0; i < tasks.size(); i++) {
  //   cout << "BPTask index:" << tasks[i].get_index()
  //        << " BPTask id:" << tasks[i].get_id()
  //        << " BPTask priority:" << tasks[i].get_priority() << endl;
  // }
}

void BPTaskSet::update_requests(const ResourceSet &resources) {
  foreach(tasks, task) { task->update_requests(resources); }
}

void BPTaskSet::export_taskset(string path) {
  ofstream file(path, ofstream::app);
  stringstream buf;
  buf << "Utilization:" << get_utilization_sum() << "\n";
  foreach(tasks, task) {
    buf << task->get_wcet() << "," << task->get_period() << ","
        << task->get_deadline();
    foreach(task->get_requests(), request) {
      buf << "," << request->get_resource_id() << ","
          << request->get_num_requests() << "," << request->get_max_length();
    }
    buf << "\n";
  }
  file << buf.str() << "\n";
  file.flush();
  file.close();
}

void BPTaskSet::task_gen(ResourceSet *resourceset, Param param,                         //2022/1
                       double utilization, uint task_number) {
  // Generate task set utilization...
  double SumU;
  vector<double> u_set;
  bool re_gen = true;

  switch (param.u_gen) {
    case GEN_UNIFORM:
    cout << "GEN_UNIFORM" << endl;
      SumU = 0;
      while (SumU < utilization) {
        double u = Random_Gen::uniform_real_gen(0, 1);           
        if (1 + _EPS <= u)
          continue;
        if (SumU + u > utilization) {
          u = utilization - SumU;
          u_set.push_back(u);
          SumU += u;
          break;
        }
        u_set.push_back(u);
        SumU += u;
      }
      break;

    case GEN_EXPONENTIAL:
    cout << "GEN_EXPONENTIAL" << endl;
      SumU = 0;
      while (SumU < utilization) {
        double u = Random_Gen::exponential_gen(1/param.mean);
        if (1 + _EPS <= u)
          continue;
        if (SumU + u > utilization) {
          u = utilization - SumU;
          u_set.push_back(u);
          SumU += u;
          break;
        }
        u_set.push_back(u);
        SumU += u;
      }
      break;

    case GEN_UUNIFAST:  // Same as the UUnifast_Discard
    cout << "GEN_UUNIFAST" << endl;
    case GEN_UUNIFAST_D:
    cout << "GEN_UUNIFAST_D" << endl;
      while (re_gen) {
        SumU = utilization;
        re_gen = false;
        for (uint i = 1; i < task_number; i++) {
          double temp = Random_Gen::uniform_real_gen(0, 1);
          double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
          double u = SumU - NextSumU;
          if ((u - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            // cout << "Regenerate taskset." << endl;
            break;
          } else {
            u_set.push_back(u);
            SumU = NextSumU;
          }
        }
        if (!re_gen) {
          if ((SumU - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            // cout << "Regenerate taskset." << endl;
          } else {
            u_set.push_back(SumU);
          }
        }
      }
      break;

    default:
      while (re_gen) {
        SumU = utilization;
        re_gen = false;
        for (uint i = 1; i < task_number; i++) {
          double temp = Random_Gen::uniform_real_gen(0, 1);
          double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
          double u = SumU - NextSumU;
          if ((u - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            cout << "Regenerate taskset." << endl;
            break;
          } else {
            u_set.push_back(u);
            SumU = NextSumU;
          }
        }
        if (!re_gen) {
          if ((SumU - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            cout << "Regenerate taskset." << endl;
          } else {
            u_set.push_back(SumU);
          }
        }
      }
      break;
  }

  uint i = 0;
  foreach(u_set, u) {
    // ulong period = param.p_range.min + Random_Gen::exponential_gen(2) * (param.p_range.max - param.p_range.min);
    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    ulong wcet = ceil((*u) * period);
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      if (deadline < wcet) deadline = wcet;
    }
    add_task(resourceset, param, wcet, period, deadline,true);
  }
  sort_by_period();
}

void BPTaskSet::task_load(ResourceSet* resourceset, string file_name,bool status) {
  ifstream file(file_name, ifstream::in);
  string buf;
  uint id = 0;
  while (getline(file, buf)) {
    if (NULL != strstr(buf.data(), "Utilization")) continue;
    if (0 == strcmp(buf.data(), "\r")) break;

    vector<uint64_t> elements;
    extract_element(&elements, buf, 0, MAX_INT);
    if (3 <= elements.size()) {
      BPTask task(id, elements[0], elements[1], elements[2],status);
      uint n = (elements.size() - 3) / 3;
      for (uint i = 0; i < n; i++) {
        uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
        task.add_request(elements[3 + i * 3], elements[4 + i * 3],
                         elements[5 + i * 3], length);

        resourceset->add_task(elements[3 + i * 3], id);
      }
      add_task(task);

      id++;
    }
  }
  sort_by_period();
}

void BPTaskSet::task_copy(int size)
{
   int times=0;
   foreach(tasks,ti)
   {
    times++;
    if(times>size)
    break;
   bool status=false;
    BPTask temp_task(tasks.size()+ti->get_id(),ti->get_wcet(),ti->get_period(),ti->get_deadline(),status);
    add_task(temp_task);
    cout<<"added id:"<<tasks.size()+ti->get_id()<<"wcet"<<ti->get_wcet()<<"period"<<ti->get_period()<<"deadline"<<ti->get_deadline()<<endl;
   }
}


/** Task DAG_Task */
// : Task(dt.get_id(), 0, dt.get_period(), dt.get_deadline(), dt.get_priority())
DAG_Task::DAG_Task(const DAG_Task &dt) {
  id = dt.id;
  vnodes = dt.vnodes;
  arcnodes = dt.arcnodes;
  wcet = dt.wcet;  // total wcet of the jobs in graph
  this->wcet_non_critical_sections = dt.wcet_non_critical_sections;
  this->wcet_critical_sections = dt.wcet_critical_sections;
  len = dt.len;
  deadline = dt.deadline;
  period = dt.period;
  utilization = dt.utilization;
  density = dt.density;
  vexnum = dt.vexnum;
  arcnum = dt.arcnum;
  spin = dt.spin;
  self_suspension = dt.self_suspension;
  local_blocking = dt.local_blocking;
  remote_blocking = dt.remote_blocking;
  total_blocking = dt.total_blocking;
  jitter = dt.jitter;
  response_time = dt.response_time;  // initialization as WCET
  priority = dt.priority;
  partition = dt.partition;  // 0XFFFFFFFF
  ratio = dt.ratio;          // for heterogeneous platform
  requests = dt.requests;
  parallel_degree = dt.parallel_degree;
  other_attr = dt.other_attr;

  refresh_relationship();
  // update_parallel_degree();
}

// : Task(task_id, 0, period, deadline, priority)
DAG_Task::DAG_Task(uint task_id, ulong period, ulong deadline, uint priority) {
  len = 0;
  wcet = 0;
  if (0 == deadline) this->deadline = period;
  this->period = period;
  vexnum = 0;
  arcnum = 0;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  remote_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  priority = priority;
  partition = 0XFFFFFFFF;
  utilization = 0;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  other_attr = 0;
  refresh_relationship();
  // update_parallel_degree();
}

// Xu Jiang's generation method
DAG_Task::DAG_Task(uint task_id, ResourceSet *resourceset, Param param,
                   uint priority) {
  this->id = task_id;
  this->len = 0;
  this->wcet_critical_sections = 0;
  vexnum = 0;
  arcnum = 0;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  remote_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  this->priority = priority;
  partition = 0XFFFFFFFF;
  other_attr = 0;

  vnodes.clear();
  arcnodes.clear();
  // Generate DAG
  uint v_num;
  if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
    v_num = param.job_num_range.max;
  } else {
    v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                             param.job_num_range.max);
  }
  Erdos_Renyi_v2(v_num, param.edge_prob);
  update_wcet();
  wcet_non_critical_sections = wcet;

  // Request generation
  uint critical_section_num = 0;
  for (int i = 0; i < resourceset->size(); i++) {
    if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
      if (Random_Gen::probability(param.rrp)) {
        uint32_t num;
        uint64_t max_len;
        if (0 == strcmp(param.graph_x_label.c_str(), "Critical Section Number per Task")) {
          // cout << "Critical Section Number per Task." << endl;
          num = min(static_cast<uint>(param.reserve_double_2),
                      static_cast<uint>(param.mcsn - critical_section_num));
        } else {
          num = Random_Gen::uniform_integral_gen(
              1, min(param.rrn,
                    static_cast<uint>(param.mcsn - critical_section_num)));
        }
        // uint num = min(static_cast<uint>(param.reserve_double_2),
        //            static_cast<uint>(param.mcsn - critical_section_num));
        // uint num = min(param.rrn,
        //                static_cast<uint>(param.mcsn - critical_section_num));
        if (0 == strcmp(param.graph_x_label.c_str(), "Critical Section Length per Request")) {
          // cout << "Critical Section Length per Request." << endl;
          max_len = min(static_cast<double>(wcet), param.reserve_double_2);
        } else {
          max_len = Random_Gen::uniform_integral_gen(
              param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
        }
        // 
        // uint max_len = min(wcet, static_cast<uint64_t>(param.reserve_int_3));
        // uint max_len =
        // min(static_cast<double>(wcet), param.rrr.max);
        ulong length = num * max_len;
        if (length >= wcet_non_critical_sections) continue;

        // wcet_non_critical_sections -= length;
        // wcet_critical_sections += length;

        

        if (0 == strcmp(param.graph_x_label.c_str(), "Minimum Ratio")) {
          // cout << "[adding request]Rate Parameter:" << param.reserve_double_2 << endl;
          max_len = min(static_cast<double>(wcet), param.rrr.max);
          add_request(i, num, max_len, length,
                      resourceset->get_resource_by_id(i).get_locality(), true, param.reserve_double_2);
        } else {
          add_request(i, num, max_len, length,
                      resourceset->get_resource_by_id(i).get_locality());
        }

        resourceset->add_task(i, id);
        critical_section_num += num;
      }
    }
  }

  // display_vertices();
  // display_arcs();
  refresh_relationship();
  dcores = get_parallel_degree();
  update_wcet();
  update_len();
  // double ratio = Random_Gen::uniform_real_gen(0.125, 0.25);
  // double ratio = Random_Gen::uniform_real_gen(0.25, 0.5);
  double ratio = Random_Gen::uniform_real_gen(0.125, param.reserve_double_4);
  this->deadline = get_critical_path_length() / ratio;
  this->period = this->deadline;
  this->response_time = this->deadline;
  utilization = this->wcet;
  utilization /=this->period;
  density = len;
  density /= this->deadline;
}

DAG_Task::DAG_Task(uint task_id, ResourceSet* resourceset, Param param,
           double utilization, uint priority) {
  this->id = task_id;
  this->len = 0;
  this->wcet_critical_sections = 0;
  vexnum = 0;
  arcnum = 0;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  remote_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  this->priority = priority;
  partition = 0XFFFFFFFF;
  other_attr = 0;

  do {
    vnodes.clear();
    arcnodes.clear();
    requests.clear();
    foreach(resourceset->get_resources(), resource) {
      resource->remove_task(id);
    }
    // Generate DAG
    if (DAG_GEN_ERDOS_RENYI == param.dag_gen) {
      period =
          Random_Gen::uniform_ulong_gen(static_cast<ulong>(param.p_range.min),
                                        static_cast<ulong>(param.p_range.max));
      wcet = ceil(utilization* period);
      // cout << "generate period in ["<< param.p_range.min << "," << param.p_range.max << "]" << endl;
      // cout << "T_1=" << this->period << endl;
      uint v_num;
      if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
        v_num = param.job_num_range.max;
      } else {
        v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                                param.job_num_range.max);
      }
      Erdos_Renyi(v_num, param.edge_prob);
      update_wcet();
      wcet_non_critical_sections = wcet;
    } else if (DAG_GEN_ERDOS_RENYI_v2 == param.dag_gen) {
      uint v_num;
      if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
        v_num = param.job_num_range.max;
      } else {
        v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                                param.job_num_range.max);
      }
      Erdos_Renyi_v2(v_num, param.edge_prob);
      update_wcet();
      wcet_non_critical_sections = wcet;
    } else if (DAG_GEN_TEST == param.dag_gen) {
      cout << "DAG_GEN_TEST_1" << endl;
      // period =
      //     Random_Gen::uniform_ulong_gen(static_cast<ulong>(param.p_range.min),
      //                                   static_cast<ulong>(param.p_range.max));
      // wcet = ceil(utilization* period);
      // // cout << wcet << " " << period << endl;
      // uint v_num;
      // if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
      //   v_num = param.job_num_range.max;
      // } else {
      //   v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
      //                                           param.job_num_range.max);
      // }
      // Erdos_Renyi(v_num, param.edge_prob);
      // update_wcet();
      // wcet_non_critical_sections = wcet;
      uint v_num;
      if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
        v_num = param.job_num_range.max;
      } else {
        v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                                param.job_num_range.max);
      }
      Erdos_Renyi_v2(v_num, param.edge_prob);
      update_wcet();
      wcet_non_critical_sections = wcet;
    } else if (DAG_GEN_TEST == param.dag_gen) {
      cout << "DAG_GEN_TEST_2" << endl;
      // period =
      //     Random_Gen::uniform_ulong_gen(static_cast<ulong>(param.p_range.min),
      //                                   static_cast<ulong>(param.p_range.max));
      // wcet = ceil(utilization* period);
      // // cout << wcet << " " << period << endl;
      // uint v_num;
      // if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
      //   v_num = param.job_num_range.max;
      // } else {
      //   v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
      //                                           param.job_num_range.max);
      // }
      // Erdos_Renyi(v_num, param.edge_prob);
      // update_wcet();
      // wcet_non_critical_sections = wcet;
      uint v_num;
      if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
        v_num = param.job_num_range.max;
      } else {
        v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                                param.job_num_range.max);
      }
      Erdos_Renyi_v2(v_num, param.edge_prob);
      update_wcet();
      wcet_non_critical_sections = wcet;
    } else if (DAG_GEN_SIM == param.dag_gen) {
    }

    // Request generation
    uint critical_section_num = 0;
    for (int i = 0; i < resourceset->size(); i++) {
      if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
        if (Random_Gen::probability(param.rrp)) {
          uint32_t num;
          uint64_t max_len;
          if (0 == strcmp(param.graph_x_label.c_str(), "Critical Section Number per Task")) {
            // cout << "Critical Section Number per Task." << endl;
            num = min(static_cast<uint>(param.reserve_double_2),
                        static_cast<uint>(param.mcsn - critical_section_num));
          } else {
            num = Random_Gen::uniform_integral_gen(
                1, min(param.rrn,
                      static_cast<uint>(param.mcsn - critical_section_num)));
          }
          if (0 == strcmp(param.graph_x_label.c_str(), "Critical Section Length per Request")) {
            // cout << "Critical Section Length per Request." << endl;
            max_len = min(static_cast<double>(wcet), param.reserve_double_2);
          } else {
            max_len = Random_Gen::uniform_integral_gen(
                param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
          }
          ulong length = num * max_len;
          if (length >= wcet_non_critical_sections) continue;
          // cout << length << endl;
          // wcet_non_critical_sections -= length;
          // wcet_critical_sections += length;

          add_request(i, num, max_len, length,
                      resourceset->get_resource_by_id(i).get_locality());

          resourceset->add_task(i, id);
          critical_section_num += num;
        }
      }
    }
    

    if (DAG_GEN_SIM != param.dag_gen) {
      // allocate requests to vnodes
      foreach(requests, request) {
        vector<uint32_t> points;
        vector<uint32_t> v_id;
        points.push_back(0);
        uint32_t r_num = Random_Gen::uniform_ulong_gen(1, max((uint32_t)1, min((uint32_t)vnodes.size(),request->get_num_requests()/2)));

        for (uint i = 1; i < r_num; i++) {
          bool test;
          ulong temp;
          do {
            test = false;
            temp = Random_Gen::uniform_ulong_gen(1, request->get_num_requests() - 1);
            for (uint j = 0; j < points.size(); j++)
              if (temp == points[j]) test = true;
          } while (test);
          points.push_back(temp);
        }

        points.push_back(request->get_num_requests());
        sort(points.begin(), points.end());  // increasae order

        uint32_t v_num = points.size() - 1;
        do {
          bool test = false;
          uint32_t rv_id = Random_Gen::uniform_ulong_gen(0, vnodes.size()-1);

          for (uint j = 0; j < v_id.size(); j++)
            if (rv_id == v_id[j]) {
              test = true;
              break;
            }
          if (!test) {
            v_id.push_back(rv_id);
            v_num--;
          }
        } while(0 < v_num);

    // cout << "5555" << endl;
        for (uint i = 0; i < r_num; i++) {
          uint32_t num = points[i+1]-points[i];
          Request r_q(request->get_resource_id(), num, request->get_max_length(), num*request->get_max_length(),request->get_locality());
          vnodes[v_id[i]].requests.push_back(r_q);
        }
    // cout << "6666" << endl;
      }
    }
    
    // display_vertices();
    // display_arcs();
    // Time_Record timer;
    // double ms_time = 0;
    // timer.Record_MS_A();
    if (DAG_GEN_SIM != param.dag_gen) {
      refresh_relationship();
      dcores = get_parallel_degree();
      update_wcet();
      update_len();
      this->period = get_wcet() / utilization;
      // cout << "T_2=" << this->period << endl;
      this->deadline = this->period;
      this->response_time = this->deadline;
      this->utilization = utilization;
      density = len;
      density /= this->deadline;
    } else {
      this->period = Random_Gen::uniform_ulong_gen(
            static_cast<ulong>(param.p_range.min),
            static_cast<ulong>(param.p_range.max));
      this->wcet = ceil(utilization * period);
      wcet_non_critical_sections = wcet;
      if (fabs(param.d_range.max - 1) > _EPS) {
        this->deadline = ceil(period * Random_Gen::uniform_real_gen(
                                     param.d_range.min, param.d_range.max));
        // if (deadline < wcet) deadline = wcet;
      } else {
        this->deadline = this->period;
      }
      this->response_time = this->deadline;
      double ratio = Random_Gen::uniform_real_gen(0.125, param.reserve_double_4);
      density = min(ratio, utilization);
      len = density * this->deadline;
      this->utilization = utilization;
    }
    // timer.Record_MS_B();
    // ms_time = timer.Record_MS();
    // cout << "time to update CPL:" << ms_time << endl;
    // double ratio = Random_Gen::uniform_real_gen(0.125, 0.25);
    double ratio = len;
    ratio /= deadline;
    cout << "Ratio:" << ratio << " CPL:" << len << " D:" << deadline << " U:" << utilization << " parallel degree:" << parallel_degree << endl;
  } while (len > param.reserve_double_4 * deadline);
  // display();
  // cout << "------------------------" << endl;
}

// : Task(task_id, resourceset, param, wcet, period, deadline, priority)
DAG_Task::DAG_Task(uint task_id, ResourceSet *resourceset, Param param,
                   ulong wcet, ulong period, ulong deadline, uint priority) {
  this->id = task_id;
  this->len = 0;
  this->wcet = wcet;
  this->wcet_non_critical_sections = this->wcet;
  this->wcet_critical_sections = 0;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  vexnum = 0;
  arcnum = 0;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  remote_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  this->priority = priority;
  partition = 0XFFFFFFFF;
  utilization = wcet;
  utilization /= period;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  other_attr = 0;

  do {
    vnodes.clear();
    arcnodes.clear();
    requests.clear();
    // Generate DAG
    if (DAG_GEN_FORK_JOIN == param.dag_gen) {
      // fork-join method
      uint JobNode_num = 1;
      graph_gen(&vnodes, &arcnodes, param, JobNode_num);
      vector<VNode> v1, v2;
      vector<ArcNode> a1, a2;
      sequential_graph_gen(&v1, &a1, 3);
      graph_insert(&v1, &a1, 1);

      uint pd;
      if (2 > wcet)
        pd = 1;
      else
        pd = Random_Gen::uniform_integral_gen(
            2, min(static_cast<uint32_t>(wcet), param.max_para_job));
        // pd = min(static_cast<uint32_t>(wcet), param.max_para_job);
        // pd = min(static_cast<uint32_t>(wcet),
        //          static_cast<uint32_t>(param.reserve_double_2));
        // pd = min(static_cast<double>(wcet), static_cast<double>(param.p_num));
      sub_graph_gen(&v2, &a2, pd, G_TYPE_P);
      graph_insert(&v2, &a2, 2);
      parallel_degree = pd;
      // Alocate wcet
      uint job_node_num = 0;
      for (uint i = 0; i < vnodes.size(); i++) {
        if (J_NODE == vnodes[i].type) job_node_num++;
      }
      vector<ulong> wcets;
      wcets.push_back(0);
      for (uint i = 1; i < job_node_num; i++) {
        bool test;
        ulong temp;
        do {
          test = false;
          temp = Random_Gen::uniform_ulong_gen(1, wcet - 1);
          for (uint j = 0; j < wcets.size(); j++)
            if (temp == wcets[j]) test = true;
        } while (test);
        wcets.push_back(temp);
      }
      wcets.push_back(wcet);
      sort(wcets.begin(), wcets.end());
      for (uint i = 0, j = 0; i < vnodes.size(); i++) {
        if (J_NODE == vnodes[i].type) {
          vnodes[i].wcet = wcets[j + 1] - wcets[j];
          j++;
        }
        vnodes[i].deadline = this->deadline;
      }
    } else if (DAG_GEN_ERDOS_RENYI == param.dag_gen) {
      // Erdos Renyi method
      uint v_num;
      if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
        v_num = param.job_num_range.max;
      } else {
        v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                                param.job_num_range.max);
      }
      Erdos_Renyi(v_num, param.edge_prob);
    } else {
      // default method
      uint v_num;
      if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
        v_num = param.job_num_range.max;
      } else {
        v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                                param.job_num_range.max);
      }
      if (v_num > wcet) v_num = wcet;
      graph_gen(&vnodes, &arcnodes, param, v_num);

      // Alocate wcet
      uint job_node_num = 0;
      for (uint i = 0; i < vnodes.size(); i++) {
        if (J_NODE == vnodes[i].type) job_node_num++;
      }
      vector<ulong> wcets;
      wcets.push_back(0);
      for (uint i = 1; i < job_node_num; i++) {
        bool test;
        ulong temp;
        do {
          test = false;
          temp = Random_Gen::uniform_ulong_gen(1, wcet - 1);
          for (uint j = 0; j < wcets.size(); j++)
            if (temp == wcets[j]) test = true;
        } while (test);
        wcets.push_back(temp);
      }
      wcets.push_back(wcet);
      sort(wcets.begin(), wcets.end());
      for (uint i = 0, j = 0; i < vnodes.size(); i++) {
        if (J_NODE == vnodes[i].type) {
          vnodes[i].wcet = wcets[j + 1] - wcets[j];
          j++;
        }
        vnodes[i].deadline = this->deadline;
      }
    }

    // Insert conditional branches
    for (uint i = 1; i < vnodes.size() - 1; i++) {
      if (Random_Gen::probability(param.cond_prob)) {
        uint cond_job_num = Random_Gen::uniform_integral_gen(
            2, max(2, static_cast<int>(param.max_cond_branch)));
        vector<VNode> v;
        vector<ArcNode> a;
        sub_graph_gen(&v, &a, cond_job_num, G_TYPE_C);
        graph_insert(&v, &a, i);
        break;
      }
    }

    // Request generation
    uint critical_section_num = 0;

    for (int i = 0; i < resourceset->size(); i++) {
      if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
        if (Random_Gen::probability(param.rrp)) {
          // uint num = Random_Gen::uniform_integral_gen(
          //     1, min(param.rrn,
          //            static_cast<uint>(param.mcsn - critical_section_num)));
          uint num = min(static_cast<uint>(param.reserve_double_2),
                     static_cast<uint>(param.mcsn - critical_section_num));
          // uint num = min(param.rrn,
          //                static_cast<uint>(param.mcsn - critical_section_num));
          uint max_len = Random_Gen::uniform_integral_gen(
              param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
          // uint max_len = min(static_cast<double>(wcet), param.reserve_double_2);
          // uint max_len = min(wcet, static_cast<uint64_t>(param.reserve_int_3));
          // uint max_len =
          // min(static_cast<double>(wcet), param.rrr.max);
          ulong length = num * max_len;
          if (length >= wcet_non_critical_sections) continue;

          // wcet_non_critical_sections -= length;
          // wcet_critical_sections += length;

          add_request(i, num, max_len, length,
                      resourceset->get_resource_by_id(i).get_locality());

          resourceset->add_task(i, id);
          critical_section_num += num;
        }
      }
    }

    // display_vertices();
    // display_arcs();
    refresh_relationship();
    update_parallel_degree();
    dcores = get_parallel_degree();
    update_wcet();
    update_len();
    density = len;
    density /= this->deadline;
  } while (len > param.reserve_double_4 * deadline);
}

// fixed critical path proportion
DAG_Task::DAG_Task(uint task_id, ResourceSet *resourceset, Param param,
                   double cpp, ulong wcet, ulong period, ulong deadline,
                   uint priority) {
  this->id = task_id;
  this->len = 0;
  this->wcet = wcet;
  this->wcet_non_critical_sections = this->wcet;
  this->wcet_critical_sections = 0;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  vexnum = 0;
  arcnum = 0;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  remote_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  priority = 0;
  partition = 0XFFFFFFFF;
  utilization = wcet;
  utilization /= period;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  other_attr = 0;

  // Generate DAG
  if (DAG_GEN_FORK_JOIN == param.dag_gen) {
    // fork-join method
    uint JobNode_num = 1;
    graph_gen(&vnodes, &arcnodes, param, JobNode_num);
    vector<VNode> v1, v2;
    vector<ArcNode> a1, a2;
    sequential_graph_gen(&v1, &a1, 3);
    graph_insert(&v1, &a1, 1);

    uint pd;
    if (2 > wcet)
      pd = 1;
    else
      pd = Random_Gen::uniform_integral_gen(
          2, min(static_cast<uint32_t>(wcet), param.max_para_job));
      // pd = min(static_cast<uint32_t>(wcet), param.max_para_job);
      // pd = min(static_cast<uint32_t>(wcet),
      //          static_cast<uint32_t>(param.reserve_double_2));
      // pd = min(static_cast<double>(wcet), static_cast<double>(param.p_num));
    sub_graph_gen(&v2, &a2, (pd - 2), G_TYPE_P);
    graph_insert(&v2, &a2, 2);
    parallel_degree = pd;
    // Alocate wcet
    uint job_node_num = 0;
    for (uint i = 0; i < vnodes.size(); i++) {
      if (J_NODE == vnodes[i].type) job_node_num++;
    }
    ulong sum = wcet;
    ulong block = wcet/job_node_num;
    // ulong block = (cpp * static_cast<double>(period) / 3.0);
    for (uint i = 0, j = 0; j < job_node_num; i++) {
      if (J_NODE == vnodes[i].type) {
        j++;
        if (j == job_node_num) {
          vnodes[i].wcet = sum;
        } else {
          vnodes[i].wcet = block;
          sum -= block;
        }
      }
      vnodes[i].deadline = this->deadline;
    }
  } else if (DAG_GEN_ERDOS_RENYI == param.dag_gen) {
    // Erdos Renyi method
    uint v_num;
    if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
      v_num = param.job_num_range.max;
    } else {
      v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                              param.job_num_range.max);
    }
    Erdos_Renyi(v_num, param.edge_prob);
  } else {
    // default method
    uint v_num;
    if (0 == strcmp(param.graph_x_label.c_str(), "Number of Vertices")) {
      v_num = param.job_num_range.max;
    } else {
      v_num = Random_Gen::uniform_integral_gen(param.job_num_range.min,
                                              param.job_num_range.max);
    }
    if (v_num > wcet) v_num = wcet;
    graph_gen(&vnodes, &arcnodes, param, v_num);

    // Alocate wcet
    uint job_node_num = 0;
    for (uint i = 0; i < vnodes.size(); i++) {
      if (J_NODE == vnodes[i].type) job_node_num++;
    }
    ulong sum = wcet;
    ulong block = wcet/job_node_num;
    // ulong block = (cpp * static_cast<double>(period) / 3.0);
    for (uint i = 0, j = 0; j < job_node_num; i++) {
      if (J_NODE == vnodes[i].type) {
        j++;
        if (j == job_node_num) {
          vnodes[i].wcet = sum;
        } else {
          vnodes[i].wcet = block;
          sum -= block;
        }
      }
      vnodes[i].deadline = this->deadline;
    }
  }

  // Insert conditional branches
  for (uint i = 1; i < vnodes.size() - 1; i++) {
    if (Random_Gen::probability(param.cond_prob)) {
      uint cond_job_num = Random_Gen::uniform_integral_gen(
          2, max(2, static_cast<int>(param.max_cond_branch)));
      vector<VNode> v;
      vector<ArcNode> a;
      sub_graph_gen(&v, &a, cond_job_num, G_TYPE_C);
      graph_insert(&v, &a, i);
      break;
    }
  }

  // Request generation
  uint critical_section_num = 0;

  for (int i = 0; i < resourceset->size(); i++) {
    if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
      if (Random_Gen::probability(param.rrp)) {
        uint num = Random_Gen::uniform_integral_gen(
            1, min(param.rrn,
                   static_cast<uint>(param.mcsn - critical_section_num)));
        // uint num = min(static_cast<uint>(param.reserve_double_2),
        //            static_cast<uint>(param.mcsn - critical_section_num));
        // uint num = min(param.rrn,
        //                static_cast<uint>(param.mcsn - critical_section_num));
        // int max_len = (param.rrr.min + param.rrr.max)/2;
        uint max_len = Random_Gen::uniform_integral_gen(
            param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
        // uint max_len = min(static_cast<double>(wcet),
        // param.reserve_double_2);
        // uint max_len = min(static_cast<double>(wcet),
        // param.reserve_double_3); uint max_len =
        // min(static_cast<double>(wcet), param.rrr.max);
        ulong length = num * max_len;
        if (length >= wcet_non_critical_sections) continue;

        // wcet_non_critical_sections -= length;
        // wcet_critical_sections += length;

        add_request(i, num, max_len, length,
                    resourceset->get_resource_by_id(i).get_locality());

        resourceset->add_task(i, id);
        critical_section_num += num;
      }
    }
  }

  // display_vertices();
  // display_arcs();
  refresh_relationship();
  update_parallel_degree();
  dcores = get_parallel_degree();
  update_wcet();
  update_len();
  density = len;
  density /= this->deadline;

  // uint64_t CPL = get_critical_path_length();

  // cout << "CPL:" << get_critical_path_length() << endl;

  // cout << "CPP:"
  //      << (static_cast<double>(get_critical_path_length()) /
  //          static_cast<double>(get_period()))
  //      << endl;
}

const Resource_Requests &DAG_Task::get_requests() const { return requests; }

const Request &DAG_Task::get_request_by_id(uint id) const {
  Request *result = NULL;
  for (uint i = 0; i < requests.size(); i++) {
    if (id == requests[i].get_resource_id()) return requests[i];
  }
  return *result;
}

bool DAG_Task::is_request_exist(uint resource_id) const {
  for (uint i = 0; i < requests.size(); i++) {
    if (resource_id == requests[i].get_resource_id()) return true;
  }
  return false;
}

void DAG_Task::add_request(uint res_id, uint num, ulong max_len,
                           ulong total_len, uint locality, bool FG, double mean) {
  wcet_non_critical_sections -= total_len;
  wcet_critical_sections += total_len;
  requests.push_back(Request(res_id, num, max_len, total_len, locality, FG, mean));
}

uint DAG_Task::get_max_job_num(ulong interval) const {
  uint num_jobs;
  // if (get_wcet() > (interval + get_response_time()))
  // cout << "id " << id << " wcet:" << get_wcet()
  //      << " response time:" << get_response_time() << endl;
  // num_jobs =
  //     ceiling(interval + get_response_time() - get_wcet() / parallel_degree,
  //             get_period());
  num_jobs =
      ceiling(interval + get_deadline(),
              get_period());
  return num_jobs;
}

uint DAG_Task::get_max_request_num(uint resource_id, ulong interval) const {
  if (is_request_exist(resource_id)) {
    const Request &request = get_request_by_id(resource_id);
    // cout << "get_max_job_num(interval) " << get_max_job_num(interval)
    //      << endl;
    return get_max_job_num(interval) * request.get_num_requests();
  } else {
    return 0;
  }
}

uint DAG_Task::get_max_request_num_nu(uint resource_id, ulong interval) const {
  if (is_request_exist(resource_id)) {
    const Request &request = get_request_by_id(resource_id);
    // cout << "get_max_job_num(interval) " << get_max_job_num(interval)
    //      << endl;
    return get_max_job_num(interval) * request.get_num_requests();
  } else {
    return 0;
  }
}

void DAG_Task::graph_gen(vector<VNode> *v, vector<ArcNode> *a, Param param,
                         uint n_num, double arc_density) {
  v->clear();
  a->clear();
  // creating vnodes
  VNode polar_start, polar_end;
  polar_start.job_id = 0;
  polar_end.job_id = n_num + 1;
  polar_start.type = P_NODE | S_NODE;
  polar_end.type = P_NODE | E_NODE;
  polar_start.pair = polar_end.job_id;
  polar_end.pair = polar_start.job_id;
  polar_start.wcet = 0;
  polar_end.wcet = 0;

  v->push_back(polar_start);
  for (uint i = 0; i < n_num; i++) {
    VNode temp_node;
    temp_node.job_id = v->size();
    temp_node.type = J_NODE;
    temp_node.pair = MAX_INT;
    temp_node.wcet = 0;
    v->push_back(temp_node);
  }
  v->push_back(polar_end);
  // creating arcs
  uint ArcNode_num;
  if (param.is_cyclic)  // cyclic graph
    ArcNode_num =
        Random_Gen::uniform_integral_gen(0, n_num * (n_num - 1)) * param.mean;
  else  // acyclic graph
    ArcNode_num =
        Random_Gen::uniform_integral_gen(0, (n_num * (n_num - 1)) / 2) *
        param.mean;
  for (uint i = 0; i < ArcNode_num; i++) {
    uint tail, head, temp;
    do {
      if (param.is_cyclic) {  // cyclic graph
        tail = Random_Gen::uniform_integral_gen(1, n_num);
        head = Random_Gen::uniform_integral_gen(1, n_num);
        if (tail == head) continue;
      } else {  // acyclic graph
        tail = Random_Gen::uniform_integral_gen(1, n_num);
        head = Random_Gen::uniform_integral_gen(1, n_num);
        if (tail == head) {
          continue;
        } else if (tail > head) {
          temp = tail;
          tail = head;
          head = temp;
        }
      }

      if (is_arc_exist(*a, tail, head)) {
        continue;
      }
      break;
    } while (true);
    add_arc(a, tail, head);
  }
  refresh_relationship(v, a);
  for (uint i = 1; i <= n_num; i++) {
    if (0 == (*v)[i].precedences.size()) add_arc(a, 0, i);
    if (0 == (*v)[i].follow_ups.size()) add_arc(a, i, n_num + 1);
  }
  refresh_relationship(v, a);
}

void DAG_Task::sub_graph_gen(vector<VNode> *v, vector<ArcNode> *a, uint n_num,
                             int G_TYPE) {
  v->clear();
  a->clear();

  // creating vnodes
  VNode polar_start, polar_end;
  polar_start.job_id = 0;
  polar_end.job_id = n_num + 1;
  if (G_TYPE == G_TYPE_P) {
    polar_start.type = P_NODE | S_NODE;
    polar_end.type = P_NODE | E_NODE;
  } else {
    polar_start.type = C_NODE | S_NODE;
    polar_end.type = C_NODE | E_NODE;
  }
  polar_start.pair = polar_end.job_id;
  polar_end.pair = polar_start.job_id;
  polar_start.wcet = 0;
  polar_end.wcet = 0;

  v->push_back(polar_start);
  for (uint i = 0; i < n_num; i++) {
    VNode temp_node;
    temp_node.job_id = v->size();
    temp_node.type = J_NODE;
    temp_node.pair = MAX_INT;
    temp_node.wcet = 0;
    v->push_back(temp_node);
  }
  v->push_back(polar_end);
  // creating arcs
  for (uint i = 1; i <= n_num; i++) {
    if (0 == (*v)[i].precedences.size()) add_arc(a, 0, i);
    if (0 == (*v)[i].follow_ups.size()) add_arc(a, i, n_num + 1);
  }
  refresh_relationship(v, a);
}

void DAG_Task::sequential_graph_gen(vector<VNode> *v, vector<ArcNode> *a,
                                    uint n_num) {
  v->clear();
  a->clear();
  for (uint i = 0; i < n_num; i++) {
    VNode temp_node;
    temp_node.job_id = v->size();
    temp_node.type = J_NODE;
    temp_node.pair = MAX_INT;
    temp_node.wcet = 0;
    v->push_back(temp_node);
  }
  for (uint i = 0; i < n_num - 1; i++) {
    add_arc(a, i, i + 1);
  }
  refresh_relationship(v, a);
}

void DAG_Task::graph_insert(vector<VNode> *v, vector<ArcNode> *a,
                            uint replace_node) {
  if (replace_node >= vnodes.size()) {
    cout << "Out of bound." << endl;
    return;
  }
  if (J_NODE != vnodes[replace_node].type) {
    cout << "Only job node could be replaced." << endl;
    return;
  }
  uint v_num = v->size();
  uint a_num = a->size();
  for (uint i = 0; i < v_num; i++) {
    (*v)[i].job_id += replace_node;
    // cout << (*v)[i].job_id << endl;
    if (MAX_INT != (*v)[i].pair) (*v)[i].pair += replace_node;
  }
  int gap = v_num - 1;
  for (uint i = replace_node + 1; i < vnodes.size(); i++) {
    vnodes[i].job_id += gap;
  }
  for (uint i = 0; i < vnodes.size(); i++) {
    if ((vnodes[i].pair > replace_node) && (MAX_INT != vnodes[i].pair))
      vnodes[i].pair += gap;
  }
  for (uint i = 0; i < arcnodes.size(); i++) {
    if (arcnodes[i].tail >= replace_node) arcnodes[i].tail += gap;
    if (arcnodes[i].head > replace_node) arcnodes[i].head += gap;
  }
  vnodes.insert(vnodes.begin() + replace_node + 1, v->begin(), v->end());
  vnodes.erase(vnodes.begin() + replace_node);
  vector<ArcNode>::iterator it2 = arcnodes.begin();
  for (uint i = 0; i < a_num; i++) {
    (*a)[i].tail += replace_node;
    (*a)[i].head += replace_node;
  }
  for (vector<ArcNode>::iterator it = a->begin(); it < a->end(); it++)
    arcnodes.push_back(*it);

  refresh_relationship();
  // update_parallel_degree();
}


void DAG_Task::Erdos_Renyi(uint v_num, double e_prob) {
  // generate wcet for each vnode
  // vector<ulong> points;
  // points.push_back(0);
  // for (uint i = 1; i < v_num; i++) {
  //   bool test;
  //   ulong temp;
  //   do {
  //     test = false;
  //     temp = Random_Gen::uniform_ulong_gen(1, wcet - 1);
  //     for (uint j = 0; j < points.size(); j++)
  //       if (temp == points[j]) test = true;
  //   } while (test);
  //   points.push_back(temp);
  // }
  // points.push_back(wcet);
  // sort(points.begin(), points.end());  // increasae order

  // // add vnodes
  // for (uint i = 0; i < v_num; i++) {
  //   add_job(points[i + 1] - points[i]);
  // }

  uint64_t uniformed_wcet = wcet/v_num;
  for (uint i = 0; i < v_num; i++) {
    add_job(uniformed_wcet);
  }

  // generate arcs with Erdos Renyi method
  for (uint i = 0; i < v_num - 1; i++) {
    for (uint j = i + 1; j < v_num; j++) {
      if (Random_Gen::probability(e_prob)) {
        add_arc(i, j);
        refresh_relationship();
      }
    }
  }
  update_parallel_degree();
}

void DAG_Task::Erdos_Renyi_v2(uint v_num, double e_prob) {  // Xu Jiang's method
  // generate wcet for each vnode
  vector<ulong> wcet_subjobs;
  uint64_t wcet_sum = 0;
// cout << "1111" << endl;
  for (uint i = 0; i < v_num; i++) {
    uint64_t wcet_subjob = Random_Gen::uniform_ulong_gen(300, 1500);
    // uint64_t wcet_subjob = Random_Gen::uniform_ulong_gen(1000, 100000);
    wcet_subjobs.push_back(wcet_subjob);
    wcet_sum += wcet_subjob;
  }
// cout << "2222" << endl;

  // add vnodes
  for (uint i = 0; i < v_num; i++) {
    add_job(wcet_subjobs[i]);
  }
// cout << "3333" << endl;
  this->wcet = wcet_sum;
  // generate arcs with Erdos Renyi method
  for (uint i = 0; i < v_num - 1; i++) {
    for (uint j = i + 1; j < v_num; j++) {
      if (Random_Gen::probability(e_prob)) {
        add_arc(i, j);
        refresh_relationship();
      }
    }
  }
// cout << "4444" << endl;
  // Time_Record timer;
  // double ms_time = 0;
  // timer.Record_MS_A();
  update_parallel_degree();
// cout << "5555" << endl;
  // timer.Record_MS_B();
  // ms_time = timer.Record_MS();
  // cout << "time to update parallel degree:" << ms_time << endl;
}

void DAG_Task::init() {
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  // priority = MAX_INT;
  speedup = 1;
  other_attr = 0;
  dcores = get_parallel_degree();
}

uint DAG_Task::task_model() { return model; }

uint DAG_Task::get_id() const { return id; }
void DAG_Task::set_id(uint id) { this->id = id; }
uint DAG_Task::get_index() const { return index; }
void DAG_Task::set_index(uint index) { this->index = index; }
uint DAG_Task::get_vnode_num() const { return vnodes.size(); }
uint DAG_Task::get_arcnode_num() const { return arcnodes.size(); }
const vector<VNode> &DAG_Task::get_vnodes() const { return vnodes; }
const VNode &DAG_Task::get_vnode_by_id(uint job_id) const {
  return vnodes[job_id];
}
const vector<ArcNode> &DAG_Task::get_arcnodes() const { return arcnodes; }
ulong DAG_Task::get_deadline() const { return deadline; }
ulong DAG_Task::get_period() const { return period; }
ulong DAG_Task::set_period(uint64_t period)  { this->period=period; }
ulong DAG_Task::get_wcet() const { return wcet; }
ulong DAG_Task::get_wcet_critical_sections() const {
  return wcet_critical_sections;
}
void DAG_Task::set_wcet_critical_sections(ulong csl) {
  wcet_critical_sections = csl;
}
ulong DAG_Task::get_wcet_non_critical_sections() const {
  return wcet_non_critical_sections;
}
void DAG_Task::set_wcet_non_critical_sections(ulong ncsl) {
  wcet_non_critical_sections = ncsl;
}
ulong DAG_Task::get_len() const { return len; }
bool DAG_Task::is_connected(VNode i, VNode j) const {
  if (i.job_id == j.job_id) {
    return true;
  } else if (i.job_id > j.job_id) {
    return false;
  } else {
    for (uint k = 0; k < i.follow_ups.size(); k++) {
      if (is_connected(vnodes[i.follow_ups[k]->head], j)) return true;
    }
  }
  return false;
}
void DAG_Task::update_parallel_degree() {
  Floyd();
  Graph BPGraph = toBPGraph();
  refresh_relationship(&BPGraph.v, &BPGraph.a);
  // cout << "maximum matching:" << maximumMatch(&BPGraph) << endl;
  parallel_degree = vnodes.size() - maximumMatch(&BPGraph);
}
uint DAG_Task::get_parallel_degree() const { return parallel_degree; }
double DAG_Task::get_utilization() const { return utilization; }
double DAG_Task::get_density() const { return density; }
uint DAG_Task::get_priority() const { return priority; }
void DAG_Task::set_priority(uint prio) { priority = prio; }
uint DAG_Task::get_partition() const { return partition; }
void DAG_Task::set_partition(uint cpu, double speedup) {
  partition = cpu;
  this->speedup = speedup;
}
Cluster DAG_Task::get_cluster() const { return cluster; }
void DAG_Task::set_cluster(Cluster cluster, double speedup) {
  this->cluster = cluster;
  this->speedup = speedup;
}
void DAG_Task::add2cluster(uint32_t p_id, double speedup) {
  bool included = false;
  foreach(cluster, p_k) {
    if ((*p_k) == p_id)
      included = true;
  }
  if (!included) {
    cluster.push_back(p_id);
    this->speedup = speedup;
  }
}
bool DAG_Task::is_in_cluster(uint32_t p_id) const {
  foreach(cluster, p_k) {
    if (p_id == (*p_k))
      return true;
  }
  return false;
}
ulong DAG_Task::get_response_time() const { return response_time; }
void DAG_Task::set_response_time(ulong response) { response_time = response; }
uint DAG_Task::get_dcores() const { return dcores; }
void DAG_Task::set_dcores(uint n) { dcores = n; }
ulong DAG_Task::get_other_attr() const { return other_attr; }
void DAG_Task::set_other_attr(ulong attr) { other_attr = attr; }
void DAG_Task::add_job(ulong wcet, ulong deadline) {
  VNode vnode;
  vnode.job_id = vnodes.size();
  vnode.type = J_NODE;
  vnode.wcet = wcet;
  vnode.wcet_non_critical_section = wcet;
  if (0 == deadline) vnode.deadline = this->deadline;
  vnode.level = 0;
  vnodes.push_back(vnode);
  // update_vol();
  // update_len();
}

void DAG_Task::add_job(vector<VNode> *v, ulong wcet, ulong deadline) {
  VNode vnode;
  vnode.job_id = v->size();
  vnode.type = J_NODE;
  vnode.wcet = wcet;
  vnode.wcet_non_critical_section = wcet;
  if (0 == deadline) vnode.deadline = this->deadline;
  vnode.level = 0;
  v->push_back(vnode);
}

void DAG_Task::add_arc(uint tail, uint head) {
  ArcNode arcnode;
  arcnode.tail = tail;
  arcnode.head = head;
  arcnodes.push_back(arcnode);
}

void DAG_Task::add_arc(vector<ArcNode> *a, uint tail, uint head) {
  ArcNode arcnode;
  arcnode.tail = tail;
  arcnode.head = head;
  a->push_back(arcnode);
}

void DAG_Task::delete_arc(uint tail, uint head) {
  // int i;
  // for (i = 0; i < arcnodes.size(); i++) {
  // cout << "aa" << endl;
  // foreach(arcnodes, arcnode) {
  //   cout << arcnode->tail << "->" << arcnode->head << endl;
  // }
  // cout << "bb" << endl;
  foreach(arcnodes, arcnode) {
    // cout << arcnode->tail << "->" << arcnode->head << endl;
    if (tail == arcnode->tail && head == arcnode->head) {
      // cout << "delete" << endl;
      arcnodes.erase(arcnode);
      break;
    }
  }
  // cout << "cc" << endl;
  // foreach(arcnodes, arcnode) {
  //   cout << arcnode->tail << "->" << arcnode->head << endl;
  // }
  refresh_relationship();
}

void DAG_Task::refresh_relationship() {
  std::sort(arcnodes.begin(), arcnodes.end(), arcs_increase<ArcNode>);
  for (uint i = 0; i < vnodes.size(); i++) {
    vnodes[i].precedences.clear();
    vnodes[i].follow_ups.clear();
  }
  for (uint i = 0; i < arcnodes.size(); i++) {
    vnodes[arcnodes[i].tail].follow_ups.push_back(&arcnodes[i]);
    vnodes[arcnodes[i].head].precedences.push_back(&arcnodes[i]);
  }
}

void DAG_Task::refresh_relationship(vector<VNode> *v, vector<ArcNode> *a) {
  sort(a->begin(), a->end(), arcs_increase<ArcNode>);
  for (uint i = 0; i < v->size(); i++) {
    (*v)[i].precedences.clear();
    (*v)[i].follow_ups.clear();
  }
  for (uint i = 0; i < a->size(); i++) {
    (*v)[(*a)[i].tail].follow_ups.push_back(&(*a)[i]);
    (*v)[(*a)[i].head].precedences.push_back(&(*a)[i]);
  }
}

void DAG_Task::update_wcet() {
  wcet = 0;
  for (uint i = 0; i < vnodes.size(); i++) wcet += vnodes[i].wcet;
}

// void DAG_Task::update_len() {
//   uint64_t max = 0;
//   for (uint i = 0; i < vnodes.size(); i++) {
//     if (0 != vnodes[i].precedences.size())
//       continue;
//     uint64_t temp = DFS(vnodes[i]);
//     if (temp > max)
//       max = temp;
//   }
//   len = max;
// }

void DAG_Task::update_len() {
  uint64_t max = 0;
  uint64_t sub_max = 0;
  vector<uint64_t> lengths;

  for (uint i = 0; i < vnodes.size(); i++) {
    lengths.push_back(vnodes[i].wcet);
  }

  for (uint i = 0; i < vnodes.size(); i++) {
    sub_max = 0;
    foreach(vnodes[i].precedences, arc) {
      if(lengths[(*arc)->tail] > sub_max)
        sub_max = lengths[(*arc)->tail];
    }
    lengths[i] += sub_max;

    if (0 == vnodes[i].follow_ups.size() && lengths[i] > max)
      max = lengths[i];
  }
  len = max;
}

void DAG_Task::update_len_2() {
  uint64_t max = 0;
  uint64_t sub_max = 0;
  vector<uint64_t> lengths;

  for (uint i = 0; i < vnodes.size(); i++) {
    lengths.push_back(vnodes[i].wcet);
  }

  for (uint i = 0; i < vnodes.size(); i++) {
    sub_max = 0;
    foreach(vnodes[i].precedences, arc) {
      if(lengths[(*arc)->tail] > sub_max)
        sub_max = lengths[(*arc)->tail];
    }
    lengths[i] += sub_max;

    if (0 == vnodes[i].follow_ups.size() && lengths[i] > max)
      max = lengths[i];
  }
  cout << "cpl[by method 1]:" << len << endl
       << "cpl[by method 2]:" << max << endl;
}

bool DAG_Task::is_acyclic() { VNodePtr job = &vnodes[0]; }


double DAG_Task::get_NCS_utilization() const {
  double temp = get_wcet_non_critical_sections();
  temp /= get_period();
  return temp;
}

ulong DAG_Task::DFS(VNode vnode) const {
  ulong result = 0;
  // cout << "job id:" << vnode.job_id << endl;
  if (0 == vnode.follow_ups.size()) {
    result = vnode.wcet;
  } else {
    for (uint i = 0; i < vnode.follow_ups.size(); i++) {
      // cout << "22" << endl;
      ulong temp = vnode.wcet + DFS(vnodes[vnode.follow_ups[i]->head]);
      if (result < temp) result = temp;
    }
  }
  return result;
}

ulong DAG_Task::BFS(VNode vnode) const {}

bool DAG_Task::is_arc_exist(uint tail, uint head) const {
  for (uint i = 0; i < arcnodes.size(); i++) {
    if (tail == arcnodes[i].tail)
      if (head == arcnodes[i].head) return true;
  }
  return false;
}

bool DAG_Task::is_arc_exist(const vector<ArcNode> &a, uint tail, uint head) {
  for (uint i = 0; i < a.size(); i++) {
    if (tail == a[i].tail)
      if (head == a[i].head) return true;
  }
  return false;
}

void DAG_Task::display() {
   cout<<"period:"<<get_period()<<endl;
   // cout<<"shorten wcet"<<endl;
  display_vertices();
  display_arcs();
}

void DAG_Task::display_vertices() {

 // for (uint i = 0; i < vnodes.size(); i++) 
 // vnodes[i].wcet=vnodes[i].wcet/500;                                    //test by wyg
  cout << "display main vertices:" << endl;


  for (uint i = 0; i < vnodes.size(); i++) {
    cout <<"job_id:"<< vnodes[i].job_id << "wcet:" << vnodes[i].wcet << "type:" << vnodes[i].type<<endl;;
    if (MAX_INT == vnodes[i].pair)
      cout << endl;
    else
      cout << "pair:" << vnodes[i].pair << endl;
    foreach(vnodes[i].requests, request) {
      cout << "  r:" << request->get_resource_id() << " N:" << request->get_num_requests() << " L:" << request->get_max_length() << endl;
    }
  }
}

void DAG_Task::display_vertices(const vector<VNode> &v) {
  cout << "display vertices:" << endl;
  for (uint i = 0; i < v.size(); i++) {
    cout << v[i].job_id << ":" << v[i].type;
    if (MAX_INT == v[i].pair)
      cout << endl;
    else
      cout << ":" << v[i].pair << endl;
    foreach(vnodes[i].requests, request) {
      cout << "  r:" << request->get_resource_id() << " N:" << request->get_num_requests() << " L:" << request->get_max_length() << endl;
    }
  }
}

void DAG_Task::display_arcs() {
  cout << "display main arcs:" << endl;
  for (uint i = 0; i < arcnodes.size(); i++) {
    cout << arcnodes[i].tail << "--->" << arcnodes[i].head
         << "\taddress:" << &arcnodes[i] << endl;
  }
}

void DAG_Task::display_arcs(const vector<ArcNode> &a) {
  cout << "display arcs:" << endl;
  for (uint i = 0; i < a.size(); i++) {
    cout << a[i].tail << "--->" << a[i].head << endl;
  }
}

void DAG_Task::display_follow_ups(uint job_id) {
  for (uint i = 0; i < vnodes[job_id].follow_ups.size(); i++) {
    cout << "follow up of node " << job_id << ":"
         << vnodes[job_id].follow_ups[i]->head
         << "\taddress:" << vnodes[job_id].follow_ups[i] << endl;
  }
}

void DAG_Task::display_precedences(uint job_id) {
  for (uint i = 0; i < vnodes[job_id].precedences.size(); i++)
    cout << "precedences of node " << job_id << ":"
         << vnodes[job_id].precedences[i]->tail
         << "\taddress:" << vnodes[job_id].precedences[i] << endl;
}

uint DAG_Task::get_indegrees(uint job_id) const {
  return vnodes[job_id].precedences.size();
}
uint DAG_Task::get_outdegrees(uint job_id) const {
  return vnodes[job_id].follow_ups.size();
}

void DAG_Task::display_in_dot() {
  cout << "digraph " << "dag_" << index << " {" << endl;
  cout << "  node [style=filled]" << endl;
  foreach(vnodes, vnode) {
    switch (vnode->type) {
      case P_NODE | S_NODE:
        cout << "  " << vnode->job_id
             << "[shape=circle,height=0.2,fillcolor=\"#dddddd\"];" << endl;
        break;
      case P_NODE | E_NODE:
        cout << "  " << vnode->job_id
             << "[shape=circle,height=0.2,fillcolor=\"#666666\"];" << endl;
        break;
      case C_NODE | S_NODE:
        cout << "  " << vnode->job_id
             << "[shape=diamond,fillcolor=\"#dddddd\"];" << endl;
        break;
      case C_NODE | E_NODE:
        cout << "  " << vnode->job_id
             << "[shape=diamond,fillcolor=\"#666666\"];" << endl;
        break;
      case J_NODE:
        cout << "  " << vnode->job_id
             << "[shape=ellipse,fillcolor=\"#ffffff\"];" << endl;
        break;
      default:
        cout << "  " << vnode->job_id
             << "[shape=ellipse,fillcolor=\"#ffffff\"];" << endl;
        break;
    }
  }
  foreach(arcnodes, arc) {
    cout << "  " <<  arc->tail << "->" << arc->head << ";" << endl;
  }
  cout << "}" << endl;
}

vector<VNode> DAG_Task::get_critical_path() {}

uint64_t DAG_Task::get_path_length(vector<VNode> path) {}

uint64_t DAG_Task::get_critical_path_length() const { return len; }

bool DAG_Task::MMDFS(Graph *graph, VNode *vnode) {
    // cout << "  follow ups of " << vnode.job_id << ":" << endl;
  for (uint i = 0; i < vnode->follow_ups.size(); i++) {
    uint follow_up = vnode->follow_ups[i]->head;
    // cout << follow_up << endl;
    // cout << "visited[follow_up] " << visited[follow_up] << endl;
    if (!visited[follow_up]) {
      visited[follow_up] = 1;
      // cout << "match[follow_up] " << match[follow_up] << endl;
      if (-1 == match[follow_up] || MMDFS(graph, &graph->v[match[follow_up]])) {
        match[follow_up] = vnode->job_id;
        match[vnode->job_id] = follow_up;
        return 1;
      }
    }
  }
  return 0;
}

int DAG_Task::maximumMatch(Graph *graph) {
  visited.clear();
  match.clear();
  uint matches = 0;
  for (uint i = 0; i < graph->v.size(); i++) {
    visited.push_back(0);
    match.push_back(-1);
  }
  for (uint i = 0; i < graph->v.size(); i++) {
      // cout << "checking " << i << endl;
    if (-1 == match[i]) {
      // cout << "  match[i]:" << match[i] << endl;
      for (uint j = 0; j < graph->v.size(); j++) {
        visited[j] = 0;
      }
      if (MMDFS(graph, &graph->v[i])) {
        // cout << "match!" << endl;
        matches++;
      }
    }
  }
  return matches;
}


Graph DAG_Task::toBPGraph() {
  Graph BPGraph;
  uint gap = vnodes.size();
  for (uint i = 0; i < 2 * gap; i++) {
    VNode vnode;
    vnode.job_id = i;
    BPGraph.v.push_back(vnode);
  }

  foreach(arcnodes,arcnode) {
    ArcNode anode = {
      .tail = arcnode->tail,
      .head = arcnode->head + gap
    };
    BPGraph.a.push_back(anode);
  }
  refresh_relationship(&BPGraph.v, &BPGraph.a);
  return BPGraph;
}

void DAG_Task::Floyd() {
  for (uint i = 0; i < vnodes.size(); i++) {
    for (uint j = 0; j < vnodes.size(); j++) {
      for (uint k = 0; k < vnodes.size(); k++) {
        if (is_arc_exist(i, j) && is_arc_exist(j, k)) {
          if (!is_arc_exist(i, k))
            add_arc(i, k);
        }
      }
    }
  }
  refresh_relationship();
}

void DAG_Task::unFloyd() {
  for (uint i = 0; i < vnodes.size(); i++) {
    for (uint j = 0; j < vnodes.size(); j++) {
      for (uint k = 0; k < vnodes.size(); k++) {
        if (is_arc_exist(i, j) && is_arc_exist(j, k)) {
          if (is_arc_exist(i, k)) {
            // cout << "unFloyd delete " << i << "->" << k << endl;
            delete_arc(i, k);
          }
        }
      }
    }
  }
  refresh_relationship();
}

Paths DAG_Task::EP_DFS(VNode vnode) const {
  Paths paths;
  if (0 == vnode.follow_ups.size()) {
    Path path;
    path.vnodes.insert(path.vnodes.begin(), vnode);
    path.wcet = vnode.wcet;
    path.wcet_critical_section = vnode.wcet_critical_section;
    path.wcet_non_critical_section = vnode.wcet_non_critical_section;
    assert(path.wcet_critical_section+path.wcet_non_critical_section == path.wcet);
    path.requests = vnode.requests;
    paths.push_back(path);
  } else {
    foreach(vnode.follow_ups, arc) {
      VNode f_node = get_vnodes()[(*arc)->head];
      Paths f_paths = EP_DFS(f_node);
      foreach(f_paths, f_path) {
        f_path->vnodes.insert(f_path->vnodes.begin(), vnode);
        f_path->wcet += vnode.wcet;
        f_path->wcet_critical_section += vnode.wcet_critical_section;
        f_path->wcet_non_critical_section += vnode.wcet_non_critical_section;
        foreach(vnode.requests, r_q) {
          bool existed = false;
          foreach(f_path->requests, r_p) {
            if (r_q->get_resource_id() == r_p->get_resource_id()) {
              existed = true;
              r_p->set_num_requests(r_p->get_num_requests() + r_q->get_num_requests());
              break;
            }
          }
          if (!existed) {
            Request request(r_q->get_resource_id(), r_q->get_num_requests(), r_q->get_max_length(), r_q->get_total_length(), r_q->get_locality());
            f_path->requests.push_back(request);
          }
        }
        paths.push_back((*f_path));
      }
    }
  }
  return paths;
}

// void DAG_Task::EP_DFS(VNode vnode, Paths *paths, uint64_t wcet, Resource_Requests requests) {
//     foreach(vnode.requests, r_q) {
//       bool existed = false;
//       foreach(requests, r_p) {
//         if (r_q->get_resource_id() == r_p->get_resource_id()) {
//           existed = true;
//           r_p->set_num_requests(r_p->get_num_requests() + r_q->get_num_requests());
//           break;
//         }
//       }
//       if (!existed) {
//         Request request(r_q->get_resource_id(), r_q->get_num_requests(), r_q->get_max_length(), r_q->get_total_length(), r_q->get_locality());
//         requests.push_back(request);
//       }
//     }

//     if (0 == vnode.follow_ups.size()) {
//       // 
//       Path new_path;
//       new_path.wcet = wcet + vnode.wcet;
//       uint64_t wcet_critical_section = 0;
//       foreach(requests, request) {
//         uint64_t total_length = request->get_max_length() * request->get_num_requests();
//         request->set_total_length(total_length);
//         wcet_critical_section += total_length;
//       }
//       new_path.wcet_critical_section = wcet_critical_section;
//       new_path.wcet_non_critical_section = new_path.wcet - wcet_critical_section;
//       new_path.requests = requests;
//       paths->push_back(new_path);
//     } else {
//       for (uint i = 0; i < vnode.follow_ups.size(); i++) {
//         VNode f_node = get_vnodes()[vnode.follow_ups[i]->head];
//         EP_DFS(f_node, paths, wcet + vnode.wcet, requests);
//       }
//     }
//   }

Paths DAG_Task::get_paths() const {
  Paths total_paths;
  foreach(vnodes, vnode) {
    if (0 == vnode->precedences.size()) {
      Paths paths = EP_DFS((*vnode));
      foreach(paths, path)
        total_paths.push_back((*path));
    }
  }
  return total_paths;
}

void DAG_Task::update_requests(ResourceSet resources) {
  foreach(requests, request) {
    uint q = request->get_resource_id();
    request->set_locality(resources.get_resource_by_id(q).get_locality());
  }
} 

void DAG_Task::calculate_RCT() {
  foreach(vnodes, vnode) {
    calculate_RCT(vnode->job_id);
  }
}

uint64_t DAG_Task::calculate_RCT(uint32_t job_id) {
  VNode &vnode = vnodes[job_id];
  uint64_t max_prev = 0;
  uint64_t temp;
  if (0 == vnode.RCT) {
    if (0 == vnode.precedences.size()) // source
      vnode.RCT = vnode.wcet;
    else {
      foreach(vnode.precedences, prev) {
        temp = calculate_RCT((*prev)->tail);
        if (temp > max_prev)
          max_prev = temp;
      }
      vnode.RCT = vnode.wcet + max_prev;
    }
  }
  return vnode.RCT;
}

uint64_t DAG_Task::get_RCT(uint32_t job_id) const {
  return vnodes[job_id].RCT;
}

/** Task DAG_TaskSet */

DAG_TaskSet::DAG_TaskSet() {
  utilization_sum = 0;
  utilization_max = 0;
  density_sum = 0;
  density_max = 0;
}

DAG_TaskSet::~DAG_TaskSet() { dag_tasks.clear(); }

void DAG_TaskSet::init() {
  foreach(dag_tasks, task)
    task->init();
}

void DAG_TaskSet::add_task(ResourceSet *resourceset, Param param) {
  uint task_id = dag_tasks.size();
  dag_tasks.push_back(
      DAG_Task(task_id, resourceset, param));
  utilization_sum += get_task_by_id(task_id).get_utilization();
  density_sum += get_task_by_id(task_id).get_density();
  if (utilization_max < get_task_by_id(task_id).get_utilization())
    utilization_max = get_task_by_id(task_id).get_utilization();
  if (density_max < get_task_by_id(task_id).get_density())
    density_max = get_task_by_id(task_id).get_density();
}

void DAG_TaskSet::add_task(ResourceSet *resourceset, Param param,
                           double utilization) {
  uint task_id = dag_tasks.size();
  dag_tasks.push_back(
      DAG_Task(task_id, resourceset, param, utilization));
  utilization_sum += get_task_by_id(task_id).get_utilization();
  density_sum += get_task_by_id(task_id).get_density();
  if (utilization_max < get_task_by_id(task_id).get_utilization())
    utilization_max = get_task_by_id(task_id).get_utilization();
  if (density_max < get_task_by_id(task_id).get_density())
    density_max = get_task_by_id(task_id).get_density();
}

void DAG_TaskSet::add_task(ResourceSet *resourceset, Param param, ulong wcet,
                           ulong period, ulong deadline) {
  uint task_id = dag_tasks.size();
  dag_tasks.push_back(
      DAG_Task(task_id, resourceset, param, wcet, period, deadline));
  utilization_sum += get_task_by_id(task_id).get_utilization();
  density_sum += get_task_by_id(task_id).get_density();
  if (utilization_max < get_task_by_id(task_id).get_utilization())
    utilization_max = get_task_by_id(task_id).get_utilization();
  if (density_max < get_task_by_id(task_id).get_density())
    density_max = get_task_by_id(task_id).get_density();
}

void DAG_TaskSet::add_task(ResourceSet *resourceset, Param param, double cpp,
                           ulong wcet, ulong period, ulong deadline) {
  uint task_id = dag_tasks.size();
  dag_tasks.push_back(
      DAG_Task(task_id, resourceset, param, cpp, wcet, period, deadline));
  utilization_sum += get_task_by_id(task_id).get_utilization();
  density_sum += get_task_by_id(task_id).get_density();
  if (utilization_max < get_task_by_id(task_id).get_utilization())
    utilization_max = get_task_by_id(task_id).get_utilization();
  if (density_max < get_task_by_id(task_id).get_density())
    density_max = get_task_by_id(task_id).get_density();
}

void DAG_TaskSet::add_task(DAG_Task dag_task) {
  dag_tasks.push_back(dag_task);
  utilization_sum += dag_task.get_utilization();
  density_sum += dag_task.get_density();
  if (utilization_max < dag_task.get_utilization())
    utilization_max = dag_task.get_utilization();
  if (density_max < dag_task.get_density())
    density_max = dag_task.get_density();
}

DAG_Tasks &DAG_TaskSet::get_tasks() { return dag_tasks; }

DAG_Task &DAG_TaskSet::get_task_by_id(uint id) {
  foreach(dag_tasks, task) {
    if (id == task->get_id()) return (*task);
  }
  return *reinterpret_cast<DAG_Task *>(0);
}

uint DAG_TaskSet::get_taskset_size() const { return dag_tasks.size(); }

double DAG_TaskSet::get_utilization_sum() const { return utilization_sum; }
double DAG_TaskSet::get_utilization_max() const { return utilization_max; }
double DAG_TaskSet::get_density_sum() const { return density_sum; }
double DAG_TaskSet::get_density_max() const { return density_max; }

void DAG_TaskSet::sort_by_period() {
  std::sort(dag_tasks.begin(), dag_tasks.end(), period_increase<DAG_Task>);
  for (int i = 0; i < dag_tasks.size(); i++) dag_tasks[i].set_index(i);
  // for (uint i = 0; i < dag_tasks.size(); i++)
  //  dag_tasks[i].set_id(i);
}

void DAG_TaskSet::RM_Order() {
  sort_by_period();
  for (uint i = 0; i < dag_tasks.size(); i++) {
    dag_tasks[i].set_priority(i);
    dag_tasks[i].refresh_relationship();
    // dag_tasks[i].update_parallel_degree();
  }
#if SORT_DEBUG
  cout << "RM Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(dag_tasks, task) {
    cout << " Task " << task->get_id() << ":" << endl;
    cout << " WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline()
         << " Period:" << task->get_period()
         << " Priority:" << task->get_priority()
         << " Gap:" << task->get_deadline() - task->get_wcet() << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void DAG_TaskSet::task_gen(ResourceSet *resourceset, Param param,
                           double utilization, uint task_number) {
  if (0 == task_number)
    return;
  if (_EPS < utilization) {
    // Generate task set utilization...
    double SumU;
    vector<double> u_set;
    bool re_gen = true;
 
    switch (param.u_gen) {
      case GEN_UNIFORM:
         cout << "GEN_UNIFORM" << endl;
        SumU = 0;
        while (SumU < utilization) {
          double u = Random_Gen::uniform_real_gen(0, 1);
          if (1 + _EPS <= u)
            continue;
          if (SumU + u > utilization) {
            u = utilization - SumU;
            u_set.push_back(u);
            SumU += u;
            break;
          }
          u_set.push_back(u);
          SumU += u;
        }
        break;

      case GEN_EXPONENTIAL:
         cout << "GEN_EXPONENTIAL" << endl;
        SumU = 0;
        while (SumU < utilization) {
          double u = Random_Gen::exponential_gen(1/param.mean);
          if (1 + _EPS <= u)
            continue;
          if (SumU + u > utilization) {
            u = utilization - SumU;
            u_set.push_back(u);
            SumU += u;
            break;
          }
          u_set.push_back(u);
          SumU += u;
        }
        break;

      case GEN_UUNIFAST:  // Same as the UUnifast_Discard
         cout << "GEN_UUNIFAST" << endl;
      case GEN_UUNIFAST_D:
         cout << "GEN_UUNIFAST_D" << endl;
        while (re_gen) {
          SumU = utilization;
          re_gen = false;
          for (uint i = 1; i < task_number; i++) {
            double temp = Random_Gen::uniform_real_gen(0, 1);
            double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
            double u = SumU - NextSumU;
            if ((u - 1) >= _EPS) {
              u_set.clear();
              re_gen = true;
               cout << " u-1>= EPS Regenerate taskset." << endl;
              break;
            } else if (param.job_num_range.max > param.p_range.min * u) {
              u_set.clear();
              re_gen = true;
               cout << "param.job_num_range.max > param.p_range.min * u Regenerate taskset." << endl;
              break;
            } else {
              u_set.push_back(u);
              SumU = NextSumU;
            }
          }
          if (!re_gen) {
            if ((SumU - 1) >= _EPS) {
              u_set.clear();
              re_gen = true;
              cout << " (SumU - 1) >= _EPS) Regenerate taskset." << endl;
            } else {
              u_set.push_back(SumU);
            }
          }
        }
        break;

      case GEN_RANDFIXSUM:
         cout << "GEN_RANDFIXSUM" << endl;
        u_set = Random_Gen::RandFixedSum(task_number, utilization, 0.1, 2*param.mean);
        break;

      default:
        while (re_gen) {
         cout<<"start default1"<<endl;
          SumU = utilization;
          re_gen = false;
          for (uint i = 1; i < task_number; i++) {
            double temp = Random_Gen::uniform_real_gen(0, 1);
            double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
            double u = SumU - NextSumU;
            if ((u - 1) >= _EPS) {
              u_set.clear();
              re_gen = true;
              cout << "Regenerate taskset." << endl;
              break;
            } else {
              u_set.push_back(u);
              SumU = NextSumU;
            }
          }
          if (!re_gen) {
            if ((SumU - 1) >= _EPS) {
              u_set.clear();
              re_gen = true;
              cout << "Regenerate taskset." << endl;
            } else {
              u_set.push_back(SumU);
            }
          }
        }
        break;
    }  
    uint i = 0;
    foreach(u_set, u) {
      re_gen = false;
      uint64_t period, wcet;
      do {
        period = Random_Gen::uniform_ulong_gen(
            static_cast<ulong>(param.p_range.min),
            static_cast<ulong>(param.p_range.max));
        wcet = ceil((*u) * period);
        if (0 == wcet)
          wcet++;
        if (param.job_num_range.max > wcet)
          re_gen = true;
      } while (re_gen);
      // else if (wcet > period)
      //   wcet = period;
      ulong deadline = 0;
      if (fabs(param.d_range.max - 1) > _EPS) {
        deadline = ceil(period * Random_Gen::uniform_real_gen(
                                     param.d_range.min, param.d_range.max));
        // if (deadline < wcet) deadline = wcet;
      }
      // cout << wcet << " " << period << endl;
      add_task(resourceset, param, wcet, period, deadline);
      // add_task(resourceset, param, param.reserve_double_4, wcet, period, deadline);
    }
    // foreach(dag_tasks, dag_task) {
    //   dag_task->refresh_relationship();
    //   // dag_task->update_parallel_degree();
    // }
    sort_by_period();
  }
   display();
}

void DAG_TaskSet::task_gen_v2(ResourceSet *resourceset, Param param,
                           double utilization, uint task_number) {
  if (0 == task_number)
    return;
  if (_EPS < utilization) {
    // Generate task set utilization...
    double SumU;
    vector<double> u_set;
    bool re_gen = true;
    u_set = Random_Gen::RandFixedSum(task_number, utilization, 0.025 * param.p_num / task_number,
                                     2 * param.p_num / task_number);
    // u_set = Random_Gen::RandFixedSum(task_number, utilization, 0.025 * param.p_num / task_number,
    //                                  sqrt(param.p_num));
    // u_set = Random_Gen::RandFixedSum(task_number, utilization, 0.1,
    //                                  4);
    // u_set = Random_Gen::RandFixedSum(task_number, utilization, 0,
    //                                  param.mean * 2);
    foreach(u_set, u) {
      // cout << *u << endl;
      add_task(resourceset, param, (*u));
    }
    sort_by_period();
  }
  // display();
}

void DAG_TaskSet::task_gen(ResourceSet *resourceset, Param param, 
                           uint task_number) {
  for (uint i = 0; i < task_number; i++) {
    add_task(resourceset, param);
  }
  // foreach(dag_tasks, dag_task) {
  //     dag_task->refresh_relationship();
  //     // dag_task->update_parallel_degree();
  // }
  sort_by_period();
}

void DAG_TaskSet::task_load(ResourceSet* resourceset, string file_name) {
// Remains blank.
}

void DAG_TaskSet::display() {
  foreach(dag_tasks, task) {
    cout << "Task" << task->get_index() << ": id:"
         << task->get_id()
         << ": partition:"
         << task->get_partition()
         << ": priority:"
         << task->get_priority() << endl;
    cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
         << " cs-wcet:" << task->get_wcet_critical_sections()
         << " wcet:" << task->get_wcet()
         << " CPL:" << task->get_critical_path_length()
         << " response time:" << task->get_response_time()
         << " deadline:" << task->get_deadline()
         << " period:" << task->get_period()
         << " utilization:" << task->get_utilization() << endl;
    foreach(task->get_requests(), request) {
      cout << "request" << request->get_resource_id() << ":"
           << " num:" << request->get_num_requests()
           << " length:" << request->get_max_length() << endl;
    }
    // if (task->get_wcet() > task->get_response_time()) exit(0);
    // task->display_vertices();
    // task->display_arcs();
    // task->display_in_dot();
    cout << "-------------------------------------------" << endl;
  }
}

void DAG_TaskSet::update_requests(const ResourceSet &resources) {
  foreach(dag_tasks, task) { task->update_requests(resources); }
}

/** Others */

void task_gen(TaskSet *taskset, ResourceSet *resourceset, Param param,
              double utilization) {
  while (taskset->get_utilization_sum() < utilization) {  // generate tasks
    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    // double u = Random_Gen::uniform_real_gen(0.2, 0.5);
    double u = Random_Gen::exponential_gen(1/param.mean);
    ulong wcet = period * u;
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      // if (deadline > period)
      //    deadline = period;
      if (deadline < wcet) deadline = wcet;
    }
    double temp = wcet;
    temp /= period;
    if (taskset->get_utilization_sum() + temp > utilization) {
      temp = utilization - taskset->get_utilization_sum();
      wcet = period * temp + 1;
      if (deadline != 0 && deadline < wcet) deadline = wcet;
      // taskset->add_task(wcet, period);
      taskset->add_task(resourceset, param, wcet, period, deadline);
      break;
    }
    // taskset->add_task(wcet,period);
    taskset->add_task(resourceset, param, wcet, period, deadline);
  }
  taskset->sort_by_period();
  // foreach(taskset->get_tasks(), task) {
    // cout << "Task" << task->get_id() << ": partition:"
    //      << task->get_partition()
    //      << ": priority:"
    //      << task->get_priority() << endl;
  //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
  //        << " cs-wcet:" << task->get_wcet_critical_sections()
  //        << " wcet:" << task->get_wcet()
  //        << " response time:" << task->get_response_time()
  //        << " deadline:" << task->get_deadline()
  //        << " period:" << task->get_period() << endl;
  //   foreach(task->get_requests(), request) {
  //     cout << "request" << request->get_resource_id() << ":"
  //          << " num:" << request->get_num_requests()
  //          << " length:" << request->get_max_length() << " locality:"
  //          << resourceset->get_resource_by_id(request->get_resource_id())
  //                 .get_locality()
  //          << endl;
  //   }
  //   cout << "-------------------------------------------" << endl;
  //   if (task->get_wcet() > task->get_response_time()) exit(0);
  // }
}

void task_gen_UUnifast_Discard(TaskSet *taskset, ResourceSet *resourceset,
                               Param param, double utilization) {
  double SumU;

  // uint n = 10;
  uint n = utilization / param.mean;
  // uint n = 4 * param.p_num;

  vector<double> u_set;
  bool re_gen = true;

  while (re_gen) {
    SumU = utilization;
    re_gen = false;
    for (uint i = 1; i < n; i++) {
      double temp = Random_Gen::uniform_real_gen(0, 1);
      double NextSumU = SumU * pow(temp, 1.0 / (n - i));
      double u = SumU - NextSumU;
      if ((u - 1) >= _EPS) {
        u_set.clear();
        re_gen = true;
        cout << "Regenerate taskset." << endl;
        break;
      } else {
        u_set.push_back(u);
        SumU = NextSumU;
      }
    }
    if (!re_gen) {
      if ((SumU - 1) >= _EPS) {
        u_set.clear();
        re_gen = true;
        cout << "Regenerate taskset." << endl;
      } else {
        u_set.push_back(SumU);
      }
    }
  }

  uint i = 0;
  foreach(u_set, u) {
    // cout << "U_" << (i++) << ":" << (*u) << endl;

    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    ulong wcet = ceil((*u) * period);
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      // if (deadline > period)
      //    deadline = period;
      if (deadline < wcet) deadline = wcet;
    }

    taskset->add_task(resourceset, param, wcet, period, deadline);
  }

  // cout << "U_sum:" << taskset->get_utilization_sum() << endl;

  taskset->sort_by_period();

  // foreach(taskset->get_tasks(), task) {
    // cout << "Task" << task->get_id() << ": partition:"
    //      << task->get_partition()
    //      << ": priority:" << task->get_priority() << endl;
  //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
  //        << " cs-wcet:" << task->get_wcet_critical_sections()
  //        << " wcet:" << task->get_wcet()
  //        << " response time:" << task->get_response_time()
  //        << " deadline:" << task->get_deadline()
  //        << " period:" << task->get_period() << endl;
  //   foreach(task->get_requests(), request) {
  //     cout << "request" << request->get_resource_id() << ":"
  //          << " num:" << request->get_num_requests()
  //          << " length:" << request->get_max_length() << " locality:"
  //          << resourceset->get_resource_by_id(request->get_resource_id())
  //                 .get_locality()
  //          << endl;
  //   }
  //   cout << "-------------------------------------------" << endl;
  //   if (task->get_wcet() > task->get_response_time()) exit(0);
  // }
}

void task_gen_UUnifast_Discard(TaskSet *taskset, ResourceSet *resourceset,
                               Param param, uint taskset_size,
                               double utilization) {
  double SumU;

  // uint n = 10;
  uint n = taskset_size;

  vector<double> u_set;
  bool re_gen = true;

  while (re_gen) {
    SumU = utilization;
    re_gen = false;
    for (uint i = 1; i < n; i++) {
      double temp = Random_Gen::uniform_real_gen(0, 1);
      double NextSumU = SumU * pow(temp, 1.0 / (n - i));
      double u = SumU - NextSumU;
      if ((u - 1) >= _EPS) {
        u_set.clear();
        re_gen = true;
        cout << "Regenerate taskset." << endl;
        break;
      } else {
        u_set.push_back(u);
        SumU = NextSumU;
      }
    }
    if (!re_gen) {
      if ((SumU - 1) >= _EPS) {
        u_set.clear();
        re_gen = true;
        cout << "Regenerate taskset." << endl;
      } else {
        u_set.push_back(SumU);
      }
    }
  }

  uint i = 0;
  foreach(u_set, u) {
    // cout << "U_" << (i++) << ":" << (*u) << endl;

    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    ulong wcet = ceil((*u) * period);
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      // if (deadline > period)
      //    deadline = period;
      if (deadline < wcet) deadline = wcet;
    }

    taskset->add_task(resourceset, param, wcet, period, deadline);
  }

  // cout << "U_sum:" << taskset->get_utilization_sum() << endl;

  taskset->sort_by_period();
}

void task_load(TaskSet *taskset, ResourceSet *resourceset, string file_name) {
  ifstream file(file_name, ifstream::in);
  string buf;
  uint id = 0;
  while (getline(file, buf)) {
    if (NULL != strstr(buf.data(), "Utilization")) continue;
    if (0 == strcmp(buf.data(), "\r")) break;
    // cout << buf <<endl;
    vector<uint64_t> elements;
    // extract_element(&elements, buf, 0, 3 * (1 +
    // resourceset->get_resourceset_size()));
    extract_element(&elements, buf, 0, MAX_INT);
    if (3 <= elements.size()) {
      Task task(id, elements[0], elements[1], elements[2]);
      uint n = (elements.size() - 3) / 3;
      // cout << n <<endl;
      for (uint i = 0; i < n; i++) {
        uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
        // cout << length<<endl;
        task.add_request(elements[3 + i * 3], elements[4 + i * 3],
                         elements[5 + i * 3], length);
        // task.set_wcet_non_critical_sections(task.get_wcet_non_critical_sections()
        // - length); cout<<task.get_wcet_non_critical_sections()<<endl;
        // task.set_wcet_critical_sections(task.get_wcet_critical_sections() +
        // length); cout<<task.get_wcet_critical_sections()<<endl;
        resourceset->add_task(elements[3 + i * 3], id);
        // cout<<"add to resourceset"<<endl;
      }
      taskset->add_task(task);

      id++;
    }
  }
  taskset->sort_by_period();

  foreach(taskset->get_tasks(), task) {
    cout << "Task" << task->get_id() << ": partition:" << task->get_partition()
         << ": priority:" << task->get_priority() << endl;
    cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
         << " cs-wcet:" << task->get_wcet_critical_sections()
         << " wcet:" << task->get_wcet()
         << " response time:" << task->get_response_time()
         << " deadline:" << task->get_deadline()
         << " period:" << task->get_period() << endl;
    foreach(task->get_requests(), request) {
      cout << "request" << request->get_resource_id() << ":"
           << " num:" << request->get_num_requests()
           << " length:" << request->get_max_length() << " locality:"
           << resourceset->get_resource_by_id(request->get_resource_id())
                  .get_locality()
           << endl;
    }
    cout << "-------------------------------------------" << endl;
    if (task->get_wcet() > task->get_response_time()) exit(0);
  }
}

void dag_task_gen(DAG_TaskSet *dag_taskset, ResourceSet *resourceset,
                  Param param, double utilization) {
  // generate tasks
  while (dag_taskset->get_utilization_sum() < utilization) {
    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    double u = Random_Gen::exponential_gen(1/param.mean);
    ulong wcet = period * u;
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      // if (deadline > period)
      //     deadline = period;
      if (deadline < wcet) deadline = wcet;
    }
    double temp = wcet;
    temp /= period;
    if (dag_taskset->get_utilization_sum() + temp > utilization) {
      temp = utilization - dag_taskset->get_utilization_sum();
      wcet = period * temp + 1;
      if (deadline != 0 && deadline < wcet) deadline = wcet;
      dag_taskset->add_task(resourceset, param, wcet, period, deadline);
      break;
    }
    dag_taskset->add_task(resourceset, param, wcet, period, deadline);
  }

  foreach(dag_taskset->get_tasks(), task) {
    task->refresh_relationship();
    // task->update_parallel_degree();
  }

  // dag_taskset.sort_by_period();
}

void dag_task_gen_UUnifast_Discard(DAG_TaskSet *dag_taskset,
                                   ResourceSet *resourceset, Param param,
                                   double utilization) {
  double SumU;

  // uint n = 10;
  uint n = utilization/param.mean;
  // uint n = taskset_size;

  if (0 != n) {
    vector<double> u_set;
    bool re_gen = true;

    while (re_gen) {
      SumU = utilization;
      re_gen = false;
      for (uint i = 1; i < n; i++) {
        double temp = Random_Gen::uniform_real_gen(0, 1);
        double NextSumU = SumU * pow(temp, 1.0 / (n - i));
        double u = SumU - NextSumU;
        if ((u - 1) >= _EPS || u < 0.001) {
          u_set.clear();
          re_gen = true;
          cout << "Regenerate taskset." << endl;
          break;
        } else {
          u_set.push_back(u);
          SumU = NextSumU;
        }
      }
      if (!re_gen) {
        if ((SumU - 1) >= _EPS || SumU < 0.001) {
          u_set.clear();
          re_gen = true;
          cout << "Regenerate taskset." << endl;
        } else {
          u_set.push_back(SumU);
        }
      }
    }

    uint i = 0;
    foreach(u_set, u) {
      // cout << "U_" << (i++) << ":" << (*u) << endl;

      ulong period =
          Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                          static_cast<int>(param.p_range.max));
      ulong wcet = ceil((*u) * period);
      if (0 == wcet)
        wcet++;
      else if (wcet > period)
        wcet = period;
      ulong deadline = 0;
      if (fabs(param.d_range.max) > _EPS) {
        deadline = ceil(period * Random_Gen::uniform_real_gen(
                                     param.d_range.min, param.d_range.max));
        // if (deadline > period)
        //    deadline = period;
        if (deadline < wcet) deadline = wcet;
      }

      double temp = wcet;
      temp /= period;
      if (dag_taskset->get_utilization_sum() + temp > utilization) {
        temp = utilization - dag_taskset->get_utilization_sum();
        wcet = period * temp + 1;
        if (deadline != 0 && deadline < wcet) deadline = wcet;
        dag_taskset->add_task(resourceset, param, wcet, period, deadline);
        break;
      }
      dag_taskset->add_task(resourceset, param, wcet, period, deadline);
    }

    foreach(dag_taskset->get_tasks(), task) {
      task->refresh_relationship();
      // task->update_parallel_degree();
    }
    dag_taskset->sort_by_period();

    // foreach(dag_taskset->get_tasks(), task) {
    //   cout << "Task" << task->get_id()
    //         << ": partition:" << task->get_partition()
    //         << ": priority:" << task->get_priority() << endl;
    //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
    //         << " cs-wcet:" << task->get_wcet_critical_sections()
    //         << " wcet:" << task->get_wcet()
    //         << " response time:" << task->get_response_time()
    //         << " deadline:" << task->get_deadline()
    //         << " period:" << task->get_period() << endl;
    //   foreach(task->get_requests(), request) {
    //     cout << "request" << request->get_resource_id() << ":"
    //           << " num:" << request->get_num_requests()
    //           << " length:" << request->get_max_length() << " locality:"
    //           << resourceset->get_resource_by_id(request->get_resource_id())
    //                 .get_locality()
    //           << endl;
    //   }
    //   cout << "-------------------------------------------" << endl;
    // }
  }
}

void dag_task_gen_UUnifast_Discard(DAG_TaskSet *dag_taskset,
                                   ResourceSet *resourceset, Param param,
                                   uint taskset_size, double utilization) {
  double SumU;

  // uint n = 10;
  uint n = taskset_size;

  if (0 != n) {
    vector<double> u_set;
    bool re_gen = true;

    while (re_gen) {
      SumU = utilization;
      re_gen = false;
      for (uint i = 1; i < n; i++) {
        double temp = Random_Gen::uniform_real_gen(0, 1);
        double NextSumU = SumU * pow(temp, 1.0 / (n - i));
        double u = SumU - NextSumU;
        if ((u - 1) >= _EPS || u < 0.001) {
          u_set.clear();
          re_gen = true;
          cout << "Regenerate taskset." << endl;
          break;
        } else {
          u_set.push_back(u);
          SumU = NextSumU;
        }
      }
      if (!re_gen) {
        if ((SumU - 1) >= _EPS || SumU < 0.001)  {
          u_set.clear();
          re_gen = true;
          cout << "Regenerate taskset." << endl;
        } else {
          u_set.push_back(SumU);
        }
      }
    }

    uint i = 0;
    foreach(u_set, u) {
      // cout << "U_" << (i++) << ":" << (*u) << endl;

      ulong period =
          Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                          static_cast<int>(param.p_range.max));
      ulong wcet = ceil((*u) * period);
      if (0 == wcet)
        wcet++;
      else if (wcet > period)
        wcet = period;
      ulong deadline = 0;
      if (fabs(param.d_range.max) > _EPS) {
        deadline = ceil(period * Random_Gen::uniform_real_gen(
                                     param.d_range.min, param.d_range.max));
        // if (deadline > period)
        //    deadline = period;
        if (deadline < wcet) deadline = wcet;
      }

      double temp = wcet;
      temp /= period;
      if (dag_taskset->get_utilization_sum() + temp > utilization) {
        temp = utilization - dag_taskset->get_utilization_sum();
        wcet = period * temp + 1;
        if (deadline != 0 && deadline < wcet) deadline = wcet;
        dag_taskset->add_task(resourceset, param, wcet, period, deadline);
        break;
      }
      dag_taskset->add_task(resourceset, param, wcet, period, deadline);
    }

    foreach(dag_taskset->get_tasks(), task) {
      task->refresh_relationship();
      // task->update_parallel_degree();
    }

    // foreach(dag_taskset->get_tasks(), task) {
    //   cout << "Task" << task->get_id()
    //         << ": partition:" << task->get_partition()
    //         << ": priority:" << task->get_priority() << endl;
    //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
    //         << " cs-wcet:" << task->get_wcet_critical_sections()
    //         << " wcet:" << task->get_wcet()
    //         << " response time:" << task->get_response_time()
    //         << " deadline:" << task->get_deadline()
    //         << " period:" << task->get_period() << endl;
    //   foreach(task->get_requests(), request) {
    //     cout << "request" << request->get_resource_id() << ":"
    //           << " num:" << request->get_num_requests()
    //           << " length:" << request->get_max_length() << " locality:"
    //           << resourceset->get_resource_by_id(request->get_resource_id())
    //                 .get_locality()
    //           << endl;
    //   }
    //   cout << "-------------------------------------------" << endl;
    // }

    dag_taskset->sort_by_period();
  }
}

void dag_task_gen_RandFixedSum(DAG_TaskSet *dag_taskset,
                                   ResourceSet *resourceset, Param param,
                                   uint taskset_size, double utilization) {
  double SumU;

  // uint n = 10;
  uint n = taskset_size;

  vector<double> u_set =
      Random_Gen::RandFixedSum(taskset_size, utilization, 1, 4);

  uint i = 0;
  foreach(u_set, u) {
    // cout << "U_" << (i++) << ":" << (*u) << endl;

    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    ulong wcet = ceil((*u) * period);
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
    }

    dag_taskset->add_task(resourceset, param, param.reserve_double_4, wcet,
                          period, deadline);
  }

  foreach(dag_taskset->get_tasks(), task) {
    task->refresh_relationship();
    // task->update_parallel_degree();
  }

  // foreach(dag_taskset->get_tasks(), task) {
  //   cout << "Task" << task->get_id()
  //         << ": partition:" << task->get_partition()
  //         << ": priority:" << task->get_priority() << endl;
  //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
  //         << " cs-wcet:" << task->get_wcet_critical_sections()
  //         << " wcet:" << task->get_wcet()
  //         << " response time:" << task->get_response_time()
  //         << " deadline:" << task->get_deadline()
  //         << " period:" << task->get_period() << endl;
  //   foreach(task->get_requests(), request) {
  //     cout << "request" << request->get_resource_id() << ":"
  //           << " num:" << request->get_num_requests()
  //           << " length:" << request->get_max_length() << " locality:"
  //           << resourceset->get_resource_by_id(request->get_resource_id())
  //                 .get_locality()
  //           << endl;
  //   }
  //   cout << "-------------------------------------------" << endl;
  // }

  dag_taskset->sort_by_period();
}



/*NPTASK*/


/*
NPTask::NPTask(uint id, ulong wcet, ulong period, ulong deadline, uint priority) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  this->startTime = -1; // 初始值为-1表示未执行
  this->finishTime = -1; // 初始值为-1表示未完成
  this->remainingExecutionTime = wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}
*/

NPTask::NPTask(uint id, ResourceSet *resourceset, Param param, ulong wcet,
           ulong period, ulong deadline, uint priority) {
  this->id = id;
  this->index = id;
  this->wcet = wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = this->deadline;
  independent = true;
  wcet_non_critical_sections = this->wcet;
  wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;

  uint critical_section_num = 0;

  for (int i = 0; i < param.p_num; i++) {
    if (i < param.ratio.size())
      ratio.push_back(param.ratio[i]);
    else
      ratio.push_back(1);
  }

  for (int i = 0; i < resourceset->size(); i++) {
    if ((param.mcsn > critical_section_num) && (param.rrr.min < wcet)) {
      if (Random_Gen::probability(param.rrp)) {
        uint num = Random_Gen::uniform_integral_gen(
            1, min(param.rrn,
                   static_cast<uint>(param.mcsn - critical_section_num)));
        // uint num = min(param.rrn,
        //            static_cast<uint>(param.mcsn - critical_section_num));
        uint max_len = Random_Gen::uniform_integral_gen(
            param.rrr.min, min(static_cast<double>(wcet), param.rrr.max));
            cout << "max_len:" << max_len << endl;
        ulong length = num * max_len;
        if (length >= wcet_non_critical_sections) continue;

        add_request(i, num, max_len, num * max_len,
                    resourceset->get_resource_by_id(i).get_locality());

        resourceset->add_task(i, id);
        critical_section_num += num;
      }
    }
  }


}

NPTask::NPTask(uint id, ulong wcet, ulong period, ulong deadline) {
        this->id = id;
        this->deadline = deadline;
        this->wcet = wcet;
        this->remainingExecutionTime = wcet;
        this->period = period;
        this->startTime = -1; // 初始值为-1表示未执行
        this->finishTime = -1; // 初始值为-1表示未完成
    }


NPTask::NPTask(uint id, ulong wcet, ulong period, ulong deadline,
       ulong arrivalTime) {
        this->id = id;
        this->arrivalTime = arrivalTime;
        this->deadline = deadline;
        this->wcet = wcet;
        this->remainingExecutionTime = wcet;
        this->period = period;
        this->startTime = -1; // 初始值为-1表示未执行
        this->finishTime = -1; // 初始值为-1表示未完成
    }


NPTask::NPTask(uint id, uint r_id, ResourceSet* resourceset, ulong ncs_wcet,
           ulong cs_wcet, ulong period, ulong deadline, uint priority) {
  this->id = id;
  this->index = id;
  this->wcet = ncs_wcet + cs_wcet;
  wcet_non_critical_sections = ncs_wcet;
  wcet_critical_sections = cs_wcet;
  if (0 == deadline)
    this->deadline = period;
  else
    this->deadline = deadline;
  this->period = period;
  this->priority = priority;
  utilization = this->wcet;
  utilization /= this->period;
  density = this->wcet;
  if (this->deadline <= this->period)
    density /= this->deadline;
  else
    density /= this->period;
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  independent = true;
  carry_in = false;
  other_attr = 0;

  add_request(r_id, 1, cs_wcet, cs_wcet,
              resourceset->get_resource_by_id(r_id).get_locality());
  resourceset->add_task(r_id, id);
}

NPTask::~NPTask() {
  // if(affinity)
  // delete affinity;
  requests.clear();
}

void NPTask::init() {
  partition = MAX_INT;
  spin = 0;
  self_suspension = 0;
  local_blocking = 0;
  total_blocking = 0;
  jitter = 0;
  response_time = deadline;
  // priority = MAX_INT;
  independent = true;
  // wcet_non_critical_sections = this->wcet;
  // wcet_critical_sections = 0;
  carry_in = false;
  other_attr = 0;
  speedup = 1;
}

uint NPTask::task_model() { return model; }
void NPTask::add_request(uint r_id, uint num, ulong max_len, ulong total_len,
                       uint locality) {
  wcet_non_critical_sections -= total_len;
  wcet_critical_sections += total_len;
  requests.push_back(Request(r_id, num, max_len, total_len, locality));
}
void NPTask::add_request(ResourceSet *resources, uint r_id, uint num,
                       ulong max_len) {
  uint64_t total_len = num * max_len;
  if (total_len < wcet_non_critical_sections) {
    resources->add_task(r_id, this->id);
    wcet_non_critical_sections -= total_len;
    wcet_critical_sections += total_len;
    uint locality = resources->get_resource_by_id(r_id).get_locality();
    requests.push_back(Request(r_id, num, max_len, total_len, locality));
  }
}
uint NPTask::get_max_job_num(ulong interval) const {
  uint num_jobs;
  // cout << "interval " << interval << endl;
  // cout << "get_response_time() " << get_response_time() << endl;
  // cout << "get_wcet() " << get_wcet() << endl;
  num_jobs = ceiling(interval + get_response_time() - get_wcet(), get_period());
  return num_jobs;
}
uint NPTask::get_max_request_num(uint resource_id, ulong interval) const {
  if (is_request_exist(resource_id)) {
    const Request &request = get_request_by_id(resource_id);
    // cout << get_max_job_num(interval) * request.get_num_requests() << endl;
    return get_max_job_num(interval) * request.get_num_requests();
  } else {
    return 0;
  }
}
double NPTask::get_utilization() const { return utilization; }
double NPTask::get_NCS_utilization() const {
  double temp = get_wcet_non_critical_sections();
  temp /= get_period();
  return temp;
}
double NPTask::get_density() const { return density; }
uint NPTask::get_id() const { return id; }
void NPTask::set_id(uint id) { this->id = id; }
uint NPTask::get_index() const { return index; }
void NPTask::set_index(uint index) { this->index = index; }
ulong NPTask::get_wcet() const { return wcet; }
ulong NPTask::get_wcet_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet, speedup);
  else
    return wcet;
}
ulong NPTask::get_deadline() const { return deadline; }
ulong NPTask::get_period() const { return period; }
ulong NPTask::get_slack() const { return deadline - wcet; }
ulong NPTask::get_arrive_time() const { return arrivalTime; }
ulong NPTask::get_start_time() const { return startTime; }
void NPTask::set_start_time(ulong sTime) { startTime = sTime; }
ulong NPTask::set_arrive_time(ulong aTime) { arrivalTime = aTime;}
ulong NPTask::set_deadline(ulong dTime) { deadline = dTime; }
ulong NPTask::get_finish_time() const { return finishTime; }
void NPTask::set_finish_time(ulong fTime) { finishTime = fTime; }
ulong NPTask::get_remaining_execution_time() const { return remainingExecutionTime; }
void NPTask::set_remaining_execution_time(ulong rETime){ remainingExecutionTime = rETime; }
bool NPTask::is_feasible() const {
  return deadline >= wcet && period >= wcet && wcet > 0;
}

const Resource_Requests &NPTask::get_requests() const { return requests; }
const GPU_Requests &NPTask::get_g_requests() const { return g_requests; }
const Request &NPTask::get_request_by_id(uint id) const {
  Request *result = NULL;
  for (uint i = 0; i < requests.size(); i++) {
    if (id == requests[i].get_resource_id()) return requests[i];
  }
  return *result;
}
const GPURequest &NPTask::get_g_request_by_id(uint id) const {
  GPURequest *result = NULL;
  for (uint i = 0; i < g_requests.size(); i++) {
    if (id == g_requests[i].get_gpu_id()) return g_requests[i];
  }
  return *result;
}
bool NPTask::is_request_exist(uint resource_id) const {
  for (uint i = 0; i < requests.size(); i++) {
    if (resource_id == requests[i].get_resource_id()) return true;
  }
  return false;
}
bool NPTask::is_g_request_exist(uint gpu_id) const {
  for (uint i = 0; i < g_requests.size(); i++) {
    if (gpu_id == g_requests[i].get_gpu_id()) return true;
  }
  return false;
}
void NPTask::update_requests(ResourceSet resources) {
  foreach(requests, request) {
    uint q = request->get_resource_id();
    request->set_locality(resources.get_resource_by_id(q).get_locality());
  }
}
ulong NPTask::get_wcet_critical_sections() const {
  return wcet_critical_sections;
}
ulong NPTask::get_wcet_critical_sections_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet_critical_sections, speedup);
  else
    return wcet_critical_sections;
}
void NPTask::set_wcet_critical_sections(ulong csl) {
  wcet_critical_sections = csl;
}
ulong NPTask::get_wcet_non_critical_sections() const {
  return wcet_non_critical_sections;
}
ulong NPTask::get_wcet_non_critical_sections_heterogeneous() const {
  if (partition != 0XFFFFFFFF)
    return ceiling(wcet_non_critical_sections, speedup);
  else
    return wcet_non_critical_sections;
}
void NPTask::set_wcet_non_critical_sections(ulong ncsl) {
  wcet_non_critical_sections = ncsl;
}

ulong NPTask::get_spin() const { return spin; }
void NPTask::set_spin(ulong spining) { spin = spining; }
ulong NPTask::get_local_blocking() const { return local_blocking; }
void NPTask::set_local_blocking(ulong lb) { local_blocking = lb; }
ulong NPTask::get_remote_blocking() const { return remote_blocking; }
void NPTask::set_remote_blocking(ulong rb) { remote_blocking = rb; }
ulong NPTask::get_total_blocking() const { return total_blocking; }
void NPTask::set_total_blocking(ulong tb) { total_blocking = tb; }
ulong NPTask::get_self_suspension() const { return self_suspension; }
void NPTask::set_self_suspension(ulong ss) { self_suspension = ss; }
ulong NPTask::get_jitter() const { return jitter; }
void NPTask::set_jitter(ulong jit) { jitter = jit; }
ulong NPTask::get_response_time() const { return response_time; }
void NPTask::set_response_time(ulong response) { response_time = response; }
uint NPTask::get_priority() const { return priority; }
void NPTask::set_priority(uint prio) { priority = prio; }
uint NPTask::get_partition() const { return partition; }
void NPTask::set_partition(uint p_id, double speedup) {
  partition = p_id;
  this->speedup = speedup;
}

Cluster NPTask::get_cluster() const { return cluster; }
void NPTask::set_cluster(Cluster cluster, double speedup) {
  this->cluster = cluster;
  this->speedup = speedup;
}
void NPTask::add2cluster(uint32_t p_id, double speedup) {
  bool included = false;
  foreach(cluster, p_k) {
    if ((*p_k) == p_id)
      included = true;
  }
  if (included) {
    cluster.push_back(p_id);
    this->speedup = speedup;
  }
}
bool NPTask::is_in_cluster(uint32_t p_id) const {
  foreach(cluster, p_k) {
    if (p_id == (*p_k))
      return true;
  }
  return false;
}

double NPTask::get_ratio(uint p_id) const { return ratio[p_id]; }
void NPTask::set_ratio(uint p_id, double speed) { ratio[p_id] = speed; }
CPU_Set *NPTask::get_affinity() const { return affinity; }
void NPTask::set_affinity(CPU_Set *affi) { affinity = affi; }
bool NPTask::is_independent() const { return independent; }
void NPTask::set_dependent() { independent = false; }
bool NPTask::is_carry_in() const { return carry_in; }
void NPTask::set_carry_in() { carry_in = true; }
void NPTask::clear_carry_in() { carry_in = false; }
ulong NPTask::get_other_attr() const { return other_attr; }
void NPTask::set_other_attr(ulong attr) { other_attr = attr; }


void NPTask::display_task() {
    cout << "NPTask" << this->get_index() << ": id:"
         << this->get_id()
         << ": partition:"
         << this->get_partition()
         << ": priority:"
         << this->get_priority() << endl;
    cout << " wcet:" << this->get_wcet()
         << " arrive time:" << this->get_arrive_time()
         << " start time:" << this->get_start_time()
         << " finish time:" << this->get_finish_time()
         << " deadline:" << this->get_deadline()
         << " period:" << this->get_period() << endl;
    cout << "-------------------------------------------" << endl;
}


/** Class NPTaskSet */

NPTaskSet::NPTaskSet() {
  utilization_sum = 0;
  utilization_max = 0;
  density_sum = 0;
  density_max = 0;
}
NPTaskSet::~NPTaskSet() { nptasks.clear(); pending_nptasks.clear();}
void NPTaskSet::init() {
  for (uint i = 0; i < nptasks.size(); i++) nptasks[i].init();
  for (uint i = 0; i < pending_nptasks.size(); i++) pending_nptasks[i].init();
}
double NPTaskSet::get_utilization_sum() const { return utilization_sum; }
double NPTaskSet::get_utilization_max() const { return utilization_max; }
double NPTaskSet::get_density_sum() const { return density_sum; }
double NPTaskSet::get_density_max() const { return density_max; }



void NPTaskSet::add_task(ulong wcet, ulong period, ulong deadline) {
  double utilization_new = wcet, density_new = wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  nptasks.push_back(NPTask(nptasks.size(), wcet, period, deadline));
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}
void NPTaskSet::add_task(ResourceSet *resourceset, Param param, ulong wcet,
                       ulong period, ulong deadline) {
  double utilization_new = wcet, density_new = wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  nptasks.push_back(
      NPTask(nptasks.size(), resourceset, param, wcet, period, deadline));
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}
void NPTaskSet::add_task(uint r_id, ResourceSet* resourceset, Param param,
                       uint64_t ncs_wcet, uint64_t cs_wcet, uint64_t period,
                       uint64_t deadline) {
  double utilization_new = ncs_wcet + cs_wcet,
             density_new = ncs_wcet + cs_wcet;
  utilization_new /= period;
  if (0 == deadline)
    density_new /= period;
  else
    density_new /= min(deadline, period);
  nptasks.push_back(NPTask(nptasks.size(), r_id, resourceset, ncs_wcet, cs_wcet,
                       period, deadline));

  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}
void NPTaskSet::add_task(ResourceSet* resourceset, string bufline) {
  uint id = nptasks.size();
  // if (NULL != strstr(bufline.data(), "Utilization")) continue;
  // if (0 == strcmp(buf.data(), "\r")) break;
  vector<uint64_t> elements;
  // cout << "bl:" << bufline << endl;
  double utilization_new, density_new;
  extract_element(&elements, bufline, 0, MAX_INT, ",");
  if (3 <= elements.size()) {
    NPTask nptask(id, elements[0], elements[1], elements[2]);
    uint n = (elements.size() - 3) / 3;
    for (uint i = 0; i < n; i++) {
      uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
      nptask.add_request(elements[3 + i * 3], elements[4 + i * 3],
                        elements[5 + i * 3], length);

      resourceset->add_task(elements[3 + i * 3], id);
    }
    //add_task(nptask);
    utilization_new = nptask.get_utilization();
    density_new = nptask.get_density();
  }
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
  sort_by_period();
}

NPTasks &NPTaskSet::get_np_tasks() { return nptasks; }
NPTasks &NPTaskSet::get_pending_tasks() { return pending_nptasks; }

NPTask &NPTaskSet::get_task_by_id(uint id) {
  foreach(nptasks, nptask) {
    if (id == nptask->get_id()) return (*nptask);
  }
  foreach(pending_nptasks, nptask) {
    if (id == nptask->get_id()) return (*nptask);
  }
  return *reinterpret_cast<NPTask *>(0);
}

NPTask &NPTaskSet::get_task_by_priority(uint pi) {
  foreach(nptasks, nptask) {
    if (pi == nptask->get_priority()) return (*nptask);
  }
  foreach(pending_nptasks, nptask) {
    if (pi == nptask->get_priority()) return (*nptask);
  }
  return *reinterpret_cast<NPTask *>(0);
}

uint NPTaskSet::get_NPTaskSet_size() const { return nptasks.size()+pending_nptasks.size() ; }

ulong NPTaskSet::get_task_wcet(uint index) const {
  return nptasks[index].get_wcet();
}
ulong NPTaskSet::get_task_deadline(uint index) const {
  return nptasks[index].get_deadline();
}
ulong NPTaskSet::get_task_period(uint index) const {
  return nptasks[index].get_period();
}

void NPTaskSet::sort_by_id() {
  sort(nptasks.begin(), nptasks.end(), id_increase<NPTask>);
}

void NPTaskSet::sort_by_index() {
  sort(nptasks.begin(), nptasks.end(), index_increase<NPTask>);
}

void NPTaskSet::sort_by_period() {
  sort(nptasks.begin(), nptasks.end(), period_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_period_pendingTasks() {
  sort(pending_nptasks.begin(), pending_nptasks.end(), period_increase<NPTask>);
  for (int i = 0; i < pending_nptasks.size(); i++) pending_nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_deadline() {
  sort(nptasks.begin(), nptasks.end(), deadline_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_deadline_pendingTasks() {
  sort(pending_nptasks.begin(), pending_nptasks.end(), deadline_increase<NPTask>);
  for (int i = 0; i < pending_nptasks.size(); i++) pending_nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_utilization() {
  sort(nptasks.begin(), nptasks.end(), utilization_decrease<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_density() {
  sort(nptasks.begin(), nptasks.end(), density_decrease<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_DC() {
  sort(nptasks.begin(), nptasks.end(), task_DC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_DCC() {
  sort(nptasks.begin(), nptasks.end(), task_DCC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_DDC() {
  sort(nptasks.begin(), nptasks.end(), task_DDC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::sort_by_UDC() {
  sort(nptasks.begin(), nptasks.end(), task_UDC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void NPTaskSet::RM_Order() {
  sort_by_period();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "RM Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void NPTaskSet::DM_Order() {
  sort_by_deadline();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void NPTaskSet::Density_Decrease_Order() {
  sort_by_density();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void NPTaskSet::DC_Order() {
  sort_by_DC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "DC Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void NPTaskSet::DCC_Order() {
  sort_by_DCC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void NPTaskSet::DDC_Order() {
  sort_by_DDC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void NPTaskSet::UDC_Order() {
  sort_by_UDC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void NPTaskSet::SM_PLUS_Order() {
  sort_by_DC();

  if (1 < nptasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < nptasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < nptasks.size() - 1; index++) {
      vector<NPTask>::iterator it = (nptasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();

      for (int index2 = index + 1; index2 < nptasks.size(); index2++) {
        vector<NPTask>::iterator it2 = (nptasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();

        if (c > gap2 && d > d2) {
          accum[(it->get_id())] += c2;

          NPTask temp = (*it2);
          nptasks.erase((it2));
          nptasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
  }

  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void NPTaskSet::SM_PLUS_2_Order() {
  sort_by_DC();

  if (1 < nptasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < nptasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < nptasks.size() - 1; index++) {
      vector<NPTask>::iterator it = (nptasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();

      for (int index2 = index + 1; index2 < nptasks.size(); index2++) {
        vector<NPTask>::iterator it2 = (nptasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();

        if (c > gap2 && d > d2 && (accum[(it->get_id())] + c2) <= gap) {
          accum[(it->get_id())] += c2;

          NPTask temp = (*it2);
          nptasks.erase((it2));
          nptasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
  }

  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP2:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void NPTaskSet::SM_PLUS_3_Order() {
  sort_by_period();

  if (1 < nptasks.size()) {
    vector<ulong> accum;
    for (uint i = 0; i < nptasks.size(); i++) {
      accum.push_back(0);
    }
    for (int index = 0; index < nptasks.size() - 1; index++) {
      vector<NPTask>::iterator it = (nptasks.begin() + index);
      ulong c = (it)->get_wcet();
      ulong gap = (it)->get_deadline() - (it)->get_wcet();
      ulong d = (it)->get_deadline();
      ulong p = (it)->get_period();

      for (int index2 = index + 1; index2 < nptasks.size(); index2++) {
        vector<NPTask>::iterator it2 = (nptasks.begin() + index2);
        ulong c2 = (it2)->get_wcet();
        ulong gap2 = (it2)->get_deadline() - (it2)->get_wcet();
        ulong d2 = (it2)->get_deadline();
        ulong p2 = (it2)->get_period();
        uint N = ceiling(p, p2);
        if (gap > gap2 && (accum[(it->get_id())] + N * c2) <= gap) {
          accum[(it->get_id())] += N * c2;

          NPTask temp = (*it2);
          nptasks.erase((it2));
          nptasks.insert(it, temp);

          index = 0;
          break;
        }
      }
    }

    for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
  }

  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP3:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

void NPTaskSet::Leisure_Order() {
  NPTasks NewSet;
  sort_by_id();

  for (int i = nptasks.size() - 1; i >= 0; i--) {
    int64_t l_max = 0xffffffffffffffff;
    uint index = i;
    for (int j = i; j >= 0; j--) {
      vector<NPTask>::iterator it = (nptasks.begin() + j);
      NPTask temp = (*it);
      nptasks.erase((it));
      nptasks.push_back(temp);
      int64_t l = leisure(i);
      if (l > l_max) {
        l_max = l;
        index = j;
      }
    }
    sort_by_id();
    vector<NPTask>::iterator it2 = (nptasks.begin() + index);
    NPTask temp2 = (*it2);
    nptasks.erase(it2);
    NewSet.insert(NewSet.begin(), temp2);
  }

  nptasks.clear();
  nptasks = NewSet;

  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_index(i);
    nptasks[i].set_priority(i);
  }

  cout << "Leisure Order:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
}

void NPTaskSet::SM_PLUS_4_Order(uint p_num) {
  sort_by_period();
#if SORT_DEBUG
  cout << "////////////////////////////////////////////" << endl;
  cout << "RM:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif

  if (1 < nptasks.size()) {
    uint min_id;
    ulong min_slack;
    vector<uint> id_stack;

    foreach(nptasks, nptask) {
      nptask->set_other_attr(0);  // accumulative adjustment
    }

    for (uint index = 0; index < nptasks.size(); index++) {
      bool is_continue = false;
      min_id = 0;
      min_slack = MAX_LONG;
      foreach(nptasks, nptask) {  // find next minimum slack
        is_continue = false;
        uint temp_id = nptask->get_id();
        foreach(id_stack, element) {
          if (temp_id == (*element)) is_continue = true;
        }
        if (is_continue) continue;
        ulong temp_slack = nptask->get_slack();
        if (min_slack > nptask->get_slack()) {
          min_id = temp_id;
          min_slack = temp_slack;
        }
      }
      id_stack.push_back(min_id);
      is_continue = false;

      // locate minimum slack
      for (uint index2 = 0; index2 < nptasks.size(); index2++) {
        if (min_id == nptasks[index2].get_id()) {
          vector<NPTask>::iterator task1 = (nptasks.begin() + index2);
          for (int index3 = index2 - 1; index3 >= 0; index3--) {
            vector<NPTask>::iterator task2 = (nptasks.begin() + index3);
            if ((p_num - 1) <= task2->get_other_attr()) {
              is_continue = true;
              break;
            }
            if (task1->get_slack() < task2->get_slack()) {
              NPTask temp = (*task1);
              nptasks.erase((task1));
              if (task2->get_deadline() < task1->get_wcet() + task2->get_wcet())
                task2->set_other_attr(task2->get_other_attr() + 1);
              nptasks.insert(task2, temp);
              task1 = (nptasks.begin() + index3);
            }
          }

          break;
        }
      }
    }

    for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
  }

  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }

#if SORT_DEBUG
  cout << "SMP4:" << endl;
  cout << "-----------------------" << endl;
  foreach(nptasks, nptask) {
    cout << " NPTask " << nptask->get_id() << ":" << endl;
    cout << " WCET:" << nptask->get_wcet() << " Deadline:" << nptask->get_deadline()
         << " Period:" << nptask->get_period()
         << " Gap:" << nptask->get_deadline() - nptask->get_wcet()
         << " Leisure:" << leisure(nptask->get_id()) << endl;
    cout << "-----------------------" << endl;
  }
#endif
}

ulong NPTaskSet::leisure(uint index) {
  // NPTask &nptask = get_task_by_id(index);
  // ulong gap = nptask.get_deadline() - nptask.get_wcet();
  // ulong period = nptask.get_period();
  // ulong remain = gap;
  // for (uint i = 0; i < index; i++) {
  //   NPTask &temp = get_task_by_id(i);
  //   remain -= temp.get_wcet() * ceiling(period, temp.get_period());
  // }
  // return remain;
}

void NPTaskSet::display() {
  foreach(nptasks, nptask) {
    cout << "NPTask" << nptask->get_index() << ": id:"
         << nptask->get_id()
         << ": partition:"
         << nptask->get_partition()
         << ": priority:"
         << nptask->get_priority() << endl;
    cout << " wcet:" << nptask->get_wcet()
         << " arrive time:" << nptask->get_arrive_time()
         << " start time:" << nptask->get_start_time()
         << " finish time:" << nptask->get_finish_time()
         << " deadline:" << nptask->get_deadline()
         << " period:" << nptask->get_period() << endl;
    cout << "-------------------------------------------" << endl;
}
}
void NPTaskSet::update_requests(const ResourceSet &resources) {
  foreach(nptasks, nptask) { nptask->update_requests(resources); }
}

void NPTaskSet::export_taskset(string path) {
  ofstream file(path, ofstream::app);
  stringstream buf;
  buf << "Utilization:" << get_utilization_sum() << "\n";
  foreach(nptasks, nptask) {
    buf << nptask->get_wcet() << "," << nptask->get_period() << ","
        << nptask->get_deadline();
    foreach(nptask->get_requests(), request) {
      buf << "," << request->get_resource_id() << ","
          << request->get_num_requests() << "," << request->get_max_length();
    }
    buf << "\n";
  }
  file << buf.str() << "\n";
  file.flush();
  file.close();
}

void NPTaskSet::task_gen(ResourceSet *resourceset, Param param,
                       double utilization, uint task_number) {
  // Generate nptask set utilization...
  double SumU;
  vector<double> u_set;
  bool re_gen = true;

  switch (param.u_gen) {
    case GEN_UNIFORM:
    cout << "GEN_UNIFORM" << endl;
      SumU = 0;
      while (SumU < utilization) {
        double u = Random_Gen::uniform_real_gen(0, 1);
        if (1 + _EPS <= u)
          continue;
        if (SumU + u > utilization) {
          u = utilization - SumU;
          u_set.push_back(u);
          SumU += u;
          break;
        }
        u_set.push_back(u);
        SumU += u;
      }
      break;

    case GEN_EXPONENTIAL:
    cout << "GEN_EXPONENTIAL" << endl;
      SumU = 0;
      while (SumU < utilization) {
        double u = Random_Gen::exponential_gen(1/param.mean);
        if (1 + _EPS <= u)
          continue;
        if (SumU + u > utilization) {
          u = utilization - SumU;
          u_set.push_back(u);
          SumU += u;
          break;
        }
        u_set.push_back(u);
        SumU += u;
      }
      break;

    case GEN_UUNIFAST:  // Same as the UUnifast_Discard
    cout << "GEN_UUNIFAST" << endl;
    case GEN_UUNIFAST_D:
    cout << "GEN_UUNIFAST_D" << endl;
      while (re_gen) {
        SumU = utilization;
        re_gen = false;
        for (uint i = 1; i < task_number; i++) {
          double temp = Random_Gen::uniform_real_gen(0, 1);
          double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
          double u = SumU - NextSumU;
          if ((u - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            // cout << "Regenerate NPTaskSet." << endl;
            break;
          } else {
            u_set.push_back(u);
            SumU = NextSumU;
          }
        }
        if (!re_gen) {
          if ((SumU - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            // cout << "Regenerate NPTaskSet." << endl;
          } else {
            u_set.push_back(SumU);
          }
        }
      }
      break;

    default:
      while (re_gen) {
        SumU = utilization;
        re_gen = false;
        for (uint i = 1; i < task_number; i++) {
          double temp = Random_Gen::uniform_real_gen(0, 1);
          double NextSumU = SumU * pow(temp, 1.0 / (task_number - i));
          double u = SumU - NextSumU;
          if ((u - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            cout << "Regenerate NPTaskSet." << endl;
            break;
          } else {
            u_set.push_back(u);
            SumU = NextSumU;
          }
        }
        if (!re_gen) {
          if ((SumU - 1) >= _EPS) {
            u_set.clear();
            re_gen = true;
            cout << "Regenerate NPTaskSet." << endl;
          } else {
            u_set.push_back(SumU);
          }
        }
      }
      break;
  }
  cout<<"start generate"<<endl;
  uint i = 0;
  foreach(u_set, u) {
    // ulong period = param.p_range.min + Random_Gen::exponential_gen(2) * (param.p_range.max - param.p_range.min);
    ulong period =
        Random_Gen::uniform_integral_gen(static_cast<int>(param.p_range.min),
                                         static_cast<int>(param.p_range.max));
    ulong wcet = ceil((*u) * period);
    if (0 == wcet)
      wcet++;
    else if (wcet > period)
      wcet = period;
    ulong deadline = 0;
    if (fabs(param.d_range.max) > _EPS) {
      deadline = ceil(period * Random_Gen::uniform_real_gen(param.d_range.min,
                                                            param.d_range.max));
      if (deadline < wcet) deadline = wcet;
    }
    add_task(resourceset, param, wcet, period, deadline);
    cout<<"add scussess"<<endl;
  }
  sort_by_period();
}

void NPTaskSet::task_load(ResourceSet* resourceset, string file_name) {
  ifstream file(file_name, ifstream::in);
  string buf;
  uint id = 0;
  while (getline(file, buf)) {
    if (NULL != strstr(buf.data(), "Utilization")) continue;
    if (0 == strcmp(buf.data(), "\r")) break;

    vector<uint64_t> elements;
    extract_element(&elements, buf, 0, MAX_INT);
    if (3 <= elements.size()) {
      NPTask nptask(id, elements[0], elements[1], elements[2]);
      uint n = (elements.size() - 3) / 3;
      for (uint i = 0; i < n; i++) {
        uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
        nptask.add_request(elements[3 + i * 3], elements[4 + i * 3],
                         elements[5 + i * 3], length);

        resourceset->add_task(elements[3 + i * 3], id);
      }
    //  add_task(nptask);

      id++;
    }
  }
  sort_by_period();
}



