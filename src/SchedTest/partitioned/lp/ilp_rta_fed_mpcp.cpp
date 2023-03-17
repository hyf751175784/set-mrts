// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <ilp_rta_fed_mpcp.h>
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

/** Class CMPCPMapper */


uint64_t CMPCPMapper::encode_request(uint64_t task_id, uint64_t res_id,
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


uint64_t CMPCPMapper::get_type(uint64_t var) {
  return (var >> 36) & (uint64_t)0xf;  // 4 bits
}

uint64_t CMPCPMapper::get_task(uint64_t var) {
  return (var >> 26) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t CMPCPMapper::get_res_id(uint64_t var) {
  return (var >> 16) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t CMPCPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0xffff;  // 16 bits
}

CMPCPMapper::CMPCPMapper(uint start_var) : VarMapperBase(start_var) {}

uint64_t CMPCPMapper::lookup(uint64_t task_id, uint64_t res_id, uint64_t req_id, var_type type) {
  uint64_t key = encode_request(task_id, res_id, req_id, type);
  uint64_t var = var_for_key(key);
  return var;
}

string CMPCPMapper::key2str(uint64_t key, uint64_t var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case CMPCPMapper::REQUEST_NUM:
      buf << "Y[";
      break;
    case CMPCPMapper::BLOCKING_DIRECT:
      buf << "b_D[";
      break;
    case CMPCPMapper::BLOCKING_INDIRECT:
      buf << "b_I[";
      break;
    case CMPCPMapper::BLOCKING_TOKEN:
      buf << "b_T[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class ILP_RTA_FED_MPCP */

ILP_RTA_FED_MPCP::ILP_RTA_FED_MPCP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

ILP_RTA_FED_MPCP::ILP_RTA_FED_MPCP(DAG_TaskSet tasks,
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

  // foreach(this->tasks.get_tasks(), ti) {
  //   cout << "Task[" << ti->get_id() << "] Priority: " << ti->get_priority() << endl;
  //   foreach(ti->get_requests(), request) {
  //     cout << "  Resource[" << request->get_resource_id() << "] MPCPCeiling: " << MPCPCeiling((*ti), request->get_resource_id()) << endl;
  //   }
  // }



}

ILP_RTA_FED_MPCP::~ILP_RTA_FED_MPCP() {}

bool ILP_RTA_FED_MPCP::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
// bool ILP_RTA_FED_MPCP::is_schedulable() {
//   uint32_t p_num = processors.get_processor_num();
//   p_num_lu = p_num;
//   uint64_t b_sum = 0;
//   foreach(tasks.get_tasks(), ti) {
//     uint64_t C_i = ti->get_wcet();
//     uint64_t L_i = ti->get_critical_path_length();
//     uint64_t D_i = ti->get_deadline();
//     uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
//     ti->set_dcores(initial_m_i);
//     ti->set_other_attr(1);
//   }
//   bool update = false;
//   uint32_t sum_core = 0;
//   do {
//     update = false;
//     foreach(tasks.get_tasks(), ti) {
//       uint64_t D_i = ti->get_deadline();
//       uint64_t temp = get_response_time((*ti));
//       // get_response_time_HUT_2((*ti));
//       if (D_i < temp) {
//         ti->set_dcores(1 + ti->get_dcores());
//         update = true;
//       } else {
//         ti->set_response_time(temp);
//       }
//     }
//     sum_core = 0;
//     foreach(tasks.get_tasks(), ti) {
//       if (ti->get_dcores() > p_num)
//         return false;
//       sum_core += ti->get_dcores();
//     }

//     uint32_t sum_mcore = 0;
//     foreach(tasks.get_tasks(), ti) {
//       if (ti->get_dcores() == 1)
//         continue;
//       sum_mcore += ti->get_dcores();
//     }
//     if (sum_mcore > p_num) {
//       return false;
//     }
//   } while (update);

//   if (sum_core <= p_num) {
//     return true;
//   }

//   // 

//   foreach(tasks.get_tasks(), ti) {
//       if (ti->get_dcores() != 1)
//         continue;
//       ti->set_other_attr(0);
//       sum_core--;
//   }

//   if (sum_core >= p_num) {
//     return false;
//   }

//   p_num_lu = p_num - sum_core;
//   if (0 == p_num_lu) {
//     return false;
//   }

//   // worst-fit partitioned scheduling for LUT
//   ProcessorSet proc_lu = ProcessorSet(p_num_lu);
//   proc_lu.update(&tasks, &resources);
//   proc_lu.init();
//   foreach(tasks.get_tasks(), ti) {
//     if (ti->get_other_attr() == 1)
//         continue;
//     proc_lu.sort_by_task_utilization(INCREASE);
//     Processor& processor = proc_lu.get_processors()[0];
//     if (processor.get_utilization() + ti->get_utilization() <= 1) {
//       ti->set_partition(processor.get_processor_id());
//       if (!processor.add_task(ti->get_id())) {
//         return false;
//       }
//     } else {
//       return false;
//     }
//   }

//   foreach(tasks.get_tasks(), ti) {
//     if (ti->get_other_attr() == 1)
//         continue;
//     ti->set_response_time(ti->get_critical_path_length());
//     uint64_t D_i = ti->get_deadline();
//     uint64_t old_response_time;
//     uint64_t response_time;
//     do {
//       update = false;
//       old_response_time = ti->get_response_time();
//       response_time = get_response_time((*ti));
//       if (old_response_time != response_time) update = true;
//       if (response_time <= D_i) {
//         ti->set_response_time(response_time);
//       } else {
//         return false;
//       }
//     } while (update);
//   }
//   return true;
// }

bool ILP_RTA_FED_MPCP::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  p_num_lu = p_num;
  bool updated = false;
  uint64_t b_sum = 0;
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
  }
  uint32_t sum_core = 0;

  do {
    sum_core = 0;
    updated = false;

    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t temp = get_response_time((*ti));
      if (D_i < temp) {
        ti->set_dcores(1 + ti->get_dcores());
        updated = true;
        // temp = get_response_time((*ti));
        if (ti->get_dcores()>p_num) {
          return false;
        }
      }
      sum_core += ti->get_dcores();
      // cout << "Task[" << ti->get_id() <<"] n_i:" << ti->get_dcores() << endl;
    }
  } while (updated);


  // foreach(tasks.get_tasks(), ti) {
  //   uint64_t D_i = ti->get_deadline();
  //   uint64_t temp = get_response_time((*ti));
  //   while (D_i < temp) {
  //     ti->set_dcores(1 + ti->get_dcores());
  //     temp = get_response_time((*ti));
  //     if (ti->get_dcores()>p_num)
  //       return false;
  //   }
  //   sum_core += ti->get_dcores();
  //   // cout << "Task[" << ti->get_id() <<"] n_i:" << ti->get_dcores() << endl;
  // }

  if (sum_core <= p_num) {
    return true;
  } else {
    return false;
  }
}


