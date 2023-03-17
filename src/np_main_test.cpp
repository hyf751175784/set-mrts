#include <stdlib.h>
#include <iteration-helper.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <resources.h>
#include <tasks.h>
#include <np_edf.h>
#include <types.h>


int main()
{
 
  vector<Param> parameters = get_parameters();
  Param param = parameters[0];
  param.p_range.max=50;
  param.p_range.min=10;

  //TaskSet tasks = TaskSet();
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
   
    NPTask task1(1, 1, 10, 10, 0);
    NPTask task2(2, 8, 30, 30, 0);
    NPTask task3(3, 17, 60, 60, 0);
    NPTask task4(1, 1, 10, 30, 20);
    NPTask task5(2, 8, 30, 60, 30);
    NPTask task6(1, 1, 10, 40, 30);
    NPTask task7(1, 1, 10, 50, 40);
    NPTask task8(1, 1, 10, 60, 50);
    NPTaskSet tasks = NPTaskSet();


  cout<<"add over"<<endl;

 /*
  foreach(tasks.get_np_tasks(),ti)
  {
    ti.display_task();
  }  
   foreach(tasks.get_pending_tasks(),ti)
  {
    ti.display_task();
  }
*/  
   cout<<"task_gen success "<<endl;

   NP_EDF np_edf= NP_EDF(tasks, processors,resources) ;
   np_edf.Scheduler();

    np_edf.add_task(task1);
    np_edf.add_task(task2);
    np_edf.add_task(task3);
    np_edf.add_task(task4);
    np_edf.add_task(task5);
    np_edf.add_task(task6);
    np_edf.add_task(task7);
    np_edf.add_task(task8);

   np_edf.scheduler();

   return 0;
}

