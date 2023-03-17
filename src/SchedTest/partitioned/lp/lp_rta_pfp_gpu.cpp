// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_pfp_gpu.h>
#include <math-helper.h>
#include <solution.h>
#include <sstream>

using std::ostringstream;

/** Class GPUMapper */
uint64_t GPUMapper::encode_request(uint64_t task_id, uint64_t gpu_id, uint64_t grequest_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(task_id < (one << 10));
  assert(gpu_id < (one << 10));
  // cout << "grequest_id:" << grequest_id << endl;
  assert(grequest_id < (one << 10));
  assert(type < (one << 2));

  key |= (type << 30);
  key |= (task_id << 20);
  key |= (gpu_id << 10);
  key |= grequest_id;
  return key;
}

uint64_t GPUMapper::get_type(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3;  // 2 bits
}

uint64_t GPUMapper::get_task(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t GPUMapper::get_gpu_id(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t GPUMapper::get_grequest_id(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

GPUMapper::GPUMapper(uint start_var) : VarMapperBase(start_var) {}

uint GPUMapper::lookup(uint tid, uint gpu_id, uint grequest_id, var_type type) {
  uint64_t key = encode_request(tid, gpu_id, grequest_id, type);
  uint var = var_for_key(key);
  // cout<<"Key:"<<key<<endl;
  // cout<<"Var:"<<var<<endl;
  return var;
}

string GPUMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case GPUMapper::DELAY_DIRECT:
      buf << "Xd[";
      break;
    case GPUMapper::DELAY_OTHER:
      buf << "Xo[";
      break;
    default:
      buf << "X?[";
  }

  buf << get_task(key) << ", " << get_gpu_id(key) << ", " << get_grequest_id(key)
      << "]";

  return buf.str();
}

/** Class LP_RTA_PFP_GPU */
LP_RTA_PFP_GPU::LP_RTA_PFP_GPU()
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "GPU") {}
LP_RTA_PFP_GPU::LP_RTA_PFP_GPU(TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "GPU") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

LP_RTA_PFP_GPU::~LP_RTA_PFP_GPU() {}

bool LP_RTA_PFP_GPU::is_schedulable() {
  foreach(tasks.get_tasks(), ti) {
    if (!BinPacking_WF(&(*ti), &tasks, &processors, &resources, UNTEST))
      return false;
  }
  if (!alloc_schedulable()) {
    return false;
  }
  return true;
}

uint64_t LP_RTA_PFP_GPU::gpu_response_time(Task* ti) {
  uint64_t gpu_time = 0;
  if (ti->get_g_requests().size() != processors.get_gpus().size())
    return 0;

  foreach(ti->get_g_requests(), request) {
    uint64_t temp_rt = 0;
    uint id = request->get_gpu_id();
    GPU dev = processors.get_gpus()[id];
    uint64_t g_workload = 0;
    uint32_t m_g= dev.se_sum;
    uint32_t g= dev.sm_num;
    uint32_t h = dev.se_sum;
    uint32_t H_max = 0;
    uint32_t L_max = 0;
    foreach(tasks.get_tasks(), tx) {
      if (tx->is_g_request_exist(id)) {
        const GPURequest& temp = tx->get_g_request_by_id(id);
        uint32_t h_x = temp.get_g_thread_num();
        uint32_t l_x = temp.get_g_wcet();
        g_workload += temp.get_g_block_num() * h_x * l_x;
        h = gcd(h_x, h);
        if (H_max < h_x)
          H_max = h_x;
        if (L_max < l_x)
          L_max = l_x;
      }
    }
    // cout << "L_max:" << L_max << endl;
    // cout << "H_max:" << H_max << endl;
    // cout << "h:" << h << endl;
    // cout << "g:" << g << endl;
    // cout << "m_g:" << m_g << endl;
    temp_rt = (L_max * (g*m_g - H_max) + g_workload - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-H_max+h)) + request->get_g_wcet();
    gpu_time += temp_rt;
  }

  // cout << "gpu_time:" << gpu_time << endl;
  return gpu_time;
}

uint64_t LP_RTA_PFP_GPU::interference(const Task& ti, uint64_t interval) {
  // uint64_t gpu_rt = ti.get_total_blocking();
  // return (ti.get_wcet_non_critical_sections() + gpu_rt) *
  //        ceiling((interval + ti.get_response_time()), ti.get_period());
  return (ti.get_wcet_non_critical_sections()) *
         ceiling((interval + ti.get_response_time()), ti.get_period());
}

uint64_t LP_RTA_PFP_GPU::response_time(Task* ti) {
  cout << "Task " << ti->get_id() << endl;
  uint64_t test_end = ti->get_deadline();
  uint64_t gpu_rt = gpu_response_time(ti);
  cout << "gpu response time:" << gpu_rt << endl;
  uint64_t test_start = gpu_rt + ti->get_wcet_non_critical_sections();
  uint64_t response = test_start;
  uint64_t demand = test_start;
  // cout << "gpu response time:" << gpu_rt << endl;
  // cout << "response time:" << response << endl;
  // cout << "deadline:" << test_end << endl;
  while (response <= test_end) {
    // demand = gpu_rt + ti->get_wcet_non_critical_sections();

    uint64_t total_interf = 0;

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), task_h) {
      // cout << ti->get_partition() << " " << task_h->get_partition() << endl;
      if (ti->get_partition() == task_h->get_partition()) {
        uint64_t interf = interference(*task_h, response);
        // cout << "interf:" << interf << endl;
        total_interf += interf;
      }
    }

    demand = test_start + total_interf;
    
    // cout << "gpu response time:" << gpu_rt << endl;
    // cout << "total interference:" << total_interf << endl;
    // cout << "response time:" << demand << endl;
    // cout << "deadline:" << test_end << endl;

    if (response == demand)
      return response + ti->get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