uint32_t ILP_RTA_FED_MPCP::MPCPCeiling(const DAG_Task& tx, uint32_t r_id) {
  uint32_t ceiling = 0xffffffff;

  foreach_task_except(tasks.get_tasks(), tx, ty) {
    uint32_t temp = ty->get_priority();
    if (ty->is_request_exist(r_id) && (temp < ceiling)) {
      ceiling = temp;
    }
  }
  return ceiling;
}

uint64_t ILP_RTA_FED_MPCP::resource_holding(const DAG_Task& tx, uint32_t r_id) {
  if (!tx.is_request_exist(r_id))
    return 0;
  uint32_t n_x = tx.get_dcores();

  uint64_t holding_time = tx.get_request_by_id(r_id).get_max_length();

  foreach(tx.get_requests(), request) {
    uint u = request->get_resource_id();
    if (MPCPCeiling(tx, u) <= MPCPCeiling(tx, r_id))
      holding_time = (request->get_num_requests() * request->get_max_length()) / n_x;
  }
  return holding_time;
}

uint64_t ILP_RTA_FED_MPCP::delay_per_request(const DAG_Task& ti, uint32_t r_id) {
  if (!ti.is_request_exist(r_id))
    return 0;

  uint32_t p_num = processors.get_processor_num();
  uint32_t priority = ti.get_priority();
  uint64_t dpr = 0;


  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if(tl->is_request_exist(r_id)) {
      uint64_t rht = resource_holding((*tl), r_id);
      if (dpr < rht)
        dpr = rht;
    }
  }

  uint32_t m_i = ti.get_parallel_degree();
  uint32_t N_i_q = ti.get_request_by_id(r_id).get_num_requests();
  dpr += (min(m_i, N_i_q) - 1) * resource_holding(ti, r_id);

  uint64_t test_start = dpr, test_end = ti.get_deadline();

  while (dpr < test_end) {
    uint64_t temp = test_start;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (!th->is_request_exist(r_id))
        continue;
      
      temp += th->get_max_job_num(dpr) * th->get_request_by_id(r_id).get_num_requests() * resource_holding((*th), r_id);
    }

    if (temp > dpr)
      dpr = temp;
    else {
      return dpr;
    }
  }
  // cout << "DPR: test_end + 100" << endl;
  return test_end + 100;
}

