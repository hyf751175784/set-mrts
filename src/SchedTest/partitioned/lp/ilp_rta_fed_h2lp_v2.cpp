// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <ilp_rta_fed_h2lp_v2.h>
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

/** Class H2LPMapper_v2 */


uint64_t H2LPMapper_v2::encode_request(uint64_t part_1, uint64_t part_2,
                                 uint64_t part_3, uint64_t part_4) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(part_1 < (one << 10));
  assert(part_2 < (one << 4));
  assert(part_3 < (one << 16));
  assert(part_4 < (one << 10));

  key |= (part_1 << 30);
  key |= (part_2 << 26);
  key |= (part_3 << 10);
  key |= part_4;
  return key;
}

uint64_t H2LPMapper_v2::get_part_1(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3ff;  // 3 bits
}

uint64_t H2LPMapper_v2::get_part_2(uint64_t var) {
  return (var >> 26) & (uint64_t)0xf;  // 10 bits
}

uint64_t H2LPMapper_v2::get_part_3(uint64_t var) {
  return (var >> 10) & (uint64_t)0xffff;  // 16 bits
}

uint64_t H2LPMapper_v2::get_part_4(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

H2LPMapper_v2::H2LPMapper_v2(uint start_var) : VarMapperBase(start_var) {}

uint H2LPMapper_v2::lookup(uint part_1, uint part_, uint part_3, uint part_4) {
  uint64_t key = encode_request(part_1, part_, part_3, part_4);
  uint var = var_for_key(key);
  return var;
}

string H2LPMapper_v2::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  buf << "b[" 
      << get_part_1(key) << ", " 
      << get_part_2(key) << ", " 
      << get_part_3(key) << ", " 
      << get_part_4(key) << "]";

  return buf.str();
}

/** Class ILP_RTA_FED_H2LP_v2 */

ILP_RTA_FED_H2LP_v2::ILP_RTA_FED_H2LP_v2()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