bool LP_RTA_PFP_GPU::alloc_schedulable() {
  bool update = false;

  // foreach(tasks.get_tasks(), tx) {
  //   tx->set_total_blocking(gpu_response_time(&(*tx)));
  // }

  do {
    update = false;
    foreach(tasks.get_tasks(), task) {
      // uint64_t response_bound = task.get_response_time();
      uint64_t old_response_time = task->get_response_time();
      if (task->get_partition() == MAX_LONG) continue;

      uint64_t response_bound = response_time(&(*task));

      if (old_response_time != response_bound) update = true;

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else
        return false;
    }
  } while (update);

  return true;
}

uint64_t LP_RTA_PFP_GPU::get_max_wait_time(const Task& ti, const Request& rq) {
  uint priority = ti.get_priority();
  uint p_id = rq.get_locality();
  uint64_t L_i_q = rq.get_max_length();
  uint64_t max_wait_time_l = 0;
  uint64_t max_wait_time_h = 0;
  uint64_t max_wait_time = 0;

  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    foreach(tx->get_requests(), request_v) {
      Resource& resource =
          resources.get_resource_by_id(request_v->get_resource_id());
      if ((resource.get_ceiling() > priority) &&
          (resource.get_locality() == p_id)) {
        if (max_wait_time_l < request_v->get_max_length())
          max_wait_time_l = request_v->get_max_length();
      }
    }
  }

  max_wait_time = max_wait_time_h + max_wait_time_l + L_i_q;

  while (true) {
    uint64_t temp = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      uint64_t request_time = 0;
      foreach(tx->get_requests(), request_y) {
        Resource& resource =
            resources.get_resource_by_id(request_y->get_resource_id());
        if ((resource.get_ceiling() > priority) &&
            (resource.get_locality() == p_id)) {
          request_time +=
              request_y->get_num_requests() * request_y->get_max_length();
        }
      }
      temp +=
          ceiling(tx->get_response_time() + max_wait_time, tx->get_period()) *
          request_time;
    }

    assert(temp >= max_wait_time_h);
    if (temp > max_wait_time_h) {
      max_wait_time_h = temp;
      max_wait_time = max_wait_time_h + max_wait_time_l + L_i_q;
      // cout<<"max_wait_time:"<<max_wait_time<<endl;
    } else {
      max_wait_time = max_wait_time_h + max_wait_time_l + L_i_q;
      // cout<<"max_wait_time:"<<max_wait_time<<endl;
      break;
    }
  }
  return max_wait_time;
}


///////////////// LP_RTA_PFP_GPU_USS ///////////////////

LP_RTA_PFP_GPU_USS::LP_RTA_PFP_GPU_USS() :LP_RTA_PFP_GPU() {}

LP_RTA_PFP_GPU_USS::LP_RTA_PFP_GPU_USS(TaskSet tasks, ProcessorSet processors,
                ResourceSet resources) : LP_RTA_PFP_GPU(tasks, processors, resources) {
  // streams = stream_partition();
  // for (uint i = 0; i < streams.size(); i++) {
  //   cout << "stream " << i << ":" << endl;
  //   foreach(streams[i], it) {
  //     cout << (*it) << " ";
  //   }
  //   cout << endl;
  // }
}

LP_RTA_PFP_GPU_USS::~LP_RTA_PFP_GPU_USS() {}

bool LP_RTA_PFP_GPU_USS::is_schedulable() {
  foreach(tasks.get_tasks(), ti) {
    if (!BinPacking_WF(&(*ti), &tasks, &processors, &resources, UNTEST))
      return false;
    // if (!BinPacking_FF(&(*ti), &tasks, &processors, &resources, TEST))
    //   return false;
  }
  if (!alloc_schedulable()) {
    return false;
  }
  return true;
}

int gthread_decrease(G_Threads t1, G_Threads t2) {
  return t1.gthreads > t2.gthreads;
}

vector<vector<uint32_t>>  LP_RTA_PFP_GPU_USS::stream_partition() {
  vector<vector<uint32_t>> streams;
  vector<G_Threads> set;
  uint thread_1;
  foreach(tasks.get_tasks(), task) {
    G_Threads temp;
    temp.t_id = task->get_id();
    temp.gthreads = task->get_g_request_by_id(0).get_g_thread_num();
    set.push_back(temp);
  }

  sort(set.begin(), set.end(), gthread_decrease);

  vector<uint> stream_0;
  stream_0.push_back(set[0].t_id);

  streams.push_back(stream_0);
  thread_1 = set[0].gthreads;
  for (uint i = 1; i < set.size(); i++) {
    if (set[i].gthreads + thread_1 > processors.get_gpus()[0].se_sum)
      streams[0].push_back(set[i].t_id);
    else {
      vector<uint> new_stream;
      new_stream.push_back(set[i].t_id);
      streams.push_back(new_stream);
    }
  }

  return streams;
}




