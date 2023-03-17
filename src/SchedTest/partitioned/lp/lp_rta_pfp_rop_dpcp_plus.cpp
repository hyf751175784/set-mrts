// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp_rta_pfp_rop_dpcp_plus.h>
#include <math-helper.h>
#include <math.h>
#include <solution.h>
#include <lp.h>
#include <sstream>

using std::min;

LP_RTA_PFP_ROP_DPCP_PLUS::LP_RTA_PFP_ROP_DPCP_PLUS()
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {}

LP_RTA_PFP_ROP_DPCP_PLUS::LP_RTA_PFP_ROP_DPCP_PLUS(TaskSet tasks,
                                                   ProcessorSet processors,
                                                   ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

LP_RTA_PFP_ROP_DPCP_PLUS::~LP_RTA_PFP_ROP_DPCP_PLUS() {}

// ROP
ulong LP_RTA_PFP_ROP_DPCP_PLUS::blocking_bound(const Task& ti, uint r_id) {
  ulong bound = 0;
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    foreach(tl->get_requests(), request_v) {
      Resource& resource_v =
          resources.get_resource_by_id(request_v->get_resource_id());
      if (resource_v.get_ceiling() <= resource_q.get_ceiling()) {
        ulong L_l_v = request_v->get_max_length();
        if (L_l_v > bound) bound = L_l_v;
      }
    }
  }
  return bound;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::request_bound(const Task& ti, uint r_id) {
  ulong deadline = ti.get_deadline();
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  uint p_id = resource_q.get_locality();
  ulong test_start = request_q.get_max_length() + blocking_bound(ti, r_id);
  ulong bound = test_start;

  while (bound <= deadline) {
    ulong temp = test_start;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      foreach(th->get_requests(), request_v) {
        Resource& resource_v =
            resources.get_resource_by_id(request_v->get_resource_id());
        if (p_id == resource_v.get_locality()) {
          temp += CS_workload(*th, request_v->get_resource_id(), bound);
        }
      }
    }

    assert(temp >= bound);

    if (temp != bound) {
      bound = temp;
    } else {
      return bound;
    }
  }
  return deadline + 100;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::formula_30(const Task& ti, uint p_id,
                                      ulong interval) {
  ulong miu = 0;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    Resource& resource = resources.get_resource_by_id(q);
    if (p_id == resource.get_locality()) {
      miu += request->get_num_requests() * request->get_max_length();
    }
  }

  foreach_task_except(tasks.get_tasks(), ti, tj) {
    foreach(tj->get_requests(), request_v) {
      uint v = request_v->get_resource_id();
      Resource& resource_v = resources.get_resource_by_id(v);
      if (p_id == resource_v.get_locality()) {
        miu += CS_workload(*tj, v, interval);
      }
    }
  }
  return miu;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::angent_exec_bound(const Task& ti, uint p_id,
                                             ulong interval) {
  ulong deadline = ti.get_deadline();
  ulong lambda = 0;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    Resource& resource = resources.get_resource_by_id(q);

    if (p_id == resource.get_locality()) {
      uint N_i_q = request->get_num_requests();
      ulong r_b = request_bound(ti, q);
      if (deadline < r_b) {
        // cout<<"exceed."<<endl;
        return r_b;
      }
      lambda += N_i_q * r_b;
    }
  }

  ulong miu = formula_30(ti, p_id, interval);

  return min(lambda, miu);
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::NCS_workload(const Task& ti, ulong interval) {
  ulong e = ti.get_wcet_non_critical_sections();
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  return ceiling((interval + r - e), p) * e;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::CS_workload(const Task& ti, uint resource_id,
                                       ulong interval) {
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  ulong agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::response_time_AP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections();
  ulong response_time = test_start;

#if RTA_DEBUG == 1
  cout << "AP "
       << "Task" << ti.get_id() << " priority:" << ti.get_priority()
       << ": partition:" << ti.get_partition() << endl;
  cout << "ncs-wcet:" << ti.get_wcet_non_critical_sections()
       << " cs-wcet:" << ti.get_wcet_critical_sections()
       << " wcet:" << ti.get_wcet()
       << " response time:" << ti.get_response_time()
       << " deadline:" << ti.get_deadline() << " period:" << ti.get_period()
       << " utilization:" << ti.get_utilization() << endl;
  foreach(ti.get_requests(), request) {
    cout << "request" << request->get_resource_id() << ":"
         << " resource_u"
         << resources.get_resource_by_id(request->get_resource_id())
                .get_utilization()
                
         << " num:" << request->get_num_requests()
         << " length:" << request->get_max_length() << " locality:"
         << resources.get_resource_by_id(request->get_resource_id()).get_locality()
         << endl;
  }
#endif
  while (response_time <= test_bound) {
    ulong temp = test_start;
    ulong temp2 = temp;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
#if RTA_DEBUG == 1
      cout << "Th:" << th->get_id() << " partition:" << th->get_partition()
           << " priority:" << th->get_priority() << endl;
#endif
      if (th->get_partition() == ti.get_partition()) {
        temp += NCS_workload(*th, response_time);
      }
    }
#if RTA_DEBUG == 1
    cout << "NCS workload:" << temp - test_start << endl;
    temp2 = temp;
#endif

    for (uint k = 0; k < processors.get_processor_num(); k++) {
      if (p_id == k) continue;
      ulong agent_bound = angent_exec_bound(ti, k, response_time);
      // cout<<"AB of processor
      // "<<k<<":"<<agent_bound<<endl;
      temp += agent_bound;
    }
#if RTA_DEBUG == 1
    cout << "agent exec bound:" << temp - temp2 << endl;

    cout << "response time:" << temp << endl;
#endif

    assert(temp >= response_time);

    // cout<<"t"<<ti.get_id()<<": wcet:"<<ti.get_wcet()<<"
    // deadline:"<<ti.get_deadline()<<" rt:"<<temp<<endl;

    if (temp != response_time) {
      response_time = temp;
    } else if (temp == response_time) {
#if RTA_DEBUG == 1
      cout << "==============================" << endl;
#endif
      return response_time;
    }
  }
    // cout<<"AP miss."<<endl;
#if RTA_DEBUG == 1
  cout << "==============================" << endl;
#endif
  return test_bound + 100;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::response_time_SP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections();
  ulong response_time = test_start;

#if RTA_DEBUG == 1
  cout << "SP "
       << "Task" << ti.get_id() << ": partition:" << ti.get_partition() << endl;
  cout << "ncs-wcet:" << ti.get_wcet_non_critical_sections()
       << " cs-wcet:" << ti.get_wcet_critical_sections()
       << " wcet:" << ti.get_wcet()
       << " response time:" << ti.get_response_time()
       << " deadline:" << ti.get_deadline() << " period:" << ti.get_period()
       << " utilization:" << ti.get_utilization() << endl;
  foreach(ti.get_requests(), request) {
    cout << "request" << request->get_resource_id() << ":"
         << " resource_u"
         << resources.get_resource_by_id(request->get_resource_id())
                .get_utilization()
                
         << " num:" << request->get_num_requests()
         << " length:" << request->get_max_length() << " locality:"
         << resources.get_resource_by_id(request->get_resource_id()).get_locality()
         << endl;
  }
#endif

  while (response_time <= test_bound) {
    ulong temp = test_start;
    ulong temp2 = temp;

    foreach(ti.get_requests(), request) {  // A
      uint q = request->get_resource_id();
      Resource& resource = resources.get_resource_by_id(q);
      if (p_id == resource.get_locality()) {
        temp += request->get_num_requests() * request->get_max_length();
      }
    }

#if RTA_DEBUG == 1
    cout << "A:" << temp - temp2 << endl;
    temp2 = temp;
#endif

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {  // W
      if (p_id == th->get_partition()) {
        temp += NCS_workload(*th, response_time);
      }
    }

#if RTA_DEBUG == 1
    cout << "NCS workload:" << temp - temp2 << endl;
    temp2 = temp;
#endif

    foreach_task_except(tasks.get_tasks(), ti, tj) {
      foreach(tj->get_requests(), request_v) {
        uint v = request_v->get_resource_id();
        Resource& resource_v = resources.get_resource_by_id(v);
        if (p_id == resource_v.get_locality()) {
          temp += CS_workload(*tj, v, response_time);
        }
      }
    }
#if RTA_DEBUG == 1
    cout << "CS workload:" << temp - temp2 << endl;
    temp2 = temp;
#endif

    for (uint k = 0; k < processors.get_processor_num(); k++) {  // Theata
      if (p_id == k) continue;

      temp += angent_exec_bound(ti, k, response_time);
    }
#if RTA_DEBUG == 1
    cout << "Theata:" << temp - temp2 << endl;
    temp2 = temp;

    cout << "response time:" << temp << endl;
#endif

    assert(temp >= response_time);

    // cout<<"t"<<ti.get_id()<<": wcet:"<<ti.get_wcet()<<"
    // deadline:"<<ti.get_deadline()<<" rt:"<<temp<<endl;

    if (temp != response_time) {
      response_time = temp;
    } else if (temp == response_time) {
#if RTA_DEBUG == 1
      cout << "==============================" << endl;
#endif
      return response_time;
    }
  }
    // cout<<"SP miss."<<endl;
#if RTA_DEBUG == 1
  cout << "==============================" << endl;
#endif
  return test_bound + 100;
}

bool LP_RTA_PFP_ROP_DPCP_PLUS::worst_fit_for_resources(
    uint active_processor_num) {
  /*
  foreach(resources.get_resources(), resource)
  {
          cout<<"task size:"<<resource->get_tasks()->get_taskset_size()<<endl;
          cout<<"resource:"<<resource->get_resource_id()<<"
  utilization:"<<resource->get_utilization();
  }
  */
  resources.sort_by_utilization();

  // processors.update(&tasks, &resources);

  foreach(resources.get_resources(), resource) {
    // cout<<"assign resource:"<<resource->get_resource_id()<<endl;
    // if(abs(resource->get_utilization()) <= _EPS)
    // continue;
    double r_utilization = 1;
    uint assignment = 0;
    for (uint i = 0; i < active_processor_num; i++) {
      Processor& p_temp = processors.get_processors()[i];
      if (r_utilization > p_temp.get_resource_utilization()) {
        r_utilization = p_temp.get_resource_utilization();
        assignment = i;
      }
    }
    Processor& processor = processors.get_processors()[assignment];
    if (processor.add_resource(resource->get_resource_id())) {
      resource->set_locality(assignment);
    } else {
      return false;
    }
  }
  return true;
}

bool LP_RTA_PFP_ROP_DPCP_PLUS::is_first_fit_for_tasks_schedulable_1(
    uint start_processor) {
  bool schedulable;
  uint p_num = processors.get_processor_num();
  // tasks.RM_Order();
  foreach(tasks.get_tasks(), ti) {
    uint assignment;
    schedulable = false;
    for (uint i = start_processor; i < start_processor + p_num; i++) {
      assignment = i % p_num;
      Processor& processor = processors.get_processors()[assignment];

      if (processor.add_task(ti->get_id())) {
        ti->set_partition(assignment);
        if (alloc_schedulable(&(*ti))) {
          schedulable = true;
          break;
        } else {
          ti->init();
          processor.remove_task(ti->get_id());
        }
      }
    }
    if (!schedulable) {
      return schedulable;
    }
  }
  return schedulable;
}

bool LP_RTA_PFP_ROP_DPCP_PLUS::is_first_fit_for_tasks_schedulable_2(
    uint start_processor) {
  bool schedulable;
  uint p_num = processors.get_processor_num();
  // tasks.RM_Order();
  foreach(tasks.get_tasks(), ti) {
    uint assignment;
    schedulable = false;
    for (uint i = start_processor; i < start_processor + p_num; i++) {
      assignment = i % p_num;
      Processor& processor = processors.get_processors()[assignment];

      if (processor.add_task(ti->get_id())) {
        ti->set_partition(assignment);
        if (alloc_schedulable()) {
          schedulable = true;
          break;
        } else {
          ti->init();
          processor.remove_task(ti->get_id());
        }
      }
    }
    if (!schedulable) {
      return schedulable;
    }
  }
  return schedulable;
}

// DPCP
ulong LP_RTA_PFP_ROP_DPCP_PLUS::local_blocking(Task* ti) {
  ulong local_blocking = 0;
  Resources& r = resources.get_resources();
  const Resource_Requests& rr = ti->get_requests();
  uint p_id = ti->get_partition();      // processor id
  ulong r_i = ti->get_response_time();  // response time of task i(t_id)
  DPCPMapper var;
  LinearProgram local_bound;
  LinearExpression* local_obj = new LinearExpression();
  lp_dpcp_objective(*ti, &local_bound, &var, local_obj, NULL);
  local_bound.set_objective(local_obj);
  // construct constraints
  lp_dpcp_add_constraints(*ti, &local_bound, &var);

  GLPKSolution* lb_solution = new GLPKSolution(local_bound, var.get_num_vars());

  assert(lb_solution != NULL);

  if (lb_solution->is_solved()) {
    local_blocking =
        lrint(lb_solution->evaluate(*(local_bound.get_objective())));
  }

  ti->set_local_blocking(local_blocking);

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  delete lb_solution;
  return local_blocking;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::remote_blocking(Task* ti) {
  // cout << "Remote blocking for Task " << ti->get_id() << endl;
  ulong remote_blocking = 0;
  Resources& r = resources.get_resources();
  const Resource_Requests& rr = ti->get_requests();
  uint p_id = ti->get_partition();      // processor id
  ulong r_i = ti->get_response_time();  // response time of task i(t_id)
  DPCPMapper var;
  LinearProgram remote_bound;
  LinearExpression* remote_obj = new LinearExpression();
  lp_dpcp_objective(*ti, &remote_bound, &var, NULL, remote_obj);
  remote_bound.set_objective(remote_obj);
  // construct constraints
  lp_dpcp_add_constraints(*ti, &remote_bound, &var);

  GLPKSolution* rb_solution =
      new GLPKSolution(remote_bound, var.get_num_vars());

  if (rb_solution->is_solved()) {
    remote_blocking =
        lrint(rb_solution->evaluate(*(remote_bound.get_objective())));

    // foreach_task_except(tasks.get_tasks(), (*ti), tx) {
    //   uint x = tx->get_index();
    //   foreach(tx->get_requests(), r_q){
    //     uint q = r_q->get_resource_id();
    //     foreach_request_instance((*ti), (*tx), q, v) {
    //       LinearExpression* exp = new LinearExpression();
    //       uint var_id;
    //       double result;
    //       cout << "X[" << x << "," << q << "," << v << "]:" << endl;
    //       var_id = var.lookup(x, q, v, DPCPMapper::BLOCKING_DIRECT);
    //       exp->add_var(var_id);
    //       result = rb_solution->evaluate(*exp);
    //       exp->sub_var(var_id);
    //       cout << "X_D:" << "[" << var_id << "]" << result << " ";
    //       var_id = var.lookup(x, q, v, DPCPMapper::BLOCKING_INDIRECT);
    //       exp->add_var(var_id);
    //       result = rb_solution->evaluate(*exp);
    //       exp->sub_var(var_id);
    //       cout << "X_I:" << "[" << var_id << "]" << result << " ";
    //       var_id = var.lookup(x, q, v, DPCPMapper::BLOCKING_PREEMPT);
    //       exp->add_var(var_id);
    //       result = rb_solution->evaluate(*exp);
    //       exp->sub_var(var_id);
    //       cout << "X_P:" << "[" << var_id << "]" << result << endl;
    //     }
    //     cout << "=====" << endl;
    //   }
    // }

  }

  ti->set_remote_blocking(remote_blocking);

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  delete rb_solution;
  return remote_blocking;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::total_blocking(Task* ti) {
  ulong total_blocking;
  // cout<<"111"<<endl;
  ulong blocking_l = local_blocking(ti);
  // ulong blocking_l = 0;
  // cout<<"222"<<endl;
  ulong blocking_r = remote_blocking(ti);
  // cout<<"333"<<endl;
  total_blocking = blocking_l + blocking_r;
  // cout << "T" << ti->get_index() << " local_b:" << blocking_l
  //      << " remote_b:" << blocking_r << " total:" << total_blocking << endl;
  ti->set_total_blocking(total_blocking);
  return total_blocking;
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::interference(const Task& ti, ulong interval) {
  uint32_t sum = 0;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    if (resources.get_resource_by_id(q).get_locality() == ti.get_partition())
      sum += request->get_max_length()*request->get_num_requests();
  }
  return (ti.get_wcet_non_critical_sections() + sum) *
         ceiling((interval + ti.get_response_time() -
                  (ti.get_wcet_non_critical_sections() + sum)),
                 ti.get_period());
}

ulong LP_RTA_PFP_ROP_DPCP_PLUS::response_time(Task* ti) {
  ulong test_end = ti->get_deadline();
  ulong test_start = ti->get_total_blocking() + ti->get_wcet();
  ulong response = test_start;
  ulong demand = 0;
  while (response <= test_end) {
    total_blocking(ti);
    // cout << "total blocking:" << ti->get_total_blocking() << endl;
    demand = ti->get_total_blocking() + ti->get_wcet();

    ulong total_interf = 0;

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), task_h) {
      if (ti->get_partition() == task_h->get_partition()) {
        total_interf += interference(*task_h, response);
      }
    }

    // cout << "total interf:" << total_interf << endl;
    demand += total_interf;

    if (response == demand)
      return response + ti->get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

// bool LP_RTA_PFP_ROP_DPCP_PLUS::alloc_schedulable() {
//   bool update = false;

//   do {
//     update = false;
//     foreach(tasks.get_tasks(), task) {
//       // ulong response_bound = task.get_response_time();
//       ulong old_response_time = task->get_response_time();
//       if (task->get_partition() == MAX_LONG) continue;

//       ulong response_bound = response_time(&(*task));

//       if (old_response_time != response_bound) update = true;

//       if (response_bound <= task->get_deadline())
//         task->set_response_time(response_bound);
//       else
//         return false;
//     }
//   } while (update);

//   return true;
// }

ulong LP_RTA_PFP_ROP_DPCP_PLUS::get_max_wait_time(const Task& ti,
                                             const Request& rq) {
  uint priority = ti.get_priority();
  uint p_id = rq.get_locality();
  ulong L_i_q = rq.get_max_length();
  ulong max_wait_time_l = 0;
  ulong max_wait_time_h = 0;
  ulong max_wait_time = 0;

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
    ulong temp = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      ulong request_time = 0;
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

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_objective(const Task& ti,
                                                 LinearProgram* lp,
                                                 DPCPMapper* vars,
                                                 LinearExpression* local_obj,
                                                 LinearExpression* remote_obj) {
  // LinearExpression *obj = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      // bool is_local = false;
      bool is_local = (request->get_locality() == ti.get_partition());
      // bool is_local = (resources.get_resource_by_id(q).get_locality() ==
      //                  ti.get_partition());
      ulong length = request->get_max_length();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_DIRECT);
        // obj->add_term(var_id, length);
        if (is_local && (local_obj != NULL))
          local_obj->add_term(var_id, length);
        else if (!is_local && (remote_obj != NULL))
          remote_obj->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_INDIRECT);
        // obj->add_term(var_id, length);
        if (is_local && (local_obj != NULL))
          local_obj->add_term(var_id, length);
        else if (!is_local && (remote_obj != NULL))
          remote_obj->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_PREEMPT);
        // obj->add_term(var_id, length);
        if (is_local && (local_obj != NULL))
          local_obj->add_term(var_id, length);
        else if (!is_local && (remote_obj != NULL))
          remote_obj->add_term(var_id, length);
      }
    }
  }
  // delete obj;
  vars->seal();
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_add_constraints(const Task& ti,
                                                  LinearProgram* lp,
                                                  DPCPMapper* vars) {
  lp_dpcp_constraint_1(ti, lp, vars);
  lp_dpcp_constraint_2(ti, lp, vars);
  lp_dpcp_constraint_3(ti, lp, vars);
  lp_dpcp_constraint_4(ti, lp, vars);
  lp_dpcp_constraint_5(ti, lp, vars);
  lp_dpcp_constraint_6(ti, lp, vars);
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_constraint_1(const Task& ti,
                                               LinearProgram* lp,
                                               DPCPMapper* vars) {
  // cout<<"Constraint 1"<<endl;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, *tx, q, v) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_INDIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_PREEMPT);
        exp->add_var(var_id);

        lp->add_inequality(exp, 1);  // Xd(x,q,v) + Xi(x,q,v) + Xp(x,q,v) <= 1
      }
    }
  }
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_constraint_2(const Task& ti,
                                               LinearProgram* lp,
                                               DPCPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach_remote_request(ti, tx->get_requests(), request_iter) {
      uint q = request_iter->get_resource_id();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;
        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_PREEMPT);
        exp->add_var(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_constraint_3(const Task& ti,
                                               LinearProgram* lp,
                                               DPCPMapper* vars) {
  uint t_id = ti.get_index();
  uint max_arrival = 1;

  foreach_remote_request(ti, ti.get_requests(), request) {
    max_arrival += request->get_num_requests();
  }

  foreach_lower_priority_local_task(tasks.get_tasks(), ti, tx) {
    LinearExpression* exp = new LinearExpression();
    uint x = tx->get_index();
    foreach_local_request(ti, tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;
        var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_PREEMPT);
        exp->add_var(var_id);
      }
    }
    lp->add_inequality(exp, max_arrival);
  }
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_constraint_4(const Task& ti,
                                               LinearProgram* lp,
                                               DPCPMapper* vars) {
  // cout << "<---C4--->" << endl;
  LinearExpression* exp = new LinearExpression();
  uint priority = ti.get_priority();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    // cout << "task_id:" << tx->get_id() << endl;
    // cout << "task_index:" << x << endl;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      Resource& resource = resources.get_resource_by_id(q);
      // cout << "Taskqueue:";
      // foreach(resource.get_taskqueue(), t_id) {
      //   cout << *t_id << " ";
      // }
      // cout << endl;
      // cout << resource.get_ceiling() << " " << priority <<endl;
      if (resource.get_ceiling() > priority) {
        foreach_request_instance(ti, *tx, q, v) {
          uint var_id;

          var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_DIRECT);
          exp->add_var(var_id);
          // cout << "X_D:" << "[" << var_id << "]" << "+";

          var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_INDIRECT);
          exp->add_var(var_id);
          // cout << "X_I:" << "[" << var_id << "]" << "=" << 0 << endl;
        }
      }
    }
  }
  lp->add_equality(exp, 0);
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_constraint_5(const Task& ti,
                                               LinearProgram* lp,
                                               DPCPMapper* vars) {
  uint priority = ti.get_priority();

  foreach(processors.get_processors(), processor) {
    LinearExpression* exp = new LinearExpression();

    uint p_id = processor->get_processor_id();
    uint cluster_request = 0;
    foreach(ti.get_requests(), request) {
      if (request->get_locality() == p_id) {
        cluster_request += request->get_num_requests();
      }
    }

    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_index();
      foreach(tx->get_requests(), request) {
        uint q = request->get_resource_id();
        Resource& resource = resources.get_resource_by_id(q);
        // test
        if ((resource.get_ceiling() <= priority) &&
            (resource.get_locality() == p_id)) {
          foreach_request_instance(ti, *tx, q, v) {
            uint var_id;

            var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_DIRECT);
            exp->add_var(var_id);

            var_id = vars->lookup(x, q, v, DPCPMapper::BLOCKING_INDIRECT);
            exp->add_var(var_id);
          }
        }
      }
    }

    lp->add_inequality(exp, cluster_request);
  }
}

