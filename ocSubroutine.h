// Filename: ocSubroutine.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCSUBROUTINE_H
#define OCSUBROUTINE_H

#include "oconfig.h"

#include <vector>
#include <map>

///////////////////////////////////////////////////////////////////
//       Class : OCSubroutine
// Description : This represents a named subroutine defined via the
//               #defsub .. #end sequence that may be invoked at any
//               time via #call.  All subroutine definitions are
//               global.
////////////////////////////////////////////////////////////////////
class OCSubroutine {
public:
  vector<string> _formals;
  vector<string> _lines;

public:
  static void define_sub(const string &name, OCSubroutine *sub);
  static const OCSubroutine *get_sub(const string &name);

  static void define_func(const string &name, OCSubroutine *sub);
  static const OCSubroutine *get_func(const string &name);

  typedef map<string, OCSubroutine *> Subroutines;
  static Subroutines _subroutines;
  static Subroutines _functions;
};

#endif