uint64_t LP_RTA_PFP_GPU_USS::get_gpurt(Task* ti, uint64_t interval) {
  uint64_t gpu_time = 0;
  
  GPUMapper var;
  LinearProgram gpu_bound;
  LinearExpression* obj = new LinearExpression();
  objective(*ti, &gpu_bound, &var, obj, interval);
  gpu_bound.set_objective(obj);
  // construct constraints
  add_constraints(*ti, &gpu_bound, &var, interval);

  GLPKSolution* solution = new GLPKSolution(gpu_bound, var.get_num_vars());

  assert(solution != NULL);

  if (solution->is_solved()) {
    gpu_time =
        lrint(solution->evaluate(*(gpu_bound.get_objective())));
  }

  // foreach(tasks.get_tasks(), tx) {
  //   uint t_id = tx->get_id();
  //   uint32_t N_i_x;
  //   if (t_id == ti->get_id())
  //     N_i_x = 1;
  //   else
  //     N_i_x = tx->get_max_job_num(interval);
  //   for (uint v = 0; v < N_i_x; v++) {

  //     LinearExpression* exp = new LinearExpression();
  //     int var_id = var.lookup(t_id, 0, v, GPUMapper::DELAY_DIRECT);
  //     exp->add_var(var_id);
  //     double result = solution->evaluate(*exp);
  //     cout << "X_" << t_id << "_" << 0 << "_" << v << "=" << result << endl;
  //     delete(exp);
  //   }
  // }
  // cout << "=============" << endl;

  // cout << "gpu_time:" << gpu_time << endl;

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  delete solution;
  // cout << "gpu time(LP):" << gpu_time << endl;
  return gpu_time;
}


uint64_t LP_RTA_PFP_GPU_USS::gpu_response_time(Task* ti) {
  uint64_t test_start = ti->get_wcet_critical_sections();
  // uint64_t test_start = ti->get_period();
  uint64_t test_end = ti->get_deadline();
  uint64_t demand = test_start;
  uint64_t gpu_response_time = test_start;

  while (gpu_response_time <= test_end) {
    demand = get_gpurt(ti, gpu_response_time);
    if (gpu_response_time == demand) {
      // cout << "gpu time(LP):" << gpu_response_time << endl;
      return gpu_response_time;
    }
    else
      gpu_response_time = demand;
  }
  return test_end + 100;
}

// uint64_t LP_RTA_PFP_GPU_USS::gpu_response_time(Task* ti) {
//   uint64_t gpu_time = 0;
//   if (ti->get_g_requests().size() != processors.get_gpus().size())
//     return 0;

//   foreach(ti->get_g_requests(), request) {
//     int64_t temp_rt = 0;
//     int id = request->get_gpu_id();
//     GPU dev = processors.get_gpus()[id];
//     int64_t g_workload = 0;
//     int32_t m_g= dev.se_sum;
//     int32_t g= dev.sm_num;
//     int32_t h = dev.se_sum;
//     int32_t H_max = 0;
//     int32_t L_max = 0;

//     vector<uint64_t> max_workloads;
//     vector<uint64_t> total_workloads;

//     for(uint i = 0; i < processors.get_processor_num(); i++) {
//       max_workloads.push_back(0);
//       total_workloads.push_back(0);
//     }

//     foreach(tasks.get_tasks(), tx) {
//       // cout << "task " << tx->get_id() << " partition:" << tx->get_partition() << endl;
//       uint p_id = tx->get_partition();
//       if ((p_id == ti->get_partition()) && (tx->get_id() != ti->get_id()))
//         continue;
//       if (tx->is_g_request_exist(id)) {
//         const GPURequest& temp = tx->get_g_request_by_id(id);
//         int32_t h_x = temp.get_g_thread_num();
//         int32_t l_x = temp.get_g_wcet();
//         g_workload += temp.get_g_block_num() * h_x * l_x;
//         h = gcd(h_x, h);
//         if (H_max < h_x)
//           H_max = h_x;
//         if (L_max < l_x)
//           L_max = l_x;
//         if (max_workloads[p_id] < temp.get_total_workload())
//           max_workloads[p_id] = temp.get_total_workload();
//         total_workloads[p_id] += temp.get_total_workload();
//       }
//     }

//     // cout << "L_max:" << L_max << endl;
//     // cout << "H_max:" << H_max << endl;
//     // cout << "h:" << h << endl;
//     // cout << "g:" << g << endl;
//     // cout << "m_g:" << m_g << endl;
//     int64_t workload_sum = 0;
//     for(uint i = 0; i < processors.get_processor_num(); i++) {
//       // cout << "max total workload of processor" << i <<  ":" << max_workloads[i] << endl;
//       // cout << "total workload of processor" << i <<  ":" << total_workloads[i] << endl;
//       workload_sum += max_workloads[i];
//     }
//     // cout << "g_workload:" << g_workload << endl;
//     // cout << "workload_sum:" << workload_sum << endl;

//     temp_rt = (L_max * (g*m_g - H_max) + workload_sum - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-H_max+h)) + request->get_g_wcet();
//     // temp_rt = (L_max * (g*m_g - H_max) + g_workload - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-H_max+h)) + request->get_g_wcet();
//     gpu_time += temp_rt;
//   }

//   // cout << "gpu_time:" << gpu_time << endl;
//   return gpu_time;
// }

uint64_t LP_RTA_PFP_GPU_USS::interference(const Task& ti, uint64_t interval) {
  return (ti.get_wcet_non_critical_sections()) *
         ceiling((interval + ti.get_response_time()), ti.get_period());
}

