// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_fed_spin.h>
#include <rta_dag_fed_spin_xu.h>
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

/** Class SPINMapper */


uint64_t SPINMapper::encode_request(uint64_t task_id, uint64_t req_1,
                                    uint64_t req_2, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  // cout << "request_1:" << req_1 << endl;
  assert(task_id < (one << 10));
  assert(req_1 < (one << 15));
  assert(req_2 < (one << 15));
  assert(type < (one << 2));

  key |= (type << 40);
  key |= (task_id << 30);
  key |= (req_1 << 15);
  key |= req_2;
  return key;
}


uint64_t SPINMapper::get_type(uint64_t var) {
  return (var >> 40) & (uint64_t)0x3;  // 2 bits
}

uint64_t SPINMapper::get_task(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t SPINMapper::get_req_1(uint64_t var) {
  return (var >> 15) & (uint64_t)0x7fff;  // 15 bits
}

uint64_t SPINMapper::get_req_2(uint64_t var) {
  return var & (uint64_t)0x7fff;  // 15 bits
}

SPINMapper::SPINMapper(uint start_var) : VarMapperBase(start_var) {}

uint64_t SPINMapper::lookup(uint64_t task_id, uint64_t req_1, uint64_t req_2, var_type type) {
  uint64_t key = encode_request(task_id, req_1, req_2, type);
  uint64_t var = var_for_key(key);
  return var;
}

string SPINMapper::key2str(uint64_t key, uint64_t var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case SPINMapper::BLOCKING_SPIN:
      buf << "bs[";
      break;
    case SPINMapper::BLOCKING_TRANS:
      buf << "bt[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_req_1(key) << ", " << get_req_2(key)
      << "]";
  return buf.str();
}

/** Class LP_RTA_FED_SPIN_FIFO */

LP_RTA_FED_SPIN_FIFO::LP_RTA_FED_SPIN_FIFO()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

LP_RTA_FED_SPIN_FIFO::LP_RTA_FED_SPIN_FIFO(DAG_TaskSet tasks,
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

LP_RTA_FED_SPIN_FIFO::~LP_RTA_FED_SPIN_FIFO() {}

bool LP_RTA_FED_SPIN_FIFO::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool LP_RTA_FED_SPIN_FIFO::is_schedulable() {

  SchedTestBase* schedTest = new RTA_DAG_FED_SPIN_FIFO_XU(tasks, processors, resources);

  if (schedTest->is_schedulable()) {
  // if (false) {
    return true;
  } else {
    uint32_t p_num = processors.get_processor_num();
    foreach(tasks.get_tasks(), ti) {
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
      ti->set_dcores(initial_m_i);
    }
    bool update = false;
    uint32_t sum_core = 0;
    do {
      update = false;
      sum_core = 0;
      foreach(tasks.get_tasks(), ti) {
        uint64_t D_i = ti->get_deadline();
        uint64_t temp = get_response_time((*ti));
        if (D_i < temp) {
          ti->set_dcores(1 + ti->get_dcores());
          update = true;
        } else {
          ti->set_response_time(temp);
        }
        sum_core += ti->get_dcores();
      }
      if (p_num < sum_core)
        return false;
    } while (update);
    if (sum_core <= p_num) {
      return true;
    }
    return false;
  }

  
}

uint64_t LP_RTA_FED_SPIN_FIFO::resource_delay(const DAG_Task& ti, uint32_t res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;

  for (uint x = 0; x < request_num; x++) {
    SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());

    if (rb_solution->is_solved()) {
      double result;
      result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
      }
      delete rb_solution;
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
  }
  return max_delay;
}

uint64_t LP_RTA_FED_SPIN_FIFO::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

uint64_t LP_RTA_FED_SPIN_FIFO::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t B_i = total_resource_delay(ti);
  uint32_t n_i = ti.get_dcores();
  return L_i + B_i + (C_i-L_i)/n_i;
}

