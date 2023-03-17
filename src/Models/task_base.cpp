#include "task_base.h"

TaskBase::TaskBase()
{
	wcet = 0;
	deadline = 0;
	period = 0;
}

TaskBase::TaskBase(ulong wcet, ulong period, ulong deadline)
{
	this->wcet = wcet;
	this->period = period;
	if(0 == deadline)
		this->deadline = period;
	else
		this->deadline = deadline;
}

TaskSetBase::TaskSetBase()
{
	utilization_sum = 0;
	utilization_max = 0;
	density_sum = 0;
	density_max = 0;
}