ILP_RTA_FED_H2LP_v2::ILP_RTA_FED_H2LP_v2(DAG_TaskSet tasks,
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

ILP_RTA_FED_H2LP_v2::~ILP_RTA_FED_H2LP_v2() {}

bool ILP_RTA_FED_H2LP_v2::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool ILP_RTA_FED_H2LP_v2::is_schedulable() {
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

uint64_t ILP_RTA_FED_H2LP_v2::blocking_spin(const DAG_Task& ti,
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

uint64_t ILP_RTA_FED_H2LP_v2::blocking_spin_total(const DAG_Task& ti) {
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

uint64_t ILP_RTA_FED_H2LP_v2::blocking_susp(const DAG_Task& ti,
                                                uint32_t res_id, int32_t Y) {
  if (!ti.is_request_exist(res_id)) return 0;
  uint32_t m_i = ti.get_parallel_degree();
  const Request& r_i_q = ti.get_request_by_id(res_id);
  int32_t N_i_q = r_i_q.get_num_requests();
  uint32_t L_i_q = r_i_q.get_max_length();
  // uint64_t b_spin = blocking_spin(ti, res_id);
  uint64_t b_spin = 0;
  return max(N_i_q - Y - 1 , 0) * (b_spin + L_i_q);
}

uint64_t ILP_RTA_FED_H2LP_v2::blocking_global(const DAG_Task& ti) {
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

uint64_t ILP_RTA_FED_H2LP_v2::blocking_local(const DAG_Task& ti) {
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

uint64_t ILP_RTA_FED_H2LP_v2::interference(const DAG_Task& ti,
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

// uint64_t ILP_RTA_FED_H2LP_v2::get_response_time_HUT(const DAG_Task& ti) {
//   uint64_t C_i = ti.get_wcet();
//   uint64_t L_i = ti.get_critical_path_length();
//   uint64_t D_i = ti.get_deadline();
//   uint64_t B_spin_total = blocking_spin_total(ti);
//   uint32_t n_i = ti.get_dcores();

//   double ratio = n_i - 1;
//   ratio /= n_i;

//   // uint64_t response_time = L_i + (C_i + B_spin_total - L_i) / n_i;
//   uint64_t response_time = L_i;
//   uint64_t sum = 0;
//   uint64_t deum = C_i + B_spin_total - L_i;
//   foreach(ti.get_requests(), r_i_q) {
//     uint r_id = r_i_q->get_resource_id();
//     uint64_t b_spin = blocking_spin(ti, r_id);
//     uint64_t max = 0;
//     uint32_t N_i_q = r_i_q->get_num_requests();
//     // for (uint y = 1; y <= N_i_q; y++) {
//     for (uint y = 1; y <= 1; y++) {
//       uint64_t temp = ratio * y * b_spin + blocking_susp(ti, r_id, y);
//       if (max < temp)
//         max = temp;
//     }

//     sum += max + blocking_spin(ti,r_id);
//     deum -= (N_i_q-1)*(blocking_spin(ti,r_id)+r_i_q->get_max_length()) + blocking_spin(ti,r_id);
//   }
//   cout << "Intra_interf: " << deum << endl;
//   response_time += sum;
//   response_time += deum/n_i;

// return response_time;
// }

uint64_t ILP_RTA_FED_H2LP_v2::get_response_time_HUT(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  uint64_t B_spin_total = blocking_spin_total(ti);
  uint32_t n_i = ti.get_dcores();
  uint64_t response_time;
  uint64_t max;

  double ratio = n_i - 1;
  ratio /= n_i;

  response_time = L_i;
  int64_t sum = 0;
  int64_t deum = C_i - L_i;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    uint require_num = 0;
    uint request_num;
    uint64_t total_b;
    max = 0;
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      if(tx->is_request_exist(q))
        require_num++;
    }
    if (require_num) {
      // for (uint Y = 1; Y <= request->get_num_requests(); Y++) {
      for (uint Y = 1; Y <= 1; Y++) {
        uint64_t b_susp = blocking_susp(ti, q, Y);
        H2LPMapper_v2 vars;
        LinearProgram blocking_bound;
        LinearExpression* obj = new LinearExpression();

        // cout << "obj" << endl;
        objective_heavy(ti, &vars, obj, q, Y);
        blocking_bound.set_objective(obj);
        // cout << "dec" << endl;
        declare_variable_bounds(ti, &blocking_bound, &vars, q);
        vars.seal();
        // cout << "c1" << endl;
        constraint_1(ti, &blocking_bound, &vars, q);
        // cout << "c2" << endl;
        constraint_2(ti, &blocking_bound, &vars, q);
        
        // cout << "end" << endl;

        GLPKSolution* bb_solution =
          new GLPKSolution(blocking_bound, vars.get_num_vars());

        if (bb_solution->is_solved()) {
          double result = bb_solution->evaluate(*obj) + b_susp;
          if (max < result) {
            max = result;
            request_num = Y;
            LinearExpression* exp = new LinearExpression();
            total_blocking(ti, &vars, exp, q);
            total_b = bb_solution->evaluate(*exp);
            delete(exp);
          }
        } else {
          cout << "unsolved." << endl;
          delete bb_solution;
          return MAX_LONG;
        }
        delete bb_solution;
      }
    }
    deum -= blocking_susp(ti, q, request_num);
    // cout << "original bound:" << blocking_spin(ti, q) * request->get_num_requests() << endl;
    // cout << "new bound:" << total_b << endl;
    sum += max;
    // response_time -= blocking_susp(ti, q, request_num)/n_i;
    // cout << "reduced by:" << blocking_susp(ti, q, request_num) << endl;
  }

  response_time += max;
  response_time += deum/n_i;

return response_time;
}

uint64_t ILP_RTA_FED_H2LP_v2::get_response_time_LUT(const DAG_Task& ti) {
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
void ILP_RTA_FED_H2LP_v2::total_blocking(const DAG_Task& ti,
                                                H2LPMapper_v2* vars,
                                                LinearExpression* exp,
                                                int32_t res_id, 
                                                double coef) {
  uint32_t var_id;
  uint64_t length;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    uint q = res_id;
    if (!tx->is_request_exist(q))
      continue;
    length = tx->get_request_by_id(q).get_max_length();
    foreach_request_instance(ti, (*tx), q, u) {
      for (uint v = 0; v < ti.get_request_by_id(q).get_num_requests(); v++) {
        var_id = vars->lookup(x, q, u, v);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}

void ILP_RTA_FED_H2LP_v2::blocking_on_cp(const DAG_Task& ti,
                                         H2LPMapper_v2* vars, LinearExpression* exp, 
                                         int32_t res_id, 
                                         int32_t request_num, double coef) {
  uint32_t var_id;
  uint64_t length;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    uint q = res_id;
    if (!tx->is_request_exist(q))
      continue;
    length = tx->get_request_by_id(q).get_max_length();
    foreach_request_instance(ti, (*tx), q, u) {
      for (uint v = 0; v < request_num; v++) {
        var_id = vars->lookup(x, q, u, v);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}

void ILP_RTA_FED_H2LP_v2::blocking_off_cp(const DAG_Task& ti,
                                         H2LPMapper_v2* vars, LinearExpression* exp, 
                                         int32_t res_id, 
                                         int32_t request_num, double coef) {
  uint32_t var_id;
  uint64_t length;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    uint q = res_id;
    if (!tx->is_request_exist(q))
      continue;
    length = tx->get_request_by_id(q).get_max_length();
    foreach_request_instance(ti, (*tx), q, u) {
      for (uint v = request_num; v < ti.get_request_by_id(q).get_num_requests(); v++) {
        var_id = vars->lookup(x, q, u, v);
        exp->add_term(var_id, coef * length);
      }
    }
  }
}

void ILP_RTA_FED_H2LP_v2::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       H2LPMapper_v2* vars, int32_t res_id) {
  uint32_t var_id;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      foreach_request_instance(ti, (*tx), q, u) {
        for (uint v = 0; v < ti.get_request_by_id(q).get_num_requests(); v++) {
          var_id = vars->lookup(x, q, u, v);
          lp->declare_variable_binary(var_id);
        }
      }
    }
  }
}

void ILP_RTA_FED_H2LP_v2::objective_heavy(const DAG_Task& ti, H2LPMapper_v2* vars, LinearExpression* obj, int32_t res_id, int32_t request_num) {
  // blocking_on_cp(ti, vars, obj, res_id, request_num);
  total_blocking(ti, vars, obj, res_id);
  // blocking_off_cp(ti, vars, obj, res_id, request_num, 1.0/ti.get_dcores());
}

void ILP_RTA_FED_H2LP_v2::objective_light(const DAG_Task& ti, H2LPMapper_v2* vars, LinearExpression* obj, int32_t res_id) {
  total_blocking(ti, vars, obj, res_id);
}

void ILP_RTA_FED_H2LP_v2::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              H2LPMapper_v2* vars, int32_t res_id) {
  constraint_1(ti, lp, vars, res_id);
  constraint_2(ti, lp, vars, res_id);
  // constraint_3(ti, lp, vars);
}

void ILP_RTA_FED_H2LP_v2::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper_v2* vars, int32_t res_id) {  // I^{intra} + B^D + B^I + B^P <= C_i + B^{spin} - L_i - \forall l_q: B^{spin}_{i,q}

  uint32_t var_id;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    uint q = res_id;
    if (!tx->is_request_exist(q))
      continue;
    foreach_request_instance(ti, (*tx), q, u) {
      LinearExpression* exp = new LinearExpression();
      for (uint v = 0; v < ti.get_request_by_id(q).get_num_requests(); v++) {
        var_id = vars->lookup(x, q, u, v);
        exp->add_term(var_id);
      }
      lp->add_inequality(exp, 1);
      // cout << "C1 added." << endl;
    }
  }
}

void ILP_RTA_FED_H2LP_v2::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper_v2* vars, int32_t res_id) {  // foreach resource q and request (q,v): X^D + X^I +X^P <= 1
  uint32_t var_id;

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (tx->get_other_attr() != 1)
      continue;
    uint q = res_id;
    if (!tx->is_request_exist(q))
      continue;
    for (uint v = 0; v < ti.get_request_by_id(q).get_num_requests(); v++) {
      LinearExpression* exp = new LinearExpression();
      foreach_request_instance(ti, (*tx), q, u) {
        var_id = vars->lookup(x, q, u, v);
        exp->add_term(var_id);
      }
      lp->add_inequality(exp, 1);
      // cout << "C2 added." << endl;
    }
  }
}

void ILP_RTA_FED_H2LP_v2::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          H2LPMapper_v2* vars, int32_t res_id, int32_t p_num_lu) {
  uint32_t var_id;

  uint p_i = ti.get_partition();
  for (uint k = 0; k < p_num_lu; k++) {
    if (k == p_i)
      continue;
    uint q = res_id;
    LinearExpression* exp = new LinearExpression();

    foreach_task_except(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_id();
      if (tx->get_other_attr() == 1)
        continue;
      uint p_x = tx->get_partition();
      if (!(p_x == k || p_x == MAX_INT))
        continue;
      if (!tx->is_request_exist(q))
        continue;
      foreach_request_instance(ti, (*tx), q, u) {
        for (uint v = 0; v < ti.get_request_by_id(q).get_num_requests(); v++) {
          var_id = vars->lookup(x, q, u, v);
          exp->add_term(var_id);
        }
      }
    }
    lp->add_inequality(exp, 1);
  }
}


