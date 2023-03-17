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
#include <rta_dag_gfp_jose.h>

int main(int argc, char **argv) {
  vector<Param> parameters = get_parameters();

  Param param = parameters[0];

  DAG_TaskSet dag_tasks = DAG_TaskSet();
  ProcessorSet processors = ProcessorSet(param);
  ResourceSet resources = ResourceSet();
  resource_gen(&resources, param);
  // dag_tasks.task_gen_v2(&resources, param, 1, 1);

  DAG_Task task = DAG_Task(0,10000);

  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);

  // task.add_arc(0,5);
  // task.add_arc(1,3);
  // task.add_arc(1,5);
  // task.add_arc(2,3);
  // task.add_arc(2,4);


  task.add_job(100);
  task.add_job(100);
  task.add_job(100);
  task.add_job(100);
  task.add_job(100);
  task.add_job(100);

  task.add_arc(0,5);
  task.add_arc(1,3);
  task.add_arc(1,5);
  task.add_arc(2,3);
  task.add_arc(2,4);

  // task.add_arc(0,2);
  // task.add_arc(1,2);
  // task.add_arc(1,4);
  // task.add_arc(3,4);
  // task.add_arc(3,5);

  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);
  // task.add_job(100);

  // task.add_arc(0,7);
  // task.add_arc(1,7);
  // task.add_arc(1,5);
  // task.add_arc(2,5);
  // task.add_arc(2,6);

  dag_tasks.add_task(task);

  resources.update(&dag_tasks);
  processors.update(&dag_tasks,&resources);

  task.update_parallel_degree();
  // cout << "maximum matching:" << task.maximumMatch() << endl;
  cout << "parallel degree:" << task.get_parallel_degree() << endl;

  // task.display();

  RTA_DAG_GFP_JOSE(dag_tasks, processors, resources);


  // foreach(dag_tasks.get_tasks(), ti) {
  //   cout << "Utilization:" << ti->get_utilization() << endl;
  //   cout << "Parallel degree:" << ti->get_parallel_degree() << endl;
  //   foreach(ti->get_requests(), request) {
  //     cout << "Request:" << request->get_resource_id() << " num:" << request->get_num_requests() << " len:" << request->get_max_length() << endl; 
  //   }
  //   ti->display();
  //   Paths paths = ti->get_paths();
  //   uint32_t i = 0;
  //   foreach(paths, path) {
  //     cout << "Path " << i++ << ":" << endl;
  //     cout << "  subjobs: ";
  //     uint64_t verified_len = 0;
  //     foreach(path->vnodes, vnode) {
  //       if (0 == vnode->follow_ups.size()) {
  //         cout << "v" << vnode->job_id << endl;
  //         verified_len += vnode->wcet;          
  //       } else {
  //        cout << "v" << vnode->job_id << "->";
  //         verified_len += vnode->wcet;   
  //       }
  //     }
  //     cout << "Verfied length:" << verified_len << endl;
  //     cout << "  wcet:" << path->wcet << " wcet_ncs:" << path->wcet_non_critical_section << " wcet-cs:" << path->wcet_critical_section << endl;
  //     foreach(path->requests, request) {
  //       cout << "  Request:" << request->get_resource_id() << " num:" << request->get_num_requests() << " len:" << request->get_max_length() << endl; 
  //     }
  //     if (verified_len != path->wcet)
  //       cout << "!!!!!!!!!!!!!!!" << endl;
  //   }
  //   cout << "----------------------" << endl; 
  // }

  // foreach(resources.get_resources(), resource) {
  //   cout << "Resource: " << resource->get_resource_id() << endl;
  //   foreach(resource->get_taskqueue(), t_id) {
  //     cout << " T" << (*t_id);
  //   }
  //   cout << endl;
  //   cout << "Utilization:" << resource->get_utilization() << endl;
  // }
  return 0;
}