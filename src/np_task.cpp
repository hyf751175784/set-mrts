#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <resources.h>
#include <sort.h>
#include <stdio.h>
#include <np_task.h>
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


NPTask::NPTask(uint id, ulong wcet, ulong period, ulong deadline, uint priority) {
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

  // Experimental: model the requests for GPU
  // only one request per nptask
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

  //  Model the GPU nptask as a shared resource
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

ulong NPTask::get_start_time() const { return start_time; }
void NPTask::set_start_time(ulong stime) { start_time = stime; }
ulong NPTask::get_finish_time() const { return finish_time; }
void NPTask::set_finish_time(ulong ftime) { finish_time = ftime; }

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


/** Class TaskSet */

TaskSet::TaskSet() {
  utilization_sum = 0;
  utilization_max = 0;
  density_sum = 0;
  density_max = 0;
}
TaskSet::~TaskSet() { nptasks.clear(); }
void TaskSet::init() {
  for (uint i = 0; i < nptasks.size(); i++) nptasks[i].init();
}
double TaskSet::get_utilization_sum() const { return utilization_sum; }
double TaskSet::get_utilization_max() const { return utilization_max; }
double TaskSet::get_density_sum() const { return density_sum; }
double TaskSet::get_density_max() const { return density_max; }

void TaskSet::add_task(NPTask nptask) { nptasks.push_back(nptask); }
void TaskSet::add_task(ulong wcet, ulong period, ulong deadline) {
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
void TaskSet::add_task(ResourceSet *resourceset, Param param, ulong wcet,
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
  nptasks.push_back(NPTask(nptasks.size(), r_id, resourceset, ncs_wcet, cs_wcet,
                       period, deadline));

  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
}
void TaskSet::add_task(ResourceSet* resourceset, string bufline) {
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
    add_task(nptask);
    utilization_new = nptask.get_utilization();
    density_new = nptask.get_density();
  }
  utilization_sum += utilization_new;
  density_sum += density_new;
  if (utilization_max < utilization_new) utilization_max = utilization_new;
  if (density_max < density_new) density_max = density_new;
  sort_by_period();
}
NPTasks &TaskSet::get_tasks() { return nptasks; }
NPTask &TaskSet::get_task_by_id(uint id) {
  foreach(nptasks, nptask) {
    if (id == nptask->get_id()) return (*nptask);
  }
  return *reinterpret_cast<NPTask *>(0);
}
NPTask &TaskSet::get_task_by_index(uint index) { return nptasks[index]; }
NPTask &TaskSet::get_task_by_priority(uint pi) {
  foreach(nptasks, nptask) {
    if (pi == nptask->get_priority()) return (*nptask);
  }
  return *reinterpret_cast<NPTask *>(0);
}
bool TaskSet::is_implicit_deadline() {
  foreach_condition(nptasks, nptasks[i].get_deadline() != nptasks[i].get_period());
  return true;
}
bool TaskSet::is_constrained_deadline() {
  foreach_condition(nptasks, nptasks[i].get_deadline() > nptasks[i].get_period());
  return true;
}
bool TaskSet::is_arbitary_deadline() {
  return !(is_implicit_deadline()) && !(is_constrained_deadline());
}
uint TaskSet::get_taskset_size() const { return nptasks.size(); }

double TaskSet::get_task_utilization(uint index) const {
  return nptasks[index].get_utilization();
}
double TaskSet::get_task_density(uint index) const {
  return nptasks[index].get_density();
}
ulong TaskSet::get_task_wcet(uint index) const {
  return nptasks[index].get_wcet();
}
ulong TaskSet::get_task_deadline(uint index) const {
  return nptasks[index].get_deadline();
}
ulong TaskSet::get_task_period(uint index) const {
  return nptasks[index].get_period();
}

void TaskSet::sort_by_id() {
  sort(nptasks.begin(), nptasks.end(), id_increase<NPTask>);
}

void TaskSet::sort_by_index() {
  sort(nptasks.begin(), nptasks.end(), index_increase<NPTask>);
}

void TaskSet::sort_by_period() {
  sort(nptasks.begin(), nptasks.end(), period_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_deadline() {
  sort(nptasks.begin(), nptasks.end(), deadline_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_utilization() {
  sort(nptasks.begin(), nptasks.end(), utilization_decrease<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_density() {
  sort(nptasks.begin(), nptasks.end(), density_decrease<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_DC() {
  sort(nptasks.begin(), nptasks.end(), task_DC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_DCC() {
  sort(nptasks.begin(), nptasks.end(), task_DCC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_DDC() {
  sort(nptasks.begin(), nptasks.end(), task_DDC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::sort_by_UDC() {
  sort(nptasks.begin(), nptasks.end(), task_UDC_increase<NPTask>);
  for (int i = 0; i < nptasks.size(); i++) nptasks[i].set_index(i);
}

void TaskSet::RM_Order() {
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

void TaskSet::DM_Order() {
  sort_by_deadline();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void TaskSet::Density_Decrease_Order() {
  sort_by_density();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void TaskSet::DC_Order() {
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

void TaskSet::DCC_Order() {
  sort_by_DCC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void TaskSet::DDC_Order() {
  sort_by_DDC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void TaskSet::UDC_Order() {
  sort_by_UDC();
  for (uint i = 0; i < nptasks.size(); i++) {
    nptasks[i].set_priority(i);
  }
}

void TaskSet::SM_PLUS_Order() {
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

void TaskSet::SM_PLUS_2_Order() {
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

void TaskSet::SM_PLUS_3_Order() {
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

void TaskSet::Leisure_Order() {
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

void TaskSet::SM_PLUS_4_Order(uint p_num) {
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

ulong TaskSet::leisure(uint index) {
  NPTask &nptask = get_task_by_id(index);
  ulong gap = nptask.get_deadline() - nptask.get_wcet();
  ulong period = nptask.get_period();
  ulong remain = gap;
  for (uint i = 0; i < index; i++) {
    NPTask &temp = get_task_by_id(i);
    remain -= temp.get_wcet() * ceiling(period, temp.get_period());
  }
  return remain;
}

void TaskSet::display() {
  foreach(nptasks, nptask) {
    cout << "NPTask" << nptask->get_index() << ": id:"
         << nptask->get_id()
         << ": partition:"
         << nptask->get_partition()
         << ": priority:"
         << nptask->get_priority() << endl;
    cout << "ncs-wcet:" << nptask->get_wcet_non_critical_sections()
         << " cs-wcet:" << nptask->get_wcet_critical_sections()
         << " wcet:" << nptask->get_wcet()
         << " response time:" << nptask->get_response_time()
         << " deadline:" << nptask->get_deadline()
         << " period:" << nptask->get_period() << endl;
    foreach(nptask->get_requests(), request) {
      cout << "request " << request->get_resource_id() << ":"
           << " num:" << request->get_num_requests()
           << " length:" << request->get_max_length() << endl;
    }
    foreach(nptask->get_g_requests(), g_request) {
      cout << "g_request " << g_request->get_gpu_id() << ":"
           << " num:" << g_request->get_num_requests()
           << " b_num:" << g_request->get_g_block_num()
           << " t_num:" << g_request->get_g_thread_num()
           << " length:" << g_request->get_g_wcet()
           << " workload:" << g_request->get_total_workload() << endl;

    }
    cout << "-------------------------------------------" << endl;
    if (nptask->get_wcet() > nptask->get_response_time()) exit(0);
  }
  cout << get_utilization_sum() << endl;
  // for (int i = 0; i < nptasks.size(); i++) {
  //   cout << "NPTask index:" << nptasks[i].get_index()
  //        << " NPTask id:" << nptasks[i].get_id()
  //        << " NPTask priority:" << nptasks[i].get_priority() << endl;
  // }
}

void TaskSet::update_requests(const ResourceSet &resources) {
  foreach(nptasks, nptask) { nptask->update_requests(resources); }
}

void TaskSet::export_taskset(string path) {
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

void TaskSet::task_gen(ResourceSet *resourceset, Param param,
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
      NPTask nptask(id, elements[0], elements[1], elements[2]);
      uint n = (elements.size() - 3) / 3;
      for (uint i = 0; i < n; i++) {
        uint64_t length = elements[4 + i * 3] * elements[5 + i * 3];
        nptask.add_request(elements[3 + i * 3], elements[4 + i * 3],
                         elements[5 + i * 3], length);

        resourceset->add_task(elements[3 + i * 3], id);
      }
      add_task(nptask);

      id++;
    }
  }
  sort_by_period();
}