///////////////////////////// FF //////////////////////////////

// ILP_RTA_FED_H2LP_v2_FF::ILP_RTA_FED_H2LP_v2_FF()
//     : ILP_RTA_FED_H2LP_v2() {}

// ILP_RTA_FED_H2LP_v2_FF::ILP_RTA_FED_H2LP_v2_FF(DAG_TaskSet tasks,
//                                          ProcessorSet processors,
//                                          ResourceSet resources)
//     : ILP_RTA_FED_H2LP_v2(tasks, processors, resources) {
//   // this->tasks = tasks;
//   // this->processors = processors;
//   // this->resources = resources;

//   // this->resources.update(&(this->tasks));
//   // this->processors.update(&(this->tasks), &(this->resources));

//   // this->tasks.RM_Order();
//   // this->processors.init();
// }

// ILP_RTA_FED_H2LP_v2_FF::~ILP_RTA_FED_H2LP_v2_FF() {}

// // Algorithm 1 Scheduling Test
// bool ILP_RTA_FED_H2LP_v2_FF::is_schedulable() {
//   uint32_t p_num = processors.get_processor_num();
//   p_num_lu = p_num;
//   uint64_t b_sum = 0;
// // cout << "111" << endl;
//   // foreach(tasks.get_tasks(), ti) {
//   //   cout << "Task " << ti->get_id() <<  " utilization:" << ti->get_utilization() << endl;
//   // }
//   // Federated scheduling for HUT
//   foreach(tasks.get_tasks(), ti) {
//     // if (ti->get_utilization() <= 1)
//     //   continue;
//     uint64_t C_i = ti->get_wcet();
//     uint64_t L_i = ti->get_critical_path_length();
//     uint64_t D_i = ti->get_deadline();
//     uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
//     ti->set_dcores(initial_m_i);
//     ti->set_other_attr(1);
//     // cout << "initial:" << initial_m_i << endl;
//   }
//   bool update = false;
//   uint32_t sum_core = 0;
//   do {
//     update = false;
// // cout << "111" << endl;
//     foreach(tasks.get_tasks(), ti) {
//       // if (ti->get_other_attr() != 1)
//       //   continue;
//       uint64_t D_i = ti->get_deadline();
//       uint64_t temp = get_response_time_HUT((*ti));
//       if (D_i < temp) {
//         ti->set_dcores(1 + ti->get_dcores());
//         update = true;
//       } else {
//         ti->set_response_time(temp);
//       }
//     }
// // cout << "222" << endl;
//     sum_core = 0;
//     foreach(tasks.get_tasks(), ti) {
//       // if (ti->get_other_attr() != 1)
//       //   continue;
//       sum_core += ti->get_dcores();
//     }

