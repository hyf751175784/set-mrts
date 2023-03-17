// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <ilp_rta_fed_msrp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>

using std::max;
using std::min;
using std::ostringstream;
using std::ofstream;

/** Class MSRPMapper */


uint64_t MSRPMapper::encode_request(uint64_t task_id, uint64_t res_id,
                                    uint64_t req_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(task_id < (one << 10));
  assert(res_id < (one << 10));
  assert(req_id < (one << 16));
  assert(type < (one << 4));

  key |= (type << 36);
  key |= (task_id << 26);
  key |= (res_id << 16);
  key |= req_id;
  return key;
}


uint64_t MSRPMapper::get_type(uint64_t var) {
  return (var >> 36) & (uint64_t)0xf;  // 4 bits
}

uint64_t MSRPMapper::get_task(uint64_t var) {
  return (var >> 26) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t MSRPMapper::get_res_id(uint64_t var) {
  return (var >> 16) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t MSRPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0xffff;  // 16 bits
}

MSRPMapper::MSRPMapper(uint start_var) : VarMapperBase(start_var) {}

uint64_t MSRPMapper::lookup(uint64_t task_id, uint64_t res_id, uint64_t req_id, var_type type) {
  uint64_t key = encode_request(task_id, res_id, req_id, type);
  uint64_t var = var_for_key(key);
  return var;
}

string MSRPMapper::key2str(uint64_t key, uint64_t var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case MSRPMapper::REQUEST_NUM:
      buf << "Y[";
      break;
    case MSRPMapper::BLOCKING_SPIN:
      buf << "bs[";
      break;
    case MSRPMapper::BLOCKING_DIRECT:
      buf << "bd[";
      break;
    case MSRPMapper::BLOCKING_INDIRECT:
      buf << "bi[";
      break;
    case MSRPMapper::BLOCKING_PREEMPT:
      buf << "bp[";
      break;
    case MSRPMapper::BLOCKING_TRANS_DIRECT:
      buf << "btd[";
      break;
    case MSRPMapper::BLOCKING_TRANS_INDIRECT:
      buf << "bti[";
      break;
    case MSRPMapper::BLOCKING_TRANS_PREEMPT:
      buf << "btp[";
      break;
    case MSRPMapper::BLOCKING_TRANS_REGULAR:
      buf << "btr[";
      break;
    case MSRPMapper::INTERF:
      buf << "I[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class ILP_RTA_FED_MSRP */

ILP_RTA_FED_MSRP::ILP_RTA_FED_MSRP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

ILP_RTA_FED_MSRP::ILP_RTA_FED_MSRP(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

ILP_RTA_FED_MSRP::~ILP_RTA_FED_MSRP() {}

bool ILP_RTA_FED_MSRP::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_MSRP::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  p_num_lu = p_num;
  uint64_t b_sum = 0;
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
    ti->set_other_attr(1);
  }
  bool update = false;
  uint32_t sum_core = 0;
  do {
    update = false;
    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t temp = get_response_time((*ti));
      // get_response_time_HUT_2((*ti));
      if (D_i < temp) {
        ti->set_dcores(1 + ti->get_dcores());
        update = true;
      } else {
        ti->set_response_time(temp);
      }
    }
    sum_core = 0;
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() > p_num)
        return false;
      sum_core += ti->get_dcores();
    }

    uint32_t sum_mcore = 0;
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() == 1)
        continue;
      sum_mcore += ti->get_dcores();
    }
    if (sum_mcore > p_num) {
      return false;
    }
  } while (update);

  if (sum_core <= p_num) {
    return true;
  }

  // 

  foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() != 1)
        continue;
      ti->set_other_attr(0);
      sum_core--;
  }

  if (sum_core >= p_num) {
    return false;
  }

  p_num_lu = p_num - sum_core;
  if (0 == p_num_lu) {
    return false;
  }

  // worst-fit partitioned scheduling for LUT
  ProcessorSet proc_lu = ProcessorSet(p_num_lu);
  proc_lu.update(&tasks, &resources);
  proc_lu.init();
  foreach(tasks.get_tasks(), ti) {
    if (ti->get_other_attr() == 1)
        continue;
    proc_lu.sort_by_task_utilization(INCREASE);
    Processor& processor = proc_lu.get_processors()[0];
    if (processor.get_utilization() + ti->get_utilization() <= 1) {
      ti->set_partition(processor.get_processor_id());
      if (!processor.add_task(ti->get_id())) {
        return false;
      }
    } else {
      return false;
    }
  }

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_other_attr() == 1)
        continue;
    ti->set_response_time(ti->get_critical_path_length());
    uint64_t D_i = ti->get_deadline();
    uint64_t old_response_time;
    uint64_t response_time;
    do {
      update = false;
      old_response_time = ti->get_response_time();
      response_time = get_response_time((*ti));
      if (old_response_time != response_time) update = true;
      if (response_time <= D_i) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    } while (update);
  }
  return true;
}