uint64_t ILP_RTA_FED_MPCP::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  uint32_t n_i = ti.get_dcores();
  uint64_t response_time;

  CMPCPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  objective(ti, &vars, obj);
  response_bound.set_objective(obj);
  declare_variable_bounds(ti, &response_bound, &vars);
  vars.seal();
  add_constraints(ti, &response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  if (rb_solution->is_solved()) {
    double result;

    result = rb_solution->evaluate(*(response_bound.get_objective()));
    response_time = result + L_i + (C_i - L_i) / n_i;
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

void ILP_RTA_FED_MPCP::objective(const DAG_Task& ti, CMPCPMapper* vars, LinearExpression* obj) {
  int64_t var_id;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    double coef = 1.0 / tx->get_dcores();
  cout << "coef:" << coef << endl;

    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      int64_t L_x_q = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num_nu(q, ti.get_deadline());

      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
        obj->add_term(var_id, L_x_q);

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_INDIRECT);
        obj->add_term(var_id, coef * L_x_q);
        // obj->add_term(var_id, 0);
      }
    }
  }

  uint i = ti.get_id();
  // int32_t n_i = ti.get_dcores();
  // double coef_2 = -1.0 / n_i;
  double coef_2 = -1.0 / ti.get_dcores();
  // cout << "coef_2:" << coef_2 << endl;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    int64_t L_i_q = request->get_max_length();
    uint request_num = request->get_num_requests();
    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_DIRECT);
      obj->add_term(var_id, coef_2 * L_i_q);

      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_INDIRECT);
      obj->add_term(var_id, coef_2 * L_i_q);
      // obj->add_term(var_id, 0);
    }
  }
}

void ILP_RTA_FED_MPCP::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      CMPCPMapper* vars) {
  int64_t var_id;

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(ti.get_id(), request->get_resource_id(), 0, CMPCPMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, true, request->get_num_requests());
  }
}

void ILP_RTA_FED_MPCP::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              CMPCPMapper* vars) {
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
  // cout << "cend" << endl;
}

void ILP_RTA_FED_MPCP::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_deadline());
      for (uint v = 0; v < request_num; v++) {

        LinearExpression *exp = new LinearExpression();

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void ILP_RTA_FED_MPCP::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  uint i = ti.get_id();
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    uint request_num = request->get_num_requests();

    LinearExpression *exp = new LinearExpression();

    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);
    }

    var_id = vars->lookup(i, q, 0, CMPCPMapper::REQUEST_NUM);
    exp->add_term(var_id);

    lp->add_inequality(exp, request_num);
  }
}

void ILP_RTA_FED_MPCP::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  uint i = ti.get_id();
  int32_t m_i = ti.get_parallel_degree();
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    uint request_num = request->get_num_requests();

    LinearExpression *exp = new LinearExpression();

    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);
    }

    var_id = vars->lookup(i, q, 0, CMPCPMapper::REQUEST_NUM);
    exp->add_term(var_id, 1 - m_i);

    lp->add_inequality(exp, 0);
  }
}

void ILP_RTA_FED_MPCP::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  uint i = ti.get_id();
  LinearExpression *exp = new LinearExpression();
  
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    uint request_num = request->get_num_requests();
  
    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id);
    }
  }

  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_MPCP::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    if (ti.is_request_exist(q)) {
      LinearExpression *exp = new LinearExpression();
      foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(q))
          continue;
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
      }
      var_id = vars->lookup(ti.get_id(), q, 0, CMPCPMapper::REQUEST_NUM);
      exp->add_term(var_id, -1);
      lp->add_inequality(exp, 0);
    } else {
      LinearExpression *exp = new LinearExpression();
      foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(q))
          continue;
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
      }
      lp->add_equality(exp, 0);
    }
  }
}


