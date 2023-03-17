// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <ilp_rta_pfp_rop.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <solution.h>
#include <lp.h>
#include <iostream>
#include <sstream>
#include <string>

using std::ostringstream;

/** Class ILPROPMapper */
uint64_t ILPROPMapper::encode_request(uint64_t type, uint64_t part_1,
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

uint64_t ILPROPMapper::get_type(uint64_t var) {
  return (var >> 40) & (uint64_t)0x1f;  // 5 bits
}

uint64_t ILPROPMapper::get_part_1(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPROPMapper::get_part_2(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPROPMapper::get_part_3(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t ILPROPMapper::get_part_4(uint64_t var) {
  return var & (uint64_t)0x3ff;  // 10 bits
}

ILPROPMapper::ILPROPMapper(uint start_var) : VarMapperBase(start_var) {}

uint ILPROPMapper::lookup(uint type, uint part_1, uint part_2, uint part_3,
                          uint part_4) {
  uint64_t key = encode_request(type, part_1, part_2, part_3, part_4);

  // cout<<"string:"<<key2str(key)<<endl;
  uint var = var_for_key(key);
  return var;
}

string ILPROPMapper::var2str(unsigned int var) const {
  uint64_t key;

  if (search_key_for_var(var, key))
    return key2str(key);
  else
    return "<?>";
}

string ILPROPMapper::key2str(uint64_t key) const {
  ostringstream buf;

  switch (get_type(key)) {
    case ILPROPMapper::SAME_LOCALITY:
      buf << "S[";
      break;
      /*		case ILPROPMapper::LOCALITY_ASSIGNMENT:
                              buf << "A[";
                              break;
                      case ILPROPMapper::RESOURCE_ASSIGNMENT:
                              buf << "Q[";
                              break;
                      case ILPROPMapper::SAME_TASK_LOCALITY:
                              buf << "U[";
                              break;
                      case ILPROPMapper::SAME_RESOURCE_LOCALITY:
                              buf << "V[";
                              break;
                      case ILPROPMapper::SAME_TR_LOCALITY:
                              buf << "W[";
                              break;
      //		case ILPROPMapper::APPLICATION_CORE:
      //			buf << "AC[";
      //			break;
      */
    case ILPROPMapper::PREEMPT_NUM:
      buf << "P[";
      break;
    case ILPROPMapper::TBT_PREEMPT_NUM:
      buf << "H[";
      break;
    case ILPROPMapper::RBT_PREEMPT_NUM:
      buf << "H[";
      break;
    case ILPROPMapper::RBR_PREEMPT_NUM:
      buf << "H[";
      break;
    case ILPROPMapper::RESPONSE_TIME:
      buf << "R[";
      break;
    case ILPROPMapper::BLOCKING_TIME:
      buf << "B[";
      break;
    case ILPROPMapper::REQUEST_BLOCKING_TIME:
      buf << "b[";
      break;
    case ILPROPMapper::INTERFERENCE_TIME_R:
      buf << "I_R[";
      break;
    case ILPROPMapper::INTERFERENCE_TIME_R_RESOURCE:
      buf << "I_R[";
      break;
    case ILPROPMapper::INTERFERENCE_TIME_C:
      buf << "I_C[";
      break;
    case ILPROPMapper::INTERFERENCE_TIME_C_TASK:
      buf << "I_C[";
      break;
    case ILPROPMapper::INTERFERENCE_TIME_C_RESOURCE:
      buf << "I_C[";
      break;
    default:
      buf << "?[";
  }

  buf << get_part_1(key) << ", " << get_part_2(key) << ", " << get_part_3(key)
      << ", " << get_part_4(key) << "]";

  return buf.str();
}

/** Class ILP_RTA_PFP_ROP */
ILP_RTA_PFP_ROP::ILP_RTA_PFP_ROP()
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "DPCP") {}

ILP_RTA_PFP_ROP::ILP_RTA_PFP_ROP(TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "DPCP") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;
    // cout << "11111" << endl;

  this->resources.update(&(this->tasks));
    // cout << "22222" << endl;
  this->processors.update(&(this->tasks), &(this->resources));
    // cout << "33333" << endl;

  this->tasks.RM_Order();
    // cout << "44444" << endl;
  this->processors.init();
    // cout << "55555" << endl;

  // specific_partition();

  // foreach(this->tasks.get_tasks(), task) {
  //   cout << "Task" << task->get_id()
  //         << ": partition:" << task->get_partition()
  //         << ": priority:" << task->get_priority() << endl;
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
  //           << this->resources.get_resource_by_id(request->get_resource_id())
  //                 .get_locality()
  //           << endl;
  //   }
  //   cout << "-------------------------------------------" << endl;
  //   if (task->get_wcet() > task->get_response_time()) exit(0);
  // }
}

ILP_RTA_PFP_ROP::~ILP_RTA_PFP_ROP() {}

bool ILP_RTA_PFP_ROP::is_schedulable() {
  if (0 == tasks.get_tasks().size())
    return true;
  else if (tasks.get_utilization_sum() -
               processors.get_processor_num() >=
           _EPS)
    return false;



  ILPROPMapper vars;
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
    // foreach(resources.get_resources(), resource) {
    //   cout << "Resource:" << resource->get_resource_id()
    //         << " locality:" << resource->get_locality() << endl;
    //   if (p_num < resource->get_locality()) exit(0);
    // }

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
        construct_exp(&vars, exp, ILPROPMapper::SAME_LOCALITY, i, j);
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

      construct_exp(&vars, exp, ILPROPMapper::NOT_ALONE, i);
      result = rb_solution->evaluate(*exp);

      cout << "Element " << i << ":" << result << endl;
    }

    cout << "|===========TASK_PREEMPTION===========|" << endl;
    for (uint i = 0; i < t_num; i++) {
      LinearExpression* exp = new LinearExpression();
      double result;
      for (uint j = 0; j < t_num; j++) {
        construct_exp(&vars, exp, ILPROPMapper::PREEMPT_NUM, i, j);
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
        construct_exp(&vars, exp, ILPROPMapper::TBT_PREEMPT_NUM, i, j);
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

      cout << "Res:" << endl;

      foreach(task.get_requests(), request_u) {
        uint u = request_u->get_resource_id();
        construct_exp(&vars, exp, ILPROPMapper::RESPONSE_TIME_CS, i, t_num + u);
        result = rb_solution->evaluate(*exp);
        cout << "request " << u << " holding time:" << result << endl;
        // foreach_higher_priority_task(tasks.get_tasks(), task, th) {
        //   uint x = th->get_index();
        //   foreach(th->get_requests(), request_v) {
        //     uint v = request_v->get_resource_id();
        //     construct_exp(&vars, exp, ILPROPMapper::PREEMPT_NUM_CS, i, x, t_num + u, t_num + v);
        //     result = rb_solution->evaluate(*exp);
        //     cout << "P:" << result << "\t";
        //     construct_exp(&vars, exp, ILPROPMapper::RBR_PREEMPT_NUM, i, x, t_num + u, t_num + v);
        //     result = rb_solution->evaluate(*exp);
        //     cout << "H:" << result << "\t";
        //   }
        //   cout << endl;
        // }
      }


      // foreach_task_except(tasks.get_tasks(), task, ty) {
      //   uint index_i = task.get_index();
      //   uint index_y = ty->get_index();

      //   foreach(ty->get_requests(), r_v) {
      //     uint r_id = r_v->get_resource_id();

      //     construct_exp(&vars, exp, ILPROPMapper::RBT_PREEMPT_NUM, index_i, index_y, t_num + r_id);
      //     result = rb_solution->evaluate(*exp);
      //     cout << "H_" << index_i << "_" << index_y << "_" << r_id << ":" << result << "\t";
      //   }
      //   cout << endl;
      // }
      

      construct_exp(&vars, exp, ILPROPMapper::RESPONSE_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "R_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPMapper::BLOCKING_TIME, i);
      result = rb_solution->evaluate(*exp);
      cout << "B_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPMapper::INTERFERENCE_TIME_C, i);
      result = rb_solution->evaluate(*exp);
      cout << "INCS_" << i << ":" << result << endl;

      construct_exp(&vars, exp, ILPROPMapper::INTERFERENCE_TIME_R, i);
      result = rb_solution->evaluate(*exp);
      cout << "ICS_" << i << ":" << result << endl;

      cout << "NCS_C_" << i << ":" << tasks.get_tasks()[i].get_wcet_non_critical_sections() << endl;
      cout << "CS_C_" << i << ":" << tasks.get_tasks()[i].get_wcet_critical_sections() << endl;
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
        construct_exp(&vars, exp, ILPROPMapper::SAME_LOCALITY, i, j);
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

      construct_exp(&vars, exp, ILPROPMapper::NOT_ALONE, i);
      result = rb_solution->evaluate(*exp);

      cout << "Element " << i << ":" << result << endl;
    }

    // cout << "|===========TASK_PREEMPTION===========|" << endl;
    // for (uint i = 0; i < t_num; i++) {
    //   LinearExpression* exp = new LinearExpression();
    //   double result;
    //   for (uint j = 0; j < t_num; j++) {
    //     construct_exp(&vars, exp, ILPROPMapper::PREEMPT_NUM, i, j);
    //     result = rb_solution->evaluate(*exp);
    //     if (result < 1)
    //       result = 0;
    //     cout << result << "  ";
    //   }
    //   cout << endl;
    // }

    // cout << "|===========TASK_PREEMPTION_H===========|" << endl;
    // for (uint i = 0; i < t_num; i++) {
    //   LinearExpression* exp = new LinearExpression();
    //   double result;
    //   for (uint j = 0; j < t_num; j++) {
    //     construct_exp(&vars, exp, ILPROPMapper::TBT_PREEMPT_NUM, i, j);
    //     result = rb_solution->evaluate(*exp);
    //     if (result < 1)
    //       result = 0;
    //     cout << result << "  ";
    //   }
    //   cout << endl;
    // }

    // for (uint i = 0; i < t_num; i++) {
    //   Task& task = tasks.get_task_by_index(i);
    //   cout << "/===========Task " << i << "===========/" << endl;
    //   LinearExpression* exp = new LinearExpression();
    //   double result;

    //   cout << "Res:";

    //   foreach(task.get_requests(), request) {
    //     uint u = request->get_resource_id();
    //     construct_exp(&vars, exp, ILPROPMapper::RESPONSE_TIME_CS, i, t_num + u);
    //     result = rb_solution->evaluate(*exp);
    //     cout << "request " << u << " holding time:" << result << endl;
    //   }
      

    //   construct_exp(&vars, exp, ILPROPMapper::RESPONSE_TIME, i);
    //   result = rb_solution->evaluate(*exp);
    //   cout << "R_" << i << ":" << result << endl;

    //   construct_exp(&vars, exp, ILPROPMapper::BLOCKING_TIME, i);
    //   result = rb_solution->evaluate(*exp);
    //   cout << "B_" << i << ":" << result << endl;

    //   construct_exp(&vars, exp, ILPROPMapper::INTERFERENCE_TIME_C, i);
    //   result = rb_solution->evaluate(*exp);
    //   cout << "INCS_" << i << ":" << result << endl;

    //   construct_exp(&vars, exp, ILPROPMapper::INTERFERENCE_TIME_R, i);
    //   result = rb_solution->evaluate(*exp);
    //   cout << "ICS_" << i << ":" << result << endl;

    //   cout << "C_" << i << ":" << tasks.get_tasks()[i].get_wcet() << endl;
    //   cout << "D_" << i << ":" << tasks.get_tasks()[i].get_deadline() << endl;
    //   delete exp;
    // }

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
#endif
    

    rb_solution->show_error();
    cout << rb_solution->get_status() << endl;
    set_status(rb_solution->get_status());
  }

  delete rb_solution;
  return false;
}

void ILP_RTA_PFP_ROP::construct_exp(ILPROPMapper* vars, LinearExpression* exp,
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

void ILP_RTA_PFP_ROP::set_objective(LinearProgram* lp, ILPROPMapper* vars) {
  // any objective function is okay.
  // assert(0 < tasks.get_tasks().size());

  LinearExpression* obj = new LinearExpression();
  uint var_id;

  // uint i = tasks.get_taskset_size() - 1;
  for (uint i = 0; i < tasks.get_taskset_size(); i ++) {
    var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, i);
    obj->add_term(var_id, 1);
  }

  lp->set_objective(obj);
}

void ILP_RTA_PFP_ROP::add_constraints(LinearProgram* lp, ILPROPMapper* vars) {
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
  constraint_8_4(lp, vars);
  constraint_9(lp, vars);
  constraint_10(lp, vars);
  constraint_11(lp, vars);

  constraint_12(lp, vars);
  constraint_13(lp, vars);

  constraint_14(lp, vars);
  constraint_15(lp, vars);
  constraint_16(lp, vars);

  constraint_17(lp, vars);
  constraint_17_1(lp, vars);
  constraint_18(lp, vars);

  constraint_19(lp, vars);
  // constraint_20(lp, vars);
  constraint_for_specific_alloc(lp, vars);
}

/** Expressions */
// C1
void ILP_RTA_PFP_ROP::constraint_1(LinearProgram* lp, ILPROPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num; i++) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, i);
    exp->add_term(var_id, 1);
    lp->declare_variable_binary(var_id);

    lp->add_equality(exp, 1);
  }
}

