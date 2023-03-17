// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <lp_htt.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <solution.h>
#include <lp.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#define O_max 100000
#define o_min 0.0001
using std::ostringstream;
using namespace std;
/** Class LPHTTMapper */
uint64_t LPHTTMapper::encode_request(uint64_t type, uint64_t part_1,uint64_t part_2, uint64_t part_3,uint64_t part_4, uint64_t part_5,uint64_t part_6,uint64_t part_7,uint64_t part_8) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(type < (one << 4));
  assert(part_1 < (one << 4));
  assert(part_2 < (one << 6));
  assert(part_3 < (one << 16));
   assert(part_4 < (one << 4));
   assert(part_5 < (one << 6));
   assert(part_6 < (one << 16));
   assert(part_7 < (one << 4));
   assert(part_8 < (one << 4));

  key |= (type << 60);
  key |= (part_1 << 56);
  key |= (part_2 << 50);
  key |= (part_3 << 34);
  key |= (part_4 << 39);
  key |= (part_5 << 24);
  key |= (part_6 << 8);
  key |= (part_7 << 4);
  key |= part_8;

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

uint64_t LPHTTMapper::get_type(uint64_t var) {
  return (var >> 50) & (uint64_t)0x1f;  // 5 bits
}

uint64_t LPHTTMapper::get_part_1(uint64_t var) {
  return (var >> 40) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t LPHTTMapper::get_part_2(uint64_t var) {
  return (var >> 30) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t LPHTTMapper::get_part_4(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t LPHTTMapper::get_part_5(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t LPHTTMapper::get_part_6(uint64_t var) {
}
uint64_t LPHTTMapper::get_part_7(uint64_t var) {
}
uint64_t LPHTTMapper::get_part_8(uint64_t var) {
}

LPHTTMapper::LPHTTMapper(uint start_var) : VarMapperBase(start_var) {}

uint LPHTTMapper::lookup(uint type, uint part_1, uint part_2,uint part_3, uint part_4,uint part_5,uint part_6,uint part_7,uint part_8) {
  uint64_t key = encode_request(type, part_1, part_2, part_3,part_4,part_5,part_6,part_7,part_8);

  // cout<<"string:"<<key2str(key)<<endl;
  uint var = var_for_key(key);
  return var;
}



/** Class LP_HTT */
LP_HTT::LP_HTT()
    : PartitionedSched(true, RTA, FIX_PRIORITY, DPCP, "", "DPCP") {}

LP_HTT::LP_HTT(DAG_TaskSet tasks, ProcessorSet processors,
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
  
  hyperperiod=get_hyperperiod();
  cout<<"hyperperiod:"<<hyperperiod;
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

LP_HTT::~LP_HTT() {}

bool LP_HTT::is_schedulable() {
  if (0 == tasks.get_tasks().size())
    {
     cout<<"no tasks"<<endl;
     return true;
     }
  else if (tasks.get_utilization_sum() -
               processors.get_processor_num() >=
           _EPS)
    return false;
  LPHTTMapper vars;
  LinearProgram schedule_table;

  set_objective(&schedule_table, &vars);

  add_constraints(&schedule_table, &vars);
  cout<<"constraint num="<<schedule_table.get_equalities().size() + schedule_table.get_inequalities().size()<<endl;
  GLPKSolution* rb_solution =
      new GLPKSolution(schedule_table, vars.get_num_vars(), 0.0, 1.0, 1, 1);

  assert(rb_solution != NULL);

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  if (rb_solution->is_solved()) {


ofstream oFile;
oFile.open("test1.txt", ios::out);
if (oFile) //条件成立，则说明文件打开出错
{cout << "writing" << endl;
 glp_write_sol(rb_solution->glpk,"test1.txt");
 oFile.close();
}
else
cout << "fail" << endl;

cout << "solved" << endl;

double result= glp_mip_obj_val(rb_solution->glpk);

cout << "obj_val="<< result<<endl;

    uint p_num = processors.get_processor_num();
    uint r_num = resources.get_resourceset_size();
    uint t_num = tasks.get_taskset_size();

    int i=0;
    

  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {
       foreach(ti_a->get_vnodes(),ns_a)
       {
         for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
         {
           foreach(tasks.get_tasks(),ti_b)
              {
                foreach(ti_b->get_vnodes(),ns_b)
                {
                  for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
                    {   
                    if(i_a<i_b)
                    { 
                    uint var_id=0;
                    var_id = vars.lookup(LPHTTMapper::SUPPORT_lambda,i_a, s_a,k_a, i_b, s_b,k_b);
                    result = rb_solution->get_value(var_id);
                     cout<<"lambda"<<i_a<<"_"<<s_a<<"_"<<k_a<<"_"<<i_b<<"_" <<s_b<<"_"<<k_b<<"="<<result<<endl;

                    foreach(processors.get_processors(),pj)
                      {
                      var_id = vars.lookup(LPHTTMapper::SUPPORT_Z,i_a, s_a,k_a, i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                      result = rb_solution->get_value(var_id);
                      cout<<"z"<<i_a<<"_"<<s_a<<"_"<<k_a<<"_"<<i_b<<"_"<<s_b<<"_"<<k_b<<"_"<<pj->get_cluster_id()<<"_"<<p_j<<"="<<result<<endl;
                      p_j++;
                      }
                    p_j=0;
                    }
                    }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;
           }
           s_a++;
          }
           s_a=0;i_a++;
         }
  
   foreach(tasks.get_tasks(),task)
     {
      cout << "/===========Task " << i << "===========/" << endl;

      double result;
      for(int s=0;s<task->get_vnode_num();s++)
       for(int k=0;k<hyperperiod/task->get_period();k++)
         {
        uint var_id=0;
        var_id =vars.lookup(LPHTTMapper::ARRIVE_TIME, i, s, k);
        result = rb_solution->get_value(var_id);
        cout << "task " << i << "_" << s <<  "_" << k << "\t ARRIVE_TIME="<< result;

        var_id=vars.lookup(LPHTTMapper::FINISH_TIME, i, s, k);
        result = rb_solution->get_value(var_id);
        cout  << "\t FINISH_TIME="<< result;

         int j=0;
         foreach(processors.get_processors(),pj)
        {
          var_id=vars.lookup(LPHTTMapper::PARTITION, i, s, k,pj->get_cluster_id(),j);
          result = rb_solution->get_value(var_id);
          if(result==1)
          cout << "\t PARTITION="<< j <<endl;
          j++;
         }
         }
      i++;
      }
    rb_solution->show_error();
    cout << rb_solution->get_status() << endl;
    set_status(rb_solution->get_status());
    delete rb_solution;
    return true;

  }

  delete rb_solution;
  return false;
}

void LP_HTT::construct_exp(LPHTTMapper* vars, LinearExpression* exp,
                                    uint type, uint part_1, uint part_2,
                                    uint part_3) {
  exp->get_terms().clear();
  uint var_id;

  var_id = vars->lookup(type, part_1, part_2, part_3);
  exp->add_term(var_id, 1);

#if ILP_SOLUTION_VAR_CHECK == 1
// cout<<vars.var2str(var_id)<<":"<<endl;
#endif
}

void LP_HTT::set_objective(LinearProgram* lp, LPHTTMapper* vars) {
  // any objective function is okay.
  // assert(0 < tasks.get_tasks().size());

  LinearExpression* obj = new LinearExpression();
  uint var_id;
  uint i=0,s=0;
  // uint i = tasks.get_taskset_size() - 1;
  foreach(tasks.get_tasks(),ti)
  {  
    foreach(ti->get_vnodes(),ns)
    {
     for(int k=0;k<hyperperiod/ti->get_period();k++) 
     {
     if(s==ti->get_vnode_num()-1)
     {var_id = vars->lookup(LPHTTMapper:: FINISH_TIME, i,s,k);
     obj->add_term(var_id, 1);
     }
     }
     s++;
    }
   i++;
   s=0;
  }
  cout<<"obj terms size="<<obj->get_terms_size()<<endl;
  lp->set_objective(obj);
}

void LP_HTT::add_constraints(LinearProgram* lp, LPHTTMapper* vars) {
  constraint_1(lp, vars);
  cout<<"add constraints 1 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_2(lp, vars);
  cout<<"add constraints 2 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_3_1(lp, vars);
  cout<<"add constraints 3-1 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_3_2(lp, vars);
  cout<<"add constraints 3-2 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_4_1(lp, vars);
  cout<<"add constraints 4-1 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_4_2(lp, vars);
  cout<<"add constraints 4-2 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
   constraint_4_3(lp, vars);
  cout<<"add constraints 4-3 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_4_4(lp, vars);
  cout<<"add constraints 4-4 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_5_1(lp, vars);
  cout<<"add constraints 5-1 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_5_2(lp, vars);
  cout<<"add constraints 5-2 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_support_z_1(lp,vars);
  cout<<"add constraints z-1 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_support_z_2(lp,vars);
  cout<<"add constraints z-2 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_support_z_3(lp,vars);
  cout<<"add constraints z-3 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_support_lambda__1(lp,vars);
  cout<<"add constraints lambda-1 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl;
  constraint_support_lambda__2(lp,vars);
  cout<<"add constraints lambda_2 ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl; 
  constraint_by_wyg(lp,vars);
  cout<<"add constraints wyg ineq_num"<<lp->get_inequalities().size()<<"eq_num"<<lp->get_equalities().size()<<endl; 
}

/** Expressions */
// C1
void LP_HTT::constraint_1(LinearProgram* lp, LPHTTMapper* vars) {
  uint var_id;
  int i=0,s=0;
  int64_t wcet=0;
  foreach(tasks.get_tasks(),ti)
  {
    foreach(ti->get_vnodes(),ns)
    { 
      for(uint k=0;k<hyperperiod/ti->get_period();k++)
	    {
           LinearExpression* exp = new LinearExpression();
           var_id = vars->lookup(LPHTTMapper::EXCUTE_TIME, i, s,k);
           exp->add_term(var_id, -1);
           lp->declare_variable_integer(var_id);
           lp->declare_variable_bounds(var_id, true, 0, false, -1);
           wcet=ns->wcet;
           lp->add_inequality(exp,-1*wcet);        
            }
	s++;
    }
    i++;
    s=0;
  }
}


void LP_HTT::constraint_2(LinearProgram* lp, LPHTTMapper* vars) {
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();
  uint t_num = tasks.get_taskset_size();
  
  uint var_id;
  uint i=0,s=0;
  foreach(tasks.get_tasks(),ti)
  {
    foreach(ti->get_vnodes(),ns)
    {
   for(uint k=0;k<hyperperiod/ti->get_period();k++)	
      {
       if(s==ti->get_vnode_num()-1)
	{LinearExpression* exp = new LinearExpression();
	var_id = vars->lookup(LPHTTMapper::FINISH_TIME, i, ti->get_vnode_num()-1,k);
           exp->add_term(var_id, 1);
	  lp->declare_variable_integer(var_id);
          lp->declare_variable_bounds(var_id, true, 0, false, -1);
          lp->add_inequality(exp, (1+k)*ti->get_period());}
       }
        s++;
    }
   s=0;
  i++;  
  }
}

  void LP_HTT::constraint_3_1(LinearProgram* lp, LPHTTMapper* vars)
  {

  uint var_id;
  uint i=0,s=0;
  int64_t period=0;
  foreach(tasks.get_tasks(),ti)
   {
   for(int k=0;k<hyperperiod/ti->get_period();k++)	
      {
  	LinearExpression* exp = new LinearExpression();
         var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME, i, 0,k);
           exp->add_term(var_id, -1);
           lp->declare_variable_integer(var_id);
	   lp->declare_variable_bounds(var_id, true, 0, false, -1);
           period=ti->get_period();
           lp->add_inequality(exp, -1*k*period);
      }    
     i++;   
    }
  }

  // 3-9
  void LP_HTT::constraint_3_2(LinearProgram* lp, LPHTTMapper* vars)           
  {
  LinearExpression* exp = new LinearExpression();
  uint var_id;
  uint i=0,s=0;
      foreach(tasks.get_tasks(),ti)
   {
    foreach(ti->get_vnodes(),ns)
    {
   for(uint k=0;k<hyperperiod/ti->get_period();k++)	
      {
        if(s<ti->get_vnode_num()-1)
       {
 	 LinearExpression* exp = new LinearExpression();
        var_id = vars->lookup(LPHTTMapper::FINISH_TIME, i, s,k);
        exp->add_term(var_id, 1); 
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
	lp->declare_variable_integer(var_id);
        var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME, i, s+1,k);   
        exp->add_term(var_id, -1);
        lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
        lp->add_inequality(exp,0);
       }
       }        
       s++;
    }
   s=0;
   i++;  
   }
  }


  // 3-10  划分任务执行类别                                                   
  void LP_HTT::constraint_4_1(LinearProgram* lp, LPHTTMapper* vars)
  {
   
  uint var_id;
  uint i=0,s=0,j=0;
  uint B=0;
      foreach(tasks.get_tasks(),ti)
   {
    foreach(ti->get_vnodes(),ns)
    {
      for(uint k=0;k<hyperperiod/ti->get_period();k++)
       {
         foreach(processors.get_processors(),pj)                     
         {
           if(pj->get_cluster_id()==ns->type)
            B=1;
            else 
	    B=0;
           LinearExpression* exp = new LinearExpression();
           var_id = vars->lookup(LPHTTMapper::EXCUTE_TIME, i, s,k,pj->get_cluster_id(),j);
           exp->add_term(var_id, 1-B);
	   lp->declare_variable_integer(var_id);
           lp->declare_variable_bounds(var_id, true, 0, false, -1);
           lp->add_equality(exp,0);
           j++;
         }
       j=0;
       }        
       s++;
    }
   s=0;
   i++;  
   }
   
  }

  // 3-13-1
  void LP_HTT::constraint_4_2(LinearProgram* lp, LPHTTMapper* vars)
  {
 
  uint var_id;
  uint i=0,s=0,j=0;
  uint map_bool;
   foreach(tasks.get_tasks(),ti)
   {
    foreach(ti->get_vnodes(),ns)
    {
     for(uint k=0;k<hyperperiod/ti->get_period();k++)	
      {
        foreach(processors.get_processors(),pj)
        {
        LinearExpression* exp = new LinearExpression();
       var_id = vars->lookup(LPHTTMapper::EXCUTE_TIME, i, s,k,pj->get_cluster_id(),j);         
           exp->add_term(var_id, -1);
	lp->declare_variable_integer(var_id);
	lp->declare_variable_bounds(var_id, true, 0, false, -1);

        var_id = vars->lookup(LPHTTMapper::PARTITION, i, s,k,pj->get_cluster_id(),j);
           exp->add_term(var_id, 1);
	   lp->declare_variable_integer(var_id);
	   lp->declare_variable_bounds(var_id, true, 0, true, 1);
           lp->add_inequality(exp,0);
           j++;
        }
	j=0;
       }        
       s++;
    }
   s=0;
   i++;  
   }
   
  }
  // 3-13-2
  void LP_HTT::constraint_4_3(LinearProgram* lp,LPHTTMapper* vars)
  {
    
  LinearExpression* exp = new LinearExpression();
  uint var_id;
  uint i=0,s=0,j=0;
  int minus_O_max=(-1)*O_max;
  uint map_bool;
      foreach(tasks.get_tasks(),ti)
   {
    foreach(ti->get_vnodes(),ns)
    {
   for(uint k=0;k<hyperperiod/ti->get_period();k++)	
      {
        foreach(processors.get_processors(),pj)
        {
	LinearExpression* exp = new LinearExpression();
       var_id = vars->lookup(LPHTTMapper::EXCUTE_TIME, i, s,k,pj->get_cluster_id(),j);         
        exp->add_term(var_id, 1);
        lp->declare_variable_integer(var_id);
	lp->declare_variable_bounds(var_id, true, 0, false, -1);
        var_id = vars->lookup(LPHTTMapper::PARTITION, i, s,k,pj->get_cluster_id(),j);
	lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);
        exp->add_term(var_id, minus_O_max);
        lp->declare_variable_integer(var_id);
	lp->declare_variable_bounds(var_id, true, 0, true, 1);
           lp->add_inequality(exp,0);
           j++;
        }
	j=0;
       }        
       s++;
    }
   s=0;
   i++;  
   }
    
   }

  // 3-12
  void LP_HTT::constraint_4_4(LinearProgram* lp, LPHTTMapper* vars)
  {

  uint var_id;
  uint i=0,s=0,j=0;
      foreach(tasks.get_tasks(),ti)
   {
    foreach(ti->get_vnodes(),ns)
    {
   for(uint k=0;k<hyperperiod/ti->get_period();k++)	
      {
 	LinearExpression* exp = new LinearExpression();
        foreach(processors.get_processors(),pj)
         {
          var_id = vars->lookup(LPHTTMapper::PARTITION, i, s,k,pj->get_cluster_id(),j);
           exp->add_term(var_id, 1);
           lp->declare_variable_integer(var_id);
	   lp->declare_variable_bounds(var_id, true, 0, true, 1);
	j++;
         }
         lp->add_equality(exp,1);
	j=0;
       }        
       s++;
    }
   s=0;
   i++;  
   }
   	
  }

  // 3-15-1
  void LP_HTT::constraint_5_1(LinearProgram* lp,LPHTTMapper* vars)
  {
 
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {   
              if(i_a!=i_b)
               { 
                 //cout<<"i_a:"<<i_a<<"i_b:"<<i_b<<"s_a:"<<s_a<<"s_b:"<<s_b<<"k_a:"<<k_a<<"k_b:"<<k_b<<endl;
 		 LinearExpression* exp = new LinearExpression(); 
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME, i_a, s_a,k_a);
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::EXCUTE_TIME, i_a, s_a,k_a);
                 exp->add_term(var_id,1);
                lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);;
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME, i_b, s_b,k_b);
                 exp->add_term(var_id,-1);
                lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_lambda,i_a, s_a,k_a, i_b, s_b,k_b);
                 exp->add_term(var_id,O_max);
		 lp->declare_variable_integer(var_id);
		  lp->declare_variable_bounds(var_id, true, 0, true, 1);

                 foreach(processors.get_processors(),pj)
                 {
                  var_id = vars->lookup(LPHTTMapper::SUPPORT_Z,i_a, s_a,k_a, i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                  exp->add_term(var_id,O_max);
		  lp->declare_variable_bounds(var_id, true, 0, true, 1);
                  lp->declare_variable_integer(var_id);
                  p_j++;
                 }
                 lp->add_inequality(exp,2*O_max);}
                 p_j=0;
                }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;}
   

  }

  
  // 3-15-2
  void LP_HTT::constraint_5_2(LinearProgram* lp, LPHTTMapper* vars)
  {
  
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {
              if(i_a!=i_b)
               {
		//cout<<"i_a:"<<i_a<<"i_b:"<<i_b<<"s_a:"<<s_a<<"s_b:"<<s_b<<"k_a:"<<k_a<<"k_b:"<<k_b<<endl;
		  LinearExpression* exp = new LinearExpression();
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME,i_b, s_b,k_b);
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::EXCUTE_TIME,i_b, s_b,k_b );
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME,i_a, s_a,k_a );
                 exp->add_term(var_id,-1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_lambda,i_a, s_a,k_a, i_b, s_b,k_b);
                 exp->add_term(var_id,-1*O_max);
		lp->declare_variable_integer(var_id);
                lp->declare_variable_bounds(var_id, true, 0, true, 1);
                 foreach(processors.get_processors(),pj)
                 {
                  var_id = vars->lookup(LPHTTMapper::SUPPORT_Z,i_a, s_a,k_a, i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                  exp->add_term(var_id,O_max);
                  lp->declare_variable_integer(var_id);
		  lp->declare_variable_bounds(var_id, true, 0, true, 1);
                  p_j++;
                 }
                 lp->add_inequality(exp,O_max);}
                  p_j=0;
            }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;}
      
  } 



  void LP_HTT::constraint_support_z_1(LinearProgram* lp, LPHTTMapper* vars)
  {
  
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {
               foreach(processors.get_processors(),pj){
                if(i_a!=i_b)
               {
		 LinearExpression* exp = new LinearExpression();
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_Z,i_a, s_a,k_a, i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, true, -1);
                 var_id = vars->lookup(LPHTTMapper::PARTITION,i_a, s_a,k_a,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,-1);
		lp->declare_variable_bounds(var_id, true, 0, true, 1);
		lp->declare_variable_integer(var_id);
                 lp->add_inequality(exp,0);
                 }

                 p_j++;
                 }
                 p_j=0;
            }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;} 
     
  }


  void LP_HTT::constraint_support_z_2(LinearProgram* lp, LPHTTMapper* vars)
  {
        
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {
              if(i_a!=i_b)
               {
                 foreach(processors.get_processors(),pj)
                 {
	         LinearExpression* exp = new LinearExpression();
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_Z,i_a, s_a,k_a, i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, true, -1);
		

                 var_id = vars->lookup(LPHTTMapper::PARTITION,i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,-1);
		lp->declare_variable_integer(var_id);	
		lp->declare_variable_bounds(var_id, true, 0, true, 1);

                 lp->add_inequality(exp,0);}
                 p_j++;
                 }
                 p_j=0;
            }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;}
     
  }

  void LP_HTT::constraint_support_z_3(LinearProgram* lp, LPHTTMapper* vars)
  {
   
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {
              foreach(processors.get_processors(),pj)
                 {
                  if(i_a!=i_b)
               {
        	 LinearExpression* exp = new LinearExpression();
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_Z,i_a, s_a,k_a, i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,-1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, true, 1);

                 var_id = vars->lookup(LPHTTMapper::PARTITION,i_b, s_b,k_b,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, true, 1);

                 var_id = vars->lookup(LPHTTMapper::PARTITION,i_a, s_a,k_a,pj->get_cluster_id(),p_j);
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		 lp->declare_variable_bounds(var_id, true, 0, true, 1);

                 lp->add_inequality(exp,1);}
                 p_j++;
                 }
                 p_j=0;
            }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;}
     
  }
  // 3-17-1
  void LP_HTT::constraint_support_lambda__1(LinearProgram* lp, LPHTTMapper* vars)
  {
	
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  int minus_O_max=(-1)*O_max;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {
              if(i_a!=i_b)
               {
  		 LinearExpression* exp = new LinearExpression();
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME,i_a, s_a,k_a);
                 exp->add_term(var_id,-1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME,i_b, s_b,k_b );
                 exp->add_term(var_id,1);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_lambda,i_a, s_a,k_a,i_b, s_b,k_b );
                 exp->add_term(var_id,minus_O_max);
		lp->declare_variable_integer(var_id);
		lp->declare_variable_bounds(var_id, true, 0, true, 1);
                 lp->add_inequality(exp,0);}
            }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;}
  	
  }
    // 3-17-2
  void LP_HTT::constraint_support_lambda__2(LinearProgram* lp, LPHTTMapper* vars)
  {  
  
  uint var_id;
  uint i_a=0,i_b=0,s_a=0,s_b=0,p_j=0;
  foreach(tasks.get_tasks(),ti_a)
    {foreach(ti_a->get_vnodes(),ns_a)
      {for(uint k_a=0;k_a<hyperperiod/ti_a->get_period();k_a++)	
       {foreach(tasks.get_tasks(),ti_b)
          {foreach(ti_b->get_vnodes(),ns_b)
            {for(uint k_b=0;k_b<hyperperiod/ti_b->get_period();k_b++)
            {
                if(i_a!=i_b)
               {
  	         LinearExpression* exp = new LinearExpression();
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME,i_a, s_a,k_a);
                 exp->add_term(var_id,1);
		 lp->declare_variable_integer(var_id);
		 lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME,i_b, s_b,k_b );
                 exp->add_term(var_id,-1);
	    	 lp->declare_variable_integer(var_id);
		 lp->declare_variable_bounds(var_id, true, 0, false, -1);
                 var_id = vars->lookup(LPHTTMapper::SUPPORT_lambda,i_a, s_a,k_a,i_b, s_b,k_b );
                 exp->add_term(var_id,O_max);
		lp->declare_variable_integer(var_id);
		 lp->declare_variable_bounds(var_id, true, 0, true, 1);
                 lp->add_inequality(exp,O_max-o_min);}
            }
           s_b++; }
           i_b++;s_b=0;}
           i_b=0;}
           s_a++;}
           s_a=0;i_a++;}
	
   }


  uint64_t LP_HTT::get_hyperperiod()
  {	uint64_t  lcm=1;
   	foreach(tasks.get_tasks(),ti) 
        {				
   		lcm = lcm*(ti->get_period())/(__gcd(lcm,ti->get_period())) ; //求最小公倍数 
	}
        return lcm;
  
  }


void LP_HTT::constraint_by_wyg(LinearProgram* lp, LPHTTMapper* vars)
{
  LinearExpression* exp = new LinearExpression();
  uint var_id;
  uint i=0,s=0;
      foreach(tasks.get_tasks(),ti)
   {
    foreach(ti->get_vnodes(),ns)
    {
   for(uint k=0;k<hyperperiod/ti->get_period();k++)	
      {
 	LinearExpression* exp = new LinearExpression();
        var_id = vars->lookup(LPHTTMapper::FINISH_TIME, i, s,k);
        exp->add_term(var_id, -1); 
	lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
        var_id = vars->lookup(LPHTTMapper::ARRIVE_TIME, i, s,k);   
        exp->add_term(var_id, 1);
    	lp->declare_variable_integer(var_id);
        lp->declare_variable_bounds(var_id, true, 0, false, -1);
        int wcet=ns->wcet;
        lp->add_inequality(exp,-1*wcet);
       }        
       s++;
    }
   s=0;
   i++;  
   }

}


bool LP_HTT::alloc_schedulable() {}
