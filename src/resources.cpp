// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <resources.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <param.h>
#include <processors.h>
#include <random_gen.h>
#include <sort.h>
#include <tasks.h>

  // if (NULL != tasks) {

  // } else if (NULL != dag_tasks) {

  // } else {

  // }

/** Class Resource */
Resource::Resource(uint id, uint locality, bool global_resource,
                   bool processor_local_resource) {
  this->resource_id = id;
  this->locality = locality;
  this->global_resource = global_resource;
  this->processor_local_resource = processor_local_resource;
  this->utilization = 0;
  this->tasks = NULL;
  this->dag_tasks = NULL;

}

Resource::~Resource() {
  /*
          foreach(queue, element)
          {
                  delete *element;
          }
  */
}

void Resource::init() {
  locality = MAX_INT;
  global_resource = false;
  processor_local_resource = false;
  // utilization = 0;
  // queue.clear();
}

uint Resource::get_resource_id() const { return resource_id; }
void Resource::set_locality(uint locality) { this->locality = locality; }
uint Resource::get_locality() const { return locality; }
double Resource::get_utilization() {
  utilization = 0;
  if (NULL != tasks) {
    foreach(queue, id) {
      Task& task = tasks->get_task_by_id(*id);
      const Request& request = task.get_request_by_id(resource_id);
      double u;
      u = request.get_num_requests() * request.get_max_length();
      u /= task.get_period();
      utilization += u;
    }
    return utilization;
  } else if (NULL != dag_tasks) {
    foreach(queue, id) {
      DAG_Task& dag_task = dag_tasks->get_task_by_id(*id);
      const Request& request = dag_task.get_request_by_id(resource_id);
      double u;
      u = request.get_num_requests() * request.get_max_length();
      u /= dag_task.get_period();
      utilization += u;
    }
    return utilization;
  } else {
    return 0;
  }
}

bool Resource::is_global_resource() {
  if (1 >= queue.size()) return false;
  global_resource = false;

  uint first_id = MAX_INT;

  if (NULL != tasks) {
    foreach(queue, id) {
      Task& task = tasks->get_task_by_id(*id);

      if (MAX_INT == task.get_partition()) continue;

      if (MAX_INT == first_id) first_id = *id;

      if (tasks->get_task_by_id(first_id).get_partition() !=
          task.get_partition()) {
        global_resource = true;
        break;
      }
    }
  } else if (NULL != dag_tasks) {
    foreach(queue, id) {
      DAG_Task& dag_task = dag_tasks->get_task_by_id(*id);
      
      if (MAX_INT == dag_task.get_partition()) return true; // heavy task

      if (MAX_INT == first_id) first_id = *id;

      // if ((MAX_INT == dag_task.get_partition()) && (1 < queue.size())) {
      //   global_resource = true;
      //   break;
      // }

      if (dag_tasks->get_task_by_id(first_id).get_partition() !=
          dag_task.get_partition()) {
        global_resource = true;
        break;
      }
    }
  }

  return global_resource;
}

bool Resource::is_processor_local_resource() const {
  return processor_local_resource;
}
// Request_Tasks Resource::get_tasks() const { return tasks; }

TaskSet* Resource::get_tasks() { return tasks; }

set<uint>& Resource::get_taskqueue() { return queue; }

void Resource::add_task(uint id) { queue.insert(id); }
void Resource::remove_task(uint t_id) { queue.erase(t_id); }

uint Resource::get_ceiling() const {
  uint32_t ceiling = MAX_INT;
  if (NULL != tasks) {
    foreach(queue, id) {
      Task& task = tasks->get_task_by_id(*id);
      if (MAX_INT == task.get_priority()) continue;
      if (ceiling > task.get_priority()) ceiling = task.get_priority();
    }
  } else if (NULL != dag_tasks) {
    foreach(queue, id) {
      DAG_Task& dag_task = dag_tasks->get_task_by_id(*id);
      if (MAX_INT == dag_task.get_priority()) continue;
      if (ceiling > dag_task.get_priority()) ceiling = dag_task.get_priority();
    }
  }

  return ceiling;
}

void Resource::update(TaskSet* tasks) {
  this->tasks = tasks;
  this->dag_tasks = NULL;
}


void Resource::update(BPTaskSet* tasks) {
  this->bptasks = tasks;
  this->dag_tasks = NULL;
}


void Resource::update(DAG_TaskSet* dag_tasks) {
  this->dag_tasks = dag_tasks;
  this->tasks = NULL;
}