//     uint32_t sum_mcore = 0;
//     foreach(tasks.get_tasks(), ti) {
//       if (ti->get_dcores() == 1)
//         continue;
//       sum_mcore += ti->get_dcores();
//     }
//     if (sum_mcore > p_num) {
//     // foreach(tasks.get_tasks(), ti) {
//     //   cout << "Task " << ti->get_id() << ":";
//     //   cout << "n_i:" << ti->get_dcores() << endl;
//     // }
//       return false;
//     }

//     // if (sum_core > p_num) {
//     //   return false;
//     // } else {
//     //   p_num_lu = p_num - sum_core;
//     // }

//   } while (update);
// // cout << "444" << endl;

//   // foreach(tasks.get_tasks(), ti) {
//   //   cout << "Task " << ti->get_id() << ":";
//   //   cout << "n_i:" << ti->get_dcores() << endl;
//   // }

//   if (sum_core > p_num) {
// // cout << "555" << endl;
//     foreach(tasks.get_tasks(), ti) {
//       if (ti->get_dcores() != 1)
//         continue;
//       ti->set_other_attr(0);
//       sum_core--;
//     }
//   } else {
// // cout << "666" << endl;
//     // b_sum = 0;
//     // foreach(tasks.get_tasks(), ti) {
//     //   b_sum += blocking_spin_total((*ti));
//     // }
//     // cout << "spin_total" << b_sum << endl;
//     // ofstream hybrid("hybrid.log", ofstream::app);
//     // hybrid << b_sum/ tasks.get_taskset_size() << "\n";
//     // hybrid.flush();
//     // hybrid.close();
//     return true;
//   }

