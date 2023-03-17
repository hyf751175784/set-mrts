// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_TIME_RECORD_H_
#define INCLUDE_TIME_RECORD_H_

#include <types.h>
#include <ctime>

using std::cout;
using std::endl;
using std::vector;


class Time_Record {
  private:
    time_t s_time_start, s_time_end;
    clock_t ms_clock_start, ms_clock_end;
    uint64_t start, end;
  public:
    Time_Record();

    void Record_S_A();
    void Record_S_B();
    void Record_S();

    void Record_MS_A();
    void Record_MS_B();
    double Record_MS();

    void Record_NS_A();
    void Record_NS_B();
    void Record_NS();
};



#endif INCLUDE_TIME_RECORD_H_
