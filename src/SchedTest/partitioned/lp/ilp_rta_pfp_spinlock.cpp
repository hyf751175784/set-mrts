// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <ilp_rta_pfp_spinlock.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <solution.h>
#include <iostream>
#include <sstream>

using std::ostringstream;

/** Class ILPSpinLockMapper */
uint64_t ILPSpinLockMapper::encode_request(uint64_t type, uint64_t part_1,
                                           uint64_t part_2, uint64_t part_3,
                                           uint64_t part_4) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(type < (one << 4));
  assert(part_1 < (one << 10));
  assert(part_2 < (one << 10));
  assert(part_3 < (one << 10));
  assert(part_4 < (one << 10));

  key |= (type << 40);
  key |= (part_1 << 30);
  key |= (part_2 << 20);
  key |= (part_3 << 10);
  key |= part_4;

  /*
  cout<<"type:"<<type<<endl;
  cout<<"part_1:"<<part_1<<endl;
  cout<<"part_2:"<<part_2<<endl;
  cout<<"part_3:"<<part_3<<endl;
  cout<<"part_4:"<<part_4<<endl;
  cout<<"key:"<<key<<endl;
  */
  return key;
}

uint64_t ILPSpinLockMapper::get_type(uint64_t var) {
  return (var >> 40) & (uint64_t)0xf;  // 4 bits
}

