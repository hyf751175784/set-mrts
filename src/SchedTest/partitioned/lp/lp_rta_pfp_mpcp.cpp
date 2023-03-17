// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_pfp_mpcp.h>
#include <math-helper.h>
#include <solution.h>
#include <sstream>

using std::ostringstream;

/** Class MPCPMapper */
uint64_t MPCPMapper::encode_request(uint64_t task_id, uint64_t res_id,
                                    uint64_t req_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(task_id < (one << 10));
  assert(res_id < (one << 10));
  assert(req_id < (one << 10));
  assert(type < (one << 2));

  key |= (type << 30);
  key |= (task_id << 20);
  key |= (res_id << 10);
  key |= req_id;
  return key;
}

uint64_t MPCPMapper::get_type(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3;  // 2 bits
}

uint64_t MPCPMapper::get_task(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t MPCPMapper::get_res_id(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t MPCPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

MPCPMapper::MPCPMapper(uint start_var) : VarMapperBase(start_var) {}

uint MPCPMapper::lookup(uint task_id, uint res_id, uint req_id, var_type type) {
  uint64_t key = encode_request(task_id, res_id, req_id, type);
  uint var = var_for_key(key);
  // cout<<"Key:"<<key<<endl;
  // cout<<"Var:"<<var<<endl;
  return var;
}

string MPCPMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case MPCPMapper::BLOCKING_DIRECT:
      buf << "Xd[";
      break;
    case MPCPMapper::BLOCKING_INDIRECT:
      buf << "Xi[";
      break;
    case MPCPMapper::BLOCKING_PREEMPT:
      buf << "Xp[";
      break;
    case MPCPMapper::BLOCKING_OTHER:
      buf << "Xo[";
      break;
    default:
      buf << "X?[";
  }

  buf << get_task(key) << ", " << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class LP_RTA_PFP_MPCP */
LP_RTA_PFP_MPCP::LP_RTA_PFP_MPCP()
    : PartitionedSched(true, RTA, FIX_PRIORITY, MPCP, "", "MPCP") {}
LP_RTA_PFP_MPCP::LP_RTA_PFP_MPCP(TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : PartitionedSched(true, RTA, FIX_PRIORITY, MPCP, "", "MPCP") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

LP_RTA_PFP_MPCP::~LP_RTA_PFP_MPCP() {}

bool LP_RTA_PFP_MPCP::is_schedulable() {
  foreach(tasks.get_tasks(), ti) {
    if (!BinPacking_WF(&(*ti), &tasks, &processors, &resources, UNTEST))
      return false;
  }
  if (!alloc_schedulable()) return false;
  return true;
}

ulong LP_RTA_PFP_MPCP::local_blocking(Task* ti) {
  ulong local_blocking = 0;
  Resources& r = resources.get_resources();
  const Resource_Requests& rr = ti->get_requests();
  uint p_id = ti->get_partition();      // processor id
  ulong r_i = ti->get_response_time();  // response time of task i(t_id)
  MPCPMapper var;
  LinearProgram local_bound;
  LinearExpression* local_obj = new LinearExpression();
  set_objective(*ti, &local_bound, &var, local_obj, NULL);
  local_bound.set_objective(local_obj);
  // construct constraints
  add_constraints(*ti, &local_bound, &var);

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

ulong LP_RTA_PFP_MPCP::remote_blocking(Task* ti) {
  ulong remote_blocking = 0;
  Resources& r = resources.get_resources();
  const Resource_Requests& rr = ti->get_requests();
  uint p_id = ti->get_partition();      // processor id
  ulong r_i = ti->get_response_time();  // response time of task i(t_id)
  MPCPMapper var;
  LinearProgram remote_bound;
  LinearExpression* remote_obj = new LinearExpression();
  set_objective(*ti, &remote_bound, &var, NULL, remote_obj);
  remote_bound.set_objective(remote_obj);
  // construct constraints
  add_constraints(*ti, &remote_bound, &var);

  GLPKSolution* rb_solution =
      new GLPKSolution(remote_bound, var.get_num_vars());

  if (rb_solution->is_solved()) {
    remote_blocking =
        lrint(rb_solution->evaluate(*(remote_bound.get_objective())));
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

ulong LP_RTA_PFP_MPCP::total_blocking(Task* ti) {
  ulong total_blocking;
  ulong blocking_l = local_blocking(ti);
  ulong blocking_r = remote_blocking(ti);
  total_blocking = blocking_l + blocking_r;
  ti->set_total_blocking(total_blocking);
  return total_blocking;
}

ulong LP_RTA_PFP_MPCP::interference(const Task& ti, ulong interval) {
  return ti.get_wcet() *
         ceiling((interval + ti.get_response_time()), ti.get_period());
}

ulong LP_RTA_PFP_MPCP::response_time(Task* ti) {
  ulong test_end = ti->get_deadline();
  ulong test_start = ti->get_total_blocking() + ti->get_wcet();
  // cout << "gpu response time:" << ti->get_total_blocking() << endl;
  ulong response = test_start;
  ulong demand = 0;
  while (response <= test_end) {
    total_blocking(ti);
    demand = ti->get_total_blocking() + ti->get_wcet();

    ulong total_interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), (*ti), task_h) {
      if (ti->get_partition() == task_h->get_partition()) {
        total_interf += interference(*task_h, response);
      }
    }

    demand += total_interf;

    if (response == demand)
      return response + ti->get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

bool LP_RTA_PFP_MPCP::alloc_schedulable() {
  bool update = false;

  do {
    update = false;
    foreach(tasks.get_tasks(), task) {
      // ulong response_bound = task.get_response_time();
      ulong old_response_time = task->get_response_time();
      if (task->get_partition() == MAX_LONG) continue;

      ulong response_bound = response_time(&(*task));

      if (old_response_time != response_bound) update = true;

      if (response_bound <= task->get_deadline())
        task->set_response_time(response_bound);
      else
        return false;
    }
  } while (update);

  // foreach(tasks.get_tasks(), task) {
  //   cout << "Task " << task->get_id() << endl;
  //   cout << "gpu response time:" << task->get_total_blocking() << endl;
  // }

  return true;
}

uint LP_RTA_PFP_MPCP::priority_ceiling(uint r_id, uint p_id) {
  uint min = MAX_INT;

  foreach(tasks.get_tasks(), tj) {
    if (p_id == tj->get_partition()) continue;

    if (!tj->is_request_exist(r_id)) continue;

    uint j = tj->get_index();

    if (min > j) min = j;
  }
  return min;
}

uint LP_RTA_PFP_MPCP::priority_ceiling(const Task& ti) {
  uint p_id = ti.get_partition();
  uint min = MAX_INT;

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();

    if (!ti.is_request_exist(q)) continue;

    uint temp = priority_ceiling(q, p_id);

    if (min > temp) min = temp;
  }
  return min;
}

uint LP_RTA_PFP_MPCP::DD(const Task& ti, const Task& tx, uint r_id) {
  if (!ti.is_request_exist(r_id)) return 0;

  if (ti.get_index() < tx.get_index())
    return ti.get_request_by_id(r_id).get_num_requests();
  else
    return tx.get_max_request_num(r_id, ti.get_response_time());
}

uint LP_RTA_PFP_MPCP::PO(const Task& ti, const Task& tx) {
  uint sum = 0;
  uint x = tx.get_index();
  uint i = ti.get_index();

  uint p_id = tx.get_partition();

  foreach(tasks.get_tasks(), ty) {
    uint y = ty->get_index();
    if ((p_id != ty->get_partition()) || (y == x) || (y == i)) continue;

    foreach(ty->get_requests(), request) {
      uint v = request->get_resource_id();

      if (priority_ceiling(v, p_id) <= priority_ceiling(tx)) continue;
      sum += (ti, tx, v);
    }
  }
  return sum;
}

uint LP_RTA_PFP_MPCP::PO(const Task& ti, const Task& tx, uint r_id) {
  uint sum = 0;
  uint x = tx.get_index();
  uint i = ti.get_index();

  uint p_id = tx.get_partition();

  foreach(tasks.get_tasks(), ty) {
    uint y = ty->get_index();
    if ((p_id != ty->get_partition()) || (y == x) || (y == i)) continue;

    foreach(ty->get_requests(), request) {
      uint v = request->get_resource_id();

      if (priority_ceiling(v, p_id) <= priority_ceiling(r_id, p_id)) continue;
      sum += (ti, tx, v);
    }
  }
  return sum;
}

ulong LP_RTA_PFP_MPCP::holding_time(const Task& tx, uint r_id) {
  ulong h_time = 0;
  uint p_id = tx.get_partition();
  if (tx.is_request_exist(r_id))
    h_time += tx.get_request_by_id(r_id).get_max_length();

  foreach_task_except(tasks.get_tasks(), tx, ty) {
    if (p_id != ty->get_partition()) continue;

    ulong max = 0;

    foreach(ty->get_requests(), request) {
      uint v = request->get_resource_id();
      if (priority_ceiling(v, p_id) > priority_ceiling(v, r_id))
        continue;
      else if (max < request->get_max_length())
        max = request->get_max_length();
    }

    h_time += max;
  }
  return h_time;
}

ulong LP_RTA_PFP_MPCP::wait_time(const Task& ti, uint r_id) {
  ulong max_h = 0;

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (!tl->is_request_exist(r_id)) continue;
    ulong H_l_q = holding_time(*tl, r_id);
    if (max_h < H_l_q) max_h = H_l_q;
  }

  ulong demand = max_h;
  ulong w_time = max_h;

  while (true) {
    demand = max_h;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (!th->is_request_exist(r_id)) continue;
      ulong H_h_q = holding_time(*th, r_id);
      uint N_h_q = th->get_request_by_id(r_id).get_num_requests();

      demand += ceiling(th->get_response_time() + w_time, th->get_period()) *
                N_h_q * H_h_q;
    }
    
    if (demand > ti.get_deadline())
      return ti.get_deadline() + 100;

    // cout << "demand:" << demand << " w_time:" << w_time << endl;
    // assert(demand >= w_time);

    if (demand > w_time)
      w_time = demand;
    else
      break;
  }
  return w_time;
}

void LP_RTA_PFP_MPCP::set_objective(const Task& ti, LinearProgram* lp,
                                    MPCPMapper* vars,
                                    LinearExpression* local_obj,
                                    LinearExpression* remote_obj) {
  // LinearExpression *obj = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      bool is_local = (request->get_locality() == ti.get_partition());
      ulong length = request->get_max_length();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_DIRECT);
        // obj->add_term(var_id, length);
        if (is_local && (local_obj != NULL))
          local_obj->add_term(var_id, length);
        else if (!is_local && (remote_obj != NULL))
          remote_obj->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_INDIRECT);
        // obj->add_term(var_id, length);
        if (is_local && (local_obj != NULL))
          local_obj->add_term(var_id, length);
        else if (!is_local && (remote_obj != NULL))
          remote_obj->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_PREEMPT);
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