uint64_t ILP_RTA_FED_MSRP::blocking_spin(const DAG_Task& ti,
                                                         uint32_t res_id) {
  uint64_t sum = 0;
  foreach(tasks.get_tasks(), th) {
    if (th->get_other_attr() != 1)
      continue;
    if (th->get_id() != ti.get_id()) {
      if (th->is_request_exist(res_id)) {
        const Request &r_h_q = th->get_request_by_id(res_id);
        sum += r_h_q.get_max_length();
      }
    }
  }

  uint p_i = ti.get_partition();
  for (uint k = 0; k < p_num_lu; k++) {
    uint64_t L_max = 0;
    if (k == p_i)
      continue;
    foreach_task_except(tasks.get_tasks(), ti, tl) {
      if (tl->get_other_attr() == 1)
        continue;
      uint p_l = tl->get_partition();
      if (p_l == k || p_l == MAX_INT) {
        if (tl->is_request_exist(res_id)) {
          const Request &r_l_q = tl->get_request_by_id(res_id);
          uint64_t L_l_q = r_l_q.get_max_length();
          if (L_max < L_l_q)
            L_max = L_l_q;
        }
      }
    }
    sum += L_max;
  }
  return sum;
}

uint64_t ILP_RTA_FED_MSRP::blocking_spin_total(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint r_id = r_i_q->get_resource_id();
    Resource& resource = resources.get_resource_by_id(r_id);
    if (0 == ti.get_other_attr())
      if (!resource.is_global_resource()) {
        continue;
      }
    uint32_t N_i_q = r_i_q->get_num_requests();
    uint64_t b_spin = blocking_spin(ti, r_id);
    sum += N_i_q * b_spin;
  }
  return sum;
}


uint64_t ILP_RTA_FED_MSRP::NCS(const DAG_Task& ti, uint32_t res_id) {
  uint32_t sum = 0;
  if (1 == ti.get_other_attr()) {
    if (ti.is_request_exist(res_id))
      return ti.get_request_by_id(res_id).get_num_requests();
    else
      return 0;
  } else {
    if (ti.is_request_exist(res_id))
      sum += ti.get_request_by_id(res_id).get_num_requests();
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      if (ti.get_partition() != tx->get_partition())
        continue;
      if (tx->is_request_exist(res_id))
        sum += tx->get_request_by_id(res_id).get_num_requests();
    }
    return sum;
  }
}


uint64_t ILP_RTA_FED_MSRP::workload_bound(const DAG_Task& ti, const DAG_Task& tx) {
  uint64_t R_i = ti.get_response_time();
  uint64_t T_x = tx.get_period();
  uint64_t R_x = tx.get_response_time();
  uint64_t C_x = tx.get_wcet();
  uint64_t L_x = tx.get_critical_path_length();
  uint64_t L = R_i + R_x - C_x;
  if (ti.get_id() == tx.get_id()) {
    return C_x - L_x;
  } else {
    if (0 == tx.get_other_attr()) {  // light task
      return (L / T_x) * C_x + min(C_x, L % T_x);
    } else {  // heavy task
      return ceiling(L, T_x) * C_x;
    }
  }
}

