#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <np_edf.h>


NP_EDF::NP_EDF()
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "", "NONE") {}
   
NP_EDF::NP_EDF(NPTaskSet tasks, ProcessorSet processors,
                       ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "", "NONE") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;
  //this->resources.update(&(this->tasks));
  //this->processors.update(&(this->tasks), &(this->resources));
  this->processors.init();
  //hyperperiod=get_hyperperiod();
}

NP_EDF::~NP_EDF(){}

void NP_EDF:: Scheduler() {
        currentTime = 0;
}

void NP_EDF:: scheduler() {

//分成两个任务队列，一个已到达，一个未到达 
        tasks.sort_by_deadline();
        tasks.sort_by_deadline_pendingTasks();
        // sort(readyTasks.begin(), readyTasks.end(), NPTask::compareByDeadlineAndPeriod);
        // sort(pendingTasks.begin(), pendingTasks.end(), NPTask::compareByDeadlineAndPeriod);

        while (!tasks.get_np_tasks().empty() || !tasks.get_pending_tasks().empty()) {
            if(!tasks.get_np_tasks().empty()){
                NPTask currentTask = tasks.get_np_tasks().front();
                tasks.get_np_tasks().erase(tasks.get_np_tasks().begin());

                if (currentTask.get_remaining_execution_time() > 0) {
                // 执行任务
                    if (currentTask.get_start_time() == -1) {
                        currentTask.set_start_time(currentTime);
                    }
                    currentTask.set_remaining_execution_time(currentTask.get_remaining_execution_time() - 1);
                    if (currentTask.get_remaining_execution_time() == 0) {
                        currentTask.set_finish_time(currentTime + 1);
                        currentTask.display_task();
                        add_period_task(currentTask);
                        tasks.sort_by_deadline();
                        if(!tasks.get_np_tasks().empty())
                            judge(currentTask);
                    } else {
                        tasks.get_np_tasks().insert(tasks.get_np_tasks().begin(),currentTask);
                    }
                }
            }

            currentTime++;

            // 将已到达的新任务加入任务向量
            for (int i = 0; i < tasks.get_pending_tasks().size(); i++) {
                NPTasks TempTask = tasks.get_pending_tasks();
                if (TempTask[i].get_arrive_time()<= currentTime) {
                    tasks.get_np_tasks().push_back(TempTask[i]);
                    tasks.get_pending_tasks().erase(tasks.get_pending_tasks().begin() + i);
                    i--;
                }
            }
            if(currentTime == 200)
              break;
        }
    }
  bool NP_EDF::alloc_schedulable(){ return true; }
  bool NP_EDF::is_schedulable(){ return true; }

  void NP_EDF::add_task(NPTask task) {
  if (task.get_arrive_time() <= currentTime) {
    tasks.get_np_tasks().push_back(task);
  } 
  else {
    tasks.get_pending_tasks().push_back(task);
  }
}

void NP_EDF::add_period_task(NPTask task) {
    // task.set_deadline(task.get_deadline() + task.get_period());
    // task.set_arrive_time(task.get_arrive_time() + task.get_period());
    // add_task(task);
}

void NP_EDF::judge(NPTask task) {
  NPTasks TempTasks = tasks.get_np_tasks();
  NPTask nextTask = TempTasks[0];
  NPTask nextPendingTask = tasks.get_pending_tasks().front();
  for(int i = 0;i < TempTasks.size();i++){
      if(task.get_id() == nextPendingTask.get_id()){
          if(nextPendingTask.get_arrive_time() + nextPendingTask.get_period() - nextPendingTask.get_wcet() >= currentTime + TempTasks[i].get_wcet() + 1){
              for(auto iter=tasks.get_np_tasks().begin();iter!=tasks.get_np_tasks().end();)
                  if( (*iter).get_id() == TempTasks[i].get_id())
                      tasks.get_np_tasks().erase(iter);
                  else
                      iter++;    
              tasks.get_np_tasks().insert(tasks.get_np_tasks().begin(),TempTasks[i]);
          }
          else{
              currentTime = nextPendingTask.get_arrive_time() - 1;
          }
      }
      else{
          if(nextPendingTask.get_arrive_time() >= currentTime + TempTasks[i].get_wcet() + 1){
              for(auto iter=tasks.get_np_tasks().begin();iter!=tasks.get_np_tasks().end();)
                  if( (*iter).get_id() == TempTasks[i].get_id())
                      tasks.get_np_tasks().erase(iter);
                  else
                      iter++;    
              tasks.get_np_tasks().insert(tasks.get_np_tasks().begin(),TempTasks[i]);
          }
          else{
              currentTime = nextPendingTask.get_arrive_time() - 1;
          }
      }
  }
}



