// Copyright [2016] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <xml.h>

XML::XML() {
  output = new XMLDocument();
  initialization();
}

XML::~XML() { delete (output); }

XMLDocument XML::config;

void XML::LoadFile(const char *path) { config.LoadFile(path); }

void XML::SaveConfig(const char *path) { config.SaveFile(path); }

const char *XML::get_server_ip() {
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("server_ip");
  return title->GetText();
}

uint XML::get_server_port() {
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("server_port");
  return atoi(title->GetText());
}

void XML::get_method(Test_Attribute_Set *t_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("schedulability_test");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    // cout<<content<<endl;
    Test_Attribute ta;
    int test_type = subtitle->IntAttribute("TEST_TYPE");
    string remark;
    string rename;
    string style;
    if (NULL == subtitle->Attribute("REMARK"))
      remark = "";
    else
      remark = subtitle->Attribute("REMARK");
    if (NULL == subtitle->Attribute("RENAME"))
      rename = "";
    else
      rename = subtitle->Attribute("RENAME");
    if (NULL == subtitle->Attribute("STYLE"))
      style = "";
    else
      style = subtitle->Attribute("STYLE");
    int transform = 0;
    if (0 == strcmp(content, "P-EDF")) {
      transform = 0;
    } else if (0 == strcmp(content, "BCL-FTP")) {
      transform = 1;
    } else if (0 == strcmp(content, "BCL-EDF")) {
      transform = 2;
    } else if (0 == strcmp(content, "WF-DM")) {
      transform = 3;
    } else if (0 == strcmp(content, "WF-EDF")) {
      transform = 4;
    } else if (0 == strcmp(content, "RTA-GFP")) {
      transform = 5;
    } else if (0 == strcmp(content, "FF-DM")) {
      transform = 6;
    } else if (0 == strcmp(content, "LP-PFP")) {
      transform = 7;
    } else if (0 == strcmp(content, "LP-GFP")) {
      transform = 8;
    } else if (0 == strcmp(content, "RO-PFP")) {
      transform = 9;
    } else if (0 == strcmp(content, "ILP-SPINLOCK")) {
      transform = 10;
    } else if (0 == strcmp(content, "GEDF-NON-PREEMPT")) {
      transform = 11;
    } else if (0 == strcmp(content, "PFP-GS")) {
      transform = 12;
    } else {
      transform = 0;
    }
    ta.test_method = transform;
    ta.test_type = test_type;
    ta.test_name = content;
    ta.remark = remark;
    ta.rename = rename;
    ta.style = style;
    t_set->push_back(ta);
    subtitle = subtitle->NextSiblingElement();
  }
}

uint XML::get_experiment_times() {
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("experiment_times");
  return atoi(title->GetText());
}

void XML::get_lambda(Int_Set *i_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("lambda");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    i_set->push_back(atoi(content));
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_mean(Double_Set *d_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("mean");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    floating_t temp(content);
    d_set->push_back(temp.get_d());
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_period_range(Range_Set *r_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("period_range");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    Range temp;
    XMLElement *SSubtitle = subtitle->FirstChildElement("min");
    content = SSubtitle->GetText();
    fraction_t data(content);
    temp.min = data.get_d();
    SSubtitle = subtitle->FirstChildElement("max");
    content = SSubtitle->GetText();
    data = content;
    temp.max = data.get_d();
    r_set->push_back(temp);
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_deadline_propotion(Range_Set *r_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("deadline_propotion");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    Range temp;
    XMLElement *SSubtitle = subtitle->FirstChildElement("min");
    content = SSubtitle->GetText();
    floating_t data(content);
    temp.min = data.get_d();
    SSubtitle = subtitle->FirstChildElement("max");
    content = SSubtitle->GetText();
    data = content;
    temp.max = data.get_d();
    r_set->push_back(temp);
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_utilization_range(Range_Set *r_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("init_utilization_range");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    Range temp;
    XMLElement *SSubtitle = subtitle->FirstChildElement("min");
    content = SSubtitle->GetText();
    floating_t data(content);
    temp.min = data.get_d();
    SSubtitle = subtitle->FirstChildElement("max");
    content = SSubtitle->GetText();
    data = content;
    temp.max = data.get_d();
    r_set->push_back(temp);
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_step(Double_Set *d_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("step");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    floating_t temp(content);
    d_set->push_back(temp.get_d());
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_processor_num(Int_Set *i_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("processor_num");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    i_set->push_back(atoi(content));
    subtitle = subtitle->NextSiblingElement();
  }
}

// defalut

void XML::get_integers(Int_Set *i_set, const char *element_name) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement(element_name);
  if (NULL == title) {
    i_set->push_back(0);
  } else {
    XMLElement *subtitle = title->FirstChildElement("data");
    if (NULL == subtitle) {
      i_set->push_back(0);
    } else {
      while (subtitle) {
        content = subtitle->GetText();
        i_set->push_back(atoi(content));
        subtitle = subtitle->NextSiblingElement();
      }
    }
  }
}

void XML::get_doubles(Double_Set *d_set, const char *element_name) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement(element_name);
  if (NULL == title) {
    d_set->push_back(1.0);
  } else {
    XMLElement *subtitle = title->FirstChildElement("data");
    if (NULL == subtitle) {
      d_set->push_back(1.0);
    } else {
      while (subtitle) {
        content = subtitle->GetText();
        floating_t temp(content);
        d_set->push_back(temp.get_d());
        subtitle = subtitle->NextSiblingElement();
      }
    }
  }
}

void XML::get_ranges(Range_Set *r_set, const char *element_name) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement(element_name);
  if (NULL == title) {
    Range temp;
    temp.min = 0;
    temp.max = 1;
    r_set->push_back(temp);
  } else {
    XMLElement *subtitle = title->FirstChildElement("data");
    if (NULL == subtitle) {
      Range temp;
      temp.min = 0;
      temp.max = 1;
      r_set->push_back(temp);
    } else {
      while (subtitle) {
        Range temp;
        XMLElement *SSubtitle = subtitle->FirstChildElement("min");
        content = SSubtitle->GetText();
        floating_t data(content);
        temp.min = data.get_d();
        SSubtitle = subtitle->FirstChildElement("max");
        content = SSubtitle->GetText();
        data = content;
        temp.max = data.get_d();
        r_set->push_back(temp);
        subtitle = subtitle->NextSiblingElement();
      }
    } 
  }

}

int XML::get_integer(const char* element_name) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement(element_name);
  if (NULL == title) {
      return 0;
  } else {
    XMLElement *subtitle = title->FirstChildElement("data");
    if (NULL == subtitle) {
      return 0;
    } else {
      content = subtitle->GetText();
      if (NULL == content) {
        return 0;
      }
      return atoi(content);
    }
  }

}

double XML::get_double(const char* element_name) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement(element_name);
  if (NULL == title) {
      return 0;
  } else {
    XMLElement *subtitle = title->FirstChildElement("data");
    if (NULL == subtitle) {
      return 0;
    } else {
      content = subtitle->GetText();
      floating_t temp(content);
      if (NULL == content) {
        return 0;
      }
      return temp.get_d();
    }
  }
}

