#include <iteration-helper.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <resources.h>
#include <tasks.h>
#include <rta_pso.h>


int main(int argc, char **argv) {
  vector<Param> parameters = get_parameters();
  Param param = parameters[0];
  BPTaskSet tasks = BPTaskSet();
  ProcessorSet processors = ProcessorSet(param);
  ResourceSet resources = ResourceSet();
  resource_gen(&resources, param);
  tasks.task_gen(&resources, param, 1, 10);
  tasks.task_copy(tasks.get_taskset_size());
  cout<<"now t_num is"<<tasks.get_taskset_size()<<endl;
  RTA_PSO *pso_test=new RTA_PSO(tasks, processors,resources) ;
  int** solution_arry=pso_test->PSO(10,20);
  int* solution=pso_test->choose_solution(solution_arry,20);
  for(int i=0;i<tasks.get_taskset_size();i++)
  printf("%d \t",solution[i]);
  cout<<endl;

  return 0;
}