/*

TaskQueue& Resource::get_taskqueue()
{
        return queue;
}

void Resource::add_task(void* taskptr)
{
        queue.push_back(taskptr);
        double u;
        switch(((Task*)taskptr)->task_model())
        {
                case TPS_TASK_MODEL:
                        Request& request_tps =
((Task*)taskptr)->get_request_by_id(resource_id); u =
request_tps.get_num_requests() * request_tps.get_max_length(); u /=
((Task*)taskptr)->get_period(); utilization += u; break; case DAG_TASK_MODEL:
                        Request& request_dag =
((DAG_Task*)taskptr)->get_request_by_id(resource_id); u =
request_dag.get_num_requests() * request_dag.get_max_length(); u /=
((DAG_Task*)taskptr)->get_period(); utilization += u; break; default: Request&
request_def = ((Task*)taskptr)->get_request_by_id(resource_id); u =
request_def.get_num_requests() * request_def.get_max_length(); u /=
((Task*)taskptr)->get_period(); utilization += u;
        }
}

uint Resource::get_ceiling()
{
        uint ceiling = 0xffffffff;
        list<void*>::iterator it = queue.begin();
        //for(uint i = 0; it != queue.end(); it++, i++)
        foreach(queue, task)
        {
                if(ceiling >= ((Task*)(*task))->get_priority())
                        ceiling = ((Task*)(*task))->get_priority();
        }
        return ceiling;
}


void Resource::update(const TaskSet tasks)
{
        queue.clear();
        utilization = 0;
        foreach(tasks.get_tasks(), task)
        {
                if(task->is_request_exist(resource_id))
                        add_task((void*)task);
        }
}
*/

// for debug
void Resource::display_task_queue() {
  cout << "Resource " << resource_id << " location:" << locality << endl;
  if (NULL != tasks) {
    foreach(queue, id) {
      Task& task = tasks->get_task_by_id(*id);
      cout << "Task:" << task.get_id() << " partition:" << task.get_partition()
          << " priority:" << task.get_priority() << endl;
    }
  } else if (NULL != dag_tasks) {
    foreach(queue, id) {
      DAG_Task& dag_task = dag_tasks->get_task_by_id(*id);
      cout << "Task:" << dag_task.get_id() << " partition:" << dag_task.get_partition()
          << " priority:" << dag_task.get_priority() << endl;
      // cout << "Task:" << dag_task.get_id()
      //      << " Cluster:[";
      
      // foreach(dag_task.get_cluster(), cluster) {
      //   cout << (*cluster) << " ";
      //   assert(dag_task.get_dcores() == dag_task.get_cluster().size());
      // }
      // cout << "] priority:" << dag_task.get_priority() << " location:" << locality << endl;
    }
  }

  cout << "X" << endl;
}

/** Class ResourceSet */

ResourceSet::ResourceSet() {}

void ResourceSet::init() {
  foreach(resources, resource) { resource->init(); }
}

void ResourceSet::add_resource() {
  bool global = false;
  if (Random_Gen::probability(0.4)) global = true;
  resources.push_back(Resource(resources.size(), 0, global));
}

uint ResourceSet::size() const { return resources.size(); }

void ResourceSet::add_task(uint resource_id, uint id) {
  resources[resource_id].add_task(id);
}

Resources& ResourceSet::get_resources() { return resources; }

Resource& ResourceSet::get_resource_by_id(uint r_id) {
  if (r_id < resources.size()) {
    foreach(resources, resource) {
      if (r_id == resource->get_resource_id())
        return (*resource);
    }
  } else {
    return *reinterpret_cast<Resource*>(0);
  }
}

uint ResourceSet::get_resourceset_size() const { return resources.size(); }

void ResourceSet::sort_by_utilization() {
  sort(resources.begin(), resources.end(), utilization_decrease<Resource>);
}

void ResourceSet::update(TaskSet* tasks) {
  foreach(resources, resource)
    resource->update(tasks);
}


void ResourceSet::update(BPTaskSet* tasks) {
  foreach(resources, resource)
    resource->update(tasks);
}

void ResourceSet::update(DAG_TaskSet* dag_tasks) {
  foreach(resources, resource)
    resource->update(dag_tasks);
}

/** Others */

void resource_gen(ResourceSet* resourceset, Param param) {
  //  For DPCP-p
  // uint32_t n_num = Random_Gen::uniform_ulong_gen(param.resource_num/2,param.resource_num);
  // if (0 != param.p_num)
  //   for (uint i = 0; i < n_num; i++) resourceset->add_resource();
  for (uint i = 0; i < param.resource_num; i++) resourceset->add_resource();
  resource_alloc(resourceset, param.p_num);
  /*
          foreach(resourceset->get_resources(), resource)
          {
                  cout<<"resource:"<<resource->get_resource_id()<<" locate at
     processor:"<<resource->get_locality()<<endl;
          }
  */
}

void resource_alloc(ResourceSet *resources, uint p_num) {
  uint p_idx = 0;

  foreach(resources->get_resources(), resource) {
    p_idx = p_idx % p_num;
    resource->set_locality(p_idx);
    p_idx++;
  }
}