// C2
void ILP_RTA_PFP_ROP::constraint_2(LinearProgram* lp, ILPROPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    for (uint j = i + 1; j < r_num + t_num; j++) {
      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, j);
      exp->add_term(var_id, 1);
      lp->declare_variable_binary(var_id);

      var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, j, i);
      exp->add_term(var_id, -1);
      lp->declare_variable_binary(var_id);

      lp->add_equality(exp, 0);
    }
  }
}

// C3
void ILP_RTA_PFP_ROP::constraint_3(LinearProgram* lp, ILPROPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    for (uint j = i + 1; j < r_num + t_num; j++) {
      for (uint k = 0; k < r_num + t_num; k++) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, j);
        exp->add_term(var_id, -1);

        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, k, i);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, k, j);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

// C4
void ILP_RTA_PFP_ROP::constraint_4(LinearProgram* lp, ILPROPMapper* vars) {
  int p_num = processors.get_processor_num();
  int r_num = resources.get_resourceset_size();
  int t_num = tasks.get_taskset_size();

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::NOT_ALONE, i);
    exp->add_term(var_id, 1);
    lp->declare_variable_binary(var_id);

    for (uint j = i + 1; j < r_num + t_num; j++) {
      var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, j);
      exp->add_term(var_id, -1);
    }

    lp->add_inequality(exp, 0);
  }
}

