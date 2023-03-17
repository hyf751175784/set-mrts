// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

PartitionedSched::PartitionedSched(bool LinearProgramming, uint TestMethod,
                                   uint PriorityAssignment,
                                   uint LockingProtocol, string name,
                                   string remark)
    : SchedTestBase(LinearProgramming, TestMethod, PARTITIONED,
                    PriorityAssignment, LockingProtocol, name, remark) {}

PartitionedSched::~PartitionedSched() {}


bool PartitionedSched::alloc_schedulable() {}

bool PartitionedSched::BinPacking_WF(Task* ti, TaskSet* tasks,
                                     ProcessorSet* processors,
                                     ResourceSet* resources, uint MODE) {
  processors->sort_by_task_utilization(INCREASE);

  Processor& processor = processors->get_processors()[0];
  if (processor.get_utilization() + ti->get_utilization() <= 1) {
    ti->set_partition(processor.get_processor_id());
    processor.add_task(ti->get_id());
  } else {
    return false;
  }

  switch (MODE) {
    case PartitionedSched::UNTEST:
      break;
    case PartitionedSched::TEST:

      if (!alloc_schedulable()) {
        ti->set_partition(MAX_INT);
        processor.remove_task(ti->get_id());
        return false;
      }
      break;
    default:
      break;
  }

  return true;
}

bool PartitionedSched::BinPacking_BF(Task* ti, TaskSet* tasks,
                                     ProcessorSet* processors,
                                     ResourceSet* resources, uint MODE) {
  uint p_num = processors->get_processor_num();
  processors->sort_by_task_utilization(DECREASE);
  uint p_id;

  for (uint assign = 0; assign < p_num; assign++) {
    Processor& processor = processors->get_processors()[assign];
    if (processor.get_utilization() + ti->get_utilization() <= 1) {
      ti->set_partition(processor.get_processor_id());
      processor.add_task(ti->get_id());
      p_id = assign;
      break;
    } else if ((p_num - 1) == assign) {
      return false;
    }
  }

  Processor& processor = processors->get_processors()[p_id];
  switch (MODE) {
    case PartitionedSched::UNTEST:
      break;
    case PartitionedSched::TEST:
      if (!alloc_schedulable()) {
        ti->set_partition(MAX_INT);
        processor.remove_task(ti->get_id());
        return false;
      }
      break;
    default:
      break;
  }

  return true;
}

bool PartitionedSched::BinPacking_FF(Task* ti, TaskSet* tasks,
                                     ProcessorSet* processors,
                                     ResourceSet* resources, uint MODE) {
  // processors->sort_by_task_utilization(INCREASE);

  uint p_num = processors->get_processor_num();

  Processor& processor = processors->get_processors()[0];

  for (uint k = 0; k < p_num; k++) {
    // cout<<"Try to assign Task"<<ti->get_id()<<" on processor"<<k<<endl;
    Processor& processor = processors->get_processors()[k];

    if (processor.add_task(ti->get_id())) {
      ti->set_partition(processor.get_processor_id());
    } else {
      continue;
    }

    switch (MODE) {
      case PartitionedSched::UNTEST:
        break;
      case PartitionedSched::TEST:
        if (!alloc_schedulable()) {
          ti->set_partition(MAX_INT);
          processor.remove_task(ti->get_id());
          continue;
        } else {
          return true;
        }
        break;
      default:
        break;
    }
  }

  return false;
}

bool PartitionedSched::BinPacking_NF(Task* ti, TaskSet* tasks,
                                     ProcessorSet* processors,
                                     ResourceSet* resources, uint MODE) {}
