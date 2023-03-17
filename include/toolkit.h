// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_TOOLKIT_H_
#define INCLUDE_TOOLKIT_H_

#include <types.h>
#include <string>
#include <vector>

using std::exception;
using std::string;
using std::vector;

void extract_element(vector<string> *elements, string bufline, uint start = 0,
                     uint num = MAX_INT, string seperator = ",");

void extract_element(vector<uint64_t> *elements, string bufline, uint start = 0,
                     uint num = MAX_INT, string seperator = ",");

#endif  // INCLUDE_TOOLKIT_H_