void LP_RTA_PFP_MPCP::add_constraints(const Task& ti, LinearProgram* lp,
                                      MPCPMapper* vars) {
  constraint_1(ti, lp, vars);
  constraint_2(ti, lp, vars);
  constraint_3(ti, lp, vars);
  constraint_4(ti, lp, vars);
  constraint_5(ti, lp, vars);
  constraint_6(ti, lp, vars);
}

// Constraint 15 [BrandenBurg 2013 RTAS Appendix-C]
void LP_RTA_PFP_MPCP::constraint_1(const Task& ti, LinearProgram* lp,
                                   MPCPMapper* vars) {
  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    uint N_i_q = 0;
    if (ti.is_request_exist(q)) {
      N_i_q = ti.get_request_by_id(q).get_num_requests();
    }

    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_index();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);
      }
    }
    lp->add_inequality(exp, N_i_q);
  }
}

// Constraint 16 [BrandenBurg 2013 RTAS Appendix-C]
void LP_RTA_PFP_MPCP::constraint_2(const Task& ti, LinearProgram* lp,
                                   MPCPMapper* vars) {
  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();

    if (ti.is_request_exist(q)) continue;
    LinearExpression* exp = new LinearExpression();
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_index();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);
      }
    }
    lp->add_equality(exp, 0);
  }
}