// C5
void ILP_RTA_PFP_ROP::constraint_5(LinearProgram* lp, ILPROPMapper* vars) {
  int p_num = processors.get_processor_num();
  int r_num = resources.get_resourceset_size();
  int t_num = tasks.get_taskset_size();

  LinearExpression* exp = new LinearExpression();
  uint var_id;

  for (uint i = 0; i < r_num + t_num - 1; i++) {
    var_id = vars->lookup(ILPROPMapper::NOT_ALONE, i);
    exp->add_term(var_id, -1);
  }

  lp->add_inequality(exp, p_num - (r_num + t_num));
}

// C6
void ILP_RTA_PFP_ROP::constraint_6(LinearProgram* lp, ILPROPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong deadline = ti->get_deadline();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, i);
    exp->add_term(var_id, 1);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    lp->add_inequality(exp, deadline);
  }
}

// C7
void ILP_RTA_PFP_ROP::constraint_7(LinearProgram* lp, ILPROPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    int64_t wcet = ti->get_wcet();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, i);
    exp->add_term(var_id, -1);
    // lp->declare_variable_integer(var_id);

    var_id = vars->lookup(ILPROPMapper::BLOCKING_TIME, i);
    exp->add_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_C, i);
    exp->add_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R, i);
    exp->add_term(var_id, 1);
    // lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    lp->add_inequality(exp, -1 * wcet);
  }
}

