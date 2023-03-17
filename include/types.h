// Copyright [2019] <Zewei Chen>
#ifndef INCLUDE_TYPES_H_
#define INCLUDE_TYPES_H_

#include <assert.h>
#include <gmpxx.h>  // including GNU Multiple Precision Arithmetic Library
#include <math.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>
// #include "iteration-helper.h"
// #include "math-helper.h"

#define uint uint32_t
#define ulong uint64_t

#define MAX_LONG 0xffffffffffffffff
#define MAX_INT 0xffffffff
#define MAX_ULONG 0xffffffffffffffff
#define MAX_UINT 0xffffffff
#define _EPS 0.000001

// output file format
#define PNG 0x01
#define EPS 0x02
#define SVG 0x04
#define TGA 0x08
#define JSON 0x10

// GEN_METHOD
#define GEN_UNIFORM       0
#define GEN_EXPONENTIAL   1
#define GEN_UUNIFAST      2
#define GEN_UUNIFAST_D    3
#define GEN_RANDFIXSUM    4

// DAG_GEN_METHOD
#define DAG_GEN_DEFAULT     0
#define DAG_GEN_FORK_JOIN   1
#define DAG_GEN_ERDOS_RENYI 2
#define DAG_GEN_ERDOS_RENYI_v2 3
#define DAG_GEN_TEST 4
#define DAG_GEN_SIM  5

// VNode type
#define P_NODE 0x02
#define C_NODE 0x04
#define J_NODE 0x08

#define S_NODE 0x00
#define E_NODE 0x01

// Graph type
#define G_TYPE_P 0  // Paralleled
#define G_TYPE_C 1  // Conditional

#define P_EDF 0
#define BCL_FTP 1
#define BCL_EDF 2
#define WF_DM 3
#define WF_EDF 4
#define RTA_GFP 5
#define FF_DM 6
#define LP_PFP 7
#define LP_GFP 8
#define RO_PFP 9
#define ILP_SPINLOCK 10
#define GEDF_NON_PREEMPT 11
#define PFP_GS 12

#define TPS_TASK_MODEL 0  // Three Parameters Sporadic Task Model
#define DAG_TASK_MODEL 1

#define PRIORITY 0

#define INCREASE 0
#define DECREASE 1

#define INTERFERENCE 0
#define BLOCKING 1

// GLPK
#define GLPK_MEM_USAGE_CHECK 0
#define GLPK_TERM_OUT 0
#define ILP_SOLUTION_VAR_CHECK 0
#define UNDEF_ABANDON 0
#define UNDEF_UNSUCCESS 1
#define TIME_LIMIT_INIT 3600000         // 1 hour
// #define TIME_LIMIT_INIT 600000         // 10 min
#define TIME_LIMIT_GAP 0                // 0 sec
#define TIME_LIMIT_UPPER_BOUND 3600000  // 1 hour

// CFG
#define SORT_DEBUG 0
#define RTA_DEBUG 0

// Varying parameter
#define TU          0
#define TN          1
#define RN          2
#define CSN         3
#define CSL         4


using std::cout;
using std::endl;
using std::list;
using std::set;
using std::string;
using std::vector;

typedef mpz_class int_t;       // multiple precision integer
typedef mpq_class fraction_t;  // multiple precision rational number
typedef mpf_class floating_t;  // multiple precision float number

class Task;
class TaskSet;
class Processor;
class ProcessorSet;
class Resource;
class ResourceSet;

typedef struct {
  double min;
  double max;
} Range;

typedef struct {
  double x;
  uint exp_num;
  uint success_num;
  double total_rt;
} Result;

typedef struct {
  bool LP;
  uint test_type;
  uint test_method;
  string sched_method;
  string priority_assignment;
  string locking_protocol;
  string test_name;
  string remark;
  string rename;
  string style;
  uint cluster;
} Test_Attribute;

typedef struct {
  uint t_id;
  uint64_t workload;
  uint32_t parallel_degree;
} Interf;

typedef vector<int> Int_Set;
typedef vector<double> Double_Set;
typedef vector<Range> Range_Set;
typedef vector<Result> Result_Set;
typedef vector<Result_Set> Result_Sets;
typedef vector<Test_Attribute> Test_Attribute_Set;
typedef list<void*> TaskQueue;
typedef list<void*> ResourceQueue;
typedef vector<double> Ratio;
typedef vector<uint32_t> Cluster;

#endif  // INCLUDE_TYPES_H_