void LP_RTA_FED_SPIN_FIFO::objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj) {
  int64_t var_id;
  uint request_num;
  uint job_num;
  double coef = 1.0 / ti.get_dcores();

  // cout << "111" << endl;
  // s-blocking to lambda_i
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    uint64_t length = tx->get_request_by_id(res_id).get_max_length();
    // cout << "max_length:" << length << endl;
    if (x == ti.get_id()) {
      request_num = res_num;
      job_num = 1;
    } else {
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());    
      job_num = tx->get_max_job_num(ti.get_deadline());
      // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    }
    vector<uint64_t> requests_length;
    for(uint i = 0; i < tx->get_request_by_id(res_id).get_num_requests(); i++) {
      for (uint j = 0; j < job_num; j++) {
        requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
    // cout << "length["<<i<<"j"<<j<<"]:" << tx->get_request_by_id(res_id).get_requests_length()[i] << endl;
      }
    }
    for (uint u = 1; u <= request_num; u++) {
      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      obj->add_term(var_id, requests_length[u-1]);
      // obj->add_term(var_id, length);
    }
  }

  // cout << "222" << endl;
  // s-blocking to subtasks off lambda_i and cause interference to lambda_i
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    int64_t length = tx->get_request_by_id(res_id).get_max_length();
    if (x == ti.get_id()) {
      request_num = res_num;
      vector<uint64_t> requests_length;
      for(uint i = 0; i < request_num; i++) {
        requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
      }
      for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        // double coef_2 = -1.0 * length * coef;
        double coef_2 = -1.0 * requests_length[u-1] * coef;
        // cout << "coef_2:" << coef_2 << endl;
        obj->add_term(var_id, coef_2);
      }
    }
    else {
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());
      job_num = tx->get_max_job_num(ti.get_deadline());
    }

  // cout << "333" << endl;
    vector<uint64_t> requests_length;
    for(uint i = 0; i < tx->get_request_by_id(res_id).get_num_requests(); i++) {
      for (uint j = 0; j < job_num; j++) {
        requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
      }
    }
  // cout << "444" << endl;
    for (uint u = 1; u <= request_num; u++) {
      for (uint v = 1; v <= res_num; v++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        // obj->add_term(var_id, length * coef);
        obj->add_term(var_id, requests_length[u-1] * coef);
      }
    }
  }
}

void LP_RTA_FED_SPIN_FIFO::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              SPINMapper* vars) {
  // cout << "C1" << endl;
  constraint_1(ti, res_id, res_num, lp, vars);
  // cout << "C2" << endl;
  constraint_2(ti, res_id, res_num, lp, vars);
  // cout << "C3" << endl;
  constraint_3(ti, res_id, res_num, lp, vars);
  // cout << "C4" << endl;
  constraint_4(ti, res_id, res_num, lp, vars);
  // cout << "C5" << endl;
  constraint_5(ti, res_id, res_num, lp, vars);
  // cout << "C6" << endl;
  constraint_6(ti, res_id, res_num, lp, vars);
  // cout << "C7" << endl;
  constraint_7(ti, res_id, res_num, lp, vars);
  // cout << "C8" << endl;
  constraint_8(ti, res_id, res_num, lp, vars);
}

void LP_RTA_FED_SPIN_FIFO::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num = res_num;
    else
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num; u++) {
      for (uint v = 1; v <= res_num; v++) {
        LinearExpression *exp = new LinearExpression();
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_FED_SPIN_FIFO::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    for (uint u = 1; u <= request_num; u++) {
      LinearExpression *exp = new LinearExpression();
      for (uint v = 1; v <= res_num; v++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);
      }
      lp->add_inequality(exp, ti.get_dcores());
    }
  }
}

void LP_RTA_FED_SPIN_FIFO::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    for (uint v = 1; v <= res_num; v++) {
      LinearExpression *exp = new LinearExpression();
      for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);
      }
      lp->add_inequality(exp, tx->get_dcores());
    }
  }
}

void LP_RTA_FED_SPIN_FIFO::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    LinearExpression *exp = new LinearExpression();
    for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, (ti.get_request_by_id(res_id).get_num_requests()-res_num)*tx->get_dcores());
  }
}

void LP_RTA_FED_SPIN_FIFO::constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {  // C2
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    for (uint u = v; u <= request_num; u++) {
      var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, 0);
  }
}


void LP_RTA_FED_SPIN_FIFO::constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {  // C3
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  for (uint u = 1; u <= request_num; u++) {
    LinearExpression *exp = new LinearExpression();
    for (uint v = 1; v <= res_num; v++) {
      var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, ti.get_dcores() - 1);
  }
}

