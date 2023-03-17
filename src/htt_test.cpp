#include <stdlib.h>
#include <iteration-helper.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <resources.h>
#include <tasks.h>
#include <lp_htt.h>
#include <lp_rta_pfp_mpcp_heterogeneous.h>
#include <types.h>


int main()
{
 
  vector<Param> parameters = get_parameters();
  Param param = parameters[0];
  param.p_range.max=50;
  param.p_range.min=10;

  DAG_TaskSet tasks = DAG_TaskSet();
  ProcessorSet processors = ProcessorSet(param);
  uint j=0;
  foreach(processors.get_processors(),pj)
  { 
    pj->set_cluster_id(8);   
  }
  ResourceSet resources = ResourceSet();
  resource_gen(&resources, param);
   cout<<"try task_gen "<<endl;
  /*
  tasks.task_gen_v2(&resources, param, 1, 4);
   int i=0;
   uint s=0;
   foreach(tasks.get_tasks(),ti)
  {
    ti->set_period(100*(i/3+1));
    i++;
  }    */
   
  DAG_Task temp_dag_task = DAG_Task(1,100,100,0);
  temp_dag_task.add_job(5,100);
  tasks.add_task(temp_dag_task);
  temp_dag_task.set_id(2);
  tasks.add_task(temp_dag_task);
  temp_dag_task.set_id(3);
  tasks.add_task(temp_dag_task);
  temp_dag_task.set_id(4);
  tasks.add_task(temp_dag_task);
   
  cout<<"add over"<<endl;

   foreach(tasks.get_tasks(),ti)
  {
    ti->display();
  }  
   cout<<"task_gen success "<<endl;
   LP_HTT *htt_test=new LP_HTT(tasks, processors,resources) ;
   if(htt_test->is_schedulable())
   cout<<"schedulable"<<endl;
   else 
   cout<<"unscheduable"<<endl;
   return 0;
}
