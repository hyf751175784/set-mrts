
CC            = gcc
CXX           = g++
DEFINES       = -DQT_NO_DEBUG -DQT_PRINTSUPPORT_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB
CFLAGS        = -pipe -O2 -Wall -W -D_REENTRANT -fPIE $(DEFINES) -fpermissive
CXXFLAGS      = -pipe -O2 -Wall -W -g -D_REENTRANT -fPIE $(DEFINES) -std=gnu++11 -fpermissive -static-libstdc++ 
#-fext-numeric-literals 
GTKMMFLAGS	  =	`pkg-config gtkmm-3.0 --cflags --libs`
GLIBMMGLAGS	  =	`pkg-config glibmm-2.4 --cflags --libs`
INCPATH       = -I. -I../include -I../include/SchedTest -I../include/SchedTest/global -I../include/SchedTest/global/lp -I../include/SchedTest/global/DAG -I../include/SchedTest/partitioned -I../include/SchedTest/partitioned/lp -I../include/SchedTest/partitioned/DAG -I../include/SchedTest/lp -I../include/QCustomPlot -I../include/mathGL -I../include/LP -I../include/SchedTest/blocking/lp -I../include/SchedTest/npedf 
LINK          = g++
LFLAGS        = #-Wl,-O1
LIBS          = -lgmp -lgmpxx -lpthread -lglpk -lmgl 
SERVER_LIBS   = -lgmp -lgmpxx -lpthread -lglpk -lmgl 
CLIENT_LIBS   = -lgmp -lgmpxx -lpthread -lglpk
COPY          = cp -f
COPY_FILE     = cp -f
COPY_DIR      = cp -f -R
DEL_FILE      = rm -f
DEL_DIR       = rmdir
MOVE          = mv -f
MKDIR         = mkdir -p
TARGET	      = np_main_test

BASIC_OBJ_FILES =tasks.o processors.o resources.o param.o random_gen.o tinyxml2.o xml.o sched_result.o time_record.o toolkit.o

SCHED_TEST_OBJ_FILES = lp.o solution.o varmapper.o sched_test_base.o sched_test_factory.o p_sched.o  lp_rta_pfp_mpcp_heterogeneous.o 

OUTPUT_OBJ_FILES = mgl_chart.o output.o

NPEDF_OBJ_FILES = np_edf.o

OBJ_FILES	  = $(BASIC_OBJ_FILES) $(SCHED_TEST_OBJ_FILES) $(OUTPUT_OBJ_FILES) $(NPEDF_OBJ_FILES) 

SERVER_OBJ_FILES	  = $(BASIC_OBJ_FILES) $(OUTPUT_OBJ_FILES)

CLIENT_OBJ_FILES	  = $(BASIC_OBJ_FILES) $(SCHED_TEST_OBJ_FILES)

np_main_test:np_main_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o $(TARGET) np_main_test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11


SchedulingAnalysis:scheduling_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o SchedulingAnalysis scheduling_test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

BlockingTest:Blocking_Test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o BlockingTest Blocking_Test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11


	
runtime_test:runtime_complexity.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o rt_test runtime_complexity.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

STJJC:STJJC.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o STJJC STJJC.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

SubTest:sub_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o SubTest sub_test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

server:server.o $(SERVER_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o server server.o $(SERVER_OBJ_FILES) $(LFLAGS) $(SERVER_LIBS) -std=gnu++11

client:client.o $(CLIENT_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o client client.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11

bt_server:bt_server.o $(SERVER_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o bt_server bt_server.o $(SERVER_OBJ_FILES) $(LFLAGS) $(SERVER_LIBS) -std=gnu++11

bt_client:bt_client.o $(CLIENT_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o bt_client bt_client.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11

task_gen_test:task_gen_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o task_gen_test task_gen_test.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11


test:test.o tasks.o processors.o tinyxml2.o random_gen.o mgl_chart.o resources.o xml.o param.o sched_result.o output.o
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test test.o tasks.o processors.o tinyxml2.o random_gen.o mgl_chart.o resources.o xml.o param.o sched_result.o output.o  $(LFLAGS) $(LIBS) -std=gnu++11

np_main_test.o:np_main_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c np_main_test.cpp $(LIBS)


scheduling_test.o:scheduling_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c scheduling_test.cpp $(LIBS)

Blocking_Test.o:Blocking_Test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c Blocking_Test.cpp $(LIBS)
	

	
runtime_complexity.o:runtime_complexity.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c runtime_complexity.cpp $(LIBS)



STJJC.o:Collaboration/UTD/STJJC.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c Collaboration/UTD/STJJC.cpp $(LIBS)

sub_test.o:sub_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c sub_test.cpp $(LIBS)

server.o:server.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c server.cpp $(LIBS)

client.o:client.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c client.cpp $(LIBS)

bt_server.o:bt_server.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c bt_server.cpp $(LIBS)

bt_client.o:bt_client.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c bt_client.cpp $(LIBS)

task_gen_test.o:task_gen_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c task_gen_test.cpp $(LIBS)


test.o:test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c test.cpp $(LIBS)
	

tasks.o:tasks.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c tasks.cpp $(LIBS)

processors.o:processors.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c processors.cpp $(LIBS)

random_gen.o:random_gen.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c random_gen.cpp $(LIBS)

resources.o:resources.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c resources.cpp $(LIBS)

tinyxml2.o:tinyxml2.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c tinyxml2.cpp $(LIBS)

mgl_chart.o:mathGL/mgl_chart.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c mathGL/mgl_chart.cpp $(LIBS)

xml.o:xml.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c xml.cpp $(LIBS)

param.o:param.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c param.cpp $(LIBS)

sched_result.o:sched_result.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c sched_result.cpp $(LIBS)

output.o:output.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c output.cpp $(LIBS)

time_record.o:time_record.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c time_record.cpp $(LIBS)

toolkit.o:toolkit.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c toolkit.cpp $(LIBS)

#GTKMM_window.o:GTKMM_window.cpp
#	$(CXX) $(GTKMMFLAGS) $(GLIBMMGLAGS) $(CXXFLAGS) $(INCPATH) -c GTKMM_window.cpp $(LIBS)

lp.o:LP/lp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c LP/lp.cpp $(LIBS)

solution.o:LP/solution.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c LP/solution.cpp $(LIBS)

varmapper.o:LP/varmapper.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c LP/varmapper.cpp $(LIBS)

sched_test_base.o:SchedTest/sched_test_base.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/sched_test_base.cpp $(LIBS)

sched_test_factory.o:SchedTest/sched_test_factory.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/sched_test_factory.cpp $(LIBS)

p_sched.o:SchedTest/partitioned/p_sched.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/p_sched.cpp $(LIBS)





lp_rta_pfp_mpcp_heterogeneous.o:SchedTest/partitioned/lp/lp_rta_pfp_mpcp_heterogeneous.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_mpcp_heterogeneous.cpp $(LIBS)




np_edf.o:SchedTest/npedf/np_edf.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/npedf/np_edf.cpp $(LIBS)




all:
	make clean
	make
