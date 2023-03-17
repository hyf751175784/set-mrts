// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <ilp_rta_pfp_rop_comp.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <solution.h>
#include <lp.h>
#include <iostream>
#include <sstream>
#include <string>

using std::ostringstream;

/** Class ILPROPCOMPMapper */
uint64_t ILPROPCOMPMapper::encode_request(uint64_t type, uint64_t part_1,
                                             uint64_t part_2, uint64_t part_3,
                                             uint64_t part_4) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(type < (one << 5));
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

uint64_t ILPROPCOMPMapper::get_type(uint64_t var) {
  return (var >> 40) & (uint64_t)0x1f;  // 5 bits
}

uint64_t ILPROPCOMPMapper::get_part_1(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPROPCOMPMapper::get_part_2(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPROPCOMPMapper::get_part_3(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPROPCOMPMapper::get_part_4(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

ILPROPCOMPMapper::ILPROPCOMPMapper(uint start_var) : VarMapperBase(start_var) {}

uint ILPROPCOMPMapper::lookup(uint type, uint part_1, uint part_2, uint part_3,
                          uint part_4) {
  uint64_t key = encode_request(type, part_1, part_2, part_3, part_4);

  // cout<<"string:"<<key2str(key)<<endl;
  uint var = var_for_key(key);
  return var;
}

string ILPROPCOMPMapper::var2str(unsigned int var) const {
  uint64_t key;

  if (search_key_for_var(var, key))
    return key2str(key);
  else
    return "<?>";
}

string ILPROPCOMPMapper::key2str(uint64_t key) const {
  ostringstream buf;

  switch (get_type(key)) {
    case ILPROPCOMPMapper::SAME_LOCALITY:
      buf << "S[";
      break;
      /*		case ILPROPCOMPMapper::LOCALITY_ASSIGNMENT:
                              buf << "A[";
                              break;
                      case ILPROPCOMPMapper::RESOURCE_ASSIGNMENT:
                              buf << "Q[";
                              break;
                      case ILPROPCOMPMapper::SAME_TASK_LOCALITY:
                              buf << "U[";
                              break;
                      case ILPROPCOMPMapper::SAME_RESOURCE_LOCALITY:
                              buf << "V[";
                              break;
                      case ILPROPCOMPMapper::SAME_TR_LOCALITY:
                              buf << "W[";
                              break;
      //		case ILPROPCOMPMapper::APPLICATION_CORE:
      //			buf << "AC[";
      //			break;
      */
    case ILPROPCOMPMapper::PREEMPT_NUM:
      buf << "P[";
      break;
    case ILPROPCOMPMapper::TBT_PREEMPT_NUM:
      buf << "H[";
      break;
    case ILPROPCOMPMapper::RBT_PREEMPT_NUM:
      buf << "H[";
      break;
    case ILPROPCOMPMapper::RBR_PREEMPT_NUM:
      buf << "H[";
      break;
    case ILPROPCOMPMapper::RESPONSE_TIME:
      buf << "R[";
      break;
    case ILPROPCOMPMapper::BLOCKING_TIME:
      buf << "B[";
      break;
    case ILPROPCOMPMapper::REQUEST_BLOCKING_TIME:
      buf << "b[";
      break;
    case ILPROPCOMPMapper::INTERFERENCE_TIME_R:
      buf << "I_R[";
      break;
    case ILPROPCOMPMapper::INTERFERENCE_TIME_R_RESOURCE:
      buf << "I_R[";
      break;
    case ILPROPCOMPMapper::INTERFERENCE_TIME_C:
      buf << "I_C[";
      break;
    case ILPROPCOMPMapper::INTERFERENCE_TIME_C_TASK:
      buf << "I_C[";
      break;
    case ILPROPCOMPMapper::INTERFERENCE_TIME_C_RESOURCE:
      buf << "I_C[";
      break;
    default:
      buf << "?[";
  }

  buf << get_part_1(key) << ", " << get_part_2(key) << ", " << get_part_3(key)
      << ", " << get_part_4(key) << "]";

  return buf.str();
}

/** Class ILP_RTA_PFP_ROP_COMP */
ILP_RTA_PFP_ROP_COMP::ILP_RTA_PFP_ROP_COMP()
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "DPCP") {}

ILP_RTA_PFP_ROP_COMP::ILP_RTA_PFP_ROP_COMP(TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "DPCP") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;
    cout << "11111" << endl;

  this->resources.update(&(this->tasks));
    cout << "22222" << endl;
  this->processors.update(&(this->tasks), &(this->resources));
    cout << "33333" << endl;

  this->tasks.RM_Order();
    cout << "44444" << endl;
  this->processors.init();
    cout << "55555" << endl;
}

ILP_RTA_PFP_ROP_COMP::~ILP_RTA_PFP_ROP_COMP() {}

bool ILP_RTA_PFP_ROP_COMP::is_schedulable() {
  if (0 == tasks.get_tasks().size())
    return true;
  else if (tasks.get_utilization_sum() -
               processors.get_processor_num() >=
           _EPS)
    return false;

  ILPROPCOMPMapper vars;
  LinearProgram response_bound;

  set_objective(&response_bound, &vars);

  add_constraints(&response_bound, &vars);

  // cout<<"var num:"<<vars.get_num_vars()<<endl;

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

    uint p_num = processors.get_processor_num();
    uint r_num = resources.get_resourceset_size();
    uint t_num = tasks.get_taskset_size();

    cout << "|===========SAME_LOCALITY===========|" << endl;
    for (uint i = 0; i < t_num + r_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      if (i == t_num) {
        for (uint k = 0; k < t_num + r_num; k++) cout << "---";
        cout << endl;
      }
      double sum = 0;
      for (uint j = 0; j < t_num + r_num; j++) {
        if (j == t_num) cout << "|";
        construct_exp(&vars, exp, ILPROPCOMPMapper::SAME_LOCALITY, i, j);
        result = rb_solution->evaluate(*exp);
        cout << result << "  ";
        if (j < t_num) {
          Task& task = tasks.get_task_by_index(j);
          double u = task.get_utilization();
          sum += u * result;
        }
      }
      cout << "\t" << sum;
      cout << endl;
    }

    cout << "|===========NOT_ALONE===========|" << endl;
    for (uint i = 0; i < t_num + r_num - 1; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;

      construct_exp(&vars, exp, ILPROPCOMPMapper::NOT_ALONE, i);
      result = rb_solution->evaluate(*exp);

      cout << "Element " << i << ":" << result << endl;
    }

    cout << "|===========TASK_PREEMPTION===========|" << endl;
    for (uint i = 0; i < t_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      for (uint j = 0; j < t_num; j++) {
        construct_exp(&vars, exp, ILPROPCOMPMapper::PREEMPT_NUM, i, j);
        result = rb_solution->evaluate(*exp);
        if (result < 1)
          result = 0;
        cout << result << "  ";
      }
      cout << endl;
    }

    cout << "|===========TASK_PREEMPTION_H===========|" << endl;
    for (uint i = 0; i < t_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      for (uint j = 0; j < t_num; j++) {
        construct_exp(&vars, exp, ILPROPCOMPMapper::TBT_PREEMPT_NUM, i, j);
        result = rb_solution->evaluate(*exp);
        if (result < 1)
          result = 0;
        cout << result << "  ";
      }
      cout << endl;
    }

    for (uint i = 0; i < t_num; i++) {
      Task& task = tasks.get_task_by_index(i);
      cout << "/===========Task " << i << "===========/" << endl;
      LinearExpression* exp = new LinearExpression();
      double result;

      cout << "Res:";

      foreach(task.get_requests(), request) {
        uint u = request->get_resource_id();
        construct_exp(&vars, exp, ILPROPCOMPMapper::RESPONSE_TIME_CS, i, t_num + u);
        result = rb_solution->evaluate(*exp);
        cout << "request " << u << " holding time:" << result << endl;
      }
      

      construct_exp(&vars, exp, ILPROPCOMPMapper::RESPONSE_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "R_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPCOMPMapper::BLOCKING_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "B_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPCOMPMapper::INTERFERENCE_TIME_C, i);
      result = rb_solution->evaluate(*exp);
      cout << "INCS_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPCOMPMapper::INTERFERENCE_TIME_R, i);
      result = rb_solution->evaluate(*exp);
      cout << "ICS_" << i << ":" << result << endl;

      cout << "C_" << i << ":" << tasks.get_tasks()[i].get_wcet() << endl;
      cout << "D_" << i << ":" << tasks.get_tasks()[i].get_deadline() << endl;
      delete exp;
    }
#endif

    rb_solution->show_error();
    cout << rb_solution->get_status() << endl;
    set_status(rb_solution->get_status());
    delete rb_solution;
    return true;
  } else {
    uint t_num = tasks.get_taskset_size();

    #if ILP_SOLUTION_VAR_CHECK == 1

    uint p_num = processors.get_processor_num();
    uint r_num = resources.get_resourceset_size();

    cout << "|===========SAME_LOCALITY===========|" << endl;
    for (uint i = 0; i < t_num + r_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      if (i == t_num) {
        for (uint k = 0; k < t_num + r_num; k++) cout << "---";
        cout << endl;
      }
      double sum = 0;
      for (uint j = 0; j < t_num + r_num; j++) {
        if (j == t_num) cout << "|";
        construct_exp(&vars, exp, ILPROPCOMPMapper::SAME_LOCALITY, i, j);
        result = rb_solution->evaluate(*exp);
        cout << result << "  ";
        if (j < t_num) {
          Task& task = tasks.get_task_by_index(j);
          double u = task.get_utilization();
          sum += u * result;
        }
      }
      cout << "\t" << sum;
      cout << endl;
    }

    cout << "|===========NOT_ALONE===========|" << endl;
    for (uint i = 0; i < t_num + r_num - 1; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;

      construct_exp(&vars, exp, ILPROPCOMPMapper::NOT_ALONE, i);
      result = rb_solution->evaluate(*exp);

      cout << "Element " << i << ":" << result << endl;
    }

    cout << "|===========TASK_PREEMPTION===========|" << endl;
    for (uint i = 0; i < t_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      for (uint j = 0; j < t_num; j++) {
        construct_exp(&vars, exp, ILPROPCOMPMapper::PREEMPT_NUM, i, j);
        result = rb_solution->evaluate(*exp);
        if (result < 1)
          result = 0;
        cout << result << "  ";
      }
      cout << endl;
    }

    cout << "|===========TASK_PREEMPTION_H===========|" << endl;
    for (uint i = 0; i < t_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      for (uint j = 0; j < t_num; j++) {
        construct_exp(&vars, exp, ILPROPCOMPMapper::TBT_PREEMPT_NUM, i, j);
        result = rb_solution->evaluate(*exp);
        if (result < 1)
          result = 0;
        cout << result << "  ";
      }
      cout << endl;
    }

    for (uint i = 0; i < t_num; i++) {
      Task& task = tasks.get_task_by_index(i);
      cout << "/===========Task " << i << "===========/" << endl;
      LinearExpression* exp = new LinearExpression();
      double result;

      cout << "Res:";

      foreach(task.get_requests(), request) {
        uint u = request->get_resource_id();
        construct_exp(&vars, exp, ILPROPCOMPMapper::RESPONSE_TIME_CS, i, t_num + u);
        result = rb_solution->evaluate(*exp);
        cout << "request " << u << " holding time:" << result << endl;
      }
      

      construct_exp(&vars, exp, ILPROPCOMPMapper::RESPONSE_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "R_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPCOMPMapper::BLOCKING_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "B_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPCOMPMapper::INTERFERENCE_TIME_C, i);
      result = rb_solution->evaluate(*exp);
      cout << "INCS_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPCOMPMapper::INTERFERENCE_TIME_R, i);
      result = rb_solution->evaluate(*exp);
      cout << "ICS_" << i << ":" << result << endl;

      cout << "C_" << i << ":" << tasks.get_tasks()[i].get_wcet() << endl;
      cout << "D_" << i << ":" << tasks.get_tasks()[i].get_deadline() << endl;
      delete exp;
    }
#endif
    for (uint i = 0; i < t_num; i++) {
      Task& task = tasks.get_task_by_index(i);
      cout << "/===========Task " << i << "===========/" << endl;
      cout << "Res:";

      foreach(task.get_requests(), request)
        cout << request->get_resource_id() << "\t"
             << request->get_num_requests() << "\t" << request->get_max_length()
             << endl;
      cout << "C_" << i << ":" << tasks.get_tasks()[i].get_wcet() << endl;
      cout << "D_" << i << ":" << tasks.get_tasks()[i].get_deadline() << endl;
    }

    rb_solution->show_error();
    cout << rb_solution->get_status() << endl;
    set_status(rb_solution->get_status());
  }

  delete rb_solution;
  return false;
}

void ILP_RTA_PFP_ROP_COMP::construct_exp(ILPROPCOMPMapper* vars, LinearExpression* exp,
                                    uint type, uint part_1, uint part_2,
                                    uint part_3, uint part_4) {
  exp->get_terms().clear();
  uint var_id;

  var_id = vars->lookup(type, part_1, part_2, part_3, part_4);
  exp->add_term(var_id, 1);

#if ILP_SOLUTION_VAR_CHECK == 1
// cout<<vars.var2str(var_id)<<":"<<endl;
#endif
}

void ILP_RTA_PFP_ROP_COMP::set_objective(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  // any objective function is okay.
  // assert(0 < tasks.get_tasks().size());

  LinearExpression* obj = new LinearExpression();
  uint var_id;

  // uint i = tasks.get_taskset_size() - 1;
  // var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, i);
  // obj->add_term(var_id, 1);
  for (uint i = 0; i < tasks.get_taskset_size(); i ++) {
    var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, i);
    obj->add_term(var_id, 1);
  }

  lp->set_objective(obj);
}

