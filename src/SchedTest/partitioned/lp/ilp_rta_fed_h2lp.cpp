// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <ilp_rta_fed_h2lp.h>
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

/** Class H2LPMapper */


uint64_t H2LPMapper::encode_request(uint64_t res_id,
                                    uint64_t req_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(res_id < (one << 10));
  assert(req_id < (one << 10));
  assert(type < (one << 3));

  key |= (type << 20);
  key |= (res_id << 10);
  key |= req_id;
  return key;
}

uint64_t H2LPMapper::get_type(uint64_t var) {
  return (var >> 20) & (uint64_t)0x7;  // 3 bits
}

uint64_t H2LPMapper::get_res_id(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t H2LPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

H2LPMapper::H2LPMapper(uint start_var) : VarMapperBase(start_var) {}

uint H2LPMapper::lookup(uint res_id, uint req_id, var_type type) {
  uint64_t key = encode_request(res_id, req_id, type);
  uint var = var_for_key(key);
  return var;
}

string H2LPMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case H2LPMapper::REQUEST_NUM:
      buf << "Y[";
      break;
    case H2LPMapper::BLOCKING_DIRECT:
      buf << "Xd[";
      break;
    case H2LPMapper::BLOCKING_INDIRECT:
      buf << "Xi[";
      break;
    case H2LPMapper::BLOCKING_PREEMPT:
      buf << "Xp[";
      break;
    case H2LPMapper::BLOCKING_OTHER:
      buf << "Xo[";
      break;
    case H2LPMapper::INTERF_INTRA:
      buf << "I_intra[";
      break;
    case H2LPMapper::INTERF_OTHER:
      buf << "I_o[";
      break;
    default:
      buf << "?[";
  }

  buf << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class ILP_RTA_FED_H2LP */

ILP_RTA_FED_H2LP::ILP_RTA_FED_H2LP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

ILP_RTA_FED_H2LP::ILP_RTA_FED_H2LP(DAG_TaskSet tasks,
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

ILP_RTA_FED_H2LP::~ILP_RTA_FED_H2LP() {}

bool ILP_RTA_FED_H2LP::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_H2LP::is_schedulable() {
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
      uint64_t temp = get_response_time_HUT((*ti));
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

  if (sum_core > p_num) {
    foreach(tasks.get_tasks(), ti) {
      if (ti->get_dcores() != 1)
        continue;
      ti->set_other_attr(0);
      sum_core--;
    }
  } else {
    return true;
  }

  if (sum_core >= p_num) {
    return false;
  }

  p_num_lu = p_num - sum_core;

  // worst-fit partitioned scheduling for LUT
  ProcessorSet proc_lu = ProcessorSet(p_num_lu);
  proc_lu.update(&tasks, &resources);
  proc_lu.init();
  foreach(tasks.get_tasks(), ti) {
    if (ti->get_other_attr() == 1)
        continue;
    if (0 == p_num_lu) {
      return false;
    }
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
    uint64_t D_i = ti->get_deadline();
    uint64_t temp = get_response_time_LUT((*ti));
    if (D_i < temp) {
      return false;
    } else {
      ti->set_response_time(temp);
    }
  }
  return true;
}

uint64_t ILP_RTA_FED_H2LP::blocking_spin(const DAG_Task& ti,
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

uint64_t ILP_RTA_FED_H2LP::blocking_spin_total(const DAG_Task& ti) {
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

uint64_t ILP_RTA_FED_H2LP::blocking_susp(const DAG_Task& ti,
                                                uint32_t res_id, int32_t Y) {
  if (!ti.is_request_exist(res_id)) return 0;
  uint32_t m_i = ti.get_parallel_degree();
  const Request& r_i_q = ti.get_request_by_id(res_id);
  int32_t N_i_q = r_i_q.get_num_requests();
  uint32_t L_i_q = r_i_q.get_max_length();
  uint64_t b_spin = blocking_spin(ti, res_id);
  return max(N_i_q - Y - 1 , 0) * (b_spin + L_i_q);
}

uint64_t ILP_RTA_FED_H2LP::blocking_global(const DAG_Task& ti) {
  uint64_t max = 0;
  uint p_i = ti.get_partition();
  foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
    if (tj->get_other_attr() == 1) continue;
    uint p_j = tj->get_partition();
    if (p_j == p_i || p_j == MAX_INT) {
      foreach(tj->get_requests(), r_j_u) {
        uint id = r_j_u->get_resource_id();
        Resource& resource_u = resources.get_resource_by_id(id);
         if (!resource_u.is_global_resource()) continue;
        uint64_t b_g = r_j_u->get_max_length() + blocking_spin((*tj), id);
        if (max < b_g) max = b_g;
      }
    }
  }
  return max;
}

uint64_t ILP_RTA_FED_H2LP::blocking_local(const DAG_Task& ti) {
  uint64_t max = 0;
  uint p_i = ti.get_partition();
  foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
    if (tj->get_other_attr() == 1) continue;
    uint p_j = tj->get_partition();
    if (p_j == p_i || p_j == MAX_INT) {
      foreach(tj->get_requests(), r_j_u) {
        uint id = r_j_u->get_resource_id();
        Resource& resource_u = resources.get_resource_by_id(id);
        uint ceiling = resource_u.get_ceiling();
        if (resource_u.is_global_resource() || ceiling >= ti.get_priority())
          continue;
        uint64_t L_j_u = r_j_u->get_max_length();
        if (max < L_j_u) max = L_j_u;
      }
    }
  }
  return max;
}

uint64_t ILP_RTA_FED_H2LP::interference(const DAG_Task& ti,
                                               uint64_t interval) {
  uint32_t p_id = ti.get_partition();
  uint64_t interf = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint32_t p_id_h = th->get_partition();
    if (th->get_other_attr() == 1 || p_id_h != p_id) continue;
    uint64_t C_h = th->get_wcet();
    uint64_t T_h = th->get_period();
    uint64_t R_h = th->get_response_time();
    uint64_t B_spin_h = blocking_spin_total((*th));
    uint64_t gamma_h = C_h + B_spin_h;
    interf += ceiling(interval + R_h - gamma_h, T_h) * gamma_h;
  }
  return interf;
}

