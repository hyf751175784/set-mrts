// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include<time_record.h>

#if defined (__i386__)
static __inline__ unsigned long long GetCycleCount(void)
{
        unsigned long long int x;
        __asm__ volatile("rdtsc":"=A"(x));
        return x;
}
#elif defined (__x86_64__)
static __inline__ unsigned long long GetCycleCount(void)
{
        unsigned hi,lo;
        __asm__ volatile("rdtsc":"=a"(lo),"=d"(hi));
        return ((unsigned long long)lo)|(((unsigned long long)hi)<<32);
}
#endif

Time_Record::Time_Record() {

}

void Time_Record::Record_S_A() {
  s_time_start = time(NULL);
}
void Time_Record::Record_S_B() {
  s_time_end = time(NULL);
}
void Time_Record::Record_S() {
  uint64_t gap = difftime(s_time_end, s_time_start);
  uint32_t hour = gap / 3600;
  uint32_t min = (gap % 3600) / 60;
  uint32_t sec = (gap % 3600) % 60;

  cout << hour << "hour " << min << "min " << sec << "sec." << endl;
}

void Time_Record::Record_MS_A() {
  ms_clock_start = clock();
}
void Time_Record::Record_MS_B() {
  ms_clock_end = clock();
}
double Time_Record::Record_MS() {
  double gap = ms_clock_end - ms_clock_start;
  double ms_time = gap/CLOCKS_PER_SEC*1000;
  cout << ms_time << "ms." << endl;
  return ms_time;
}

// void Time_Record::Record_NS_A() {
//   start = GetCycleCount();
// }
// void Time_Record::Record_NS_B() {
//   end = GetCycleCount();
// }
// void Time_Record::Record_NS() {
//   cout << (end - start)*1.0/FREQUENCY << "ns." << endl;
// }