void ILP_RTA_PFP_ROP_COMP::add_constraints(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  constraint_1(lp, vars);
  constraint_2(lp, vars);
  constraint_3(lp, vars);
  constraint_4(lp, vars);
  constraint_5(lp, vars);
  constraint_6(lp, vars);
  constraint_7(lp, vars);
  constraint_7_1(lp, vars);
  constraint_8(lp, vars);
  // constraint_8_1(lp, vars);
  constraint_8_2(lp, vars);
  // constraint_8_3(lp, vars);
  constraint_9(lp, vars);
  constraint_10(lp, vars);
  constraint_11(lp, vars);

  constraint_12(lp, vars);
  constraint_13(lp, vars);

  constraint_14(lp, vars);
  constraint_15(lp, vars);
  constraint_16(lp, vars);

  constraint_17(lp, vars);
  constraint_18(lp, vars);

  constraint_19(lp, vars);
  // constraint_20(lp, vars);
}

/** Expressions */
// C1
void ILP_RTA_PFP_ROP_COMP::constraint_1(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num; i++) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, i);
    exp->add_term(var_id, 1);
    lp->declare_variable_binary(var_id);

    lp->add_equality(exp, 1);
  }
}

// C2
void ILP_RTA_PFP_ROP_COMP::constraint_2(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    for (uint j = i + 1; j < r_num + t_num; j++) {
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, j);
      exp->add_term(var_id, 1);
      lp->declare_variable_binary(var_id);

      var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, j, i);
      exp->add_term(var_id, -1);
      lp->declare_variable_binary(var_id);

      lp->add_equality(exp, 0);
    }
  }
}