// uint64_t ILP_RTA_FED_H2LP::get_response_time_HUT(const DAG_Task& ti) {
//   uint64_t C_i = ti.get_wcet();
//   uint64_t L_i = ti.get_critical_path_length();
//   uint64_t D_i = ti.get_deadline();
//   uint64_t B_spin_total = blocking_spin_total(ti);
//   uint32_t n_i = ti.get_dcores();

//   double ratio = n_i - 1;
//   ratio /= n_i;

//   uint64_t response_time = L_i + (C_i + B_spin_total - L_i) / n_i;
//   uint64_t sum = 0;
//   foreach(ti.get_requests(), r_i_q) {
//     uint r_id = r_i_q->get_resource_id();
//     uint64_t b_spin = blocking_spin(ti, r_id);
//     uint64_t max = 0;
//     uint32_t N_i_q = r_i_q->get_num_requests();
//     for (uint y = 1; y <= N_i_q; y++) {
//       uint64_t temp = ratio * y * b_spin + blocking_susp(ti, r_id, y);
//       if (max < temp)
//         max = temp;
//     }

//     sum += max;
//   }
//   response_time += sum;

// return response_time;
// }

uint64_t ILP_RTA_FED_H2LP::get_response_time_HUT(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  uint64_t B_spin_total = blocking_spin_total(ti);
  uint32_t n_i = ti.get_dcores();
  uint64_t response_time;

  double ratio = n_i - 1;
  ratio /= n_i;


  H2LPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  lp_h2lp_objective(ti, &vars, obj);
  response_bound.set_objective(obj);
  lp_h2lp_declare_variable_bounds(ti, &response_bound, &vars);
  vars.seal();
  lp_h2lp_add_constraints(ti, &response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  if (rb_solution->is_solved()) {
    // cout<<"solved."<<endl;
    // assert(ti.get_response_time() >= ti.get_wcet());
    // ulong gap = ti.get_response_time() - ti.get_wcet();
    double result;
    // cout<<"original gap:"<<gap<<endl;

    // response_time = L_i;
    // uint64_t sum = 0;
    // foreach(ti.get_requests(), r_i_q) {
    //   uint r_id = r_i_q->get_resource_id();
    //   uint64_t b_spin = blocking_spin(ti, r_id);
    //   uint64_t max = 0;
    //   uint32_t N_i_q = r_i_q->get_num_requests();
    //   for (uint y = 1; y <= N_i_q; y++) {
    //     uint64_t temp = ratio * y * b_spin + blocking_susp(ti, r_id, y);
    //     if (max < temp)
    //       max = temp;
    //   }
    //   sum += max;
    // }
    // response_time += sum + (C_i + B_spin_total - L_i) / n_i;

    // cout << "Intra_interf: " << (C_i + B_spin_total - L_i) / n_i << endl;
    // cout<<"original response time:"<<response_time<<endl;

    // cout << "C_i - L_i:" << ti.get_wcet() - ti.get_critical_path_length() << endl;

    // cout << "dcores:"<< ti.get_dcores() << endl;

    LinearExpression* exp;

    // foreach(ti.get_requests(), request) {
    //   exp = new LinearExpression();
    //   exp->add_term(vars.lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM));
    //   result = rb_solution->evaluate(*exp);
    //   cout << "Y_" << request->get_resource_id() << ": " << result << endl;
    //   cout << "var_id:" << vars.lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM) << endl;
    //   delete(exp);
    // }

    exp = new LinearExpression();
    exp->add_term(vars.lookup(0, 0, H2LPMapper::INTERF_INTRA));
    result = rb_solution->evaluate(*exp);
    cout << "Intra_interf: " << result << endl;
    // cout << "var_id:" << vars.lookup(0, 0, H2LPMapper::INTERF_INTRA) << endl;
    delete(exp);

    // exp = new LinearExpression();
    // lp_h2lp_directed_blocking(ti, &vars, exp);
    // result = rb_solution->evaluate(*exp);
    // cout << "Direct_blocing: " << result << endl;
    // delete(exp);


    // exp = new LinearExpression();
    // lp_h2lp_indirected_blocking(ti, &vars, exp);
    // result = rb_solution->evaluate(*exp);
    // cout << "Indirect_blocing: " << result << endl;
    // delete(exp);


    // exp = new LinearExpression();
    // lp_h2lp_preemption_blocking(ti, &vars, exp);
    // result = rb_solution->evaluate(*exp);
    // cout << "Preemption_blocing: " << result << endl;
    // delete(exp);

    result = rb_solution->evaluate(*(response_bound.get_objective()));
    // cout << "lp result:"<<result << endl;
    response_time = result +L_i;
    // cout<<"new response time:"<<response_time<<endl;

    // if ((result < gap) && (gap - result < _EPS)) {
    //   response_time = ti.get_response_time();
    //   // cout<<"bingo!!!"<<endl;
    // } else {
    //   response_time = result + ti.get_wcet();
    //   assert(response_time < MAX_LONG);
    // }
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

uint64_t ILP_RTA_FED_H2LP::get_response_time_HUT_2(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  uint64_t B_spin_total = blocking_spin_total(ti);
  uint32_t n_i = ti.get_dcores();
  uint64_t response_time;

  double ratio = n_i - 1;
  ratio /= n_i;


  H2LPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  lp_h2lp_objective(ti, &vars, obj);
  response_bound.set_objective(obj);
  lp_h2lp_declare_variable_bounds_2(ti, &response_bound, &vars);
  vars.seal();
  lp_h2lp_add_constraints(ti, &response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  if (rb_solution->is_solved()) {
    // cout<<"solved."<<endl;
    // assert(ti.get_response_time() >= ti.get_wcet());
    // ulong gap = ti.get_response_time() - ti.get_wcet();
    double result;
    // cout<<"original gap:"<<gap<<endl;

    // response_time = L_i;
    // uint64_t sum = 0;
    // foreach(ti.get_requests(), r_i_q) {
    //   uint r_id = r_i_q->get_resource_id();
    //   uint64_t b_spin = blocking_spin(ti, r_id);
    //   uint64_t max = 0;
    //   uint32_t N_i_q = r_i_q->get_num_requests();
    //   for (uint y = 1; y <= N_i_q; y++) {
    //     uint64_t temp = ratio * y * b_spin + blocking_susp(ti, r_id, y);
    //     if (max < temp)
    //       max = temp;
    //   }
    //   sum += max;
    // }
    // response_time += sum + (C_i + B_spin_total - L_i) / n_i;

    // cout << "Intra_interf: " << (C_i + B_spin_total - L_i) / n_i << endl;
    // cout<<"original response time:"<<response_time<<endl;

    // cout << "C_i - L_i:" << ti.get_wcet() - ti.get_critical_path_length() << endl;

    // cout << "dcores:"<< ti.get_dcores() << endl;

    LinearExpression* exp;

    // foreach(ti.get_requests(), request) {
    //   exp = new LinearExpression();
    //   exp->add_term(vars.lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM));
    //   result = rb_solution->evaluate(*exp);
    //   cout << "Y_" << request->get_resource_id() << ": " << result << endl;
    //   cout << "var_id:" << vars.lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM) << endl;
    //   delete(exp);
    // }

    exp = new LinearExpression();
    exp->add_term(vars.lookup(0, 0, H2LPMapper::INTERF_INTRA));
    result = rb_solution->evaluate(*exp);
    cout << "Intra_interf: " << result << endl;
    // cout << "var_id:" << vars.lookup(0, 0, H2LPMapper::INTERF_INTRA) << endl;
    delete(exp);

    // exp = new LinearExpression();
    // lp_h2lp_directed_blocking(ti, &vars, exp);
    // result = rb_solution->evaluate(*exp);
    // cout << "Direct_blocing: " << result << endl;
    // delete(exp);


    // exp = new LinearExpression();
    // lp_h2lp_indirected_blocking(ti, &vars, exp);
    // result = rb_solution->evaluate(*exp);
    // cout << "Indirect_blocing: " << result << endl;
    // delete(exp);


    // exp = new LinearExpression();
    // lp_h2lp_preemption_blocking(ti, &vars, exp);
    // result = rb_solution->evaluate(*exp);
    // cout << "Preemption_blocing: " << result << endl;
    // delete(exp);

    result = rb_solution->evaluate(*(response_bound.get_objective()));
    cout << "lp result:"<<result << endl;
    response_time = result +L_i;
    cout<<"2 response time:"<<response_time<<endl;

    // if ((result < gap) && (gap - result < _EPS)) {
    //   response_time = ti.get_response_time();
    //   // cout<<"bingo!!!"<<endl;
    // } else {
    //   response_time = result + ti.get_wcet();
    //   assert(response_time < MAX_LONG);
    // }
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

uint64_t ILP_RTA_FED_H2LP::get_response_time_LUT(const DAG_Task& ti) {
  uint32_t p_id = ti.get_partition();
  uint64_t C_i = ti.get_wcet();
  uint64_t D_i = ti.get_deadline();
  uint64_t B_spin_total = blocking_spin_total(ti);
  uint64_t b_g = blocking_global(ti);
  uint64_t b_l = blocking_local(ti);
  uint64_t B_else = max(blocking_global(ti), blocking_local(ti));
  uint64_t test_start = C_i + B_spin_total + B_else;
  uint64_t test_end = ti.get_deadline();
  uint64_t response_time = test_start;

  while (response_time <= test_end) {
    uint64_t demand = test_start + interference(ti, response_time);
    if (demand > response_time)
      response_time = demand;
    else
      break;
  }
  return response_time;
}

/** Expressions */
void ILP_RTA_FED_H2LP::lp_h2lp_directed_blocking(const DAG_Task& ti,
                                                H2LPMapper* vars,
                                                LinearExpression* exp,
                                                double coef) {
  uint32_t var_id;
  uint64_t b_spin = 0;
  uint64_t length = 0;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    b_spin = blocking_spin(ti, q);
    length = request->get_max_length() + b_spin;
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void ILP_RTA_FED_H2LP::lp_h2lp_indirected_blocking(const DAG_Task& ti,
                                                  H2LPMapper* vars,
                                                  LinearExpression* exp,
                                                  double coef) {
  uint32_t var_id;
  uint64_t b_spin = 0;
  uint64_t length = 0;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    b_spin = blocking_spin(ti, q);
    length = request->get_max_length() + b_spin;
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void ILP_RTA_FED_H2LP::lp_h2lp_preemption_blocking(const DAG_Task& ti,
                                                  H2LPMapper* vars,
                                                  LinearExpression* exp,
                                                  double coef) {
  uint64_t b_spin = 0;
  uint64_t length = 0;
  uint var_id;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    b_spin = blocking_spin(ti, q);
    length = request->get_max_length() + b_spin;
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_PREEMPT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void ILP_RTA_FED_H2LP::lp_h2lp_declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       H2LPMapper* vars) {
  uint var_id;

  var_id = vars->lookup(0, 0, H2LPMapper::INTERF_INTRA);
  lp->declare_variable_bounds(var_id, true, 0, false, -1);
  // cout << "Set Intra_interf: l_b:" << 0 << endl;
  // cout << "var_id:" << var_id << endl;

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 1, true, request->get_num_requests());
    // cout << "Set Y_" << request->get_resource_id() << ": l_b:" << 1 << " u_b:" << request->get_num_requests() << endl;
    // cout << "var_id:" << var_id << endl;
  }

  // var_id = vars->lookup(0, 0, H2LPMapper::INTERF_INTER);
  // lp->declare_variable_bounds(var_id, true, 0, false, -1);
}

void ILP_RTA_FED_H2LP::lp_h2lp_declare_variable_bounds_2(const DAG_Task& ti, LinearProgram* lp,
                                       H2LPMapper* vars) {
  uint var_id;

  var_id = vars->lookup(0, 0, H2LPMapper::INTERF_INTRA);
  lp->declare_variable_bounds(var_id, true, 0, false, -1);
  // cout << "Set Intra_interf: l_b:" << 0 << endl;
  // cout << "var_id:" << var_id << endl;

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 1, true, request->get_num_requests());
    // cout << "Set Y_" << request->get_resource_id() << ": l_b:" << 1 << " u_b:" << request->get_num_requests() << endl;
    // cout << "var_id:" << var_id << endl;
  }

  // var_id = vars->lookup(0, 0, H2LPMapper::INTERF_INTER);
  // lp->declare_variable_bounds(var_id, true, 0, false, -1);
}

void ILP_RTA_FED_H2LP::lp_h2lp_objective(const DAG_Task& ti, H2LPMapper* vars, LinearExpression* obj) {
  uint32_t n_i = ti.get_dcores();
  uint64_t b_spin = 0;
  uint32_t var_id;

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM);
    obj->add_term(var_id, blocking_spin(ti, request->get_resource_id()));
  }

  // var_id = vars->lookup(0, 0, H2LPMapper::BLOCKING_DIRECT);
  // obj->add_term(var_id);
  lp_h2lp_directed_blocking(ti, vars, obj);

  // var_id = vars->lookup(0, 0, H2LPMapper::BLOCKING_INDIRECT);
  // obj->add_term(var_id, 1.0 / d_core);
  lp_h2lp_indirected_blocking(ti, vars, obj, 1.0 / n_i);

  // var_id = vars->lookup(0, 0, H2LPMapper::BLOCKING_PREEMPT);
  // obj->add_term(var_id, 1.0 / d_core);
  lp_h2lp_preemption_blocking(ti, vars, obj, 1.0 / n_i);

  var_id = vars->lookup(0, 0, H2LPMapper::INTERF_INTRA);
  obj->add_term(var_id, 1.0 / n_i);

}

void ILP_RTA_FED_H2LP::lp_h2lp_add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              H2LPMapper* vars) {
  lp_h2lp_constraint_1(ti, lp, vars);
  lp_h2lp_constraint_2(ti, lp, vars);
  lp_h2lp_constraint_3(ti, lp, vars);
  // lp_h2lp_constraint_4(ti, lp, vars);
  // lp_h2lp_constraint_5(ti, lp, vars);
  lp_h2lp_constraint_6(ti, lp, vars);
}

void ILP_RTA_FED_H2LP::lp_h2lp_constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper* vars) {  // I^{intra} + B^D + B^I + B^P <= C_i + B^{spin} - L_i - \forall l_q: B^{spin}_{i,q}
  uint var_id;
  uint64_t upper_bound = ti.get_wcet() + blocking_spin_total(ti) - ti.get_critical_path_length() ;
  LinearExpression* exp = new LinearExpression();

  var_id = vars->lookup(0, 0, H2LPMapper::INTERF_INTRA);
  exp->add_term(var_id);

  lp_h2lp_directed_blocking(ti, vars, exp);
  lp_h2lp_indirected_blocking(ti, vars, exp);
  lp_h2lp_preemption_blocking(ti, vars, exp);

  foreach(ti.get_requests(), request) {
    var_id = vars->lookup(request->get_resource_id(), 0, H2LPMapper::REQUEST_NUM);
    exp->add_term(var_id, blocking_spin(ti, request->get_resource_id()));
  }

  lp->add_inequality(exp, upper_bound);
}