// C7-1
void ILP_RTA_PFP_ROP::constraint_7_1(LinearProgram* lp, ILPROPMapper* vars) {
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

      var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME_CS, i, t_num + u);
      exp->add_term(var_id, 1);
      // lp->declare_variable_bounds(var_id, true, 0, true, deadline);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);

      var_id = vars->lookup(ILPROPMapper::REQUEST_BLOCKING_TIME, i, t_num + u);
      exp->add_term(var_id, -1);

      foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
        uint x = tx->get_index();

        foreach(tx->get_requests(), request_v) {
          uint v = request_v->get_resource_id();

          int64_t L_x_v = request_v->get_max_length();

          var_id = vars->lookup(ILPROPMapper::RBR_PREEMPT_NUM, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, -1 * L_x_v);
        }
      }

      lp->add_equality(exp, L_i_u);
    }
  }
}

// C8
void ILP_RTA_PFP_ROP::constraint_8(LinearProgram* lp, ILPROPMapper* vars) {
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

      var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM, i, x);
      exp->add_term(var_id, -1);
      lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, true, bound);

      var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, i);
      exp->add_term(var_id, 1.0 / px);

      var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, x);
      exp->add_term(var_id, 1.0 / px);

      double ux = cx;
      ux /= px;

      lp->add_inequality(exp, ux);
    }
  }
}
// C8_1
void ILP_RTA_PFP_ROP::constraint_8_1(LinearProgram* lp, ILPROPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong cx = tx->get_wcet();
      ulong px = tx->get_period();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM, i, x);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, i);
      exp->add_term(var_id, -1.0 / px);

      var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, x);
      exp->add_term(var_id, -1.0 / px);

      double ux = cx;
      ux /= px;

      lp->add_inequality(exp, 1.0-ux);
    }
  }
}