// C3
void ILP_RTA_PFP_ROP_COMP::constraint_3(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    for (uint j = i + 1; j < r_num + t_num; j++) {
      for (uint k = 0; k < r_num + t_num; k++) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, j);
        exp->add_term(var_id, -1);

        var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, k, i);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, k, j);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

// C4
void ILP_RTA_PFP_ROP_COMP::constraint_4(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  int p_num = processors.get_processor_num();
  int r_num = resources.get_resourceset_size();
  int t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::NOT_ALONE, i);
    exp->add_term(var_id, 1);
    lp->declare_variable_binary(var_id);

    for (uint j = i + 1; j < r_num + t_num; j++) {
      var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, j);
      exp->add_term(var_id, -1);
    }

    lp->add_inequality(exp, 0);
  }
}

// C5
void ILP_RTA_PFP_ROP_COMP::constraint_5(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  int p_num = processors.get_processor_num();
  int r_num = resources.get_resourceset_size();
  int t_num = tasks.get_taskset_size();

  LinearExpression* exp = new LinearExpression();
  uint var_id;

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    var_id = vars->lookup(ILPROPCOMPMapper::NOT_ALONE, i);
    exp->add_term(var_id, -1);
  }

  lp->add_inequality(exp, p_num - (r_num + t_num));
}