uint64_t ILP_RTA_FED_MSRP::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  uint64_t B_spin_total = blocking_spin_total(ti);
  uint32_t n_i = ti.get_dcores();
  uint64_t response_time;

  MSRPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  objective(ti, &vars, obj);
  response_bound.set_objective(obj);
  declare_variable_bounds(ti, &response_bound, &vars);
  vars.seal();
  lp_h2lp_add_constraints(ti, &response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  if (rb_solution->is_solved()) {
    double result;

    result = rb_solution->evaluate(*(response_bound.get_objective()));
    response_time = result +L_i;
  } else {
    cout << "unsolved." << endl;
    delete rb_solution;
    return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

delete rb_solution;
return response_time;
}

void ILP_RTA_FED_MSRP::objective(const DAG_Task& ti, MSRPMapper* vars, LinearExpression* obj) {
  int64_t var_id;
  double coef = 1.0 / ti.get_dcores();
  spining_blocking(ti, vars, obj);
  direct_blocking(ti, vars, obj);
  indirect_blocking(ti, vars, obj);

  preemption_blocking(ti, vars, obj, coef);
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    var_id = vars->lookup(x, 0, 0, MSRPMapper::INTERF);
    obj->add_term(var_id, coef);

    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_REGULAR);
        obj->add_term(var_id, coef * length);
      }
    }
  }
}

void ILP_RTA_FED_MSRP::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      MSRPMapper* vars) {
  int64_t var_id;
  foreach(tasks.get_tasks(), tx) {
    var_id = vars->lookup(tx->get_id(), 0, 0, MSRPMapper::INTERF);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);
  }

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(ti.get_id(), request->get_resource_id(), 0, MSRPMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    if (0 == ti.get_other_attr())
      lp->declare_variable_bounds(var_id, true, request->get_num_requests(), true, request->get_num_requests());
    else
      lp->declare_variable_bounds(var_id, true, 1, true, request->get_num_requests());
  }
}

void ILP_RTA_FED_MSRP::lp_h2lp_add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              MSRPMapper* vars) {
  // cout << "C1" << endl;
  constraint_1(ti, lp, vars);
  // cout << "C2" << endl;
  constraint_2(ti, lp, vars);
  // cout << "C3" << endl;
  constraint_3(ti, lp, vars);
  // cout << "C4" << endl;
  constraint_4(ti, lp, vars);
  // cout << "C5" << endl;
  constraint_5(ti, lp, vars);
  // cout << "C6" << endl;
  constraint_6(ti, lp, vars);
  // cout << "C7" << endl;
  constraint_7(ti, lp, vars);
  // cout << "C8" << endl;
  constraint_8(ti, lp, vars);
  // cout << "C9" << endl;
  constraint_9(ti, lp, vars);
  // cout << "C10" << endl;
  constraint_10(ti, lp, vars);
  // cout << "C11" << endl;
  constraint_11(ti, lp, vars);
  // cout << "C12" << endl;
  constraint_12(ti, lp, vars);
  // cout << "C13" << endl;
  constraint_13(ti, lp, vars);
  // cout << "C14" << endl;
  constraint_14(ti, lp, vars);
  // cout << "C15" << endl;
  constraint_15(ti, lp, vars);
  // cout << "cend" << endl;
}

void ILP_RTA_FED_MSRP::spining_blocking(const DAG_Task& ti,
                        MSRPMapper* vars, LinearExpression* exp,
                        double coef) {
  uint64_t var_id;
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_SPIN);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}

void ILP_RTA_FED_MSRP::direct_blocking(const DAG_Task& ti,
                        MSRPMapper* vars, LinearExpression* exp,
                        double coef) {
  uint64_t var_id;
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, coef * length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}

void ILP_RTA_FED_MSRP::indirect_blocking(const DAG_Task& ti,
                          MSRPMapper* vars, LinearExpression* exp, 
                          double coef) {
  uint64_t var_id;
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, coef * length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}
void ILP_RTA_FED_MSRP::preemption_blocking(const DAG_Task& ti,
                          MSRPMapper* vars,
                          LinearExpression* exp, double coef) {
  uint64_t var_id;
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id, coef * length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_PREEMPT);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}