void LP_RTA_FED_SPIN_FIFO::constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {  // C4
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    for (uint u = 1; u <= request_num; u++) {
      var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, ti.get_dcores() - 1);
  }
}

void LP_RTA_FED_SPIN_FIFO::constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {  // C5
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  LinearExpression *exp = new LinearExpression();
  for (uint u = 1; u <= res_num; u++) {
      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
  }
  lp->add_inequality(exp, (ti.get_request_by_id(res_id).get_num_requests()-res_num)*(ti.get_dcores()-1));

}



///////////////////////////// FF //////////////////////////////
/*
LP_RTA_FED_SPIN_FIFO_FF::LP_RTA_FED_SPIN_FIFO_FF()
    : LP_RTA_FED_SPIN_FIFO() {}

LP_RTA_FED_SPIN_FIFO_FF::LP_RTA_FED_SPIN_FIFO_FF(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : LP_RTA_FED_SPIN_FIFO(tasks, processors, resources) {
  // this->tasks = tasks;
  // this->processors = processors;
  // this->resources = resources;

  // this->resources.update(&(this->tasks));
  // this->processors.update(&(this->tasks), &(this->resources));

  // this->tasks.RM_Order();
  // this->processors.init();
}

LP_RTA_FED_SPIN_FIFO_FF::~LP_RTA_FED_SPIN_FIFO_FF() {}

// Algorithm 1 Scheduling Test
bool LP_RTA_FED_SPIN_FIFO_FF::is_schedulable() {
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

LP_RTA_FED_SPIN_FIFO_v2::LP_RTA_FED_SPIN_FIFO_v2()
    : LP_RTA_FED_SPIN_FIFO() {}

LP_RTA_FED_SPIN_FIFO_v2::LP_RTA_FED_SPIN_FIFO_v2(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : LP_RTA_FED_SPIN_FIFO(tasks, processors, resources) {
}

LP_RTA_FED_SPIN_FIFO_v2::~LP_RTA_FED_SPIN_FIFO_v2() {}

// Algorithm 1 Scheduling Test
bool LP_RTA_FED_SPIN_FIFO_v2::is_schedulable() {

  SchedTestBase* schedTest = new RTA_DAG_FED_SPIN_FIFO_XU(tasks, processors, resources);

  // if (schedTest->is_schedulable()) {
  if (false) {
    return true;
  } else {
    uint32_t p_num = processors.get_processor_num();
    foreach(tasks.get_tasks(), ti) {
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
      ti->set_dcores(initial_m_i);
    }
    bool update = false;
    uint32_t sum_core = 0;
    do {
      update = false;
      sum_core = 0;
      foreach(tasks.get_tasks(), ti) {
        uint64_t D_i = ti->get_deadline();
        uint64_t temp = get_response_time((*ti));
        if (D_i < temp) {
          ti->set_dcores(1 + ti->get_dcores());
          update = true;
        } else {
          ti->set_response_time(temp);
        }
        sum_core += ti->get_dcores();
      }
      if (p_num < sum_core)
        return false;
    } while (update);
    if (sum_core <= p_num) {
      return true;
    }
    return false;
  }
}

uint64_t LP_RTA_FED_SPIN_FIFO_v2::inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num) {
  uint32_t N_i_q, m_i;
  uint64_t L_i_q, sum = 0;
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();

  foreach_task_except(tasks.get_tasks(), ti, tj) {
    uint32_t m_j;
    uint64_t L_j_q;
    if (!tj->is_request_exist(res_id))
      continue;
    m_j = tj->get_dcores();
    L_j_q = tj->get_request_by_id(res_id).get_max_length();
    sum += L_j_q * min(m_i * tj->get_max_request_num(res_id, ti.get_deadline()), m_j *(N_i_q + (m_i-1)*request_num));
    // sum += L_j_q *  m_j *(N_i_q + (m_i-1)*request_num);
  }
  // cout << "inter_task_blocking:" << sum << endl;
  // return 0;
  return sum;
}

uint64_t LP_RTA_FED_SPIN_FIFO_v2::resource_delay(const DAG_Task& ti, uint32_t res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;
  uint32_t max_delay_request_num;

  // for (uint x = 0; x <= request_num; x++) {
  for (int x = request_num; x >= 0; --x) {
    // uint64_t inter_blocking_overall = 0;
    // uint64_t inter_blocking_cp = 0;
    // foreach_task_except(tasks.get_tasks(), ti, tx) {
    //   if (!tx->is_request_exist(res_id))
    //     continue;
    //   uint64_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
    //   uint32_t N_i_q = ti.get_request_by_id(res_id).get_num_requests();
    //   uint32_t N_i_x_q = tx->get_max_request_num(res_id, ti.get_deadline());
    //   inter_blocking_overall += min(N_i_q * tx->get_dcores(), N_i_x_q * ti.get_dcores()) * L_x_q;
    //   inter_blocking_cp += min((N_i_q - x) * tx->get_dcores(), N_i_x_q) * L_x_q;
    // }
// cout << "x:" << x << endl;
    uint64_t test_start = inter_task_blocking(ti, res_id, (request_num-x))/ti.get_dcores();
    // if (max_delay < test_start) {
    //     max_delay = test_start;
    // }

    // cout << "test_start:" << test_start << endl;
    SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());

    if (rb_solution->is_solved()) {
      double result;
      result = rb_solution->evaluate(*(delay.get_objective()));
      test_start += result;
  // cout << "[SPIN-LP]x_="<< (request_num-x) << ":"<< test_start << endl;
      if (max_delay < test_start) {
        max_delay = test_start;
        max_delay_request_num = (request_num-x);
      }
      delete rb_solution;
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
  }
  // cout << "[SPIN-LP]X_"<<ti.get_id()<<"_"<<res_id<<":"<< max_delay_request_num << endl;
  return max_delay;
}

uint64_t LP_RTA_FED_SPIN_FIFO_v2::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

uint64_t LP_RTA_FED_SPIN_FIFO_v2::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t B_i = total_resource_delay(ti);
  uint32_t n_i = ti.get_dcores();
  // cout << "[SPIN-LP] Task[" << ti.get_id() << "] with "<<n_i <<" processors. total blocking:" <<  B_i << endl;
  return L_i + B_i + (C_i-L_i)/n_i;
}

void LP_RTA_FED_SPIN_FIFO_v2::objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj) {
  int64_t var_id;
  uint request_num;
  double coef = 1.0 / ti.get_dcores();
  uint64_t length;

  // s-blocking to lambda_i
  uint i = ti.get_id();
  length = ti.get_request_by_id(res_id).get_max_length();
  request_num = res_num;
  // cout << "res_id:" << res_id <<"request num:" << request_num << endl;
  for (uint u = 1; u <= request_num; u++) {
    var_id = vars->lookup(i, u, 0, SPINMapper::BLOCKING_SPIN);
    obj->add_term(var_id, length);
  }

  // s-blocking to subtasks off lambda_i and cause interference to lambda_i
  length = ti.get_request_by_id(res_id).get_max_length();
  request_num = res_num;
  for (uint u = 1; u <= request_num; u++) {
    var_id = vars->lookup(i, u, 0, SPINMapper::BLOCKING_SPIN);
    double coef_2 = -1.0 * length * coef;
    // cout << "coef_2:" << coef_2 << endl;
    obj->add_term(var_id, coef_2);
  }
  for (uint u = 1; u <= request_num; u++) {
    for (uint v = 1; v <= res_num; v++) {
      var_id = vars->lookup(i, u, v, SPINMapper::BLOCKING_TRANS);
      obj->add_term(var_id, length * coef);
    }
  }
}

void LP_RTA_FED_SPIN_FIFO_v2::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              SPINMapper* vars) {
  // cout << "C1" << endl;
  constraint_1(ti, res_id, res_num, lp, vars);
  // cout << "C2" << endl;
  // constraint_2(ti, res_id, res_num, lp, vars);
  // cout << "C3" << endl;
  // constraint_3(ti, res_id, res_num, lp, vars);
  // cout << "C4" << endl;
  // constraint_4(ti, res_id, res_num, lp, vars);
  // cout << "C5" << endl;
  constraint_5(ti, res_id, res_num, lp, vars);
  // cout << "C6" << endl;
  constraint_6(ti, res_id, res_num, lp, vars);
  // cout << "C7" << endl;
  constraint_7(ti, res_id, res_num, lp, vars);
  // cout << "C8" << endl;
  constraint_8(ti, res_id, res_num, lp, vars);
}

void LP_RTA_FED_SPIN_FIFO_v2::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint i = ti.get_id();
  request_num = res_num;
  for (uint u = 1; u <= request_num; u++) {
    for (uint v = 1; v <= res_num; v++) {
      LinearExpression *exp = new LinearExpression();
      var_id = vars->lookup(i, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(i, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);

      lp->add_inequality(exp, 1);
    }
  }
}

void LP_RTA_FED_SPIN_FIFO_v2::constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint i = ti.get_id();
  request_num = res_num;
  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    for (uint u = v; u <= request_num; u++) {
      var_id = vars->lookup(i, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, 0);
  }
}


void LP_RTA_FED_SPIN_FIFO_v2::constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint i = ti.get_id();
  request_num = res_num;
  for (uint u = 1; u <= request_num; u++) {
    LinearExpression *exp = new LinearExpression();
    for (uint v = 1; v <= res_num; v++) {
      var_id = vars->lookup(i, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(i, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, ti.get_dcores() - 1);
  }
}

void LP_RTA_FED_SPIN_FIFO_v2::constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint i = ti.get_id();
  request_num = res_num;
  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    for (uint u = 1; u <= request_num; u++) {
      var_id = vars->lookup(i, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, ti.get_dcores() - 1);
  }
}

void LP_RTA_FED_SPIN_FIFO_v2::constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  LinearExpression *exp = new LinearExpression();
  for (uint u = 1; u <= res_num; u++) {
      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
  }
  lp->add_inequality(exp, (ti.get_request_by_id(res_id).get_num_requests()-res_num)*(ti.get_dcores()-1));
}



/** Class LP_RTA_FED_SPIN_FIFO_v3 */

