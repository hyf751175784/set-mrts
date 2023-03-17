// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <param.h>
#include <iteration-helper.h>
#include <xml.h>
#include <unistd.h>
#include <sys/stat.h> 
#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

uint Param::get_method_num() { return test_attributes.size(); }

uint Param::get_test_method(uint index) {
  return test_attributes[index].test_method;
}

uint Param::get_test_type(uint index) {
  return test_attributes[index].test_type;
}


void Param::param_panel() {
  string task_gen;
  switch (dag_gen) {
    case DAG_GEN_DEFAULT:
      task_gen = "DAG_GEN_DEFAULT";
      break;
    case DAG_GEN_FORK_JOIN:
      task_gen = "DAG_GEN_FORK_JOIN";
      break;
    case DAG_GEN_ERDOS_RENYI:
      task_gen = "DAG_GEN_ERDOS_RENYI";
      break;
    case DAG_GEN_ERDOS_RENYI_v2:
      task_gen = "DAG_GEN_ERDOS_RENYI_v2";
      break;
    case DAG_GEN_TEST:
      task_gen = "DAG_GEN_TEST";
      break;
    case DAG_GEN_SIM:
      task_gen = "DAG_GEN_SIM";
      break;
    default:
      task_gen = "DAG_GEN_DEFAULT";
      break;
  }

  cout << "\033[96mParam ID:\033[0m " << id << "  \033[96mDAG Generator:\033[0m  " <<  task_gen;
  if (0 < resource_num)
    cout << "  \033[96mResource Number:\033[0m " << resource_num << "  \033[96mCSN:\033[0m " << rrn
         << "  \033[96mCSL:\033[0m [" << rrr.min << "," << rrr.max << "]" << endl;
}

