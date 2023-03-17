// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <toolkit.h>

void extract_element(vector<string> *elements, string bufline, uint start,
                     uint num, string seperator) {
  char *charbuf;
  char *buf = const_cast<char *>(bufline.data());
  char *ptr = NULL;
  string cut = " \t\r\n";
  cut += seperator.data();

  uint count = 0;

  try {
    while (NULL != (charbuf = strtok_r(buf, cut.data(), &ptr))) {
      if (count >= start && count < start + num) {
        // cout << "element:" << charbuf << endl;
        elements->push_back(charbuf);
      }
      count++;
      buf = NULL;
    }
  } catch (exception &e) {
    cout << "extract exception." << endl;
  }
}

void extract_element(vector<uint64_t> *elements, string bufline, uint start,
                     uint num, string seperator) {
  char *charbuf;
  char *buf = const_cast<char *>(bufline.data());
  char *ptr = NULL;
  string cut = " \t\r\n";
  cut += seperator.data();
  // cut += ",";

  uint count = 0;
  try {
    while (NULL != (charbuf = strtok_r(buf, cut.data(), &ptr))) {
      if (count >= start && count < start + num) {
        // cout << "element:" << charbuf << endl;
        uint64_t element = atol(charbuf);
        elements->push_back(element);
      }
      count++;
      buf = NULL;
    }
  } catch (exception &e) {
    cout << "extract exception." << endl;
  }
}