void ILP_RTA_FED_MPCP::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num = tx->get_max_request_num(q, ti.get_deadline());
      if (ti.is_request_exist(q)) {
        LinearExpression *exp = new LinearExpression();
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
        int32_t term = tx->get_max_job_num(delay_per_request(ti, q)) * request->get_num_requests();
        var_id = vars->lookup(ti.get_id(), q, 0, CMPCPMapper::REQUEST_NUM);
        exp->add_term(var_id, -1.0 * term);
        lp->add_inequality(exp, 0);
      } else {
        LinearExpression *exp = new LinearExpression();
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
        lp->add_equality(exp, 0);
      }
    }
  }
}

void ILP_RTA_FED_MPCP::constraint_7(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach_task_except(tasks.get_tasks(), ti, tx) { // for each other cluster for heavy task
    uint x = tx->get_id();
    uint32_t minimum_ceiling = 0xffffffff;
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (ti.is_request_exist(q) && (MPCPCeiling((*tx),q) > minimum_ceiling)) {
        minimum_ceiling = MPCPCeiling((*tx),q);
      }
    }

    LinearExpression *exp = new LinearExpression();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (MPCPCeiling((*tx),q) <= minimum_ceiling)
        continue;
      uint request_num;
      request_num = tx->get_max_request_num_nu(q, ti.get_deadline());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x,q,v,CMPCPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id);
      }
    }
    lp->add_equality(exp, 0);
  }
}

void ILP_RTA_FED_MPCP::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach_task_except(tasks.get_tasks(), ti, tx) { // for each other cluster for heavy task
    uint x = tx->get_id();
    
    if (tx->get_requests().size() <= tx->get_dcores()) {
      LinearExpression *exp = new LinearExpression();
      foreach(tx->get_requests(), request) {
        uint q = request->get_resource_id();
        uint request_num = tx->get_max_request_num_nu(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x,q,v,CMPCPMapper::BLOCKING_INDIRECT);
          exp->add_term(var_id);
        }
      }
      lp->add_equality(exp, 0);
    }
  }
}



/********************* ILP_RTA_FED_CMPCP *********************/

ILP_RTA_FED_CMPCP::ILP_RTA_FED_CMPCP()
    : ILP_RTA_FED_MPCP() {}

ILP_RTA_FED_CMPCP::ILP_RTA_FED_CMPCP(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : ILP_RTA_FED_MPCP(tasks, processors, resources) {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

ILP_RTA_FED_CMPCP::~ILP_RTA_FED_CMPCP() {}



bool ILP_RTA_FED_CMPCP::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  p_num_lu = p_num;
  bool updated = false;
  uint64_t b_sum = 0;
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
  }
  uint32_t sum_core = 0;

  do {
    sum_core = 0;
    updated = false;

    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t temp = get_response_time((*ti));
      if (D_i < temp) {
        ti->set_dcores(1 + ti->get_dcores());
        updated = true;
        // temp = get_response_time((*ti));
        if (ti->get_dcores()>p_num) {
          return false;
        }
      }
      sum_core += ti->get_dcores();
      // cout << "Task[" << ti->get_id() <<"] n_i:" << ti->get_dcores() << endl;
    }
  } while (updated);

  if (sum_core <= p_num) {
    return true;
  } else {
    return false;
  }
}

uint64_t ILP_RTA_FED_CMPCP::resource_holding(const DAG_Task& tx, uint32_t r_id) {
  if (!tx.is_request_exist(r_id))
    return 0;
  else
    return tx.get_request_by_id(r_id).get_max_length();
}