void ILP_RTA_FED_MSRP::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;

  foreach(tasks.get_tasks(), tx) {
    if ((1 == tx->get_other_attr()) && (tx->get_id() != ti.get_id()))
      continue;
    if ((tx->get_partition() != ti.get_partition()) || (tx->get_priority() > ti.get_priority())) 
      continue;

    uint x = tx->get_id();
    LinearExpression *exp = new LinearExpression();

    var_id = vars->lookup(x, 0, 0, MSRPMapper::INTERF);
    exp->add_term(var_id);

    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_SPIN);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_PREEMPT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_REGULAR);
        exp->add_term(var_id, length);
      }
    }

    lp->add_inequality(exp, workload_bound(ti, (*tx)));
  }
}

void ILP_RTA_FED_MSRP::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        LinearExpression *exp = new LinearExpression();

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_SPIN);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_PREEMPT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_REGULAR);
        exp->add_term(var_id);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void ILP_RTA_FED_MSRP::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;

  foreach_task_except(tasks.get_tasks(), ti, tx) { // for each other cluster for heavy task
    uint x = tx->get_id();
    if (0 == tx->get_other_attr())  // light tasks
      continue;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (!resources.get_resource_by_id(q).is_global_resource())
        continue;
      LinearExpression *exp = new LinearExpression();
      uint request_num;
      request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_SPIN);
        exp->add_term(var_id);
      }
      if (ti.is_request_exist(q)) {
        var_id = vars->lookup(ti.get_id(), q, 0, MSRPMapper::REQUEST_NUM);
        exp->add_term(var_id, -1.0);
      }
      lp->add_inequality(exp, 0);
    }
  }

  for(uint p_id = 0; p_id < p_num_lu; p_id++) { // for each other cluster for light tasks
    if (p_id == ti.get_partition())
      continue;
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();
      if (!resource->is_global_resource())
        continue;
      LinearExpression *exp = new LinearExpression();

      foreach(tasks.get_tasks(), tx) {
        uint x = tx->get_id();
        if (1 == tx->get_other_attr())
          continue;
        if (p_id != tx->get_partition())
          continue;
        if (!tx->is_request_exist(q))
          continue;

        foreach_request_instance(ti, (*tx), q, v) {
          var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_SPIN);
          exp->add_term(var_id);
        }
      }
      if (ti.is_request_exist(q)) {
        var_id = vars->lookup(ti.get_id(), q, 0, MSRPMapper::REQUEST_NUM);
        exp->add_term(var_id, -1.0);
      }

      lp->add_inequality(exp, 0);
    }
  }
}

void ILP_RTA_FED_MSRP::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  uint i = ti.get_id();

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    LinearExpression *exp = new LinearExpression();

    for (uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(i, q, v, MSRPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);
    }

    var_id = vars->lookup(i, q, 0, MSRPMapper::REQUEST_NUM);
    exp->add_term(var_id);

    lp->add_inequality(exp, request->get_num_requests());
  }
}

void ILP_RTA_FED_MSRP::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  if (1 == ti.get_other_attr())
    return;

  LinearExpression *exp = new LinearExpression();

  foreach_lower_priority_local_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (1 == tx->get_other_attr())
      continue;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, (*tx), q, v) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id);
      }
    }
  }

  lp->add_inequality(exp, 1);
}


void ILP_RTA_FED_MSRP::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  foreach_task_except(tasks.get_tasks(), ti, tx) { // for each other cluster for heavy task
    uint x = tx->get_id();
    if (0 == tx->get_other_attr())  // light tasks
      continue;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (!resources.get_resource_by_id(q).is_global_resource())
        continue;
      LinearExpression *exp = new LinearExpression();
      uint request_num;
      request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_TRANS_REGULAR);
        exp->add_term(var_id);
      }
      if (ti.is_request_exist(q)) {
        var_id = vars->lookup(ti.get_id(), q, 0, MSRPMapper::REQUEST_NUM);
        exp->add_term(var_id);
      }
      lp->add_inequality(exp, NCS(ti, q));
    }
  }

  for(uint p_id = 0; p_id < p_num_lu; p_id++) { // for each other cluster for light tasks
    if (p_id == ti.get_partition())
      continue;
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();
      if (!resource->is_global_resource())
        continue;
      LinearExpression *exp = new LinearExpression();
      foreach(tasks.get_tasks(), tx) {
        uint x = tx->get_id();
        if (1 == tx->get_other_attr())
          continue;
        if (p_id != tx->get_partition())
          continue;
        if (!tx->is_request_exist(q))
          continue;
        foreach_request_instance(ti, (*tx), q, v) {
        var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_TRANS_REGULAR);
        exp->add_term(var_id);
        }
      }
      if (ti.is_request_exist(q)) {
        var_id = vars->lookup(ti.get_id(), q, 0, MSRPMapper::REQUEST_NUM);
        exp->add_term(var_id);
      }
      lp->add_inequality(exp, NCS(ti, q));
    }
  }
}

