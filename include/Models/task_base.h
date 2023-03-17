#ifndef TASK_BASE_H
#define TASK_BASE_H

#include "types.h"

class ResourceSet;
class Param;

class TaskBase
{
	public:
		ulong wcet;
		ulong deadline;
		ulong period;
		TaskBase();
		TaskBase(ulong wcet, ulong period, ulong deadline = 0);

		uint get_id() const;
		uint get_index() const;
		ulong get_wcet() const;
		ulong get_wcet_heterogeneous() const;
		ulong get_deadline() const;
		ulong get_period() const;
		ulong get_response_time() const;
		uint get_priority() const;
		uint get_partition() const;
};

class TaskSetBase
{
	public:
		fraction_t utilization_sum;
		fraction_t utilization_max;
		fraction_t density_sum;
		fraction_t density_max;
		TaskSetBase();

		virtual uint get_size() const = 0;
		virtual void add_task(ResourceSet& resourceset, Param& param, long wcet, long period, long deadline = 0) = 0;
		virtual fraction_t get_utilization_sum() const = 0;
		virtual fraction_t get_utilization_max() const = 0;
		virtual fraction_t get_density_sum() const = 0;
		virtual fraction_t get_density_max() const = 0;
};

#endif