LP_RTA_FED_SPIN_FIFO_v3::LP_RTA_FED_SPIN_FIFO_v3()
    : LP_RTA_FED_SPIN_FIFO() {}

LP_RTA_FED_SPIN_FIFO_v3::LP_RTA_FED_SPIN_FIFO_v3(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : LP_RTA_FED_SPIN_FIFO(tasks, processors, resources) {
}

LP_RTA_FED_SPIN_FIFO_v3::~LP_RTA_FED_SPIN_FIFO_v3() {}

// Algorithm 1 Scheduling Test
bool LP_RTA_FED_SPIN_FIFO_v3::is_schedulable() {

  SchedTestBase* schedTest = new RTA_DAG_FED_SPIN_FIFO_XU(tasks, processors, resources);

  if (schedTest->is_schedulable()) {
  // if (false) {
    return true;
  } else {
    uint32_t p_num = processors.get_processor_num();
    foreach(tasks.get_tasks(), ti) {
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
      ti->set_dcores(initial_m_i);
    }
    bool update = false;
    uint32_t sum_core = 0;
    do {
      update = false;
      sum_core = 0;
      foreach(tasks.get_tasks(), ti) {
        uint64_t D_i = ti->get_deadline();
        uint64_t temp = get_response_time((*ti));
        if (D_i < temp) {
          ti->set_dcores(1 + ti->get_dcores());
          update = true;
        } else {
          ti->set_response_time(temp);
        }
        sum_core += ti->get_dcores();
      }
      if (p_num < sum_core)
        return false;
    } while (update);
    if (sum_core <= p_num) {
      return true;
    }
    return false;
  }
}

uint64_t LP_RTA_FED_SPIN_FIFO_v3::resource_delay(const DAG_Task& ti, uint32_t res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;

  for (uint x = 0; x < request_num; x++) {
    SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());

    if (rb_solution->is_solved()) {
      double result;
      result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
      }
      delete rb_solution;
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
  }
  return max_delay;
}