void ILP_RTA_FED_MSRP::constraint_7(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;

  foreach_task_except(tasks.get_tasks(), ti, tx) { // for each other cluster for heavy task
    uint x = tx->get_id();
    if (0 == tx->get_other_attr())  // light tasks
      continue;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (!resources.get_resource_by_id(q).is_global_resource())
        continue;
      LinearExpression *exp = new LinearExpression();
      uint request_num;
      request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_TRANS_PREEMPT);
        exp->add_term(var_id);
      }

      foreach_lower_priority_local_task(tasks.get_tasks(), ti, ty) {
        if (1 == ty->get_other_attr())
          continue;
        uint y = ty->get_id();
        if (!ty->is_request_exist(q))
          continue;
        uint request_num_2;
        request_num_2 = ty->get_max_request_num(q, ti.get_response_time());
        for (uint u = 0; u < request_num_2; u++) {
          var_id = vars->lookup(y,q,u,MSRPMapper::BLOCKING_PREEMPT);
          exp->add_term(var_id, -1.0);
        }
      }
      lp->add_inequality(exp, 0);
    }
  }

  for(uint p_id = 0; p_id < p_num_lu; p_id++) { // for each other cluster for light tasks
    if (p_id == ti.get_partition())
      continue;
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();
      if (!resource->is_global_resource())
        continue;
      LinearExpression *exp = new LinearExpression();
      foreach(tasks.get_tasks(), tx) {
        uint x = tx->get_id();
        if (1 == tx->get_other_attr())
          continue;
        if (p_id != tx->get_partition())
          continue;
        if (!tx->is_request_exist(q))
          continue;
        foreach_request_instance(ti, (*tx), q, v) {
          var_id = vars->lookup(x,q,v,MSRPMapper::BLOCKING_TRANS_PREEMPT);
          exp->add_term(var_id);
        }
      }

      foreach_lower_priority_local_task(tasks.get_tasks(), ti, ty) {
        if (1 == ty->get_other_attr())
          continue;        
        uint y = ty->get_id();
        if (!ty->is_request_exist(q))
          continue;
        uint request_num_2;
        request_num_2 = ty->get_max_request_num(q, ti.get_response_time());
        for (uint u = 0; u < request_num_2; u++) {
          var_id = vars->lookup(y,q,u,MSRPMapper::BLOCKING_PREEMPT);
          exp->add_term(var_id, -1.0);
        }
      }
      lp->add_inequality(exp, 0);
    }
  }
}