uint64_t LP_RTA_PFP_GPU_USS::response_time(Task* ti) {
  cout << "Task " << ti->get_id() << endl;
  uint64_t test_end = ti->get_deadline();
  uint64_t gpu_rt = gpu_response_time(ti);
  cout << "gpu response time:" << gpu_rt << endl;
  uint64_t test_start = gpu_rt + ti->get_wcet_non_critical_sections();
  
  // uint64_t test_start = ti->get_wcet();
  uint64_t response = test_start;
  uint64_t demand = test_start;
  // cout << "gpu response time:" << gpu_rt << endl;
  // cout << "response time:" << response << endl;
  // cout << "deadline:" << test_end << endl;
  while (response <= test_end) {
    // demand = gpu_response_time(ti) + ti->get_wcet_non_critical_sections();

    uint64_t total_interf = 0;

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), task_h) {
      // cout << ti->get_partition() << " " << task_h->get_partition() << endl;
      if (ti->get_partition() == task_h->get_partition()) {
        uint64_t interf = interference(*task_h, response);
        // cout << "interf:" << interf << endl;
        total_interf += interf;
      }
    }

    demand = test_start + total_interf;
    
    // cout << "gpu response time:" << gpu_rt << endl;
    // cout << "total interference:" << total_interf << endl;
    // cout << "response time:" << demand << endl;
    // cout << "deadline:" << test_end << endl;

    if (response == demand)
      return response + ti->get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

bool LP_RTA_PFP_GPU_USS::alloc_schedulable() {
  bool update = false;

  // tasks.display();

  // foreach(tasks.get_tasks(), tx) {
  //   tx->set_total_blocking(gpu_response_time(&(*tx)));
  // }

  do {
    update = false;
    foreach(tasks.get_tasks(), task) {
      // uint64_t response_bound = task.get_response_time();
      uint64_t old_response_time = task->get_response_time();
      if (task->get_partition() == MAX_LONG) continue;

      uint64_t response_bound = response_time(&(*task));

      if (old_response_time != response_bound) update = true;

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else
        return false;
    }
  } while (update);

  return true;
}

void LP_RTA_PFP_GPU_USS::objective(const Task& ti, LinearProgram* lp, GPUMapper* vars, LinearExpression* obj, uint64_t interval) {
  // cout << "OBJ" << endl;
  uint var_id;

  foreach(ti.get_g_requests(), request) {
    double temp = 0;
    int g_id = request->get_gpu_id();
    GPU dev = processors.get_gpus()[g_id];
    int32_t m_g= dev.se_sum;
    int32_t g= dev.sm_num;
    int32_t h = dev.se_sum;
    int32_t H_max = 0;
    int32_t L_max = 0;

    foreach(tasks.get_tasks(), tx) {
      uint t_id = tx->get_id();
      if (tx->is_g_request_exist(g_id)) {
        const GPURequest& temp = tx->get_g_request_by_id(g_id);
        int32_t h_x = temp.get_g_thread_num();
        int32_t l_x = temp.get_g_wcet();
        h = gcd(h_x, h);
        if (H_max < h_x)
          H_max = h_x;
        if (L_max < l_x)
          L_max = l_x;
      }
    }

    // cout << "L_max:" << L_max << endl;
    // cout << "H_max:" << H_max << endl;
    // cout << "h:" << h << endl;
    // cout << "g:" << g << endl;
    // cout << "m_g:" << m_g << endl;

    var_id = vars->lookup(ti.get_id(), g_id, 0, GPUMapper::DELAY_OTHER);
    temp = (L_max * (g*m_g - H_max) - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-H_max+h)) + request->get_g_wcet();
    // cout << "temp:" << temp << endl;
    obj->add_term(var_id, temp);

    foreach(tasks.get_tasks(), tx) {
      uint t_id = tx->get_id();
      if (tx->is_g_request_exist(g_id)) {
        const GPURequest& g_request = tx->get_g_request_by_id(g_id);
        int64_t g_workload = g_request.get_total_workload();
        double term = g_workload;
        term /= (g*(m_g-H_max+h));

        uint32_t N_i_x;
        if (t_id == ti.get_id())
          N_i_x = 1;
        else {
          N_i_x = tx->get_max_job_num(interval);
          // cout << "interval:" << interval << endl;
          // cout << N_i_x << endl;
        }
        for (uint v = 0; v < N_i_x; v++) {
          var_id = vars->lookup(t_id, g_id, v, GPUMapper::DELAY_DIRECT);
          obj->add_term(var_id, term);
          // cout << "add " << g_workload << "/" << (g*(m_g-H_max+h)) << "*" << "X_" << t_id << "_" << g_id << "_" << v << endl;
        }
      }
    }
  }
}

void LP_RTA_PFP_GPU_USS::add_constraints(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval) {
  // cout << "C1" << endl;
  constraint_1(ti, lp, vars, interval);
  // cout << "C2" << endl;
  constraint_2(ti, lp, vars, interval);
  // cout << "C3" << endl;
  constraint_3(ti, lp, vars, interval);
}

void LP_RTA_PFP_GPU_USS::constraint_1(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval) {

  foreach(ti.get_g_requests(), g_request) {
    int g_id = g_request->get_gpu_id();

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      uint var_id;
      LinearExpression* exp = new LinearExpression();
      uint t_id = th->get_id();
      if (!th->is_g_request_exist(g_id))
        continue;
      if (ti.get_partition() != th->get_partition())
        continue;

      uint32_t N_i_h = th->get_max_job_num(interval);
      for (uint v = 0; v < N_i_h; v++) {
        var_id = vars->lookup(t_id, g_id, v, GPUMapper::DELAY_DIRECT);
        exp->add_var(var_id);
      }
      lp->add_inequality(exp, N_i_h);
    }
  }
}

void LP_RTA_PFP_GPU_USS::constraint_2(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval) {
  foreach(ti.get_g_requests(), g_request) {
    int g_id = g_request->get_gpu_id();

    LinearExpression* exp = new LinearExpression();
    uint var_id;
    foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
      uint t_id = tl->get_id();
      if (!tl->is_g_request_exist(g_id))
        continue;
      if (ti.get_partition() != tl->get_partition())
        continue;

      uint32_t N_i_l = tl->get_max_job_num(interval);
      for (uint v = 0; v < N_i_l; v++) {
        var_id = vars->lookup(t_id, g_id, v, GPUMapper::DELAY_DIRECT);
        exp->add_var(var_id);
      }
    }
    lp->add_inequality(exp, 1);
  }
}

