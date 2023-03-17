// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <processors.h>
#include <resources.h>
#include <sched_test_base.h>
#include <tasks.h>

SchedTestBase::SchedTestBase(bool LinearProgramming, uint TestMethod,
                             uint SchedMethod, uint PriorityAssignment,
                             uint LockingProtocol, string name, string remark) {
  this->LinearProgramming = LinearProgramming;
  this->TestMethod = TestMethod;
  this->SchedMethod = SchedMethod;
  this->PriorityAssignment = PriorityAssignment;
  this->LockingProtocol = LockingProtocol;
  this->name = name;
  this->remark = remark;
  this->status = INIT;
}

SchedTestBase::~SchedTestBase() {}

string SchedTestBase::get_test_name() {
  stringstream test_name;

  if (LinearProgramming) test_name << "LP-";

  switch (TestMethod) {
    case UTI_BOUND:
      test_name << "UB-";
      break;
    case RTA:
      test_name << "RTA-";
      break;
  }

  switch (SchedMethod) {
    case GLOBAL:
      test_name << "G";
      break;
    case PARTITIONED:
      test_name << "P";
      break;
  }

  switch (PriorityAssignment) {
    case FIX_PRIORITY:
      test_name << "FP";
      break;
    case EDF:
      test_name << "EDF";
      break;
  }

  switch (LockingProtocol) {
    case NONE:
      test_name << "";
      break;
    case SPIN:
      test_name << "-SPIN";
      break;
    case PIP:
      test_name << "-PIP";
      break;
    case DPCP:
      test_name << "-DPCP";
      break;
    case MPCP:
      test_name << "-MPCP";
      break;
    case FMLP:
      test_name << "-FMLP";
      break;
  }

  return test_name.str();
}

void SchedTestBase::set_status(int status) { this->status = status; }

int SchedTestBase::get_status() { return status; }
