// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_BIN_PACKING_H_
#define INCLUDE_SCHEDTEST_BIN_PACKING_H_

#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_pfp.h>
#include <tasks.h>
#include <algorithm>
#include <iostream>


bool worst_fit_dm(TaskSet* tasks, ProcessorSet* processors,
                  ResourceSet* resources, uint t_id, uint TEST_TYPE,
                  uint ITER_BLOCKING) {
  fraction_t min_u = 1;
  uint assign;
  Task& task = tasks->get_tasks()[t_id];
  for (uint cpu_id = 0; cpu_id < processors->get_processor_num(); cpu_id++) {
    Processor& processor = processors->get_processors()[cpu_id];
    if (min_u >= processor.get_utilization()) {
      min_u = processor.get_utilization();
      assign = cpu_id;
    }
  }
  Processor& processor = processors->get_processors()[assign];
  task.set_partition(assign);
  processor.add_task(task.get_id());

  if (is_pfp_rta_schedulable(*tasks, *processors, *resources, TEST_TYPE,
                             ITER_BLOCKING)) {
    return true;
  }
  return false;
}

bool first_fit_dm(TaskSet* tasks, ProcessorSet* processors,
                  ResourceSet* resources, uint t_id, uint TEST_TYPE,
                  uint ITER_BLOCKING) {
  uint assign;
  Task& task = tasks->get_tasks()[t_id];
  for (uint cpu_id = 0; cpu_id < processors->get_processor_num(); cpu_id++) {
    assign = cpu_id;
    Processor& processor = processors->get_processors()[assign];
    processor.add_task(task.get_id());
    task.set_partition(assign);
    if (is_pfp_rta_schedulable(*tasks, *processors, *resources, TEST_TYPE,
                               ITER_BLOCKING)) {
      return true;
    } else {
      processor.remove_task(task.get_id());
    }
  }

  return false;
}

bool worst_fit_edf(TaskSet* tasks, ProcessorSet* processors,
                   ResourceSet* resources, uint t_id, uint TEST_TYPE,
                   uint ITER_BLOCKING) {
  fraction_t min_u = 1;
  uint assign;
  Task& task = tasks->get_tasks()[t_id];
  for (uint cpu_id = 0; cpu_id < processors->get_processor_num(); cpu_id++) {
    Processor& processor = processors->get_processors()[cpu_id];
    if (min_u >= processor.get_utilization()) {
      min_u = processor.get_utilization();
      assign = cpu_id;
    }
  }

  Processor& processor = processors->get_processors()[assign];
  if (processor.get_utilization() + task.get_utilization() <= 1) {
    task.set_partition(assign);
    processor.add_task(task.get_id());
    return true;
  }
  return false;
}

bool worst_fit_pfp(TaskSet* tasks, ProcessorSet* processors,
                   ResourceSet* resources, uint t_id, uint TEST_TYPE,
                   uint ITER_BLOCKING) {
  fraction_t min_u = 1;
  uint assign;
  Task& task = tasks->get_tasks()[t_id];
  for (uint cpu_id = 0; cpu_id < processors->get_processor_num(); cpu_id++) {
    Processor& processor = processors->get_processors()[cpu_id];
    if (min_u >= processor.get_utilization()) {
      min_u = processor.get_utilization();
      assign = cpu_id;
    }
  }

  Processor& processor = processors->get_processors()[assign];
  if (processor.get_utilization() + task.get_utilization() <= 1) {
    task.set_partition(assign);
    processor.add_task(task.get_id());
    return true;
  }
  return false;
}

#endif  // INCLUDE_SCHEDTEST_BIN_PACKING_H_