void LP_RTA_PFP_GPU_USS::constraint_3(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval) {
  int p_num = processors.get_processor_num();
  foreach(ti.get_g_requests(), g_request) {
    int g_id = g_request->get_gpu_id();

    for(uint p_id = 0; p_id < p_num; p_id++) {
      if (p_id == ti.get_partition())
        continue;
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      foreach(tasks.get_tasks(), tx) {
        uint t_id = tx->get_id();
        if (p_id != tx->get_partition())
          continue;
        if (!tx->is_g_request_exist(g_id))
          continue;

        uint32_t N_i_x = tx->get_max_job_num(interval);
        for (uint v = 0; v < N_i_x; v++) {
          var_id = vars->lookup(t_id, g_id, v, GPUMapper::DELAY_DIRECT);
          exp->add_var(var_id);
        }
      }

      foreach(tasks.get_tasks(), ty) {
        uint t_id = ty->get_id();
        if (ti.get_partition() != ty->get_partition())
          continue;
        if (!ty->is_g_request_exist(g_id))
          continue;

        uint32_t N_i_y;
        if (t_id == ti.get_id())
          N_i_y = 1;
        else
          N_i_y = ty->get_max_job_num(interval);
        for (uint v = 0; v < N_i_y; v++) {
          var_id = vars->lookup(t_id, g_id, v, GPUMapper::DELAY_DIRECT);
          exp->add_term(var_id, -1);
        }
      }
      lp->add_inequality(exp, 0);
    }
  }
}



///////////////// LP_RTA_PFP_GPU_USS_v2 ///////////////////

LP_RTA_PFP_GPU_USS_v2::LP_RTA_PFP_GPU_USS_v2() :LP_RTA_PFP_GPU_USS() {}

LP_RTA_PFP_GPU_USS_v2::LP_RTA_PFP_GPU_USS_v2(TaskSet tasks, ProcessorSet processors,
                ResourceSet resources) : LP_RTA_PFP_GPU_USS(tasks, processors, resources) {}

LP_RTA_PFP_GPU_USS_v2::~LP_RTA_PFP_GPU_USS_v2() {}


uint64_t exclusice_access_time(uint32_t block_size, uint32_t thread_num, uint64_t thread_length) {
  return ceiling(block_size, 2*(2048/thread_num)) * thread_length;
}

bool LP_RTA_PFP_GPU_USS_v2::is_schedulable() {

  vector<uint64_t> Y;
  foreach(tasks.get_tasks(), ti) {
    if (!BinPacking_WF(&(*ti), &tasks, &processors, &resources, UNTEST))
      return false;
  }

  foreach(tasks.get_tasks(), tx) {
    const GPURequest& g_request = tx->get_g_requests()[0];
    uint32_t b = g_request.get_g_block_num();
    uint32_t h = g_request.get_g_thread_num();
    uint64_t l = g_request.get_g_wcet();
    Y.push_back(exclusice_access_time(b, h, l));
  }

  sort(Y.begin(),Y.end());

  for (int i = 0; i < Y.size(); i++) {
    threshold = Y[i];
    bool schedulable = true;

    foreach(tasks.get_tasks(), task) {
      if (task->get_partition() == MAX_LONG) continue;

      uint64_t response_bound = response_time(&(*task));

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else {
        schedulable = false;
      }
    }

    if (schedulable) 
      return true;
  }
  return false;
}