string XML::get_string(const char* element_name) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement(element_name);
  if (NULL == title) {
    return "";
  } else {
    XMLElement *subtitle = title->FirstChildElement("data");
    if (NULL == subtitle) {
      return "";
    } else {
      content = subtitle->GetText();
      if (NULL == content) {
        return "";
      }
      return content;
    }
  }
}

// resource

void XML::get_resource_num(Int_Set *i_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("resource_num");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    i_set->push_back(atoi(content));
    subtitle = subtitle->NextSiblingElement();
  }
}

// resource request

void XML::get_resource_request_probability(Double_Set *d_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("rrp");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    floating_t temp(content);
    d_set->push_back(temp.get_d());
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_resource_request_num(Int_Set *i_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("rrn");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    i_set->push_back(atoi(content));
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_resource_request_range(Range_Set *r_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("rrr");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    Range temp;
    XMLElement *SSubtitle = subtitle->FirstChildElement("min");
    content = SSubtitle->GetText();
    floating_t data(content);
    temp.min = data.get_d();
    SSubtitle = subtitle->FirstChildElement("max");
    content = SSubtitle->GetText();
    data = content;
    temp.max = data.get_d();
    r_set->push_back(temp);
    subtitle = subtitle->NextSiblingElement();
  }
}

void XML::get_total_len_factor(Double_Set *d_set) {
  const char *content;
  XMLElement *root = config.RootElement();
  XMLElement *title = root->FirstChildElement("total_len_factor");
  XMLElement *subtitle = title->FirstChildElement("data");
  while (subtitle) {
    content = subtitle->GetText();
    floating_t temp(content);
    d_set->push_back(temp.get_d());
    subtitle = subtitle->NextSiblingElement();
  }
}

// xml construction
void XML::initialization() {
  XMLDeclaration *declaration = output->NewDeclaration();
  output->InsertFirstChild(declaration);
}

XMLElement *XML::get_element(const char *parent) {
  XMLElement *title = output->FirstChildElement(parent);
  return title;
}

XMLElement *XML::get_element(XMLElement *parent, const char *name,
                             int index) {
  XMLElement *subtitle = parent->FirstChildElement(name);
  while (index--) {
    subtitle = subtitle->NextSiblingElement();
  }
  return subtitle;
}

void XML::add_element(const char *name) {
  XMLElement *root = output->NewElement(name);
  root->SetName(name);
  output->InsertEndChild(root);
}

void XML::add_element(const char *parent, const char *name, const char *text) {
  XMLElement *title = output->FirstChildElement(parent);
  XMLElement *element = output->NewElement(name);
  element->SetText(text);
  title->InsertEndChild(element);
}

void XML::add_element(XMLElement *parent, const char *name,
                      const char *text) {
  XMLElement *element = output->NewElement(name);
  element->SetText(text);
  parent->InsertEndChild(element);
}

void XML::add_element(const char *parent, int index, const char *name,
                      const char *text) {
  XMLElement *title = output->FirstChildElement(parent);
  for (int i = 0; i < index; i++) title = title->NextSiblingElement();
  XMLElement *element = output->NewElement(name);
  element->SetText(text);
  title->InsertEndChild(element);
}

void XML::add_range(const char *parent, Range range) {
  XMLElement *root = output->RootElement();
  XMLElement *title = root->FirstChildElement(parent);
  XMLElement *data = output->NewElement("data");
  XMLElement *min = output->NewElement("min");
  XMLElement *max = output->NewElement("max");
  fraction_t transform = range.min;
  min->SetText(transform.get_str().data());
  data->InsertEndChild(min);
  transform = range.max;
  max->SetText(transform.get_str().data());
  data->InsertEndChild(max);
  title->InsertFirstChild(data);
}

void XML::set_text(XMLElement *element, const char *text) {
  element->SetText(text);
}

void XML::set_text(const char *parent, int index1, const char *element,
                   int index2, const char *text) {
  XMLElement *title = output->FirstChildElement(parent);
  for (int i = 0; i < index1; i++) title = title->NextSiblingElement();
  XMLElement *subtitle = title->FirstChildElement(element);
  for (int i = 0; i < index2; i++) subtitle = subtitle->NextSiblingElement();
  subtitle->SetText(text);
}

void XML::save_file(const char *path) { output->SaveFile(path); }

void XML::clear() { output->Clear(); }
