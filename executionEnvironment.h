// Filename: executionEnvironment.h
// Created by:  frang (03June04)
//
////////////////////////////////////////////////////////////////////

#ifndef EXECUTIONENVIRONMENT_H
#define EXECUTIONENVIRONMENT_H

#include "oconfig.h"
#include "filename.h"

////////////////////////////////////////////////////////////////////
//       Class : ExecutionEnvironment
// Description : This class is stripped down to just the bare
//               minimum that Filename needs.
////////////////////////////////////////////////////////////////////
class ExecutionEnvironment {
public:
  static string get_environment_variable(const string &var);
  static string expand_string(const string &str);
  static Filename get_cwd();
};

#endif