uint64_t LP_RTA_PFP_GPU_USS_v2::get_gpurt(Task* ti) {
  uint64_t response = 0;
  uint64_t response_2 = 0;

  vector<uint32_t> ST_set, LT_set;

  foreach(tasks.get_tasks(), tx) {
    if (tx->get_wcet_critical_sections() >= threshold) {
      LT_set.push_back(tx->get_id());
    } else {
      ST_set.push_back(tx->get_id());
    }
  }

  foreach(ti->get_g_requests(), request) {
    if (ti->get_wcet_critical_sections() <= threshold) {  // in low channel
    // if (request->get_g_thread_num() * request->get_g_block_num() <= threshold) {  // in low channel
      cout << "short-term:" << endl;
      uint64_t temp_rt = 0;
      uint id = request->get_gpu_id();
      GPU dev = processors.get_gpus()[id];
      uint64_t g_workload = 0;
      uint32_t m_g= dev.se_sum;
      uint32_t g= dev.sm_num;
      uint32_t h = dev.se_sum;
      uint32_t H_max = 0;
      uint64_t L_max = 0;
      uint64_t max_rt = 0;

      foreach(ST_set, st_id) {
        const Task &tx = tasks.get_task_by_id((*st_id));
        if (tx.is_g_request_exist(id)) {
          const GPURequest& temp = tx.get_g_request_by_id(id);
          uint32_t b_x = temp.get_g_block_num();
          uint32_t h_x = temp.get_g_thread_num();
          uint64_t l_x = temp.get_g_wcet();
          g_workload += temp.get_g_block_num() * h_x * l_x;
          h = gcd(h_x, h);
          if (H_max < h_x)
            H_max = h_x;
          if (L_max < l_x)
            L_max = l_x;
        }
      }

      foreach(LT_set, lt_id) {
        const Task &tl = tasks.get_task_by_id((*lt_id));
        if (tl.is_g_request_exist(id)) {
          const GPURequest& temp = tl.get_g_request_by_id(id);
          uint32_t b_l = temp.get_g_block_num();
          uint32_t h_l = temp.get_g_thread_num();
          uint64_t l_l = temp.get_g_wcet();
          uint32_t temp_H_max = std::max(H_max, h_l);
          uint32_t temp_L_max = std::max(L_max, l_l);
          uint32_t temp_h = gcd(h_l, h);
          temp_rt = temp_rt = (temp_L_max * (g*m_g - temp_H_max) + (g_workload + temp.get_total_workload()) - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-temp_H_max+temp_h)) + request->get_g_wcet();
          if (max_rt<temp_rt)
            max_rt = temp_rt;
        }
      }
      response = max_rt;
    } else {
      cout << "long-term:" << endl;
      uint64_t temp_rt = 0;
      uint id = request->get_gpu_id();
      GPU dev = processors.get_gpus()[id];
      uint64_t g_workload = 0;
      uint32_t m_g= dev.se_sum;
      uint32_t g= dev.sm_num;
      uint32_t h = dev.se_sum;
      uint32_t H_max = 0;
      uint64_t L_max = 0;
      uint64_t max_rt = 0;

      foreach(ST_set, st_id) {
        const Task &tx = tasks.get_task_by_id((*st_id));
        if (tx.is_g_request_exist(id)) {
          const GPURequest& temp = tx.get_g_request_by_id(id);
          uint32_t b_x = temp.get_g_block_num();
          uint32_t h_x = temp.get_g_thread_num();
          uint64_t l_x = temp.get_g_wcet();
          g_workload += temp.get_g_block_num() * h_x * l_x;
          h = gcd(h_x, h);
          if (H_max < h_x)
            H_max = h_x;
          if (L_max < l_x)
            L_max = l_x;
        }
      }

      foreach(LT_set, lt_id) {
        const Task &tl = tasks.get_task_by_id((*lt_id));
        if (tl.is_g_request_exist(id)) {
          const GPURequest& temp = tl.get_g_request_by_id(id);
          uint32_t b_l = temp.get_g_block_num();
          uint32_t h_l = temp.get_g_thread_num();
          uint64_t l_l = temp.get_g_wcet();
          uint32_t temp_H_max = std::max(H_max, h_l);
          uint32_t temp_L_max = std::max(L_max, l_l);
          uint32_t temp_h = gcd(h_l, h);
          temp_rt = temp_rt = (temp_L_max * (g*m_g - temp_H_max) + (g_workload + temp.get_total_workload()) - temp.get_g_thread_num() * temp.get_g_wcet())/(g*(m_g-temp_H_max+temp_h)) + temp.get_g_wcet();
          response += temp_rt;
        }
      }
    }
  }
  cout << "response:" << response << endl;
  return response;

}

// uint64_t LP_RTA_PFP_GPU_USS_v2::get_gpurt(Task* ti) {
//   uint64_t gpu_time = 0;
  
//   GPUMapper var;
//   LinearProgram gpu_bound;
//   LinearExpression* obj = new LinearExpression();
//   objective(*ti, &gpu_bound, &var, obj);
//   gpu_bound.set_objective(obj);
//   // construct constraints
//   add_constraints(*ti, &gpu_bound, &var);

//   GLPKSolution* solution = new GLPKSolution(gpu_bound, var.get_num_vars());

//   assert(solution != NULL);

//   if (solution->is_solved()) {
//     gpu_time =
//         lrint(solution->evaluate(*(gpu_bound.get_objective())));
//   }

//   foreach(tasks.get_tasks(), tx) {
//     uint t_id = tx->get_id();
//     uint32_t N_i_x;
//     if (t_id == ti->get_id())
//       N_i_x = 1;
//     else
//       N_i_x = tx->get_max_job_num(ti->get_period());
//     for (uint v = 0; v < N_i_x; v++) {

//       LinearExpression* exp = new LinearExpression();
//       int var_id = var.lookup(t_id, 0, v, GPUMapper::DELAY_DIRECT);
//       exp->add_var(var_id);
//       double result = solution->evaluate(*exp);
//       cout << "X_" << t_id << "_" << 0 << "_" << v << "=" << result << endl;
//       delete(exp);
//     }
//   }
//   cout << "=============" << endl;

//   cout << "gpu_time:" << gpu_time << endl;

// #if GLPK_MEM_USAGE_CHECK == 1
//   int peak;
//   glp_mem_usage(NULL, &peak, NULL, NULL);
//   cout << "Peak memory usage:" << peak << endl;
// #endif

//   delete solution;
//   // cout << "gpu time(LP):" << gpu_time << endl;
//   return gpu_time;
// }


uint64_t LP_RTA_PFP_GPU_USS_v2::gpu_response_time(Task* ti) {
  return get_gpurt(ti);
  // uint64_t test_end = ti->get_deadline();
  // uint64_t test_start = ti->get_wcet_critical_sections();
  // uint64_t response = test_start;
  // uint64_t demand = test_start;

  // while (response <= test_end) {
  //   demand = get_gpurt(ti, response);

  //   if (response == demand)
  //     return response;
  //   else
  //     response = demand;
  // }

  // return test_end + 100;
}

