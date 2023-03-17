/**Copyright [2016] <Zewei Chen>
 * ------By Zewei Chen------
 * Email:czwking1991@gmail.com
 * This program tests task set generation.
 */

#include <iteration-helper.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <resources.h>
#include <tasks.h>
#include <math-helper.h>
#include <math.h>
#include <sort.h>
#include <stdio.h>
#include <time_record.h>
#include <toolkit.h>
#include <xml.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <types.h>
#include <algorithm>

int main(int argc, char **argv) {
  vector<Param> parameters = get_parameters();

  Param param = parameters[0];

  extern TaskSet tasks ;
  tasks = TaskSet();
  ProcessorSet processors = ProcessorSet(param);
  ResourceSet resources = ResourceSet();
  resource_gen(&resources, param);
  tasks.task_gen(&resources, param, 1, 10);
  uint id,index;
  uint priority;
  uint partition; 
  for(int i=0;i<10;i++)
   { 
   index=tasks.tasks[i].get_index();
   id=tasks.tasks[i].get_id();
   priority=tasks.tasks[i].get_partition();
   
   cout << "Task" << index << ": id:"
         << id
         << ": priority:"
         << priority << endl;
  }

  return 0;
}
