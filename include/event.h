// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_EVENT_H_
#define INCLUDE_EVENT_H_

template <typename Object, typename Time>
class Event {
 private:
  Object *object;
  Time time;

 public:
  Object *object() const;
  const &Time time() const;
  bool operator<(const Event<Object, Time> &that) const;
  bool operator>(const Event<Object, Time> &that) const;
};

#endif  // INCLUDE_EVENT_H_