void LP_RTA_PFP_ROP_DPCP_PLUS::lp_dpcp_constraint_6(const Task& ti,
                                               LinearProgram* lp,
                                               DPCPMapper* vars) {
  ulong max_wait_time_l = 0;
  ulong max_wait_time_h = 0;
  ulong max_wait_time = 0;

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach(tx->get_requests(), request_y) {
      LinearExpression* exp = new LinearExpression();
      uint y = request_y->get_resource_id();
      ulong max_request_num = 0;

      foreach(ti.get_requests(), request_q) {
        if (request_q->get_locality() == request_y->get_locality()) {
          uint N_i_q = request_q->get_num_requests();
          ulong mwt = get_max_wait_time(ti, *request_q);
          ulong D = ceiling(tx->get_response_time() + mwt, tx->get_period()) *
                    request_y->get_num_requests();
          max_request_num += D * N_i_q;
        }
      }

      foreach_request_instance(ti, *tx, y, v) {
        uint var_id;

        var_id = vars->lookup(x, y, v, DPCPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, y, v, DPCPMapper::BLOCKING_INDIRECT);
        exp->add_var(var_id);
      }
      lp->add_inequality(exp, max_request_num);
    }
  }
}

// Others
// bool LP_RTA_PFP_ROP_DPCP_PLUS::alloc_schedulable() {
//   ulong response_bound = 0;
//   foreach(tasks.get_tasks(), ti) {
//     uint p_id = ti->get_partition();
//     if (MAX_INT == p_id) continue;

