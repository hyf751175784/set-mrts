// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_MATHGL_MGL_CHART_H_
#define INCLUDE_MATHGL_MGL_CHART_H_

#include <mgl2/mgl.h>
#include <sched_result.h>
#include <types.h>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::stringstream;
using std::hex;

typedef struct {
  string name;
  string style;
  mglData data;
} Chart_Data;

typedef vector<string> Color;
typedef vector<string> Width;
typedef vector<string> Dot;

class Chart {
 private:
  // vector<Chart_Data> data_set;
  SchedResultSet srs;
  mglGraph graph;
  Color color;   // already has 8 colors
  string width;  // from 0-9
  Dot dot;       // already has 8 dots
  string get_line_style(uint index);

 public:
  Chart();
  ~Chart() {}

  void AddColor(string color);
  void SetLineWidth(uint w);
  // void AddData(string name, double* d, int size);
  // void AddData(string name, Result_Set r_set);
  void AddData(SchedResultSet srs);
  void SetGraphSize(uint width, uint height);
  void SetGraphQual(uint quality);
  void ExportLineChart(string path, string title, double min, double max,
                       double step, int format = 0, string x_label = "", string y_label = "", uint pos = 0);
  void ExportJSON(string path);
};

#endif  // INCLUDE_MATHGL_MGL_CHART_H_