void ILP_RTA_FED_MSRP::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if ((1 ==tx->get_other_attr()) || (tx->get_partition() != ti.get_partition()) || (tx->get_priority() > ti.get_priority())) {
      var_id = vars->lookup(x, 0, 0, MSRPMapper::INTERF);
      exp->add_term(var_id);
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_9(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, (*tx), q, v) {
        if (!resources.get_resource_by_id(q).is_global_resource()) {
          var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_SPIN);
          exp->add_term(var_id);

          var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_DIRECT);
          exp->add_term(var_id);

          var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
          exp->add_term(var_id);

          var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_PREEMPT);
          exp->add_term(var_id);

          var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_REGULAR);
          exp->add_term(var_id);
        }
        
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_10(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();
  

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    for (uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(ti.get_id(), q, v, MSRPMapper::BLOCKING_SPIN);
      exp->add_term(var_id);
    }
  }
  if (1 == ti.get_other_attr()) {
    lp->add_equality(exp, 0);
    return;
  }
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (1 == tx->get_other_attr())
      continue;
    if (ti.get_partition() != tx->get_partition())
      continue;

    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, (*tx), q, v) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_SPIN);
        exp->add_term(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_11(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_12(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();

  foreach(tasks.get_tasks(), tx) {
    if (0 == tx->get_other_attr())
      if ((ti.get_partition() == tx->get_partition()) && (ti.get_priority() < tx->get_priority()))
        continue;
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_13(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (1 == tx->get_other_attr() || (ti.get_partition() != tx->get_partition()))
      continue;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (resources.get_resource_by_id(q).get_ceiling() <= ti.get_priority())
        continue;
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id);
      }
    }
  }

  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_14(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();
  

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    for (uint v = 0; v < request->get_num_requests(); v++) {
        var_id = vars->lookup(ti.get_id(), q, v, MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(ti.get_id(), q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(ti.get_id(), q, v, MSRPMapper::BLOCKING_TRANS_PREEMPT);
        exp->add_term(var_id);

        var_id = vars->lookup(ti.get_id(), q, v, MSRPMapper::BLOCKING_TRANS_REGULAR);
        exp->add_term(var_id);
    }
  }
  if (1 == ti.get_other_attr()) {
    lp->add_equality(exp, 0);
    return;
  }
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (1 == tx->get_other_attr())
      continue;
    if (ti.get_partition() != tx->get_partition())
      continue;

    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, (*tx), q, v) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_PREEMPT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_REGULAR);
        exp->add_term(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MSRP::constraint_15(const DAG_Task& ti, LinearProgram* lp,
                          MSRPMapper* vars) {
  int64_t var_id;
  LinearExpression *exp = new LinearExpression();
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, MSRPMapper::BLOCKING_TRANS_INDIRECT);
        exp->add_term(var_id);
      }
    }
  }
  lp->add_equality(exp, 0);
}