uint64_t ILP_RTA_FED_CMPCP::delay_per_request(const DAG_Task& ti, uint32_t r_id) {
  if (!ti.is_request_exist(r_id))
    return 0;

  uint32_t p_num = processors.get_processor_num();
  uint32_t priority = ti.get_priority();
  uint64_t dpr = 0;


  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if(tl->is_request_exist(r_id)) {
      uint64_t rht = resource_holding((*tl), r_id);
      if (dpr < rht)
        dpr = rht;
    }
  }

  uint32_t n_i = ti.get_dcores();
  uint32_t N_i_q = ti.get_request_by_id(r_id).get_num_requests();
  dpr += (min(n_i, N_i_q) - 1) * resource_holding(ti, r_id);

  uint64_t test_start = dpr, test_end = ti.get_deadline();

  while (dpr < test_end) {
    uint64_t temp = test_start;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (!th->is_request_exist(r_id))
        continue;
      
      temp += th->get_max_job_num(dpr) * th->get_request_by_id(r_id).get_num_requests() * resource_holding((*th), r_id);
    }

    if (temp > dpr)
      dpr = temp;
    else {
      return dpr;
    }
  }
  // cout << "DPR: test_end + 100" << endl;
  return test_end + 100;
}

uint64_t ILP_RTA_FED_CMPCP::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  uint32_t n_i = ti.get_dcores();
  uint64_t response_time;

  CMPCPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  objective(ti, &vars, obj);
  response_bound.set_objective(obj);
  declare_variable_bounds(ti, &response_bound, &vars);
  vars.seal();
  add_constraints(ti, &response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  if (rb_solution->is_solved()) {
    double result;

    result = rb_solution->evaluate(*(response_bound.get_objective()));
    response_time = result + L_i + (C_i - L_i) / n_i;
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

void ILP_RTA_FED_CMPCP::objective(const DAG_Task& ti, CMPCPMapper* vars, LinearExpression* obj) {
  int64_t var_id;
  double coef;
  double coef_2;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    coef = 1.0 / tx->get_dcores();
    coef_2 = 1.0 / ti.get_dcores();

    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      int64_t L_x_q = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num_nu(q, ti.get_deadline());

      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
        obj->add_term(var_id, L_x_q);

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_INDIRECT);
        obj->add_term(var_id, coef * L_x_q);

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_TOKEN);
        obj->add_term(var_id, coef_2 * L_x_q);
        // obj->add_term(var_id, 0);
      }
    }
  }

  uint i = ti.get_id();
  coef_2 = 1.0 / ti.get_dcores();
  // int32_t n_i = ti.get_dcores();
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    int64_t L_i_q = request->get_max_length();
    uint request_num = request->get_num_requests();
    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_DIRECT);
      obj->add_term(var_id, -1.0 * coef_2 * L_i_q);

      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_INDIRECT);
      obj->add_term(var_id, -1.0 * coef_2 * L_i_q);

      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_TOKEN);
      obj->add_term(var_id, -1.0 * coef_2 * L_i_q);
      // obj->add_term(var_id, 0);
    }
  }
}

void ILP_RTA_FED_CMPCP::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      CMPCPMapper* vars) {
  int64_t var_id;

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(ti.get_id(), request->get_resource_id(), 0, CMPCPMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, true, request->get_num_requests());
  }
}

void ILP_RTA_FED_CMPCP::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              CMPCPMapper* vars) {
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
  // cout << "cend" << endl;
}

void ILP_RTA_FED_CMPCP::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_deadline());
      for (uint v = 0; v < request_num; v++) {

        LinearExpression *exp = new LinearExpression();

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_TOKEN);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void ILP_RTA_FED_CMPCP::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  LinearExpression *exp = new LinearExpression();
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num;
      if (x == ti.get_id())
        request_num = request->get_num_requests();
      else
        request_num = tx->get_max_request_num(q, ti.get_deadline());
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, 1);
      }
    }
  }
  lp->add_equality(exp, 0);
}

void ILP_RTA_FED_CMPCP::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  uint i = ti.get_id();
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    uint request_num = request->get_num_requests();

    LinearExpression *exp = new LinearExpression();

    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);
    }

    var_id = vars->lookup(i, q, 0, CMPCPMapper::REQUEST_NUM);
    exp->add_term(var_id);

    lp->add_inequality(exp, request_num);
  }
}