// C6
void ILP_RTA_PFP_ROP_COMP::constraint_6(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong deadline = ti->get_deadline();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, i);
    exp->add_term(var_id, 1);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    lp->add_inequality(exp, deadline);
  }
}

// C7
void ILP_RTA_PFP_ROP_COMP::constraint_7(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong wcet = ti->get_wcet();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, i);
    exp->add_term(var_id, 1);
    // lp->declare_variable_integer(var_id);

    var_id = vars->lookup(ILPROPCOMPMapper::BLOCKING_TIME, i);
    exp->add_term(var_id, -1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_C, i);
    exp->add_term(var_id, -1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_R, i);
    exp->add_term(var_id, -1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    lp->add_equality(exp, wcet);
  }
}

// C7-1
void ILP_RTA_PFP_ROP_COMP::constraint_7_1(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong wcet = ti->get_wcet();
    ulong deadline = ti->get_deadline();

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      int64_t L_i_u = request_u->get_max_length();

      var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME_CS, i, t_num + u);
      exp->add_term(var_id, 1);
      lp->declare_variable_bounds(var_id, true, 0, true, deadline);

      var_id = vars->lookup(ILPROPCOMPMapper::REQUEST_BLOCKING_TIME, i, t_num + u);
      exp->add_term(var_id, -1);

      foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
        uint x = tx->get_index();

        foreach(tx->get_requests(), request_v) {
          uint v = request_v->get_resource_id();

          int64_t L_x_v = request_v->get_max_length();

          var_id = vars->lookup(ILPROPCOMPMapper::RBR_PREEMPT_NUM, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, -1 * L_x_v);
        }
      }

      lp->add_equality(exp, L_i_u);
    }
  }
}

