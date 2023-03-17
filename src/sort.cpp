// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <sort.h>

int period_increase(Task t1, Task t2) {
  return t1.get_period() < t2.get_period();
}

int utilization_decrease(Task t1, Task t2) {
  return t1.get_utilization() > t2.get_utilization();
}
