// Filename: ocNamedScopes.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCNAMEDSCOPES_H
#define OCNAMEDSCOPES_H

#include "oconfig.h"

#include <map>
#include <vector>

class OCScope;

///////////////////////////////////////////////////////////////////
//       Class : OCNamedScopes
// Description : A collection of named scopes, as defined by #begin
//               .. #end sequences within a series of command files,
//               each associated with the directory name of the
//               command file in which it was read.
////////////////////////////////////////////////////////////////////
class OCNamedScopes {
public:
  OCNamedScopes();
  ~OCNamedScopes();

  typedef vector<OCScope *> Scopes;

  OCScope *make_scope(const string &name);
  void get_scopes(const string &name, Scopes &scopes) const;
  static void sort_by_dependency(Scopes &scopes);

  void set_current(const string &dirname);

  string get_scope_name(OCScope*) const;

private:  
  typedef map<string, Scopes> Named;

  void p_get_scopes(const Named &named, const string &name,
                    Scopes &scopes) const;

  typedef map<string, Named> Directories;
  Directories _directories;
  string _current;
};

#endif
