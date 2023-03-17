// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <output.h>
#include <iteration-helper.h>
#include <math-helper.h>

Output::Output(const char* path) {
  this->path = path;
	chart.SetGraphSize(800, 600);
	chart.SetGraphQual(3);
}

Output::Output(Param param) {
  this->param = param;
  if (0 == access(string("results/" + output_filename()).data(), 0)) {
    int suffix = 0;
    // printf("result folder exsists.\n");
    do {
      suffix++;
    } while (0 == access(string("results/" + output_filename() + "-" +
                                to_string(suffix))
                             .data(),
                         0));
    if (0 ==
        mkdir(string("results/" + output_filename() + "-" + to_string(suffix))
                  .data(),
              S_IRWXU))

      this->path =
          "results/" + output_filename() + "-" + to_string(suffix) + "/";
  } else {
    printf("result folder does not exsist.\n");
    if (0 == mkdir(string("results/" + output_filename()).data(), S_IRWXU))
      printf("result folder has been created.\n");
    this->path = "results/" + output_filename() + "/";
    printf("result folder has been created.\n");
  }

  chart.SetGraphSize(param.graph_width, param.graph_height);
  chart.SetGraphQual(param.graph_quality);
}

string Output::get_path() { return path; }

SchedResultSet& Output::get_results() { return srs; }

bool Output::add_result(string test_name, string line_style, double x,
                        uint e_num, uint s_num, double runtime) {
  SchedResult& sr = srs.get_sched_result(test_name, line_style);
  if (sr.get_result_by_utilization(x).exp_num + e_num <=
      param.exp_times) {
    sr.insert_result(x, e_num, s_num, runtime);
    return true;
  }
  return false;
}

uint Output::get_exp_time_by_utilization(double utilization) {
  if (0 == srs.size()) return 0;

  Result result =
      srs.get_sched_result_set()[0].get_result_by_utilization(utilization);
  return result.exp_num;
}


uint Output::get_exp_time_by_x(double x) {
  if (0 == srs.size()) return 0;

  Result result =
      srs.get_sched_result_set()[0].get_result_by_x(x);
  return result.exp_num;
}


bool Output::no_success_at_x(double x) {
  if (0 == srs.size()) return true;
  foreach(srs.get_sched_result_set(), sr) {
    if (0 < sr->get_result_by_x(x).success_num) {
      return false;
    }
  }
  return true;
}

string Output::output_filename() {
  stringstream buf;
  buf << "id[" << param.id << "]-M[" << param.mean << "]-"
      << "P[" << param.p_num << "]-"
      << "rn[" << param.resource_num << "]-"
      << "rrn[" << param.rrn << "]-"
      << "rrp[" << param.rrp << "]-"
      << "rrr[" << param.rrr.min << "," << param.rrr.max << "]";
  return buf.str();
}

string Output::get_method_name(Test_Attribute ta) {
  string name;
  name = ta.test_name;

  if (0 == strcmp(ta.remark.data(), ""))
    return name;
  else
    return name + "-" + ta.remark;
}

// output to console
void Output::proceeding() {}

// void Output::proceeding(string test_name, double utilization, uint e_num,
//                         uint s_num) {}

void Output::finish() {}