// C8
void ILP_RTA_PFP_ROP_COMP::constraint_8(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong cx = tx->get_wcet();
      ulong px = tx->get_period();

      uint bound = ceiling(pi + px, px);
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM, i, x);
      exp->add_term(var_id, -1);
      lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, true, bound);

      var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, i);
      exp->add_term(var_id, 1.0 / px);

      var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, x);
      exp->add_term(var_id, 1.0 / px);

      double ux = cx;
      ux /= px;

      lp->add_inequality(exp, ux);
    }
  }
}
// C8_1
void ILP_RTA_PFP_ROP_COMP::constraint_8_1(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong cx = tx->get_wcet();
      ulong px = tx->get_period();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM, i, x);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, i);
      exp->add_term(var_id, -1.0 / px);

      var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, x);
      exp->add_term(var_id, -1.0 / px);

      double ux = cx;
      ux /= px;

      lp->add_inequality(exp, 1.0-ux);
    }
  }
}

// C8-2
void ILP_RTA_PFP_ROP_COMP::constraint_8_2(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();

      foreach(tasks.get_tasks(), tx) {
        uint x = tx->get_index();
        ulong cx = tx->get_wcet();
        ulong px = tx->get_period();

        foreach(tx->get_requests(), request_v) {
          uint v = request_v->get_resource_id();
          uint bound = ceiling(pi + px, px);
          int64_t A_x_v =
              request_v->get_num_requests() * request_v->get_max_length();

          LinearExpression* exp = new LinearExpression();
          uint var_id;

          var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM_CS, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, -1);
          lp->declare_variable_integer(var_id);
          lp->declare_variable_bounds(var_id, true, 0, true, bound);

          var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME_CS, i, t_num + u);
          exp->add_term(var_id, 1.0 / px);

          var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, x);
          exp->add_term(var_id, 1.0 / px);

          double ux = A_x_v;
          ux /= px;

          lp->add_inequality(exp, ux);
        }
      }
    }
  }
}

// C8-3
void ILP_RTA_PFP_ROP_COMP::constraint_8_3(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();

      foreach(tasks.get_tasks(), tx) {
        uint x = tx->get_index();
        ulong cx = tx->get_wcet();
        ulong px = tx->get_period();

        foreach(tx->get_requests(), request_v) {
          uint v = request_v->get_resource_id();
          uint bound = ceiling(pi + px, px);
          int64_t A_x_v =
              request_v->get_num_requests() * request_v->get_max_length();

          LinearExpression* exp = new LinearExpression();
          uint var_id;

          var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM_CS, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME_CS, i, t_num + u);
          exp->add_term(var_id, -1.0 / px);

          var_id = vars->lookup(ILPROPCOMPMapper::RESPONSE_TIME, x);
          exp->add_term(var_id, -1.0 / px);

          double ux = A_x_v;
          ux /= px;

          lp->add_inequality(exp, 1.0-ux);
        }
      }
    }
  }
}