uint64_t LP_RTA_FED_SPIN_FIFO_v3::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

uint64_t LP_RTA_FED_SPIN_FIFO_v3::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t B_i = total_resource_delay(ti);
  uint32_t n_i = ti.get_dcores();
  return L_i + B_i + (C_i-L_i)/n_i;
}

void LP_RTA_FED_SPIN_FIFO_v3::objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj) {
  int64_t var_id;
  uint request_num;
  uint job_num;
  double coef = 1.0 / ti.get_dcores();

  // cout << "111" << endl;
  // s-blocking to lambda_i
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    uint64_t length = tx->get_request_by_id(res_id).get_max_length();
    // cout << "max_length:" << length << endl;
    if (x == ti.get_id()) {
      request_num = res_num;
      job_num = 1;
    } else {
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());    
      job_num = tx->get_max_job_num(ti.get_deadline());
      // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    }
    // vector<uint64_t> requests_length;
    // for(uint i = 0; i < tx->get_request_by_id(res_id).get_num_requests(); i++) {
    //   for (uint j = 0; j < job_num; j++) {
    //     requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
    // // cout << "length["<<i<<"j"<<j<<"]:" << tx->get_request_by_id(res_id).get_requests_length()[i] << endl;
    //   }
    // }
    for (uint u = 1; u <= request_num; u++) {
      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      // obj->add_term(var_id, requests_length[u-1]);
      obj->add_term(var_id, length);
    }
  }

  // cout << "222" << endl;
  // s-blocking to subtasks off lambda_i and cause interference to lambda_i
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    int64_t length = tx->get_request_by_id(res_id).get_max_length();
    if (x == ti.get_id()) {
      request_num = res_num;
      // vector<uint64_t> requests_length;
      // for(uint i = 0; i < request_num; i++) {
      //   requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
      // }
      for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        double coef_2 = -1.0 * length * coef;
        // double coef_2 = -1.0 * requests_length[u-1] * coef;
        // cout << "coef_2:" << coef_2 << endl;
        obj->add_term(var_id, coef_2);
      }
    }
    else {
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());
      job_num = tx->get_max_job_num(ti.get_deadline());
    }

  // cout << "333" << endl;
    // vector<uint64_t> requests_length;
    // for(uint i = 0; i < tx->get_request_by_id(res_id).get_num_requests(); i++) {
    //   for (uint j = 0; j < job_num; j++) {
    //     requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
    //   }
    // }
  // cout << "444" << endl;
    for (uint u = 1; u <= request_num; u++) {
      for (uint v = 1; v <= res_num; v++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        obj->add_term(var_id, length * coef);
        // obj->add_term(var_id, requests_length[u-1] * coef);
      }
    }
  }
}