///////////////////////////// FF //////////////////////////////
/*
ILP_RTA_FED_MSRP_FF::ILP_RTA_FED_MSRP_FF()
    : ILP_RTA_FED_MSRP() {}

ILP_RTA_FED_MSRP_FF::ILP_RTA_FED_MSRP_FF(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : ILP_RTA_FED_MSRP(tasks, processors, resources) {
  // this->tasks = tasks;
  // this->processors = processors;
  // this->resources = resources;

  // this->resources.update(&(this->tasks));
  // this->processors.update(&(this->tasks), &(this->resources));

  // this->tasks.RM_Order();
  // this->processors.init();
}

ILP_RTA_FED_MSRP_FF::~ILP_RTA_FED_MSRP_FF() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_MSRP_FF::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  p_num_lu = p_num;
  uint64_t b_sum = 0;
// cout << "111" << endl;
  // foreach(tasks.get_tasks(), ti) {
  //   cout << "Task " << ti->get_id() <<  " utilization:" << ti->get_utilization() << endl;
  // }
  // Federated scheduling for HUT
  foreach(tasks.get_tasks(), ti) {
    // if (ti->get_utilization() <= 1)
    //   continue;
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
    ti->set_other_attr(1);
    // cout << "initial:" << initial_m_i << endl;
  }
  bool update = false;
  uint32_t sum_core = 0;
  do {
    update = false;
// cout << "111" << endl;
    foreach(tasks.get_tasks(), ti) {
      // if (ti->get_other_attr() != 1)
      //   continue;
      uint64_t D_i = ti->get_deadline();
      uint64_t temp = get_response_time_HUT((*ti));
      if (D_i < temp) {
        ti->set_dcores(1 + ti->get_dcores());
        update = true;
      } else {
        ti->set_response_time(temp);
      }
    }
// cout << "222" << endl;
    sum_core = 0;
    foreach(tasks.get_tasks(), ti) {
      // if (ti->get_other_attr() != 1)
      //   continue;
      sum_core += ti->get_dcores();
    }

    uint32_t sum_mcore = 0;
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() == 1)
        continue;
      sum_mcore += ti->get_dcores();
    }
    if (sum_mcore > p_num) {
    // foreach(tasks.get_tasks(), ti) {
    //   cout << "Task " << ti->get_id() << ":";
    //   cout << "n_i:" << ti->get_dcores() << endl;
    // }
      return false;
    }

    // if (sum_core > p_num) {
    //   return false;
    // } else {
    //   p_num_lu = p_num - sum_core;
    // }

  } while (update);
// cout << "444" << endl;

  // foreach(tasks.get_tasks(), ti) {
  //   cout << "Task " << ti->get_id() << ":";
  //   cout << "n_i:" << ti->get_dcores() << endl;
  // }

  if (sum_core > p_num) {
// cout << "555" << endl;
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() != 1)
        continue;
      ti->set_other_attr(0);
      sum_core--;
    }
  } else {
// cout << "666" << endl;
    // b_sum = 0;
    // foreach(tasks.get_tasks(), ti) {
    //   b_sum += blocking_spin_total((*ti));
    // }
    // cout << "spin_total" << b_sum << endl;
    // ofstream hybrid("hybrid.log", ofstream::app);
    // hybrid << b_sum/ tasks.get_taskset_size() << "\n";
    // hybrid.flush();
    // hybrid.close();
    return true;
  }

  if (sum_core >= p_num) {
// cout << "777" << endl;
    return false;
  }

  p_num_lu = p_num - sum_core;

  // cout << "p_num_lu:" << p_num_lu << endl;

  // first-fit partitioned scheduling for LUT
  ProcessorSet proc_lu = ProcessorSet(p_num_lu);
  proc_lu.update(&tasks, &resources);
  proc_lu.init();
  foreach(tasks.get_tasks(), ti) {
    // cout << "Task " << ti->get_id() << ":" << endl;
    if (ti->get_other_attr() == 1)
        continue;
    if (0 == p_num_lu)
      return false;

    bool schedulable = false;
    for (uint32_t p_id = 0; p_id < p_num_lu; p_id++) {
      Processor& processor = proc_lu.get_processors()[p_id];

  // cout << "p_id:" << p_id << endl;
      if (processor.get_utilization() + ti->get_utilization() <= 1) {
        ti->set_partition(p_id);
        if (!processor.add_task(ti->get_id())) {
          ti->set_partition(MAX_INT);
          continue;
        }
        uint64_t D_i = ti->get_deadline();
        uint64_t temp = get_response_time_LUT((*ti));
        if (D_i < temp) {
          ti->set_partition(MAX_INT);
          continue;
        } else {
          ti->set_response_time(temp);
          schedulable = true;
  // cout << "Success." << endl;
          break;
        }
      } else {
        ti->set_partition(MAX_INT);
        continue;
      }
    }
    if (!schedulable) {
  // cout << "Failed." << endl;
      return false;
    }
  }
    // b_sum = 0;
    // foreach(tasks.get_tasks(), ti) {
    //   b_sum += blocking_spin_total((*ti));
    // }
    // cout << "spin_total" << b_sum << endl;
    // ofstream hybrid("hybrid.log", ofstream::app);
    // hybrid << b_sum/ tasks.get_taskset_size() << "\n";
    // hybrid.flush();
    // hybrid.close();
  return true;
}

*/
///////////////////////////// HEAVY //////////////////////////////

ILP_RTA_FED_MSRP_HEAVY::ILP_RTA_FED_MSRP_HEAVY()
    : ILP_RTA_FED_MSRP() {}

ILP_RTA_FED_MSRP_HEAVY::ILP_RTA_FED_MSRP_HEAVY(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : ILP_RTA_FED_MSRP(tasks, processors, resources) {
}

ILP_RTA_FED_MSRP_HEAVY::~ILP_RTA_FED_MSRP_HEAVY() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_MSRP_HEAVY::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  p_num_lu = p_num;
  uint64_t b_sum = 0;
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
    ti->set_other_attr(1);
  }
  bool update = false;
  uint32_t sum_core = 0;
  do {
    update = false;
    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t temp = get_response_time((*ti));
      // get_response_time_HUT_2((*ti));
      if (D_i < temp) {
        ti->set_dcores(1 + ti->get_dcores());
        update = true;
      } else {
        ti->set_response_time(temp);
      }
    }
    sum_core = 0;
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() > p_num)
        return false;
      sum_core += ti->get_dcores();
    }

    uint32_t sum_mcore = 0;
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() == 1)
        continue;
      sum_mcore += ti->get_dcores();
    }
    if (sum_mcore > p_num) {
      return false;
    }
  } while (update);

  if (sum_core <= p_num)
    return true;
  else
    return false;
}