// C9
void ILP_RTA_PFP_ROP_COMP::constraint_9(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong px = tx->get_period();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      uint bound = ceiling(pi + px, px);

      var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM, i, x);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(ILPROPCOMPMapper::TBT_PREEMPT_NUM, i, x);
      exp->add_term(var_id, -1);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, true, bound);

      var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, x);
      exp->add_term(var_id, bound);

      lp->add_inequality(exp, bound);
    }
  }
}

// C10
void ILP_RTA_PFP_ROP_COMP::constraint_10(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong px = tx->get_period();

      foreach(tx->get_requests(), request_v) {
        uint v = request_v->get_resource_id();
        int N_x_v = request_v->get_num_requests();

        LinearExpression* exp = new LinearExpression();
        uint var_id;

        uint bound = ceiling(pi + px, px);

        var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM, i, x);
        exp->add_term(var_id, N_x_v);

        var_id = vars->lookup(ILPROPCOMPMapper::RBT_PREEMPT_NUM, i, x, t_num + v);
        exp->add_term(var_id, -1);
        // lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, true, N_x_v * bound);

        var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, t_num + v);
        exp->add_term(var_id, N_x_v * bound);

        lp->add_inequality(exp, N_x_v * bound);
      }
    }
  }
}

// C11
void ILP_RTA_PFP_ROP_COMP::constraint_11(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong px = tx->get_period();

      foreach(ti->get_requests(), request_u) {
        uint u = request_u->get_resource_id();

        foreach(tx->get_requests(), request_v) {
          uint v = request_v->get_resource_id();
          int N_x_v = request_v->get_num_requests();

          LinearExpression* exp = new LinearExpression();
          uint var_id;

          uint bound = ceiling(pi + px, px);

          var_id = vars->lookup(ILPROPCOMPMapper::PREEMPT_NUM_CS, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, N_x_v);

          var_id = vars->lookup(ILPROPCOMPMapper::RBR_PREEMPT_NUM, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, -1);
          // lp->declare_variable_integer(var_id);
          lp->declare_variable_bounds(var_id, true, 0, true, N_x_v * bound);

          var_id =
              vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, t_num + u, t_num + v);
          exp->add_term(var_id, N_x_v * bound);

          // FOR TEST
          var_id =
              vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, t_num + v);
          exp->add_term(var_id, (-1) * N_x_v * bound);

          lp->add_inequality(exp, N_x_v * bound);
        }
      }
    }
  }
}

// C12
void ILP_RTA_PFP_ROP_COMP::constraint_12(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    foreach(ti->get_requests(), ru) {
      uint u = ru->get_resource_id();
      Resource& res_u = resources.get_resource_by_id(u);

      foreach_lower_priority_task(tasks.get_tasks(), (*ti), tx) {
        uint x = tx->get_index();
        foreach(tx->get_requests(), rv) {
          uint v = rv->get_resource_id();
          Resource& res_v = resources.get_resource_by_id(v);

          if (res_u.get_ceiling() >= res_v.get_ceiling()) {
            LinearExpression* exp = new LinearExpression();
            uint var_id;

            int64_t L_x_v = rv->get_max_length();

            var_id =
                vars->lookup(ILPROPCOMPMapper::REQUEST_BLOCKING_TIME, i, t_num + u);
            exp->add_term(var_id, -1);

            var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, t_num + u);
            exp->add_term(var_id, -L_x_v);

            var_id =
                vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, t_num + u, t_num + v);
            exp->add_term(var_id, L_x_v);

            lp->add_inequality(exp, 0);
          }
        }
      }
    }
  }
}