void ILP_RTA_FED_CMPCP::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  uint i = ti.get_id();
  int32_t n_i = ti.get_dcores();
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    uint request_num = request->get_num_requests();

    LinearExpression *exp = new LinearExpression();

    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(i, q, v, CMPCPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);
    }

    var_id = vars->lookup(i, q, 0, CMPCPMapper::REQUEST_NUM);
    exp->add_term(var_id, 1 - n_i);

    lp->add_inequality(exp, 0);
  }
}

void ILP_RTA_FED_CMPCP::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    if (ti.is_request_exist(q)) {
      LinearExpression *exp = new LinearExpression();
      foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(q))
          continue;
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
      }
      var_id = vars->lookup(ti.get_id(), q, 0, CMPCPMapper::REQUEST_NUM);
      exp->add_term(var_id, -1);
      lp->add_inequality(exp, 0);
    } else {
      LinearExpression *exp = new LinearExpression();
      foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(q))
          continue;
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
      }
      lp->add_equality(exp, 0);
    }
  }
}


void ILP_RTA_FED_CMPCP::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint request_num = tx->get_max_request_num(q, ti.get_deadline());
      if (ti.is_request_exist(q)) {
        LinearExpression *exp = new LinearExpression();
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
        int32_t term = tx->get_max_job_num(delay_per_request(ti, q)) * request->get_num_requests();
        var_id = vars->lookup(ti.get_id(), q, 0, CMPCPMapper::REQUEST_NUM);
        exp->add_term(var_id, -1.0 * term);
        lp->add_inequality(exp, 0);
      } else {
        LinearExpression *exp = new LinearExpression();
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_DIRECT);
          exp->add_term(var_id);
        }
        lp->add_equality(exp, 0);
      }
    }
  }
}

void ILP_RTA_FED_CMPCP::constraint_7(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  int m_i = ti.get_parallel_degree();

  LinearExpression *exp = new LinearExpression();
  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    uint32_t request_num = ti.get_request_by_id(q).get_num_requests();
    for (uint v = 0; v < request_num; v++) {
      var_id = vars->lookup(ti.get_id(), q, v, CMPCPMapper::BLOCKING_TOKEN);
      exp->add_term(var_id);
    }
    
    var_id = vars->lookup(ti.get_id(), q, 0, CMPCPMapper::REQUEST_NUM);
    exp->add_term(var_id, 1 - m_i);
  }
  lp->add_inequality(exp, 0);
}


void ILP_RTA_FED_CMPCP::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    if (ti.is_request_exist(q)) {
      LinearExpression *exp = new LinearExpression();
      foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(q))
          continue;
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_TOKEN);
          exp->add_term(var_id);
        }
      }

      uint N_i_q = ti.get_request_by_id(q).get_num_requests();
      for (uint u = 0; u < N_i_q; u++) {
        var_id = vars->lookup(ti.get_id(), q, u, CMPCPMapper::BLOCKING_TOKEN);
        exp->add_term(var_id, -1);
      }
      lp->add_inequality(exp, 0);
    } else {
      LinearExpression *exp = new LinearExpression();
      foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(q))
          continue;
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_TOKEN);
          exp->add_term(var_id);
        }
      }
      lp->add_equality(exp, 0);
    }
  }
}

void ILP_RTA_FED_CMPCP::constraint_9(const DAG_Task& ti, LinearProgram* lp,
                          CMPCPMapper* vars) {
  int64_t var_id;

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (ti.is_request_exist(q)) {
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());

        LinearExpression *exp = new LinearExpression();
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_TOKEN);
          exp->add_term(var_id);
        }

        uint N_i_q = ti.get_request_by_id(q).get_num_requests();
        int32_t term = tx->get_max_job_num(delay_per_request(ti, q)) * request->get_num_requests();
        for (uint u = 0; u < N_i_q; u++) {
          var_id = vars->lookup(ti.get_id(), q, u, CMPCPMapper::BLOCKING_TOKEN);
          exp->add_term(var_id, -1 * term);
        }
        lp->add_inequality(exp, 0);
      } else {
        uint request_num = tx->get_max_request_num(q, ti.get_deadline());

        LinearExpression *exp = new LinearExpression();
        for (uint v = 0; v < request_num; v++) {
          var_id = vars->lookup(x, q, v, CMPCPMapper::BLOCKING_TOKEN);
          exp->add_term(var_id);
        }

        lp->add_equality(exp, 0);
      }
    }
  }
}