// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_XML_H_
#define INCLUDE_XML_H_

#include <types.h>
#include <string.h>
#include <tinyxml2.h>
#include <iostream>
#include <string>

using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;
using tinyxml2::XMLDeclaration;
using tinyxml2::XMLNode;

class XML {
 private:
  static XMLDocument config;
  XMLDocument* output;

 public:
  XML();

  ~XML();

  static void LoadFile(const char* path);

  static void SaveConfig(const char* path);

  // Network
  static const char* get_server_ip();

  static uint get_server_port();

  static void get_method(Test_Attribute_Set* t_set);

  static uint get_experiment_times();

  static void get_lambda(Int_Set* i_set);

  static void get_mean(Double_Set* d_set);

  static void get_period_range(Range_Set* r_set);

  static void get_deadline_propotion(Range_Set* r_set);

  static void get_utilization_range(Range_Set* r_set);

  static void get_step(Double_Set* d_set);

  static void get_processor_num(Int_Set* i_set);

  // resource

  static void get_resource_num(Int_Set* i_set);

  // resource request

  static void get_resource_request_probability(Double_Set* d_set);

  static void get_resource_request_num(Int_Set* i_set);

  static void get_resource_request_range(Range_Set* r_set);

  static void get_total_len_factor(Double_Set* d_set);

  // output

  static void get_output(Int_Set* i_set);

  // defalut

  static void get_integers(Int_Set* i_set, const char* element_name);

  static void get_doubles(Double_Set* d_set, const char* element_name);

  static void get_ranges(Range_Set* r_set, const char* element_name);

  static int get_integer(const char* element_name);

  static double get_double(const char* element_name);

  static string get_string(const char* element_name);

  // xml construction
  void initialization();
  XMLElement* get_element(const char* parent);
  XMLElement* get_element(XMLElement* parent, const char* name,
                          int index = 0);
  void add_element(const char* name);
  void add_element(const char* parent, const char* name, const char* text = "");
  void add_element(XMLElement* parent, const char* name,
                   const char* text = "");
  void add_element(const char* parent, int index, const char* name,
                   const char* text = "");
  void add_range(const char* parent, Range range);
  void set_text(XMLElement* element, const char* text);
  void set_text(const char* parent, int index1, const char* element, int index2,
                const char* text);
  void save_file(const char* path);
  void clear();
};

#endif  // INCLUDE_XML_H_