// export to csv
void Output::export_param() {
  string file_name = path + "parameters.txt";
  ofstream output_file(file_name);

  output_file << "U_avg: " << param.mean << "\n";
  output_file << "P_num: " << param.p_num << "\n";
  output_file << "P_range: [" << param.p_range.min << "-"
              << param.p_range.max << "]"
              << "\n";
  output_file << "D_range: [" << param.d_range.min << "-"
              << param.d_range.max << "]"
              << "\n";
  output_file << "EXP_t: " << param.exp_times << "\n";
  output_file << "R_num: " << param.resource_num << "\n";
  output_file << "MCSN: " << param.mcsn << "\n";
  output_file << "RRN: " << param.rrn << "\n";
  output_file << "RRP: " << param.rrp << "\n";
  output_file << "RR_range: [" << param.rrr.min << "-"
              << param.rrr.max << "]"
              << "\n";
  output_file << "MPJ " << param.max_para_job << "\n";
  output_file << "RI_1: " << param.reserve_int_1 << "\n";
  output_file << "RI_2: " << param.reserve_int_2 << "\n";
  output_file << "RI_3: " << param.reserve_int_3 << "\n";
  output_file << "RI_4: " << param.reserve_int_4 << "\n";
  output_file << "RD_1: " << param.reserve_double_1 << "\n";
  output_file << "RD_2: " << param.reserve_double_2 << "\n";
  output_file << "RD_3: " << param.reserve_double_3 << "\n";
  output_file << "RD_4: " << param.reserve_double_4 << "\n";
  output_file << "RR_1: [" << param.reserve_range_1.min << "-"
              << param.reserve_range_1.max << "]"
              << "\n";
  output_file << "RR_2: [" << param.reserve_range_2.min << "-"
              << param.reserve_range_2.max << "]"
              << "\n";

  output_file.flush();
  output_file.close();
}
void Output::export_csv() {
  double x = param.reserve_range_1.min;
  double ratio;
  string file_name = path + "result.csv";
  ofstream output_file(file_name);

  if (0 == strcmp(param.graph_x_label.c_str(), "Normalized Utilization")) {
    output_file << "NR";
  } else if (0 == strcmp(param.graph_x_label.c_str(), "Number of Tasks")) {
    output_file << "TN";
  } else if (0 == strcmp(param.graph_x_label.c_str(), "Number of Resources")) {
    output_file << "RN";
  } else if (0 == strcmp(param.graph_x_label.c_str(), "Shared Resource Request Probability")) {
    output_file << "SRP";
  } else if (0 == strcmp(param.graph_x_label.c_str(), "Critical Section Length per Request")) {
    output_file << "CSL";
  } else if (0 == strcmp(param.graph_x_label.c_str(), "Critical Section Number per Task")) {
    output_file << "CSN";
  } else {
    output_file << "NR";
  }

  foreach(srs.get_sched_result_set(), sr) {
    output_file <<"," << sr->get_test_name();
  }
  output_file << "\n";

  do {
    output_file << x ;

    foreach(srs.get_sched_result_set(), sr) {
      ratio = sr->get_result_by_x(x).success_num;
      ratio /= sr->get_result_by_x(x).exp_num;
      output_file <<"," << ratio;
    }
    output_file << "\n";
    
    x += param.reserve_double_1;
  } while (x < param.reserve_range_1.max ||
             fabs(param.reserve_range_1.max - x) < _EPS);

  // output_file << "Mean:" << param.mean << ",";
  // output_file << " processor number:" << param.p_num << ",";
  // output_file << " step:" << param.step << ",";
  // output_file << " utilization range:[" << param.u_range.min << "-"
  //             << param.u_range.max << "],";
  // output_file << setprecision(0) << " period range:[" << param.p_range.min
  //             << "-" << param.p_range.max << "]\n"
  //             << setprecision(3);
  // output_file << "Utilization,";
  // for (uint i = 0; i < param.test_attributes.size(); i++) {
  //   output_file << get_method_name(param.test_attributes[i]) << " ratio,";
  // }
  // output_file << "\n";
  /*
          for(uint i = 0; i < result_sets[0].size(); i++)
          {
                  output_file<<result_sets[0][i].x<<",";
                  for(uint j = 0; j < result_sets.size(); j++)
                  {
                          output_file<<result_sets[j][i].y<<",";
                  }
                  output_file<<"\n";
          }
  */

  // for (double i = param.u_range.min; i - param.u_range.max < _EPS;
  //      i += param.step) {
  //   output_file << i << ",";
  //   foreach(srs.get_sched_result_set(), sched_result) {
  //     if (0 == sched_result->get_result_by_utilization(i).exp_num) {
  //       output_file << ",";
  //     } else {
  //       double ratio = sched_result->get_result_by_utilization(i).success_num;
  //       ratio /= sched_result->get_result_by_utilization(i).exp_num;
  //       output_file << ratio << ",";
  //     }
  //   }
  //   output_file << "\n";
  // }

  output_file.flush();
  output_file.close();
}