// C8_2
void ILP_RTA_PFP_ROP::constraint_8_2(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong cx = tx->get_wcet();
      ulong px = tx->get_period();
      
      foreach(tx->get_requests(), request_v) {
        uint v = request_v->get_resource_id();

        int32_t N_x_v = request_v->get_num_requests();

        int64_t L_x_v = request_v->get_max_length();

        int64_t A_x_v = N_x_v * L_x_v;

        uint bound = ceiling(pi + px, px);

        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM_REQUEST, i, x, t_num + v);
        exp->add_term(var_id, -1);
        lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, true, bound);

        var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, i);
        exp->add_term(var_id, 1.0 / px);

        var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, x);
        exp->add_term(var_id, 1.0 / px);

        double ux = A_x_v;
        ux /= px;

        lp->add_inequality(exp, ux);
      }
    }
  }
}

// C8-4
void ILP_RTA_PFP_ROP::constraint_8_4(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();

      // foreach(tasks.get_tasks(), tx) {
      foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
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

          var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM_CS, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, -1);
          lp->declare_variable_integer(var_id);
          lp->declare_variable_bounds(var_id, true, 0, true, bound);

          var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME_CS, i, t_num + u);
          exp->add_term(var_id, 1.0 / px);

          var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, x);
          exp->add_term(var_id, 1.0 / px);

          double ux = A_x_v;
          ux /= px;

          lp->add_inequality(exp, ux);
          // lp->add_equality(exp, 2);

        }
      }
    }
  }
}

// C8-5
void ILP_RTA_PFP_ROP::constraint_8_5(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();

      // foreach(tasks.get_tasks(), tx) {
      foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
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

          var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM_CS, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME_CS, i, t_num + u);
          exp->add_term(var_id, -1.0 / px);

          var_id = vars->lookup(ILPROPMapper::RESPONSE_TIME, x);
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
void ILP_RTA_PFP_ROP::constraint_9(LinearProgram* lp, ILPROPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      ulong px = tx->get_period();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      uint bound = ceiling(pi + px, px);

      var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM, i, x);
      exp->add_term(var_id, 1);

      var_id = vars->lookup(ILPROPMapper::TBT_PREEMPT_NUM, i, x);
      exp->add_term(var_id, -1);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, true, bound);

      var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, x);
      exp->add_term(var_id, bound);

      lp->add_inequality(exp, bound);
    }
  }
}

// C10
void ILP_RTA_PFP_ROP::constraint_10(LinearProgram* lp, ILPROPMapper* vars) {
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

        var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM_REQUEST, i, x);
        exp->add_term(var_id, N_x_v);

        var_id = vars->lookup(ILPROPMapper::RBT_PREEMPT_NUM, i, x, t_num + v);
        exp->add_term(var_id, -1);
        // lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, true, N_x_v * bound);

        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + v);
        exp->add_term(var_id, N_x_v * bound);

        lp->add_inequality(exp, N_x_v * bound);
      }
    }
  }
}