/** Class LP_RTA_FED_SPIN_PRIO */

LP_RTA_FED_SPIN_PRIO::LP_RTA_FED_SPIN_PRIO()
    : PartitionedSched(false, RTA, FIX_PRIORITY, SPIN, "", "") {}

LP_RTA_FED_SPIN_PRIO::LP_RTA_FED_SPIN_PRIO(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, SPIN, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

LP_RTA_FED_SPIN_PRIO::~LP_RTA_FED_SPIN_PRIO() {}

bool LP_RTA_FED_SPIN_PRIO::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool LP_RTA_FED_SPIN_PRIO::is_schedulable() {

  SchedTestBase* schedTest = new RTA_DAG_FED_SPIN_PRIO_XU(tasks, processors, resources);

  if (schedTest->is_schedulable()) {
  // if (false) {
    return true;
  } else {
    uint32_t p_num = processors.get_processor_num();
    foreach(tasks.get_tasks(), ti) {
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
      ti->set_dcores(initial_m_i);
    }
    bool update = false;
    uint32_t sum_core = 0;
    do {
      update = false;
      sum_core = 0;
      foreach(tasks.get_tasks(), ti) {
        uint64_t D_i = ti->get_deadline();
        uint64_t temp = get_response_time((*ti));
        if (D_i < temp) {
          ti->set_dcores(1 + ti->get_dcores());
          update = true;
        } else {
          ti->set_response_time(temp);
        }
        sum_core += ti->get_dcores();
      }
      if (p_num < sum_core)
        return false;
    } while (update);
    if (sum_core <= p_num) {
      return true;
    }
    return false;
  }

  
}


uint64_t LP_RTA_FED_SPIN_PRIO::higher(const DAG_Task& ti, uint32_t res_id,
                                       uint64_t interval) {
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (th->is_request_exist(res_id)) {
      const Request& request_q = th->get_request_by_id(res_id);
      uint32_t n = th->get_max_job_num(interval);
      uint32_t N_h_q = request_q.get_num_requests();
      uint64_t L_h_q = request_q.get_max_length();
      sum += n * N_h_q * L_h_q;
    }
  }
  return sum;
}