// Constraint 17 [BrandenBurg 2013 RTAS Appendix-C]
void LP_RTA_PFP_MPCP::constraint_3(const Task& ti, LinearProgram* lp,
                                   MPCPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    LinearExpression* exp = new LinearExpression();
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();

      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_INDIRECT);
        exp->add_var(var_id);
      }
    }
    lp->add_inequality(exp, PO(ti, *tx));
  }
}

// Constraint 18 [BrandenBurg 2013 RTAS Appendix-C]
void LP_RTA_PFP_MPCP::constraint_4(const Task& ti, LinearProgram* lp,
                                   MPCPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();
      LinearExpression* exp = new LinearExpression();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_INDIRECT);
        exp->add_var(var_id);
      }
      lp->add_inequality(exp, PO(ti, *tx, q));
    }
  }
}

// Constraint 19 [BrandenBurg 2013 RTAS Appendix-C]
void LP_RTA_PFP_MPCP::constraint_5(const Task& ti, LinearProgram* lp,
                                   MPCPMapper* vars) {
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_index();
    foreach(ti.get_requests(), request) {
      uint q = request->get_resource_id();
      LinearExpression* exp = new LinearExpression();

      uint N_x_q = 0, N_i_q = request->get_num_requests();

      if (tx->is_request_exist(q))
        N_x_q = tx->get_request_by_id(q).get_num_requests();

      ulong delay_bound = ceiling(tx->get_response_time() + wait_time(ti, q),
                                  tx->get_period()) *
                          N_x_q * N_i_q;

      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);
      }
      lp->add_inequality(exp, delay_bound);
    }
  }
}

// Constraint 20 [BrandenBurg 2013 RTAS Appendix-C]
void LP_RTA_PFP_MPCP::constraint_6(const Task& ti, LinearProgram* lp,
                                   MPCPMapper* vars) {
  uint p_id = ti.get_partition();
  LinearExpression* exp = new LinearExpression();

  ulong total_wait_time = 0;

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    uint N_i_q = 0;
    if (ti.is_request_exist(q))
      N_i_q = ti.get_request_by_id(q).get_num_requests();
    total_wait_time += wait_time(ti, q);
  }

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_index();
    if (p_id == tx->get_partition()) continue;
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();
      ulong L_x_q = 0;
      if (tx->is_request_exist(q))
        L_x_q = tx->get_request_by_id(q).get_max_length();
      else
        continue;
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, L_x_q);

        var_id = vars->lookup(x, q, v, MPCPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, L_x_q);
      }
    }
  }

  lp->add_inequality(exp, total_wait_time);
}