void ILP_RTA_FED_H2LP::lp_h2lp_constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper* vars) {  // foreach resource q and request (q,v): X^D + X^I +X^P <= 1
  uint var_id;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    for(uint v = 0; v < request->get_num_requests(); v++) {
      LinearExpression* exp = new LinearExpression();
      
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);

      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id);

      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_PREEMPT);
      exp->add_term(var_id);

      lp->add_inequality(exp, 1);
    }
  }
}

void ILP_RTA_FED_H2LP::lp_h2lp_constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper* vars) {
  uint var_id;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);

      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id);

      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_PREEMPT);
      exp->add_term(var_id);
    }
    var_id = vars->lookup(q, 0, H2LPMapper::REQUEST_NUM);
    exp->add_term(var_id);
    lp->add_inequality(exp, request->get_num_requests());
  }
}

void ILP_RTA_FED_H2LP::lp_h2lp_constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper* vars) {
  uint var_id;
  int32_t tmp;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id);
    }
    var_id = vars->lookup(q, 0, H2LPMapper::REQUEST_NUM);
    tmp = -1 * request->get_num_requests();
    // cout << "tmp:" << tmp << endl;
    exp->add_term(var_id, tmp);
    lp->add_inequality(exp, 0);
  }
  
  // foreach(ti.get_requests(), request) {
  //   uint q = request->get_resource_id();
  //   LinearExpression* exp = new LinearExpression();
  //   for(uint v = 0; v < request->get_num_requests(); v++) {
  //     var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_DIRECT);
  //     exp->add_term(var_id);
  //   }
  //   var_id = vars->lookup(q, 0, H2LPMapper::REQUEST_NUM);
  //   tmp = 1 - ti.get_parallel_degree();
  //   cout << "tmp:" << tmp << endl;
  //   exp->add_term(var_id, tmp);
  //   lp->add_inequality(exp, 0);
  // }
}