uint64_t LP_RTA_FED_SPIN_PRIO::lower(const DAG_Task& ti,
                                      uint32_t res_id) {
  uint64_t max = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (tl->is_request_exist(res_id)) {
      const Request& request_q = tl->get_request_by_id(res_id);
      uint64_t L_l_q = request_q.get_max_length();
      if (L_l_q > max) max = L_l_q;
    }
  }
  return max;
}

uint64_t LP_RTA_FED_SPIN_PRIO::equal(const DAG_Task& ti,
                                      uint32_t res_id) {
  if (ti.is_request_exist(res_id)) {
    const Request& request_q = ti.get_request_by_id(res_id);
    uint32_t N_i_q = request_q.get_num_requests();
    uint64_t L_i_q = request_q.get_max_length();
    uint32_t n_i = ti.get_dcores();
    uint32_t m_i = min(n_i - 1, N_i_q - 1);
    return m_i * L_i_q;
  } else {
    return 0;
  }
}

uint64_t LP_RTA_FED_SPIN_PRIO::dpr(const DAG_Task& ti, uint32_t res_id) {
  uint64_t test_start = lower(ti, res_id) + equal(ti, res_id);
  uint64_t test_end = ti.get_deadline();
  uint64_t delay = test_start;

  while (delay <= test_end) {
    uint64_t temp = test_start;

    temp += higher(ti, res_id, delay);

    if (temp > delay)
      delay = temp;
    else if (temp == delay)
      return delay;
  }
  return test_end + 100;
}

uint64_t LP_RTA_FED_SPIN_PRIO::resource_delay(const DAG_Task& ti, uint32_t res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;
  // cout << "task["<<ti.get_id()<<"]" << endl;

  for (uint x = 0; x < request_num; x++) {
    SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    // cout << "obj" << endl;
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    // cout << "111" << endl;
    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());
    // cout << "222" << endl;

    if (rb_solution->is_solved()) {
      double result;
      result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
      }
      delete rb_solution;
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
  }
  return max_delay;
}

uint64_t LP_RTA_FED_SPIN_PRIO::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

uint64_t LP_RTA_FED_SPIN_PRIO::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t B_i = total_resource_delay(ti);
  uint32_t n_i = ti.get_dcores();
  cout << "[LP-SPIN-p] Task[" << ti.get_id() << "] with "<<n_i <<" processors. total blocking:" <<  B_i << endl;
  return L_i + B_i + (C_i-L_i)/n_i;
}

void LP_RTA_FED_SPIN_PRIO::objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj) {
  int64_t var_id;
  uint request_num;
  uint job_num;
  double coef = 1.0 / ti.get_dcores();

  // s-blocking to lambda_i
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    uint64_t length = tx->get_request_by_id(res_id).get_max_length();
    if (x == ti.get_id()) {
      request_num = res_num;
      job_num = 1;
    } else {
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());    
      job_num = tx->get_max_job_num(ti.get_deadline());
      // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    }
    vector<uint64_t> requests_length;
    for(uint i = 0; i < tx->get_request_by_id(res_id).get_num_requests(); i++) {
      for (uint j = 0; j < job_num; j++) {
        requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
      }
    }
    for (uint u = 1; u <= request_num; u++) {
      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      obj->add_term(var_id, requests_length[u-1]);
      // obj->add_term(var_id, length);
    }
  }

  // s-blocking to subtasks off lambda_i and cause interference to lambda_i
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    int64_t length = tx->get_request_by_id(res_id).get_max_length();
    if (x == ti.get_id()) {
      request_num = res_num;
      vector<uint64_t> requests_length;
      for(uint i = 0; i < request_num; i++) {
        requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
      }
      for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        // double coef_2 = -1.0 * length * coef;
        double coef_2 = -1.0 * requests_length[u-1] * coef;
        // cout << "coef_2:" << coef_2 << endl;
        obj->add_term(var_id, coef_2);
      }
    }
    else {
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());
      job_num = tx->get_max_job_num(ti.get_deadline());
    }
    vector<uint64_t> requests_length;
    for(uint i = 0; i < tx->get_request_by_id(res_id).get_num_requests(); i++) {
      for (uint j = 0; j < job_num; j++) {
        requests_length.push_back(tx->get_request_by_id(res_id).get_requests_length()[i]);
      }
    }
    for (uint u = 1; u <= request_num; u++) {
      for (uint v = 1; v <= res_num; v++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        // obj->add_term(var_id, length * coef);
        obj->add_term(var_id, requests_length[u-1] * coef);
      }
    }
  }
}

