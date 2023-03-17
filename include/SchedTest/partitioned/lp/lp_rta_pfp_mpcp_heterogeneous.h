// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_MPCP_HETEROGENEOUS_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_MPCP_HETEROGENEOUS_H_

/*
** LinearProgramming approach for partitioned fix-priority scheduling under MPCP
*locking protocol
**
** RTAS 2013 Bjorn B. Brandenburg [Improved Analysis and Evaluation of Real-Time
*Semaphore Protocols for P-FP
** Scheduling]
*/

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <varmapper.h>
#include <string>

class Task;
class TaskSet;
class Request;
class Resource;
class ResourceSet;
class ProcessorSet;
class LinearExpression;
class LinearProgram;

/*
|________________|_____________________|______________________|______________________|______________________|
|                |                     |                      |                      |                      |
|(63-34)Reserved |(31-30) Blocking type|(29-20) Task          |(19-10) Resource      |(9-0) Request         |
|________________|_____________________|______________________|______________________|______________________|
*/
class MPCPMapper_heterogeneous : public VarMapperBase {
 public:
  enum var_type {
    BLOCKING_DIRECT,    // 0x000
    BLOCKING_INDIRECT,  // 0x001
    BLOCKING_PREEMPT,   // 0x010
    BLOCKING_OTHER,     // 0x011
  };

 protected:
  static uint64_t encode_request(uint64_t task_id, uint64_t res_id,
                                 uint64_t req_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_res_id(uint64_t var);
  static uint64_t get_req_id(uint64_t var);

 public:
  explicit MPCPMapper_heterogeneous(uint start_var = 0);
  uint lookup(uint task_id, uint res_id, uint req_id, var_type type);
  string key2str(uint64_t key, uint var) const;
};

class LP_RTA_PFP_MPCP_HETEROGENEOUS : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  bool BinPacking_WF(Task* ti, TaskSet* tasks, ProcessorSet* processors,
                     ResourceSet* resources, uint MODE);
  ulong local_blocking(Task* ti);
  ulong remote_blocking(Task* ti);
  ulong total_blocking(Task* ti);
  ulong interference(const Task& ti, ulong interval);
  ulong response_time(Task* ti);
  bool alloc_schedulable();
  uint priority_ceiling(uint r_id, uint p_id);
  uint priority_ceiling(const Task& ti);
  uint DD(const Task& ti, const Task& tx, uint r_id);
  uint PO(const Task& ti, const Task& tx);
  uint PO(const Task& ti, const Task& tx, uint r_id);
  ulong holding_time(const Task& tx, uint r_id);
  ulong wait_time(const Task& ti, uint r_id);

  void set_objective(const Task& ti, LinearProgram* lp,
                     MPCPMapper_heterogeneous* vars,
                     LinearExpression* local_obj, LinearExpression* remote_obj);

  void add_constraints(const Task& ti, LinearProgram* lp,
                       MPCPMapper_heterogeneous* vars);

  // Constraint 15 [BrandenBurg 2013 RTAS Appendix-C]
  void constraint_1(const Task& ti, LinearProgram* lp,
                    MPCPMapper_heterogeneous* vars);
  // Constraint 16 [BrandenBurg 2013 RTAS Appendix-C]
  void constraint_2(const Task& ti, LinearProgram* lp,
                    MPCPMapper_heterogeneous* vars);
  // Constraint 17 [BrandenBurg 2013 RTAS Appendix-C]
  void constraint_3(const Task& ti, LinearProgram* lp,
                    MPCPMapper_heterogeneous* vars);
  // Constraint 18 [BrandenBurg 2013 RTAS Appendix-C]
  void constraint_4(const Task& ti, LinearProgram* lp,
                    MPCPMapper_heterogeneous* vars);
  // Constraint 19 [BrandenBurg 2013 RTAS Appendix-C]
  void constraint_5(const Task& ti, LinearProgram* lp,
                    MPCPMapper_heterogeneous* vars);
  // Constraint 20 [BrandenBurg 2013 RTAS Appendix-C]
  void constraint_6(const Task& ti, LinearProgram* lp,
                    MPCPMapper_heterogeneous* vars);

 public:
  LP_RTA_PFP_MPCP_HETEROGENEOUS();
  LP_RTA_PFP_MPCP_HETEROGENEOUS(TaskSet tasks, ProcessorSet processors,
                                ResourceSet resources);
  ~LP_RTA_PFP_MPCP_HETEROGENEOUS();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_MPCP_HETEROGENEOUS_H_