uint64_t ILPSpinLockMapper::get_part_1(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPSpinLockMapper::get_part_2(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPSpinLockMapper::get_part_3(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPSpinLockMapper::get_part_4(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

ILPSpinLockMapper::ILPSpinLockMapper(uint start_var)
    : VarMapperBase(start_var) {}

uint ILPSpinLockMapper::lookup(uint type, uint part_1, uint part_2, uint part_3,
                               uint part_4) {
  uint64_t key = encode_request(type, part_1, part_2, part_3, part_4);

  // cout<<"string:"<<key2str(key)<<endl;
  uint var = var_for_key(key);
  return var;
}

string ILPSpinLockMapper::var2str(unsigned int var) const {
  uint64_t key;

  if (search_key_for_var(var, key))
    return key2str(key);
  else
    return "<?>";
}

string ILPSpinLockMapper::key2str(uint64_t key) const {
  ostringstream buf;

  switch (get_type(key)) {
    case ILPSpinLockMapper::LOCALITY_ASSIGNMENT:
      buf << "A[";
      break;
    case ILPSpinLockMapper::PRIORITY_ASSIGNMENT:
      buf << "Pi[";
      break;
    case ILPSpinLockMapper::SAME_LOCALITY:
      buf << "V[";
      break;
    case ILPSpinLockMapper::HIGHER_PRIORITY:
      buf << "X[";
      break;
    case ILPSpinLockMapper::MAX_PREEMEPT:
      buf << "H[";
      break;
    case ILPSpinLockMapper::INTERFERENCE_TIME:
      buf << "I[";
      break;
    case ILPSpinLockMapper::SPIN_TIME:
      buf << "S[";
      break;
    case ILPSpinLockMapper::BLOCKING_TIME:
      buf << "B[";
      break;
    case ILPSpinLockMapper::AB_DEISION:
      buf << "Z[";
      break;
    case ILPSpinLockMapper::RESPONSE_TIME:
      buf << "R[";
      break;
    default:
      buf << "?[";
  }

  buf << get_part_1(key) << ", " << get_part_2(key) << ", " << get_part_3(key)
      << ", " << get_part_4(key) << "]";

  return buf.str();
}

/** Class ILP_RTA_PFP_spinlock */
ILP_RTA_PFP_spinlock::ILP_RTA_PFP_spinlock()
    : PartitionedSched(true, RTA, FIX_PRIORITY, SPIN, "", "spinlock") {}

ILP_RTA_PFP_spinlock::ILP_RTA_PFP_spinlock(TaskSet tasks,
                                           ProcessorSet processors,
                                           ResourceSet resources)
    : PartitionedSched(true, RTA, FIX_PRIORITY, SPIN, "", "spinlock") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

ILP_RTA_PFP_spinlock::~ILP_RTA_PFP_spinlock() {}

bool ILP_RTA_PFP_spinlock::is_schedulable() {
  if (0 == tasks.get_tasks().size()) return true;

  ILPSpinLockMapper vars;
  LinearProgram response_bound;

  ILP_SpinLock_set_objective(&response_bound, &vars);

  ILP_SpinLock_add_constraints(&response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars(), 0.0, 1.0, 1, 1);

  assert(rb_solution != NULL);

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  if (rb_solution->is_solved()) {
#if ILP_SOLUTION_VAR_CHECK == 1
    for (uint i = 0; i < tasks.get_tasks().size(); i++) {
      cout << "/===========Task " << i << "===========/" << endl;
      LinearExpression* exp = new LinearExpression();
      double result;
      ILP_SpinLock_construct_exp(&vars, exp, ILPSpinLockMapper::RESPONSE_TIME,
                                 i);
      result = rb_solution->evaluate(*exp);
      cout << "response time of task " << i << ":" << result << endl;

      ILP_SpinLock_construct_exp(&vars, exp,
                                 ILPSpinLockMapper::INTERFERENCE_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "interference time of task " << i << ":" << result << endl;

      ILP_SpinLock_construct_exp(&vars, exp, ILPSpinLockMapper::BLOCKING_TIME,
                                 i);
      result = rb_solution->evaluate(*exp);
      cout << "blocking time of task " << i << ":" << result << endl;

      ILP_SpinLock_construct_exp(&vars, exp, ILPSpinLockMapper::SPIN_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "spin time of task " << i << ":" << result << endl;

      cout << "|===========Locailty Assignment===========|" << endl;
      for (uint k = 1; k <= processors.get_processor_num(); k++) {
        ILP_SpinLock_construct_exp(
            &vars, exp, ILPSpinLockMapper::LOCALITY_ASSIGNMENT, i, k);
        result = rb_solution->evaluate(*exp);
        cout << "allocation of processor " << k << ":" << result << endl;
      }
      cout << "|===========Priority Assignment===========|" << endl;
      for (uint p = 1; p <= tasks.get_tasks().size(); p++) {
        ILP_SpinLock_construct_exp(
            &vars, exp, ILPSpinLockMapper::PRIORITY_ASSIGNMENT, i, p);
        result = rb_solution->evaluate(*exp);
        cout << "Priority assignment " << p << ":" << result << endl;
      }

      cout << "wcet of task " << i << ":" << tasks.get_tasks()[i].get_wcet()
           << endl;
      cout << "deadline of task " << i << ":"
           << tasks.get_tasks()[i].get_deadline() << endl;
      delete exp;
    }
#endif

    // rb_solution->show_error();
    delete rb_solution;
    return true;
  } else {
    // rb_solution->show_error();
  }

  delete rb_solution;
  return false;
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_construct_exp(ILPSpinLockMapper* vars,
                                                      LinearExpression* exp,
                                                      uint type, uint part_1,
                                                      uint part_2, uint part_3,
                                                      uint part_4) {
  exp->get_terms().clear();
  uint var_id;

  var_id = vars->lookup(type, part_1, part_2, part_3, part_4);
  exp->add_term(var_id, 1);

#if ILP_SOLUTION_VAR_CHECK == 1
  cout << vars->var2str(var_id) << ":" << endl;
#endif
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_set_objective(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  // any objective function is okay.
  // assert(0 < tasks.get_tasks().size());

  LinearExpression* obj = new LinearExpression();
  uint var_id;

  var_id = vars->lookup(ILPSpinLockMapper::RESPONSE_TIME, 0, 0, 0);
  obj->add_term(var_id, 1);

  lp->set_objective(obj);
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_add_constraints(
    LinearProgram* lp, ILPSpinLockMapper* vars) {
  // cout<<"Set C1"<<endl;
  ILP_SpinLock_constraint_1(lp, vars);
  // cout<<"Set C2"<<endl;
  ILP_SpinLock_constraint_2(lp, vars);
  // cout<<"Set C2-1"<<endl;
  ILP_SpinLock_constraint_2_1(lp, vars);
  // cout<<"Set C3"<<endl;
  ILP_SpinLock_constraint_3(lp, vars);
  // cout<<"Set C4"<<endl;
  ILP_SpinLock_constraint_4(lp, vars);
  // cout<<"Set C5"<<endl;
  ILP_SpinLock_constraint_5(lp, vars);
  // cout<<"Set C6"<<endl;
  ILP_SpinLock_constraint_6(lp, vars);
  // cout<<"Set C7"<<endl;
  ILP_SpinLock_constraint_7(lp, vars);
  // cout<<"Set C8"<<endl;
  ILP_SpinLock_constraint_8(lp, vars);
  // cout<<"Set C9"<<endl;
  ILP_SpinLock_constraint_9(lp, vars);
  // cout<<"Set C10"<<endl;
  ILP_SpinLock_constraint_10(lp, vars);
  // cout<<"Set C11"<<endl;
  ILP_SpinLock_constraint_11(lp, vars);
  // cout<<"Set C12"<<endl;
  ILP_SpinLock_constraint_12(lp, vars);
  // cout<<"Set C13"<<endl;
  ILP_SpinLock_constraint_13(lp, vars);
  // cout<<"Set C14"<<endl;
  ILP_SpinLock_constraint_14(lp, vars);
  // cout<<"Set C15"<<endl;
  ILP_SpinLock_constraint_15(lp, vars);
  // cout<<"Set C16"<<endl;
  ILP_SpinLock_constraint_16(lp, vars);
  // cout<<"Set C17"<<endl;
  ILP_SpinLock_constraint_17(lp, vars);
  // cout<<"Set C18"<<endl;
  ILP_SpinLock_constraint_18(lp, vars);
  // cout<<"Set C19"<<endl;
  ILP_SpinLock_constraint_19(lp, vars);
  // cout<<"Set all constraints."<<endl;
}

/** Expressions */
void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_1(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), ti) {
    LinearExpression* exp = new LinearExpression();
    uint i = ti->get_index();
    for (uint k = 1; k <= p_num; k++) {
      uint var_id;
      var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, i, k);
      exp->add_term(var_id, 1);
      lp->declare_variable_binary(var_id);
    }

    lp->add_equality(exp, 1);
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_2(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  uint t_num = tasks.get_tasks().size();

  foreach(tasks.get_tasks(), ti) {
    LinearExpression* exp = new LinearExpression();
    uint i = ti->get_index();
    for (uint p = 1; p <= t_num; p++) {
      uint var_id;
      var_id = vars->lookup(ILPSpinLockMapper::PRIORITY_ASSIGNMENT, i, p);
      exp->add_term(var_id, 1);
      lp->declare_variable_binary(var_id);
    }
    lp->add_equality(exp, 1);
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_2_1(
    LinearProgram* lp, ILPSpinLockMapper* vars) {
  uint t_num = tasks.get_tasks().size();

  for (uint p = 1; p <= t_num; p++) {
    LinearExpression* exp = new LinearExpression();
    foreach(tasks.get_tasks(), ti) {
      uint i = ti->get_index();
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::PRIORITY_ASSIGNMENT, i, p);
      exp->add_term(var_id, 1);
    }
    lp->add_equality(exp, 1);
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_3(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();
      for (uint k = 1; k <= p_num; k++) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, i, k);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, x, k);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(ILPSpinLockMapper::SAME_LOCALITY, i, x);
        exp->sub_term(var_id, 1);
        lp->declare_variable_binary(var_id);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_4(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      for (uint p = 1; p <= t_num - 1; p++) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, i, x);
        exp->add_term(var_id, 1);
        lp->declare_variable_binary(var_id);

        var_id = vars->lookup(ILPSpinLockMapper::PRIORITY_ASSIGNMENT, i, p);
        exp->add_term(var_id, 1);

        for (uint j = p + 1; j <= t_num; j++) {
          var_id = vars->lookup(ILPSpinLockMapper::PRIORITY_ASSIGNMENT, x, j);
          exp->sub_term(var_id, 1);
        }

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_5(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, i, x);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, x, i);
      exp->add_term(var_id, 1);

      lp->add_equality(exp, 1);
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_6(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  foreach(tasks.get_tasks(), task) {
    uint i = task->get_index();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    ulong upper_bound = task->get_deadline();

    var_id = vars->lookup(ILPSpinLockMapper::RESPONSE_TIME, i);
    exp->add_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    // cout<<"var_id:"<<var_id<<endl;
    // cout<<"R_"<<i<<" lower bound:"<<task->get_wcet()<<endl;
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    lp->add_inequality(exp, upper_bound);
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_7(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  foreach(tasks.get_tasks(), task) {
    uint i = task->get_index();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPSpinLockMapper::RESPONSE_TIME, i);
    exp->add_term(var_id, 1);

    var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i);
    exp->sub_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(ILPSpinLockMapper::SPIN_TIME, i);
    exp->sub_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(ILPSpinLockMapper::INTERFERENCE_TIME, i);
    exp->sub_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);
    // cout<<"wcet:"<<task->get_wcet()<<endl;
    lp->add_equality(exp, task->get_wcet());
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_8(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPSpinLockMapper::INTERFERENCE_TIME, i);
    exp->add_term(var_id, 1);

    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();

      var_id = vars->lookup(ILPSpinLockMapper::MAX_PREEMEPT, i, x);
      exp->sub_term(var_id, tx->get_wcet());
      lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    lp->add_equality(exp, 0);
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_9(LinearProgram* lp,
                                                     ILPSpinLockMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      uint upper_bound = ceiling(ti->get_deadline(), tx->get_period());

      var_id = vars->lookup(ILPSpinLockMapper::MAX_PREEMEPT, i, x);
      exp->add_term(var_id, 1);

      lp->add_inequality(exp, upper_bound);
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_10(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      uint c1 = ceiling(ti->get_deadline(), tx->get_period());
      double c2 =
          static_cast<double>(ti->get_response_time()) / (tx->get_period());

      var_id = vars->lookup(ILPSpinLockMapper::SAME_LOCALITY, i, x);
      exp->add_term(var_id, c1);

      var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, i, x);
      exp->sub_term(var_id, c1);

      var_id = vars->lookup(ILPSpinLockMapper::MAX_PREEMEPT, i, x);
      exp->sub_term(var_id, 1);

      lp->add_inequality(exp, c1 - c2);
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_11(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    LinearExpression* exp = new LinearExpression();
    uint var_id;
    var_id = vars->lookup(ILPSpinLockMapper::SPIN_TIME, i);
    exp->add_term(var_id, 1);

    for (uint k = 1; k <= p_num; k++) {
      var_id = vars->lookup(ILPSpinLockMapper::SPIN_TIME, i, k);
      exp->sub_term(var_id, 1);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }
    lp->add_equality(exp, 0);
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_12(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    for (uint k = 1; k <= p_num; k++) {
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::SPIN_TIME, i, k);
      exp->add_term(var_id, 1);

      for (uint q = 1; q <= r_num; q++) {
        var_id = vars->lookup(ILPSpinLockMapper::SPIN_TIME, i, k, q);
        exp->sub_term(var_id, 1);
        // lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
      }

      lp->add_equality(exp, 0);
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_13(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint t_num = tasks.get_taskset_size();
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    ulong max_L = 0;
    uint max_N = 0;

    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      foreach(tx->get_requests(), request) {
        if (request->get_num_requests() > max_N)
          max_N = request->get_num_requests();
        if (request->get_max_length() > max_L)
          max_L = request->get_max_length();
      }
    }
    ulong M = max_L * t_num * max_N;

    foreach_task_except(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();

      for (uint q = 1; q <= r_num; q++) {
        ulong L_x_q;
        uint N_i_q;
        if (tx->is_request_exist(q - 1)) {
          const Request& request_x = tx->get_request_by_id(q - 1);
          L_x_q = request_x.get_max_length();
        } else {
          L_x_q = 0;
        }

        if (ti->is_request_exist(q - 1)) {
          const Request& request_i = ti->get_request_by_id(q - 1);
          N_i_q = request_i.get_num_requests();
        } else {
          N_i_q = 0;
        }

        for (uint k = 1; k <= p_num; k++) {
          LinearExpression* exp = new LinearExpression();
          uint var_id;

          var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, x, k);
          exp->add_term(var_id, M);

          foreach_higher_priority_task(tasks.get_tasks(), (*ti), th) {
            uint h = th->get_index();
            uint N_h_q;
            if (th->is_request_exist(q - 1)) {
              const Request& request_h = th->get_request_by_id(q - 1);
              N_h_q = request_h.get_num_requests();
            } else {
              N_h_q = 0;
            }

            var_id = vars->lookup(ILPSpinLockMapper::MAX_PREEMEPT, i, h);
            exp->add_term(var_id, L_x_q * N_h_q);
          }

          var_id = vars->lookup(ILPSpinLockMapper::SPIN_TIME, i, k, q);
          exp->sub_term(var_id, 1);

          lp->add_inequality(exp, M - L_x_q * N_i_q);
        }
      }
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_14(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    for (uint q = 1; q <= r_num; q++) {
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i);
      exp->sub_term(var_id, 1);

      var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i, q);
      exp->add_term(var_id, 1);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);

      lp->add_inequality(exp, 0);
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_15(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    for (uint q = 1; q <= r_num; q++) {
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i, q);
      exp->sub_term(var_id, 1);

      for (uint k = 1; k <= r_num; k++) {
        var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i, q, k);
        exp->add_term(var_id, 1);
        // lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
      }

      lp->add_equality(exp, 0);
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_16(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    for (uint q = 1; q <= r_num; q++) {
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::AB_DEISION, i, q);
      lp->declare_variable_binary(var_id);

      foreach_task_except(tasks.get_tasks(), (*ti), tx) {
        uint x = tx->get_index();
        if (tx->is_request_exist(q - 1)) {
          foreach_higher_priority_task(tasks.get_tasks(), (*ti), th) {
            uint h = th->get_index();

            if (th->is_request_exist(q - 1)) {
              LinearExpression* exp = new LinearExpression();

              var_id = vars->lookup(ILPSpinLockMapper::AB_DEISION, i, q);
              exp->sub_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::SAME_LOCALITY, x, i);
              exp->add_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::SAME_LOCALITY, i, h);
              exp->add_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, i, x);
              exp->add_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, i, h);
              exp->sub_term(var_id, 1);

              lp->add_inequality(exp, 2);
            }
          }
        }
      }
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_17(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    for (uint q = 1; q <= r_num; q++) {
      uint var_id;

      var_id = vars->lookup(ILPSpinLockMapper::AB_DEISION, i, q);
      lp->declare_variable_binary(var_id);

      foreach_task_except(tasks.get_tasks(), (*ti), tx) {
        uint x = tx->get_index();
        if (tx->is_request_exist(q - 1)) {
          foreach_higher_priority_task(tasks.get_tasks(), (*ti), th) {
            uint h = th->get_index();

            if (th->is_request_exist(q - 1)) {
              LinearExpression* exp = new LinearExpression();

              var_id = vars->lookup(ILPSpinLockMapper::AB_DEISION, i, q);
              exp->sub_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::SAME_LOCALITY, x, i);
              exp->add_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::SAME_LOCALITY, h, i);
              exp->sub_term(var_id, 1);

              var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, i, x);
              exp->add_term(var_id, 1);

              lp->add_inequality(exp, 1);
            }
          }
        }
      }
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_18(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    for (uint q = 1; q <= r_num; q++) {
      for (uint k = 1; k <= p_num; k++) {
        foreach(tasks.get_tasks(), tx) {
          LinearExpression* exp = new LinearExpression();
          uint var_id;

          uint x = tx->get_index();
          ulong L_x_q = 0;
          if (tx->is_request_exist(q - 1)) {
            const Request& request = tx->get_request_by_id(q - 1);
            L_x_q = request.get_max_length();
          }

          var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, x, k);
          exp->sub_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, i, k);
          exp->add_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::AB_DEISION, i, q);
          exp->add_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::HIGHER_PRIORITY, x, i);
          exp->sub_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i, q, k);
          exp->sub_term(var_id, 1);

          lp->add_inequality(exp, L_x_q);
        }
      }
    }
  }
}

void ILP_RTA_PFP_spinlock::ILP_SpinLock_constraint_19(LinearProgram* lp,
                                                      ILPSpinLockMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    for (uint q = 1; q <= r_num; q++) {
      for (uint k = 1; k <= p_num; k++) {
        foreach(tasks.get_tasks(), tx) {
          LinearExpression* exp = new LinearExpression();
          uint var_id;

          uint x = tx->get_index();
          ulong L_x_q = 0;
          if (tx->is_request_exist(q - 1)) {
            const Request& request = tx->get_request_by_id(q - 1);
            L_x_q = request.get_max_length();
          }

          var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, x, k);
          exp->add_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::LOCALITY_ASSIGNMENT, i, k);
          exp->sub_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::AB_DEISION, i, q);
          exp->add_term(var_id, L_x_q);

          var_id = vars->lookup(ILPSpinLockMapper::BLOCKING_TIME, i, q, k);
          exp->sub_term(var_id, 1);

          lp->add_inequality(exp, L_x_q);
        }
      }
    }
  }
}

bool ILP_RTA_PFP_spinlock::alloc_schedulable() {}
