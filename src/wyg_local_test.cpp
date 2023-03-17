#include <output.h>
#include <glpk.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <param.h>
#include <processors.h>
#include <pthread.h>
#include <random_gen.h>
#include <resources.h>
#include <sched_test_base.h>
#include <sched_test_factory.h>
#include <solution.h>
#include <tasks.h>
#include <time_record.h>
#include <toolkit.h>
#include <unistd.h>
#include <xml.h>
// Socket
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define MAX_LEN 100
#define MAX_METHOD 8
#define MAXBUFFER 100

#define typeof(x) __typeof(x)

using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::ifstream;


void getFiles(string path, string dir) {
  string cmd = "ls " + path + " > " + path + dir;
  system(cmd.data());
}

void read_line(string path, vector<string> *files) {
  string buf;
  ifstream dir(path.data(), ifstream::in);
  getline(dir, buf);
  while (getline(dir, buf)) {
    files->push_back("config/" + buf);
    // cout<<"file name:"<<buf<<endl;
  }
}


int main(int argc,char **argv)
{
   
	string config_file="config.xml";
	if(1!=argc){
	for(uint arg =1;arg<argc;arg++){
	  if(0==strcmp(argv[arg],"-c"))
		  config_file =argv[++arg];
	}
      }


  vector<Param> parameters = get_parameters(config_file);

  uint index_s = 0, index_e = parameters.size();
  if (1 != argc) {
    for (uint arg = 1; arg < argc; arg++) {
      if (0 == strcmp(argv[arg], "-s")) {
        index_s = atoi(argv[++arg]);
      } else if (0 == strcmp(argv[arg], "-i")) {
        index_s = atoi(argv[++arg]);
        index_e = index_s + 1;
      } else if (0 == strcmp(argv[arg], "-r")) {
        index_s = atoi(argv[++arg]);
        index_e = atoi(argv[++arg]) + 1;
      } else if (0 == strcmp(argv[arg], "-c")) {
        config_file = argv[++arg];
      } else {
        cout << "Unrecognized parameter." << endl;
        exit(0);
      }
    }
  }

  
          Param* param = &(parameters[0]);
          Result_Set results[MAX_METHOD];
          Output output(*param);
          XML::SaveConfig((output.get_path() + "config.xml").data());
          output.export_param();
  


	SchedTestFactory STFactory;
	Random_Gen::uniform_integral_gen(0,10);
	Random_Gen::exponential_gen(10);
	GLPKSolution::set_time_limit(TIME_LIMIT_INIT);



	TaskSet tasks;
	DAG_TaskSet dag_tasks;
	ResourceSet resourceset = ResourceSet();
	resource_gen(&resourceset,*param);

	double untilization;
	int task_number;

	dag_tasks = DAG_TaskSet();
	ProcessorSet processorset = ProcessorSet(*param);

        for(uint j=0;j<param->get_method_num();j++)//get_method_num from server
      {  dag_tasks.init();
	processorset.init();
	resourceset.init();



	SchedTestBase* schedTest;                //param needs initialization
	  schedTest = STFactory.createSchedTest(
                  param->test_attributes[j].test_name, NULL, &dag_tasks,
                  &processorset, &resourceset);
       }
       return 0;


}