uint64_t LP_RTA_PFP_GPU_USS_v2::response_time(Task* ti) {
  uint64_t test_end = ti->get_deadline();
  // uint64_t gpu_rt = ti->get_total_blocking();
  uint64_t test_start = gpu_response_time(ti) + ti->get_wcet_non_critical_sections();
  // uint64_t test_start = ti->get_wcet();
  uint64_t response = test_start;
  uint64_t demand = test_start;
  // cout << "gpu response time:" << gpu_rt << endl;
  // cout << "response time:" << response << endl;
  // cout << "deadline:" << test_end << endl;
  while (response <= test_end) {
    // demand = gpu_response_time(ti) + ti->get_wcet_non_critical_sections();

    uint64_t total_interf = 0;

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), task_h) {
      // cout << ti->get_partition() << " " << task_h->get_partition() << endl;
      if (ti->get_partition() == task_h->get_partition()) {
        uint64_t interf = interference(*task_h, response);
        // cout << "interf:" << interf << endl;
        total_interf += interf;
      }
    }

    demand = test_start + total_interf;
    
    // cout << "gpu response time:" << gpu_rt << endl;
    // cout << "total interference:" << total_interf << endl;
    // cout << "response time:" << demand << endl;
    // cout << "deadline:" << test_end << endl;

    if (response == demand)
      return response + ti->get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

bool LP_RTA_PFP_GPU_USS_v2::alloc_schedulable() {
  bool update = false;

  // tasks.display();

  // foreach(tasks.get_tasks(), tx) {
  //   tx->set_total_blocking(gpu_response_time(&(*tx)));
  // }

  do {
    update = false;
    foreach(tasks.get_tasks(), task) {
      // uint64_t response_bound = task.get_response_time();
      uint64_t old_response_time = task->get_response_time();
      if (task->get_partition() == MAX_LONG) continue;

      uint64_t response_bound = response_time(&(*task));

      if (old_response_time != response_bound) update = true;

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else
        return false;
    }
  } while (update);

  return true;
}





////////////////////
LP_RTA_PFP_GPU_prio::LP_RTA_PFP_GPU_prio() :LP_RTA_PFP_GPU_USS_v2() {}

LP_RTA_PFP_GPU_prio::LP_RTA_PFP_GPU_prio(TaskSet tasks, ProcessorSet processors,
                ResourceSet resources) : LP_RTA_PFP_GPU_USS_v2(tasks, processors, resources) {}

LP_RTA_PFP_GPU_prio::~LP_RTA_PFP_GPU_prio() {}


uint64_t LP_RTA_PFP_GPU_prio::get_gpurt(Task* ti) {
  uint64_t response = 0;
  uint64_t response_2 = 0;

  vector<uint32_t> ST_set, LT_set;

  foreach(tasks.get_tasks(), tx) {
    if (tx->get_wcet_critical_sections() >= threshold) {
      LT_set.push_back(tx->get_id());
    } else {
      ST_set.push_back(tx->get_id());
    }
  }

  foreach(ti->get_g_requests(), request) {
    if (ti->get_wcet_critical_sections() <= threshold) {  // in low channel
    // if (request->get_g_thread_num() * request->get_g_block_num() <= threshold) {  // in low channel
      cout << "short-term:" << endl;
      uint64_t temp_rt = 0;
      uint id = request->get_gpu_id();
      GPU dev = processors.get_gpus()[id];
      uint64_t g_workload = 0;
      uint32_t m_g= dev.se_sum;
      uint32_t g= dev.sm_num;
      uint32_t h = dev.se_sum;
      uint32_t H_max = 0;
      uint64_t L_max = 0;
      uint64_t max_rt = 0;

      foreach(ST_set, st_id) {
        const Task &tx = tasks.get_task_by_id((*st_id));
        if (tx.is_g_request_exist(id)) {
          const GPURequest& temp = tx.get_g_request_by_id(id);
          uint32_t b_x = temp.get_g_block_num();
          uint32_t h_x = temp.get_g_thread_num();
          uint64_t l_x = temp.get_g_wcet();
          g_workload += temp.get_g_block_num() * h_x * l_x;
          h = gcd(h_x, h);
          if (H_max < h_x)
            H_max = h_x;
          if (L_max < l_x)
            L_max = l_x;
        }
      }

      foreach(LT_set, lt_id) {
        const Task &tl = tasks.get_task_by_id((*lt_id));
        if (tl.is_g_request_exist(id)) {
          const GPURequest& temp = tl.get_g_request_by_id(id);
          uint32_t b_l = temp.get_g_block_num();
          uint32_t h_l = temp.get_g_thread_num();
          uint64_t l_l = temp.get_g_wcet();
          uint32_t temp_H_max = std::max(H_max, h_l);
          uint32_t temp_L_max = std::max(L_max, l_l);
          uint32_t temp_h = gcd(h_l, h);
          temp_rt = (temp_L_max * (g*m_g - temp_H_max) + (g_workload + temp.get_total_workload()) - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-temp_H_max+temp_h)) + request->get_g_wcet();
          if (max_rt<temp_rt)
            max_rt = temp_rt;
        }
      }
      response = max_rt;
    } else {
      cout << "long-term:" << endl;
      uint64_t temp_rt = 0;
      uint id = request->get_gpu_id();
      GPU dev = processors.get_gpus()[id];
      uint64_t g_workload = 0;
      uint32_t m_g= dev.se_sum;
      uint32_t g= dev.sm_num;
      uint32_t h = dev.se_sum;
      uint32_t H_max = 0;
      uint64_t L_max = 0;
      uint64_t max_rt = 0;

      foreach(ST_set, st_id) {
        const Task &tx = tasks.get_task_by_id((*st_id));
        if (tx.is_g_request_exist(id)) {
          const GPURequest& temp = tx.get_g_request_by_id(id);
          uint32_t b_x = temp.get_g_block_num();
          uint32_t h_x = temp.get_g_thread_num();
          uint64_t l_x = temp.get_g_wcet();
          g_workload += temp.get_g_block_num() * h_x * l_x;
          h = gcd(h_x, h);
          if (H_max < h_x)
            H_max = h_x;
          if (L_max < l_x)
            L_max = l_x;
        }
      }



      // response time since G_i is issued
      uint32_t temp_H_max = std::max(H_max, request->get_g_thread_num());
      uint32_t temp_L_max = std::max(L_max, request->get_g_wcet());
      uint32_t temp_h = gcd(request->get_g_thread_num(), h);
      temp_rt = (temp_L_max * (g*m_g - temp_H_max) + (g_workload + request->get_total_workload()) - request->get_g_thread_num() * request->get_g_wcet())/(g*(m_g-temp_H_max+temp_h)) + request->get_g_wcet();
      response += temp_rt;



      // blocking time
      uint64_t blocking = 0;
      foreach(LT_set, lt_id) {
        const Task &tl = tasks.get_task_by_id((*lt_id));
        if (tl.get_priority() < ti->get_priority()) {  // lower priority long-time GPU task
          if (tl.is_g_request_exist(id)) {
            const GPURequest& temp = tl.get_g_request_by_id(id);
            uint32_t b_l = temp.get_g_block_num();
            uint32_t h_l = temp.get_g_thread_num();
            uint64_t l_l = temp.get_g_wcet();
            uint32_t temp_H_max = std::max(H_max, h_l);
            uint32_t temp_L_max = std::max(L_max, l_l);
            uint32_t temp_h = gcd(h_l, h);
            temp_rt = (temp_L_max * (g*m_g - temp_H_max) + (g_workload + temp.get_total_workload()) - temp.get_g_thread_num() * temp.get_g_wcet())/(g*(m_g-temp_H_max+temp_h)) + temp.get_g_wcet();
            if (blocking < temp_rt)
              blocking = temp_rt;
          }
        }
      }

      uint64_t test_start = blocking;
      uint64_t demand = test_start;
      uint64_t test_end = ti->get_deadline();

      while (blocking < test_end) {
        demand = test_start;
        foreach(LT_set, lt_id) {
          const Task &tl = tasks.get_task_by_id((*lt_id));
          if (tl.get_priority() > ti->get_priority()) {  // higher priority long-time GPU task
            if (tl.is_g_request_exist(id)) {
              const GPURequest& temp = tl.get_g_request_by_id(id);
              uint32_t b_l = temp.get_g_block_num();
              uint32_t h_l = temp.get_g_thread_num();
              uint64_t l_l = temp.get_g_wcet();
              uint32_t temp_H_max = std::max(H_max, h_l);
              uint32_t temp_L_max = std::max(L_max, l_l);
              uint32_t temp_h = gcd(h_l, h);
              temp_rt = (temp_L_max * (g*m_g - temp_H_max) + (g_workload + temp.get_total_workload()) - temp.get_g_thread_num() * temp.get_g_wcet())/(g*(m_g-temp_H_max+temp_h)) + temp.get_g_wcet();
              demand += tl.get_max_job_num(blocking) * temp_rt;
            }
          }
        }

        if (blocking < demand)
          blocking = demand;
        else
          break;
      }
      response += blocking;
    }
  }
  cout << "response:" << response << endl;
  return response;
}