void LP_RTA_FED_SPIN_PRIO::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              SPINMapper* vars) {
  // cout << "C1" << endl;
  constraint_1(ti, res_id, res_num, lp, vars);
  // cout << "C2" << endl;
  constraint_2(ti, res_id, res_num, lp, vars);
  // cout << "C3" << endl;
  constraint_3(ti, res_id, res_num, lp, vars);
  constraint_3_p(ti, res_id, res_num, lp, vars);
  // cout << "C4" << endl;
  constraint_4(ti, res_id, res_num, lp, vars);
  constraint_4_p(ti, res_id, res_num, lp, vars);
  // cout << "C5" << endl;
  constraint_5(ti, res_id, res_num, lp, vars);
  // cout << "C6" << endl;
  constraint_6(ti, res_id, res_num, lp, vars);
  // cout << "C7" << endl;
  constraint_7(ti, res_id, res_num, lp, vars);
  // cout << "C8" << endl;
  constraint_8(ti, res_id, res_num, lp, vars);
}

void LP_RTA_FED_SPIN_PRIO::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num = res_num;
    else
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num; u++) {
      for (uint v = 1; v <= res_num; v++) {
        LinearExpression *exp = new LinearExpression();
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_FED_SPIN_PRIO::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    for (uint u = 1; u <= request_num; u++) {
      LinearExpression *exp = new LinearExpression();
      for (uint v = 1; v <= res_num; v++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);
      }
      lp->add_inequality(exp, ti.get_dcores());
    }
  }
}

void LP_RTA_FED_SPIN_PRIO::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_id();
      if (!tx->is_request_exist(res_id))
        continue;
      request_num = tx->get_max_request_num(res_id, ti.get_deadline());
      for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);
      }
    }
    lp->add_inequality(exp, 1);
  }

}

void LP_RTA_FED_SPIN_PRIO::constraint_3_p(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;
  
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    uint32_t ub = ceiling(dpr(ti, res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests();
    for (uint v = 1; v <= res_num; v++) {
      LinearExpression *exp = new LinearExpression();
      for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
        exp->add_term(var_id, 1);
      }
      lp->add_inequality(exp, ub);
    }
  }
}

void LP_RTA_FED_SPIN_PRIO::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  LinearExpression *exp = new LinearExpression();
  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);
    }
  }
  lp->add_inequality(exp, (ti.get_request_by_id(res_id).get_num_requests()-res_num));
}

void LP_RTA_FED_SPIN_PRIO::constraint_4_p(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    request_num = tx->get_max_request_num(res_id, ti.get_deadline());
    LinearExpression *exp = new LinearExpression();
    for (uint u = 1; u <= request_num; u++) {
        var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
        exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, request_num);
  }
}



void LP_RTA_FED_SPIN_PRIO::constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    for (uint u = v; u <= request_num; u++) {
      var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, 0);
  }
}


void LP_RTA_FED_SPIN_PRIO::constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  for (uint u = 1; u <= request_num; u++) {
    LinearExpression *exp = new LinearExpression();
    for (uint v = 1; v <= res_num; v++) {
      var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, ti.get_dcores() - 1);
  }
}

void LP_RTA_FED_SPIN_PRIO::constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;

  uint x = ti.get_id();
  request_num = res_num;
  for (uint v = 1; v <= res_num; v++) {
    LinearExpression *exp = new LinearExpression();
    for (uint u = 1; u <= request_num; u++) {
      var_id = vars->lookup(x, u, v, SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);
    }
    lp->add_inequality(exp, ti.get_dcores() - 1);
  }
}

void LP_RTA_FED_SPIN_PRIO::constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          SPINMapper* vars) {
  int64_t var_id;
  uint request_num;
  uint x = ti.get_id();
  request_num = res_num;
  LinearExpression *exp = new LinearExpression();
  for (uint u = 1; u <= res_num; u++) {
      var_id = vars->lookup(x, u, 0, SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
  }
  lp->add_inequality(exp, (ti.get_request_by_id(res_id).get_num_requests()-res_num)*(ti.get_dcores()-1));

}