//   if (sum_core >= p_num) {
// // cout << "777" << endl;
//     return false;
//   }

//   p_num_lu = p_num - sum_core;

//   // cout << "p_num_lu:" << p_num_lu << endl;

//   // first-fit partitioned scheduling for LUT
//   ProcessorSet proc_lu = ProcessorSet(p_num_lu);
//   proc_lu.update(&tasks, &resources);
//   proc_lu.init();
//   foreach(tasks.get_tasks(), ti) {
//     // cout << "Task " << ti->get_id() << ":" << endl;
//     if (ti->get_other_attr() == 1)
//         continue;
//     if (0 == p_num_lu)
//       return false;

//     bool schedulable = false;
//     for (uint32_t p_id = 0; p_id < p_num_lu; p_id++) {
//       Processor& processor = proc_lu.get_processors()[p_id];

//   // cout << "p_id:" << p_id << endl;
//       if (processor.get_utilization() + ti->get_utilization() <= 1) {
//         ti->set_partition(p_id);
//         if (!processor.add_task(ti->get_id())) {
//           ti->set_partition(MAX_INT);
//           continue;
//         }
//         uint64_t D_i = ti->get_deadline();
//         uint64_t temp = get_response_time_LUT((*ti));
//         if (D_i < temp) {
//           ti->set_partition(MAX_INT);
//           continue;
//         } else {
//           ti->set_response_time(temp);
//           schedulable = true;
//   // cout << "Success." << endl;
//           break;
//         }
//       } else {
//         ti->set_partition(MAX_INT);
//         continue;
//       }
//     }
//     if (!schedulable) {
//   // cout << "Failed." << endl;
//       return false;
//     }
//   }
//     // b_sum = 0;
//     // foreach(tasks.get_tasks(), ti) {
//     //   b_sum += blocking_spin_total((*ti));
//     // }
//     // cout << "spin_total" << b_sum << endl;
//     // ofstream hybrid("hybrid.log", ofstream::app);
//     // hybrid << b_sum/ tasks.get_taskset_size() << "\n";
//     // hybrid.flush();
//     // hybrid.close();
//   return true;
// }

// ///////////////////////////// HEAVY //////////////////////////////

// ILP_RTA_FED_H2LP_v2_HEAVY::ILP_RTA_FED_H2LP_v2_HEAVY()
//     : ILP_RTA_FED_H2LP_v2() {}

// ILP_RTA_FED_H2LP_v2_HEAVY::ILP_RTA_FED_H2LP_v2_HEAVY(DAG_TaskSet tasks,
//                                          ProcessorSet processors,
//                                          ResourceSet resources)
//     : ILP_RTA_FED_H2LP_v2(tasks, processors, resources) {
// }

// ILP_RTA_FED_H2LP_v2_HEAVY::~ILP_RTA_FED_H2LP_v2_HEAVY() {}

// // Algorithm 1 Scheduling Test
// bool ILP_RTA_FED_H2LP_v2_HEAVY::is_schedulable() {
//   uint32_t p_num = processors.get_processor_num();
//   p_num_lu = p_num;
//   uint64_t b_sum = 0;

//   // Federated scheduling for HUT
//   foreach(tasks.get_tasks(), ti) {
//     // if (ti->get_utilization() <= 1)
//     //   continue;
//     uint64_t C_i = ti->get_wcet();
//     uint64_t L_i = ti->get_critical_path_length();
//     uint64_t D_i = ti->get_deadline();
//     uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
//     ti->set_dcores(initial_m_i);
//     ti->set_other_attr(1);
//     // cout << "initial:" << initial_m_i << endl;
//   }
//   bool update = false;
//   uint32_t sum_core = 0;
//   do {
//     update = false;
//     foreach(tasks.get_tasks(), ti) {
//       uint64_t D_i = ti->get_deadline();
//       uint64_t temp = get_response_time_HUT((*ti));
//       if (D_i < temp) {
//         ti->set_dcores(1 + ti->get_dcores());
//         update = true;
//       } else {
//         ti->set_response_time(temp);
//       }
//     }
//     sum_core = 0;
//     foreach(tasks.get_tasks(), ti) {
//       sum_core += ti->get_dcores();
//     }

//     if (sum_core > p_num) {
//       return false;
//     }
//   } while (update);

//   return true;
// }