uint64_t LP_RTA_PFP_GPU_prio::gpu_response_time(Task* ti) {
  return get_gpurt(ti);
}

bool LP_RTA_PFP_GPU_prio::is_schedulable() {

  vector<uint64_t> Y;
  foreach(tasks.get_tasks(), ti) {
    if (!BinPacking_WF(&(*ti), &tasks, &processors, &resources, UNTEST))
      return false;
  }

  foreach(tasks.get_tasks(), tx) {
    const GPURequest& g_request = tx->get_g_requests()[0];
    uint32_t b = g_request.get_g_block_num();
    uint32_t h = g_request.get_g_thread_num();
    uint64_t l = g_request.get_g_wcet();
    Y.push_back(exclusice_access_time(b, h, l));
  }

  sort(Y.begin(),Y.end());

  for (int i = 0; i < Y.size(); i++) {
    threshold = Y[i];
    bool schedulable = true;

    foreach(tasks.get_tasks(), task) {
      if (task->get_partition() == MAX_LONG) continue;

      uint64_t response_bound = response_time(&(*task));

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else {
        schedulable = false;
      }
    }

    if (schedulable) 
      return true;
  }
  return false;
}

uint64_t LP_RTA_PFP_GPU_prio::response_time(Task* ti) {
  uint64_t test_end = ti->get_deadline();
  // uint64_t gpu_rt = ti->get_total_blocking();
  uint64_t test_start = gpu_response_time(ti) + ti->get_wcet_non_critical_sections();
  // uint64_t test_start = ti->get_wcet();
  uint64_t response = test_start;
  uint64_t demand = test_start;
  // cout << "gpu response time:" << gpu_rt << endl;
  // cout << "response time:" << response << endl;
  // cout << "deadline:" << test_end << endl;
  while (response <= test_end) {
    // demand = gpu_response_time(ti) + ti->get_wcet_non_critical_sections();

    uint64_t total_interf = 0;

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), task_h) {
      // cout << ti->get_partition() << " " << task_h->get_partition() << endl;
      if (ti->get_partition() == task_h->get_partition()) {
        uint64_t interf = interference(*task_h, response);
        // cout << "interf:" << interf << endl;
        total_interf += interf;
      }
    }

    demand = test_start + total_interf;
    
    // cout << "gpu response time:" << gpu_rt << endl;
    // cout << "total interference:" << total_interf << endl;
    // cout << "response time:" << demand << endl;
    // cout << "deadline:" << test_end << endl;

    if (response == demand)
      return response + ti->get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}