#ifndef CONTEXT_H
#define CONTEXT_H

class Context
{
	private:
		TaskSet tasks;
		ResourceSet resources;
		ProcessorSet Processors;
	public:
		Context(TaskSet tasks, ResourceSet resources, ProcessorSet Processors);
		~Context();
};

#endif