//     Processor& processor = processors.get_processors()[p_id];
//     if (0 == processor.get_resourcequeue().size()) {
//       response_bound = response_time_AP((*ti));
//     } else {  // Synchronization Processor
//       response_bound = response_time_SP((*ti));
//     }
//     if (response_bound <= ti->get_deadline()) {
//       ti->set_response_time(response_bound);
//     } else {
//       return false;
//     }
//   }
//   return true;
// }

bool LP_RTA_PFP_ROP_DPCP_PLUS::alloc_schedulable() {
  bool update = false;

  do {
    update = false;
    foreach(tasks.get_tasks(), task) {
      // ulong response_bound = task.get_response_time();
      ulong old_response_time = task->get_response_time();
      if (task->get_partition() == MAX_INT) continue;

      ulong response_bound = response_time(&(*task));

      if (old_response_time != response_bound) update = true;

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else
        return false;
    }
  } while (update);

  return true;
}

bool LP_RTA_PFP_ROP_DPCP_PLUS::alloc_schedulable(Task* ti) {
  uint p_id = ti->get_partition();
  if (MAX_INT == p_id) return false;

  Processor& processor = processors.get_processors()[p_id];
  ulong response_bound = 0;
  if (0 == processor.get_resourcequeue().size()) {  // Application Processor
    response_bound = response_time_AP(*ti);
    ti->set_response_time(response_bound);
  } else {  // Synchronization Processor
    response_bound = response_time_SP(*ti);
    ti->set_response_time(response_bound);
  }

  if (response_bound <= ti->get_deadline()) {
    return true;
  } else {
    return false;
  }
}

