// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_ITERATION_HELPER_H_
#define INCLUDE_ITERATION_HELPER_H_

#define foreach_condition(tasks, condition) \
  for (uint i = 0; i < tasks.size(); i++) { \
    if (condition) return false;            \
  }

#define foreach(collection, it)                              \
  for (typeof(collection.begin()) it = (collection).begin(); \
       it != (collection).end(); it++)
/*
#define foreach_task(taskset, tx) \
        for (uint t_id = 0, task tx = taskset.get_task_by_id(t_id);	\
             t_id < taskset.get_task_size(); \
                     it++)
*/
#define enumerate(collection, it, i)      \
  for (typeof(collection.begin()) it = ({ \
         i = 0;                           \
         (collection).begin();            \
       });                                \
       it != (collection).end(); it++, i++)

#define apply_foreach(collection, fun, ...)       \
  foreach(collection, __apply_it_##collection) { \
    fun(*__apply_it_##collection, ##__VA_ARGS__); \
  }

#define map_ref(from, to, init, fun, ...)             \
  {                                                   \
    (to).clear();                                     \
    (to).reserve((from).size());                      \
    foreach(from, __map_ref_it) {                    \
      (to).push_back(init());                         \
      fun(*__map_ref_it, (to).back(), ##__VA_ARGS__); \
    }                                                 \
  }

#define foreach_task_assign_to_processor(collection, p_id, tx) \
  foreach(collection, tx)                                     \
    if (p_id == tx->get_partition())

#define foreach_task_assign_to_other_processor(collection, p_id, tx) \
  foreach(collection, tx)                                           \
    if (p_id != tx->get_partition())

#define foreach_task_except(collection, ti, tx) \
  foreach(collection, tx)                      \
    if (tx->get_id() != ti.get_id())

#define foreach_resource_except(collection, ru, rv) \
  foreach(collection, rv)                          \
    if (ru.get_resource_id() != rv->get_resource_id())

#define foreach_higher_priority_task(tasks, ti, tx) \
  foreach(tasks, tx)                               \
    if (tx->get_priority() < ti.get_priority())

#define foreach_higher_priority_task_then(tasks, priority, tx) \
  foreach(tasks, tx)                                          \
    if (tx->get_priority() < priority)

#define foreach_lower_priority_task(tasks, ti, tx) \
  foreach(tasks, tx)                              \
    if (tx->get_priority() > ti.get_priority())

#define foreach_lower_priority_task_then(tasks, priority, tx) \
  foreach(tasks, tx)                                         \
    if (tx->get_priority() > priority)

#define foreach_request_instance(task_ti, task_tx, resource_id, \
                                 request_index_variable)        \
  for (uint request_index_variable = 0,                         \
            max_request_num = (task_tx).get_max_request_num(    \
                resource_id, (task_ti).get_response_time());    \
       request_index_variable < max_request_num; request_index_variable++)



#define foreach_request_instance_nu(task_ti, task_tx, resource_id, \
                                 request_index_variable)        \
  for (uint request_index_variable = 0,                         \
            max_request_num = (task_tx).get_max_request_num_nu( \
                resource_id, (task_ti).get_deadline());    \
       request_index_variable < max_request_num; request_index_variable++)

#define foreach_remote_request(task_ti, requests, request_iter) \
  foreach(requests, request_iter)                              \
    if (task_ti.get_partition() != request_iter->get_locality())

#define foreach_local_request(task_ti, requests, request_iter) \
  foreach(requests, request_iter)                             \
    if (task_ti.get_partition() == request_iter->get_locality())

#define foreach_lower_priority_local_task(tasks, ti, tx) \
  foreach(tasks, tx)                                    \
    if ((tx->get_priority() > ti.get_priority()) &&      \
        (tx->get_partition() == ti.get_partition()))

#endif  // INCLUDE_ITERATION_HELPER_H_