void ILP_RTA_FED_H2LP::lp_h2lp_constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper* vars) {
  uint var_id;
  
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id);

      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_PREEMPT);
      exp->add_term(var_id);
    }
    var_id = vars->lookup(q, 0, H2LPMapper::REQUEST_NUM);
    exp->add_term(var_id);
    lp->add_inequality(exp, request->get_num_requests());
  }
}

void ILP_RTA_FED_H2LP::lp_h2lp_constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper* vars) {
  uint var_id;

  LinearExpression* exp = new LinearExpression();
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    for(uint v = 0; v < request->get_num_requests(); v++) {
      var_id = vars->lookup(q, v, H2LPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id);
    }
  }
  lp->add_equality(exp, 0); 
}


///////////////////////////// FF //////////////////////////////

ILP_RTA_FED_H2LP_FF::ILP_RTA_FED_H2LP_FF()
    : ILP_RTA_FED_H2LP() {}

ILP_RTA_FED_H2LP_FF::ILP_RTA_FED_H2LP_FF(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : ILP_RTA_FED_H2LP(tasks, processors, resources) {
  // this->tasks = tasks;
  // this->processors = processors;
  // this->resources = resources;

  // this->resources.update(&(this->tasks));
  // this->processors.update(&(this->tasks), &(this->resources));

  // this->tasks.RM_Order();
  // this->processors.init();
}

ILP_RTA_FED_H2LP_FF::~ILP_RTA_FED_H2LP_FF() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_H2LP_FF::is_schedulable() {
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

///////////////////////////// HEAVY //////////////////////////////

ILP_RTA_FED_H2LP_HEAVY::ILP_RTA_FED_H2LP_HEAVY()
    : ILP_RTA_FED_H2LP() {}

ILP_RTA_FED_H2LP_HEAVY::ILP_RTA_FED_H2LP_HEAVY(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : ILP_RTA_FED_H2LP(tasks, processors, resources) {
}

ILP_RTA_FED_H2LP_HEAVY::~ILP_RTA_FED_H2LP_HEAVY() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_H2LP_HEAVY::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  p_num_lu = p_num;
  uint64_t b_sum = 0;

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
    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t temp = get_response_time_HUT((*ti));
      if (D_i < temp) {
        ti->set_dcores(1 + ti->get_dcores());
        update = true;
      } else {
        ti->set_response_time(temp);
      }
    }
    sum_core = 0;
    foreach(tasks.get_tasks(), ti) {
      sum_core += ti->get_dcores();
    }

    if (sum_core > p_num) {
      return false;
    }
  } while (update);

  return true;
}