void Output::export_table_head() {
  string file_name = path + "result-step-by-step.csv";
  ofstream output_file(file_name);

  output_file << "Mean:" << param.mean << ",";
  output_file << "processor number:" << param.p_num << ",";
  output_file << "step:" << param.step << ",";
  output_file << "utilization range:[" << param.u_range.min << "-"
              << param.u_range.max << "],";
  output_file << setprecision(0) << "period range:[" << param.p_range.min << "-"
              << param.p_range.max << "]\n"
              << setprecision(3);
  output_file << "Scheduling test method:,";
  for (uint i = 0; i < param.test_attributes.size(); i++) {
    output_file << get_method_name(param.test_attributes[i]) << ","
                << ",,";
  }
  output_file << "\n";
  output_file << "Utilization,";
  for (uint i = 0; i < param.test_attributes.size(); i++) {
    output_file << "experiment times,success times,success ratio,";
  }
  output_file << "\n";
  output_file.flush();
  output_file.close();
}

void Output::export_result_append(double utilization) {
  string file_name = path + "result-step-by-step.csv";
  if (0 != access(file_name.data(), 0)) {
    export_table_head();
  }
  ofstream output_file(file_name, ofstream::app);

  /*
          uint last_index = result_sets[0].size() - 1;
          output_file<<result_sets[0][last_index].x<<",";
          for(uint i = 0; i < param.test_attributes.size(); i++)
          {
                  Result result = result_sets[i][last_index];
                  output_file<<result.exp_num<<",";
                  output_file<<result.success_num<<",";
                  output_file<<result.y<<",";
          }
          output_file<<"\n";
  */

  output_file << utilization << ",";
  foreach(srs.get_sched_result_set(), sched_result) {
    uint e_num = sched_result->get_result_by_utilization(utilization).exp_num;
    uint s_num =
        sched_result->get_result_by_utilization(utilization).success_num;
    if (0 == e_num) {
      output_file << ",,,";
    } else {
      double ratio = s_num;
      ratio /= e_num;
      output_file << e_num << ",";
      output_file << s_num << ",";
      output_file << ratio << ",";
    }
  }
  output_file << "\n";

  output_file.flush();
  output_file.close();
}

void Output::append2file(string flie_name, string buffer) {
  string file_name = path + flie_name;

  ofstream output_file(file_name, ofstream::app);

  output_file << buffer << "\n";
  output_file.flush();
  output_file.close();
}

void Output::export_runtime() {
  double x = param.reserve_range_1.min;
  double avg_rt;
  string file_name = path + "result_runtime.csv";
  ofstream output_file(file_name);

  output_file << "runtime";


  foreach(srs.get_sched_result_set(), sr) {
    output_file <<"," << sr->get_test_name();
  }
  output_file << "\n";

  do {
    output_file << x ;

    foreach(srs.get_sched_result_set(), sr) {
      avg_rt = sr->get_result_by_x(x).total_rt;
      avg_rt /= sr->get_result_by_x(x).exp_num;
      output_file <<"," << avg_rt;
    }
    output_file << "\n";
    
    x += param.reserve_double_1;
  } while (x < param.reserve_range_1.max ||
             fabs(param.reserve_range_1.max - x) < _EPS);

}

// export to graph
void Output::SetGraphSize(int width, int height) {
  chart.SetGraphSize(width, height);
}

void Output::SetGraphQual(int quality) { chart.SetGraphQual(quality); }

void Output::Export(int format) {
  string temp, file_name = path + "result";
  /*
          for(uint i = 0; i < get_sets_num(); i++)
          {
                  chart.AddData(get_method_name(param.test_attributes[i]),
     result_sets[i]);
          }
  */
  chart.AddData(srs);

  if (0x0f & format) {
    chart.ExportLineChart(file_name, "", param.u_range.min, param.u_range.max,
                          param.step, format);
  }
  if (0x10 & format) {
    temp = file_name + ".json";
    chart.ExportJSON(temp);
  }
}

void Output::Export(double from, double to, double step, int format, string x_label, string y_label, uint pos) {
  string temp, file_name = path + "result";

  chart.AddData(srs);

  if (0x0f & format) {
    chart.ExportLineChart(file_name, "", from, to,
                          step, format, x_label, y_label, pos);
  }
}