vector<Param> get_parameters(string filename) {
  vector<Param> parameters;

  XML::LoadFile(filename.data());

  if (0 == access(string("results").data(), 0)) {
    // printf("results folder exsists.\n");
  } else {
    printf("results folder does not exsist.\n");
    if (0 == mkdir(string("results").data(), S_IRWXU))
      printf("results folder has been created.\n");
    else
      return parameters;
  }

  uint model;

  string task_model = XML::get_string("task_model");

  if (0 == strcmp(task_model.data(), "TPS"))
    model = TPS_TASK_MODEL;
  else if (0 == strcmp(task_model.data(), "DAG"))
    model = DAG_TASK_MODEL;
  else
    model = TPS_TASK_MODEL;

  // Server info
  const char* ip = XML::get_server_ip();
  uint port = XML::get_server_port();

  // scheduling parameter
  Int_Set p_nums;
  Double_Set means, steps, ratio;
  Range_Set p_ranges, u_ranges, d_ranges;
  Test_Attribute_Set test_attributes;
  uint exp_times;
  uint u_gen, dag_gen;

  XML::get_method(&test_attributes);
  exp_times = XML::get_experiment_times();
  XML::get_mean(&means);
  XML::get_processor_num(&p_nums);
  XML::get_period_range(&p_ranges);
  XML::get_deadline_propotion(&d_ranges);
  XML::get_utilization_range(&u_ranges);
  XML::get_step(&steps);
  XML::get_doubles(&ratio, "processor_ratio");

  string u_gen_method = XML::get_string("utilization_gen");
  if (0 == strcmp(u_gen_method.data(), "UNIFORM"))
    u_gen = 0;
  else if (0 == strcmp(u_gen_method.data(), "EXPONENTIAL"))
    u_gen = 1;
  else if (0 == strcmp(u_gen_method.data(), "UUNIFAST"))
    u_gen = 2;
  else if (0 == strcmp(u_gen_method.data(), "UUNIFAST-DISCARD"))
    u_gen = 3;
  else if (0 == strcmp(u_gen_method.data(), "RANDFIXSUM"))
    u_gen = 4;
  else
    u_gen = 3;

  string dag_gen_method = XML::get_string("dag_gen");
  if (0 == strcmp(dag_gen_method.data(), "DEFAULT"))
    dag_gen = 0;
  else if (0 == strcmp(dag_gen_method.data(), "FORK_JOIN"))
    dag_gen = 1;
  else if (0 == strcmp(dag_gen_method.data(), "ERDOS_RENYI"))
    dag_gen = 2;
  else if (0 == strcmp(dag_gen_method.data(), "ERDOS_RENY_v2"))
    dag_gen = 3;
  else if (0 == strcmp(dag_gen_method.data(), "TEST"))
    dag_gen = 4;
  else if (0 == strcmp(dag_gen_method.data(), "SIM"))
    dag_gen = 5;
  else
    dag_gen = 0;

  // resource parameter
  Int_Set resource_nums, rrns, mcsns;
  Double_Set rrps, tlfs;
  Range_Set rrrs;
  XML::get_resource_num(&resource_nums);
  XML::get_resource_request_probability(&rrps);
  XML::get_resource_request_num(&rrns);
  XML::get_resource_request_range(&rrrs);
  XML::get_total_len_factor(&tlfs);
  XML::get_integers(&mcsns, "mcsn");

  // graph parameters
  Range_Set job_num_ranges;
  Range_Set arc_num_ranges;
  Int_Set is_cyclics;
  Int_Set max_indegrees;
  Int_Set max_outdegrees;
  Double_Set para_probs, cond_probs, edge_probs, arc_densities;
  Int_Set max_para_jobs, max_cond_branches;

  XML::get_ranges(&job_num_ranges, "dag_job_num_range");
  XML::get_ranges(&arc_num_ranges, "dag_arc_num_range");
  XML::get_integers(&is_cyclics, "is_cyclic");
  XML::get_integers(&max_indegrees, "max_indegree");
  XML::get_integers(&max_outdegrees, "max_outdegree");
  XML::get_doubles(&para_probs, "paralleled_probability");
  XML::get_doubles(&cond_probs, "conditional_probability");
  XML::get_doubles(&edge_probs, "edge_probability");
  XML::get_doubles(&arc_densities, "dag_arc_density");
  XML::get_integers(&max_para_jobs, "max_paralleled_job");
  XML::get_integers(&max_cond_branches, "max_conditional_branch");

  // reserved parameters
  Int_Set reserve_ints_1;
  Int_Set reserve_ints_2;
  Int_Set reserve_ints_3;
  Int_Set reserve_ints_4;
  Double_Set reserve_doubles_1;
  Double_Set reserve_doubles_2;
  Double_Set reserve_doubles_3;
  Double_Set reserve_doubles_4;
  Range_Set reserve_ranges_1;
  Range_Set reserve_ranges_2;

  XML::get_integers(&reserve_ints_1, "reserve_int_1");
  XML::get_integers(&reserve_ints_2, "reserve_int_2");
  XML::get_integers(&reserve_ints_3, "reserve_int_3");
  XML::get_integers(&reserve_ints_4, "reserve_int_4");
  XML::get_doubles(&reserve_doubles_1, "reserve_double_1");
  XML::get_doubles(&reserve_doubles_2, "reserve_double_2");
  XML::get_doubles(&reserve_doubles_3, "reserve_double_3");
  XML::get_doubles(&reserve_doubles_4, "reserve_double_4");
  XML::get_ranges(&reserve_ranges_1, "reserve_range_1");
  XML::get_ranges(&reserve_ranges_2, "reserve_range_2");

  foreach(means, mean)
  foreach(p_nums, p_num)
  foreach(steps, step)
  foreach(p_ranges, p_range)
  foreach(u_ranges, u_range)
  foreach(d_ranges, d_range)
  foreach(resource_nums, resource_num)
  foreach(rrns, rrn)
  foreach(mcsns, mcsn)
  foreach(rrps, rrp)
  foreach(tlfs, tlf)
  foreach(rrrs, rrr)
  foreach(job_num_ranges, job_num_range)
  foreach(arc_num_ranges, arc_num_range)
  foreach(max_indegrees, max_indegree)
  foreach(max_outdegrees, max_outdegree)
  foreach(para_probs, para_prob)
  foreach(cond_probs, cond_prob)
  foreach(edge_probs, edge_prob)
  foreach(arc_densities, arc_density)
  foreach(max_para_jobs, max_para_job)
  foreach(max_cond_branches, max_cond_branch)
  foreach(reserve_ints_1, reserve_int_1)
  foreach(reserve_ints_2, reserve_int_2)
  foreach(reserve_ints_3, reserve_int_3)
  foreach(reserve_ints_4, reserve_int_4)
  foreach(reserve_doubles_1, reserve_double_1)
  foreach(reserve_doubles_2, reserve_double_2)
  foreach(reserve_doubles_3, reserve_double_3)
  foreach(reserve_doubles_4, reserve_double_4)
  foreach(reserve_ranges_1, reserve_range_1)
  foreach(reserve_ranges_2, reserve_range_2) {
    Param param;
    // set parameters
    param.id = parameters.size();
    param.model = model;
    param.server_ip = ip;
    param.port = port;
    param.u_gen = u_gen;
    param.mean = *mean;
    param.p_num = *p_num;
    param.ratio = ratio;
    param.step = *step;
    param.p_range = *p_range;
    param.u_range = *u_range;
    param.d_range = *d_range;
    param.test_attributes = test_attributes;
    param.exp_times = exp_times;
    param.resource_num = *resource_num;
    param.mcsn = *mcsn;
    param.rrn = *rrn;
    param.rrp = *rrp;
    param.tlf = *tlf;
    param.rrr = *rrr;

    param.dag_gen = dag_gen;
    param.job_num_range = *job_num_range;
    param.arc_num_range = *arc_num_range;

    if (0 == is_cyclics[0])
      param.is_cyclic = false;
    else
      param.is_cyclic = true;

    param.max_indegree = *max_indegree;
    param.max_outdegree = *max_outdegree;
    param.para_prob = *para_prob;
    param.cond_prob = *cond_prob;
    param.edge_prob = *edge_prob;
    param.arc_density = *arc_density;
    param.max_para_job = *max_para_job;
    param.max_cond_branch = *max_cond_branch;

    param.graph_width = XML::get_integer("graph_width");
    param.graph_height = XML::get_integer("graph_height");
    param.graph_quality = XML::get_integer("graph_quality");
    param.graph_legend_pos = XML::get_integer("graph_legend_pos");
    param.graph_title = XML::get_string("graph_title");
    param.graph_x_label = XML::get_string("graph_x_label");
    param.graph_y_label = XML::get_string("graph_y_label");
    param.runtime_check = XML::get_integer("runtime_check");

    param.reserve_int_1 = *reserve_int_1;
    param.reserve_int_2 = *reserve_int_2;
    param.reserve_int_3 = *reserve_int_3;
    param.reserve_int_4 = *reserve_int_4;
    param.reserve_double_1 = *reserve_double_1;
    param.reserve_double_2 = *reserve_double_2;
    param.reserve_double_3 = *reserve_double_3;
    param.reserve_double_4 = *reserve_double_4;
    param.reserve_range_1 = *reserve_range_1;
    param.reserve_range_2 = *reserve_range_2;

    parameters.push_back(param);
  }
  // cout << "param num:" << parameters.size() << endl;
  return parameters;
}