// C13
void ILP_RTA_PFP_ROP_COMP::constraint_13(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::BLOCKING_TIME, i);
    exp->add_term(var_id, 1);

    foreach(ti->get_requests(), request) {
      uint u = request->get_resource_id();

      int N_i_u = request->get_num_requests();

      var_id = vars->lookup(ILPROPCOMPMapper::REQUEST_BLOCKING_TIME, i, t_num + u);
      exp->add_term(var_id, -N_i_u);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    lp->add_equality(exp, 0);
  }
}

// C14
void ILP_RTA_PFP_ROP_COMP::constraint_14(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong di = ti->get_deadline();

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();
      ulong px = tx->get_period();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_C_TASK, i, x);
      exp->add_term(var_id, -1);

      int64_t NC_WCET_x = tx->get_wcet_non_critical_sections();

      var_id = vars->lookup(ILPROPCOMPMapper::TBT_PREEMPT_NUM, i, x);
      exp->add_term(var_id, NC_WCET_x);

      lp->add_equality(exp, 0);
      // lp->add_inequality(exp, 0);
    }
  }
}

// C15
void ILP_RTA_PFP_ROP_COMP::constraint_15(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong di = ti->get_deadline();

    foreach_task_except(tasks.get_tasks(), (*ti), ty) {
      uint y = ty->get_index();

      foreach(ty->get_requests(), request_u) {
        uint u = request_u->get_resource_id();
        int64_t L_y_u = request_u->get_max_length();
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_C_RESOURCE, i, y,
                              t_num + u);
        exp->add_term(var_id, -1);

        var_id = vars->lookup(ILPROPCOMPMapper::RBT_PREEMPT_NUM, i, y, t_num + u);
        exp->add_term(var_id, L_y_u);

        lp->add_equality(exp, 0);
        // lp->add_inequality(exp, 0);
      }
    }
  }
}

// C16
void ILP_RTA_PFP_ROP_COMP::constraint_16(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_C, i);
    exp->add_term(var_id, 1);

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();

      var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_C_TASK, i, x);
      exp->add_term(var_id, -1);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    foreach_task_except(tasks.get_tasks(), (*ti), ty) {
      uint y = ty->get_index();
      foreach(ty->get_requests(), request_u) {
        uint u = request_u->get_resource_id();

        var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_C_RESOURCE, i, y,
                              t_num + u);
        exp->add_term(var_id, -1);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
      }
    }

    lp->add_equality(exp, 0);
  }
}

// C17
void ILP_RTA_PFP_ROP_COMP::constraint_17(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_R_RESOURCE, i,
                            t_num + u);
      exp->add_term(var_id, -1);

      foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
        uint x = tx->get_index();
        ulong px = tx->get_period();

        foreach(tx->get_requests(), request_v) {
          uint v = request_v->get_resource_id();
          int N_x_v = request_v->get_num_requests();
          int64_t L_x_v = request_v->get_max_length();

          var_id = vars->lookup(ILPROPCOMPMapper::RBR_PREEMPT_NUM, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, L_x_v);

          int term = ceiling(pi + px, px);

          var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, t_num +u);
          exp->add_term(var_id, -1*term*N_x_v*L_x_v);
        }
      }

      lp->add_inequality(exp, 0);
    }
  }
}

// C18
void ILP_RTA_PFP_ROP_COMP::constraint_18(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_R, i);
    exp->add_term(var_id, 1);

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();
      int N_i_u = request_u->get_num_requests();

      var_id = vars->lookup(ILPROPCOMPMapper::INTERFERENCE_TIME_R_RESOURCE, i,
                            t_num + u);
      exp->add_term(var_id, -1 * N_i_u);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    lp->add_equality(exp, 0);
  }
}

void ILP_RTA_PFP_ROP_COMP::constraint_19(LinearProgram* lp, ILPROPCOMPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < t_num; i++) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    for (uint j = 0; j < t_num; j++) {
      Task& task = tasks.get_task_by_index(j);
      double utilization = task.get_utilization();

      var_id = vars->lookup(ILPROPCOMPMapper::SAME_LOCALITY, i, j);
      exp->add_term(var_id, utilization);
    }

    lp->add_inequality(exp, 1);
  }
}

void ILP_RTA_PFP_ROP_COMP::constraint_20(LinearProgram* lp, ILPROPCOMPMapper* vars) {}

bool ILP_RTA_PFP_ROP_COMP::alloc_schedulable() {}
