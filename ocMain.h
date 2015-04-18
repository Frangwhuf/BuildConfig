// Filename: ppMain.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCMAIN_H
#define OCMAIN_H

#include "oconfig.h"
#include "ocDirectoryTree.h"
#include "ocNamedScopes.h"
#include "filename.h"

class OCScope;
class OCCommandFile;

///////////////////////////////////////////////////////////////////
//   Class : OCMain
// Description : Handles the toplevel processing in this program:
//               holds the tree of source files, and all the scopes,
//               etc.  Generally get the ball rolling.
////////////////////////////////////////////////////////////////////
class OCMain {
public:
  OCMain(OCScope *global_scope);
  ~OCMain();

  bool read_source(const string &root);

  bool process_all();
  bool process(string dirname);

  void report_depends(const string &dirname) const;
  void report_reverse_depends(const string &dirname) const;

  static string get_root();
  static void chdir_root();

private:
  bool r_process_all(OCDirectory *dir);
  bool p_process(OCDirectory *dir);
  bool read_global_file();
  static Filename get_cwd();


  OCScope *_global_scope;
  OCScope *_def_scope;
  OCCommandFile *_defs;

  OCDirectoryTree _tree;
  OCNamedScopes _named_scopes;
  OCScope *_parent_scope;

  static Filename _root;
  string _original_working_dir;
};

#endif