bool LP_RTA_PFP_ROP_DPCP_PLUS::is_schedulable() {
  bool schedulable = false;
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  for (uint i = 1; i <= p_num; i++) {
  // for (uint i = 1; i <= 2; i++) {
    // initialzation
    tasks.init();
    processors.init();
    resources.init();

    if (!worst_fit_for_resources(i)) continue;

    tasks.update_requests(resources);

    // foreach(resources.get_resources(), resource) {
    //   cout << "Resource:" << resource->get_resource_id()
    //        << " locality:" << resource->get_locality()
    //        << " Utilization:" << resource->get_utilization() << endl;
    //   if (p_num < resource->get_locality()) exit(0);
    // }
// cout << "I:" << i << endl;

    schedulable = is_first_fit_for_tasks_schedulable_2(i % p_num);
    if (schedulable) {
      // cout << "<=====R-ROP=====>" << endl;
      // cout << "<=====LP-DPCP-R=====>" << endl;
      // if (schedulable) 
      //   cout << "schedulable" << endl;
      // else
      //   cout << "unschedulable" << endl;
        
      // foreach(resources.get_resources(), resource) {
      //   cout << "Resource:" << resource->get_resource_id()
      //        << " locality:" << resource->get_locality()
      //        << " priority ceiling:" << resource->get_ceiling()
      //        << " Utilization:" << resource->get_utilization() << endl;
      //   if (p_num < resource->get_locality()) exit(0);
      //   cout << "Taskqueue:";
      //   foreach(resource->get_taskqueue(), t_id) {
      //     cout << *t_id << " ";
      //   }
      //   cout << endl;
      // }
      // foreach(tasks.get_tasks(), task) {
      //   cout << "Task" << task->get_id()
      //        << ": partition:" << task->get_partition()
      //        << " priority:" << task->get_priority() << endl;
      //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
      //        << " cs-wcet:" << task->get_wcet_critical_sections()
      //        << " wcet:" << task->get_wcet()
      //        << " response time:" << task->get_response_time()
      //        << " deadline:" << task->get_deadline()
      //        << " period:" << task->get_period() << endl;
      //   cout << "Total blocking:" << total_blocking(&(*task)) << endl;
      //   foreach(task->get_requests(), request) {
      //     cout << "request" << request->get_resource_id() << ":"
      //          << " num:" << request->get_num_requests()
      //          << " length:" << request->get_max_length() << " locality:"
      //          << resources.get_resource_by_id(request->get_resource_id())
      //                 .get_locality()
      //          << endl;
      //   }
      //   cout << "-------------------------------------------" << endl;
      //   if (task->get_wcet() > task->get_response_time()) exit(0);
      // }

      // bool lp_schedulable = is_first_fit_for_tasks_schedulable_2(i % p_num);
      // bool lp_schedulable = alloc_schedulable();
      // cout << "<=====LP-DPCP-R=====>" << endl;
      // if (lp_schedulable) 
      //   cout << "schedulable" << endl;
      // else
      //   cout << "unschedulable" << endl;

      // foreach(resources.get_resources(), resource) {
      //   cout << "Resource:" << resource->get_resource_id()
      //        << " locality:" << resource->get_locality()
      //        << " Utilization:" << resource->get_utilization() << endl;
      //   if (p_num < resource->get_locality()) exit(0);
      // }
      // foreach(tasks.get_tasks(), task) {
      //   cout << "Task" << task->get_id()
      //        << ": partition:" << task->get_partition()
      //        << " priority:" << task->get_priority() << endl;
      //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
      //        << " cs-wcet:" << task->get_wcet_critical_sections()
      //        << " wcet:" << task->get_wcet()
      //        << " response time:" << task->get_response_time()
      //        << " deadline:" << task->get_deadline()
      //        << " period:" << task->get_period() << endl;
      //   cout << "Total blocking:" << total_blocking(&(*task)) << endl;
      //   foreach(task->get_requests(), request) {
      //     cout << "request" << request->get_resource_id() << ":"
      //          << " num:" << request->get_num_requests()
      //          << " length:" << request->get_max_length() << " locality:"
      //          << resources.get_resource_by_id(request->get_resource_id())
      //                 .get_locality()
      //          << endl;
      //   }
      //   cout << "-------------------------------------------" << endl;
      //   if (task->get_wcet() > task->get_response_time()) exit(0);
      // }
      // if (schedulable && !lp_schedulable) exit(0);
      return schedulable;
    }
  }

  // cout << "<=====LP-DPCP-R=====>" << endl;
  //     if (schedulable) 
  //       cout << "schedulable" << endl;
  //     else
  //       cout << "unschedulable" << endl;
        
  //     foreach(resources.get_resources(), resource) {
  //       cout << "Resource:" << resource->get_resource_id()
  //            << " locality:" << resource->get_locality()
  //            << " priority ceiling:" << resource->get_ceiling()
  //            << " Utilization:" << resource->get_utilization() << endl;
  //       if (p_num < resource->get_locality()) exit(0);
  //       cout << "Taskqueue:";
  //       foreach(resource->get_taskqueue(), t_id) {
  //         cout << *t_id << " ";
  //       }
  //       cout << endl;
  //     }
  //     foreach(tasks.get_tasks(), task) {
  //       cout << "Task" << task->get_id()
  //            << ": partition:" << task->get_partition()
  //            << " priority:" << task->get_priority() << endl;
  //       cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
  //            << " cs-wcet:" << task->get_wcet_critical_sections()
  //            << " wcet:" << task->get_wcet()
  //            << " response time:" << task->get_response_time()
  //            << " deadline:" << task->get_deadline()
  //            << " period:" << task->get_period() << endl;
  //       cout << "Total blocking:" << total_blocking(&(*task)) << endl;
  //       foreach(task->get_requests(), request) {
  //         cout << "request" << request->get_resource_id() << ":"
  //              << " num:" << request->get_num_requests()
  //              << " length:" << request->get_max_length() << " locality:"
  //              << resources.get_resource_by_id(request->get_resource_id())
  //                     .get_locality()
  //              << endl;
  //       }
  //       cout << "-------------------------------------------" << endl;
  //       if (task->get_wcet() > task->get_response_time()) exit(0);
  //     }
  // foreach(tasks.get_tasks(), task) {
  //   cout << "Task" << task->get_id()
  //         << ": partition:" << task->get_partition() << endl;
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
  //           << resources.get_resource_by_id(request->get_resource_id())
  //                 .get_locality()
  //           << endl;
  //   }
  //   cout << "-------------------------------------------" << endl;
  //   if (task->get_wcet() > task->get_response_time()) exit(0);
  // }
  /*
          foreach(tasks.get_tasks(), task)
          {
                  cout<<"Task"<<task->get_id()<<":
     partition:"<<task->get_partition()<<endl;
                  cout<<"ncs-wcet:"<<task->get_wcet_non_critical_sections()<<"
     cs-wcet:"<<task->get_wcet_critical_sections()<<"
     wcet:"<<task->get_wcet()<<" response time:"<<task->get_response_time()<<"
     deadline:"<<task->get_deadline()<<" period:"<<task->get_period()<<endl;
                  foreach(task->get_requests(), request)
                  {
                          cout<<"request"<<request->get_resource_id()<<":"<<"
     num:"<<request->get_num_requests()<<"
     length:"<<request->get_max_length()<<"
     locality:"<<resources.get_resource_by_id(request->get_resource_id()).get_locality()<<endl;
                  }
                  cout<<"-------------------------------------------"<<endl;
          }
  */
  return schedulable;
}