// C11
void ILP_RTA_PFP_ROP::constraint_11(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong pi = ti->get_period();

    // foreach(tasks.get_tasks(), tx) {
    foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
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

          var_id = vars->lookup(ILPROPMapper::PREEMPT_NUM_CS, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, N_x_v);

          var_id = vars->lookup(ILPROPMapper::RBR_PREEMPT_NUM, i, x, t_num + u,
                                t_num + v);
          exp->add_term(var_id, -1);
          // lp->declare_variable_integer(var_id);
          lp->declare_variable_bounds(var_id, true, 0, true, N_x_v * bound);
          // lp->declare_variable_bounds(var_id, true, 0, false, -1);

          var_id =
              vars->lookup(ILPROPMapper::SAME_LOCALITY, t_num + u, t_num + v);
          exp->add_term(var_id, N_x_v * bound);

          // FOR TEST
          // var_id =
          //     vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + v);
          // exp->add_term(var_id, (-1) * N_x_v * bound);

          lp->add_inequality(exp, N_x_v * bound);
        }
      }
    }
  }
}

// C12
void ILP_RTA_PFP_ROP::constraint_12(LinearProgram* lp, ILPROPMapper* vars) {
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
                vars->lookup(ILPROPMapper::REQUEST_BLOCKING_TIME, i, t_num + u);
            exp->add_term(var_id, -1);

            var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + u);
            exp->add_term(var_id, -L_x_v);

            var_id =
                vars->lookup(ILPROPMapper::SAME_LOCALITY, t_num + u, t_num + v);
            exp->add_term(var_id, L_x_v);

            lp->add_inequality(exp, 0);
          }
        }
      }
    }
  }
}

// C13
void ILP_RTA_PFP_ROP::constraint_13(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::BLOCKING_TIME, i);
    exp->add_term(var_id, 1);

    foreach(ti->get_requests(), request) {
      uint u = request->get_resource_id();

      int N_i_u = request->get_num_requests();

      var_id = vars->lookup(ILPROPMapper::REQUEST_BLOCKING_TIME, i, t_num + u);
      exp->add_term(var_id, -N_i_u);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    lp->add_equality(exp, 0);
  }
}

// C14
void ILP_RTA_PFP_ROP::constraint_14(LinearProgram* lp, ILPROPMapper* vars) {
  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    ulong di = ti->get_deadline();

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();
      ulong px = tx->get_period();

      LinearExpression* exp = new LinearExpression();
      uint var_id;

      var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_C_TASK, i, x);
      exp->add_term(var_id, -1);

      int64_t NC_WCET_x = tx->get_wcet_non_critical_sections();

      var_id = vars->lookup(ILPROPMapper::TBT_PREEMPT_NUM, i, x);
      exp->add_term(var_id, NC_WCET_x);

      lp->add_equality(exp, 0);
      // lp->add_inequality(exp, 0);
    }
  }
}

// C15
void ILP_RTA_PFP_ROP::constraint_15(LinearProgram* lp, ILPROPMapper* vars) {
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

        var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_C_RESOURCE, i, y,
                              t_num + u);
        exp->add_term(var_id, -1);

        var_id = vars->lookup(ILPROPMapper::RBT_PREEMPT_NUM, i, y, t_num + u);
        exp->add_term(var_id, L_y_u);

        lp->add_equality(exp, 0);
        // lp->add_inequality(exp, 0);
      }
    }
  }
}

// C16
void ILP_RTA_PFP_ROP::constraint_16(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_C, i);
    exp->add_term(var_id, 1);

    foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
      uint x = tx->get_index();

      var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_C_TASK, i, x);
      exp->add_term(var_id, -1);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    foreach_task_except(tasks.get_tasks(), (*ti), ty) {
      uint y = ty->get_index();
      foreach(ty->get_requests(), request_u) {
        uint u = request_u->get_resource_id();

        var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_C_RESOURCE, i, y,
                              t_num + u);
        exp->add_term(var_id, -1);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
      }
    }

    lp->add_equality(exp, 0);
  }
}

// // C17
// void ILP_RTA_PFP_ROP::constraint_17(LinearProgram* lp, ILPROPMapper* vars) {
//   uint t_num = tasks.get_taskset_size();

//   foreach(tasks.get_tasks(), ti) {
//     uint i = ti->get_index();
//     ulong pi = ti->get_period();

//     foreach(ti->get_requests(), request_u) {
//       uint u = request_u->get_resource_id();

//       LinearExpression* exp = new LinearExpression();
//       uint var_id;

//       var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R_RESOURCE, i,
//                             t_num + u);
//       exp->add_term(var_id, -1);

//       foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
//         uint x = tx->get_index();
//         ulong px = tx->get_period();

