// Copyright 2022/7/1 by wyg
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_LP_HTT_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_LP_HTT_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <types.h>
#include <varmapper.h>
#include <string>

/*
|________________|_______________|_____________|_____________|_____________|____________|
|                |               |             |             |             |            |
|(63-45)Reserved |(44-40)var type|(39-30)part1 |(29-20)part2 |(19-10)part3 |(9-0)part4  |
|________________|_______________|_____________|_____________|_____________|____________|
*/

class LPHTTMapper : public VarMapperBase {
 public:
  enum var_type {
    EXCUTE_TIME,                      //X_i_s_k_h_j      task i part j excute k round in hyperperiod                 
    PARTITION,                     //b_i_s_k_h_j 
    FINISH_TIME,                    //F_i_j_k
    ARRIVE_TIME ,                   //A_i_j_k  
    SUPPORT_Z,                     //Z_i_s_k_i'_s'_k'
    SUPPORT_lambda                     //lambda_i_s_k_i'_s'_k'
  };

 protected:
  static uint64_t encode_request(uint64_t type, uint64_t part_1 = 0, uint64_t part_2 = 0,uint64_t part_3 = 0 ,uint64_t part_4 = 0,uint64_t part_5 = 0,uint64_t part_6 = 0,uint64_t part_7 = 0,uint64_t part_8 = 0);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_part_1(uint64_t var);
  static uint64_t get_part_2(uint64_t var);
  static uint64_t get_part_3(uint64_t var);
  static uint64_t get_part_4(uint64_t var);
  static uint64_t get_part_5(uint64_t var);
  static uint64_t get_part_6(uint64_t var);
  static uint64_t get_part_7(uint64_t var);
  static uint64_t get_part_8(uint64_t var);

 public:
  explicit LPHTTMapper(uint start_var = 0);
  uint lookup(uint type, uint part_1=0,uint part_2 = 0,uint part_3 = 0 ,uint part_4 = 0,uint part_5 = 0,uint part_6 = 0,uint part_7 = 0,uint part_8 = 0);

};

/** Class ILP_RTA_PFP_ROP */

class LP_HTT : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  uint64_t  hyperperiod;
  uint64_t  get_hyperperiod();
  void construct_exp(LPHTTMapper* vars, LinearExpression* exp, uint type,
                     uint part_1 = 0, uint part_2 = 0, uint part_3 = 0);
  void set_objective(LinearProgram* lp, LPHTTMapper* vars);
  void add_constraints(LinearProgram* lp, LPHTTMapper* vars);

  // Constraints
  // 3-6
  void constraint_1(LinearProgram* lp, LPHTTMapper* vars);
  // 3-7
  void constraint_2(LinearProgram* lp, LPHTTMapper* vars);
  // 3-8
  void constraint_3_1(LinearProgram* lp, LPHTTMapper* vars);
  // 3-9
  void constraint_3_2(LinearProgram* lp, LPHTTMapper* vars);
  // 3-10
  void constraint_4_1(LinearProgram* lp, LPHTTMapper* vars);
  // 3-13-1
  void constraint_4_2(LinearProgram* lp, LPHTTMapper* vars);
  // 3-13-2
  void constraint_4_3(LinearProgram* lp, LPHTTMapper* vars);
  // 3-12
  void constraint_4_4(LinearProgram* lp, LPHTTMapper* vars);
  // 3-15-1
  void constraint_5_1(LinearProgram* lp, LPHTTMapper* vars);
  // 3-15-2
  void constraint_5_2(LinearProgram* lp, LPHTTMapper* vars);
  // 3-17-1
  void constraint_support_z_1(LinearProgram* lp, LPHTTMapper* vars);
  void constraint_support_z_2(LinearProgram* lp, LPHTTMapper* vars);
  void constraint_support_z_3(LinearProgram* lp, LPHTTMapper* vars);
  void constraint_support_lambda__1(LinearProgram* lp, LPHTTMapper* vars);
  void constraint_support_lambda__2(LinearProgram* lp, LPHTTMapper* vars);
  void LP_HTT::constraint_by_wyg(LinearProgram* lp, LPHTTMapper* vars);
  bool alloc_schedulable();

 public:
  LP_HTT();
  LP_HTT(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_HTT();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_LP_ILP_RTA_PFP_ROP_H_