//         foreach(tx->get_requests(), request_v) {
//           uint v = request_v->get_resource_id();
//           int N_x_v = request_v->get_num_requests();
//           int64_t L_x_v = request_v->get_max_length();

//           var_id = vars->lookup(ILPROPMapper::RBR_PREEMPT_NUM, i, x, t_num + u,
//                                 t_num + v);
//           exp->add_term(var_id, L_x_v);

//           int term = ceiling(pi + px, px);

//           var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num +u);
//           exp->add_term(var_id, -1*term*N_x_v*L_x_v);
//         }
//       }

//       lp->add_inequality(exp, 0);
//     }
//   }
// }


// C17
void ILP_RTA_PFP_ROP::constraint_17(LinearProgram* lp, ILPROPMapper* vars) {
  uint32_t t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint32_t i = ti->get_index();

    foreach(ti->get_requests(), request_u) {
      uint32_t u = request_u->get_resource_id();

      LinearExpression* exp = new LinearExpression();
      uint32_t var_id;

      var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R_RESOURCE, i,
                            t_num + u);
      exp->add_term(var_id, -1);

      foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
        uint32_t x = tx->get_index();

        foreach(tx->get_requests(), request_v) {
          uint32_t v = request_v->get_resource_id();

          var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R_REQUEST, i, x, t_num + u, t_num + v);
          exp->add_term(var_id, 1);

        }
      }

      // lp->add_inequality(exp, 0);
      lp->add_equality(exp, 0);
    }
  }
}

// C17-1
void ILP_RTA_PFP_ROP::constraint_17_1(LinearProgram* lp, ILPROPMapper* vars) {
  int32_t t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    int32_t i = ti->get_index();
    ulong pi = ti->get_period();

    // foreach(tasks.get_tasks(), tx) {
    foreach_higher_priority_task(tasks.get_tasks(), (*ti), tx) {
      int32_t x = tx->get_index();
      ulong px = tx->get_period();

      foreach(ti->get_requests(), request_u) {
        int32_t u = request_u->get_resource_id();

        foreach(tx->get_requests(), request_v) {
          int32_t v = request_v->get_resource_id();

          int32_t N_x_v = request_v->get_num_requests();

          int64_t L_x_v = request_v->get_max_length();

          int32_t bound = ceiling(pi + px, px);

          LinearExpression* exp = new LinearExpression();
          int32_t var_id;

          var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R_REQUEST, i, x, t_num + u, t_num + v);
          exp->add_term(var_id, -1);
          lp->declare_variable_bounds(var_id, true, 0, false, -1);

          var_id = vars->lookup(ILPROPMapper::RBR_PREEMPT_NUM, i, x, t_num + u, t_num + v);
          exp->add_term(var_id, L_x_v);

          var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + u);
          exp->add_term(var_id, (-1) * L_x_v * bound);

          lp->add_inequality(exp, 0);
        }
      }
    }
  }
}

// C18
void ILP_RTA_PFP_ROP::constraint_18(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();

    LinearExpression* exp = new LinearExpression();
    uint var_id;

    var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R, i);
    exp->add_term(var_id, 1);

    foreach(ti->get_requests(), request_u) {
      uint u = request_u->get_resource_id();
      int N_i_u = request_u->get_num_requests();

      var_id = vars->lookup(ILPROPMapper::INTERFERENCE_TIME_R_RESOURCE, i,
                            t_num + u);
      exp->add_term(var_id, -1 * N_i_u);
      // lp->declare_variable_integer(var_id);
      lp->declare_variable_bounds(var_id, true, 0, false, -1);
    }

    lp->add_equality(exp, 0);
  }
}

void ILP_RTA_PFP_ROP::constraint_19(LinearProgram* lp, ILPROPMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();

  for (uint i = 0; i < t_num; i++) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;

    for (uint j = 0; j < t_num; j++) {
      Task& task = tasks.get_task_by_index(j);
      double utilization = task.get_utilization();

      var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, j);
      exp->add_term(var_id, utilization);
    }

    lp->add_inequality(exp, 1);
  }
}

void ILP_RTA_PFP_ROP::constraint_20(LinearProgram* lp, ILPROPMapper* vars) {}


void ILP_RTA_PFP_ROP::constraint_for_specific_alloc(LinearProgram* lp, ILPROPMapper* vars) {
  uint t_num = tasks.get_taskset_size();
  uint p_num = processors.get_processor_num();
  // for (uint p_id = 0; p_id < p_num; p_id++) {
  //   foreach(tasks.get_tasks(), ti) {
  //     uint i = ti->get_index();
  //     if (p_id == ti->get_partition()) {
  //       foreach(tasks.get_tasks(), tx) {
  //         uint x = tx->get_index();
  //         if (p_id == tx->get_partition()) {
  //           LinearExpression* exp = new LinearExpression();
  //           uint var_id;
  //           var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, x);
  //           exp->add_term(var_id, 1);
  //           lp->add_equality(exp, 1);
  //         }
  //       }
  //       foreach(resources.get_resources(), rq) {
  //         uint q = rq->get_resource_id();
  //         LinearExpression* exp = new LinearExpression();
  //         uint var_id;
  //         var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + q);
  //         if (p_id == rq->get_locality()) {
  //           exp->add_term(var_id, 1);
  //           lp->add_equality(exp, 1);
  //         } else {
  //           exp->add_term(var_id, 1);
  //           lp->add_equality(exp, 0);
  //         }
  //       }
  //     }
  //   }
  // }

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_index();
    if (MAX_INT == ti->get_partition())
      continue;
    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      if(ti->get_partition() == tx->get_partition()) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, x);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 1);
      } else {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, x);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 0);
      }
    }

    foreach(resources.get_resources(), rv) {
      uint v = rv->get_resource_id();
      if(ti->get_partition() == rv->get_locality()) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + v);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 1);
      } else {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, i, t_num + v);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 0);
      }
    }
  }

  foreach(resources.get_resources(), ru) {
    uint u = ru->get_resource_id();
    if (MAX_INT == ru->get_locality())
      continue;
    foreach(tasks.get_tasks(), tx) {
      uint x = tx->get_index();
      if(ru->get_locality() == tx->get_partition()) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, t_num + u, x);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 1);
      } else {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, t_num + u, x);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 0);
      }
    }

    foreach(resources.get_resources(), rv) {
      uint v = rv->get_resource_id();
      if(ru->get_locality() == rv->get_locality()) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, t_num + u, t_num + v);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 1);
      } else {
        LinearExpression* exp = new LinearExpression();
        uint var_id;
        var_id = vars->lookup(ILPROPMapper::SAME_LOCALITY, t_num + u, t_num + v);
        exp->add_term(var_id, 1);
        lp->add_equality(exp, 0);
      }
    }
  }
}


void ILP_RTA_PFP_ROP::specific_partition() {
  uint p_id = 0;

  // resource allocation
  Resources &rs = resources.get_resources();

  p_id = 0;
  rs[0].set_locality(p_id);
  processors.get_processors()[p_id].add_resource(rs[0].get_resource_id());

  p_id = 0;
  rs[1].set_locality(p_id);
  processors.get_processors()[p_id].add_resource(rs[1].get_resource_id());

  // task allocation
  tasks.update_requests(resources);
  Tasks &ts = tasks.get_tasks();

  p_id = 1;
  ts[0].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[0].get_id())) {
    cout<<"task 0 allocation failed"<<endl;
  }

  p_id = 1;
  ts[1].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[1].get_id())) {
    cout<<"task 1 allocation failed"<<endl;
  }

  p_id = 1;
  ts[2].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[2].get_id())) {
    cout<<"task 2 allocation failed"<<endl;
  }

  p_id = 1;
  ts[3].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[3].get_id())) {
    cout<<"task 3 allocation failed"<<endl;
  }

  p_id = 1;
  ts[4].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[4].get_id())) {
    cout<<"task 4 allocation failed"<<endl;
  }

  p_id = 2;
  ts[5].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[5].get_id())) {
    cout<<"task 5 allocation failed"<<endl;
  }

  p_id = 2;
  ts[6].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[6].get_id())) {
    cout<<"task 6 allocation failed"<<endl;
  }

  p_id = 3;
  ts[7].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[7].get_id())) {
    cout<<"task 7 allocation failed"<<endl;
  }

  p_id = 0;
  ts[8].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[8].get_id())) {
    cout<<"task 8 allocation failed"<<endl;
  }

  p_id = 0;
  ts[9].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[9].get_id())) {
    cout<<"task 9 allocation failed"<<endl;
  }
}

bool ILP_RTA_PFP_ROP::alloc_schedulable() {}
