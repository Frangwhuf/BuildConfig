// Filename: ocDependableFile.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCDEPENDABLEFILE_H
#define OCDEPENDABLEFILE_H

#include "oconfig.h"
#include <set>
#include <vector>
#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

class OCDirectory;

///////////////////////////////////////////////////////////////////
//       Class : OCDependableFile
// Description : Corresponds to a single C/C++ source file, either a
//               .c file or a .h file, that can be scanned for a
//               number of #include statements.  This file may both
//               depend on other files, as well as being depended upon
//               in turn.  This is used to resolved inter-file
//               dependencies.
////////////////////////////////////////////////////////////////////
class OCDependableFile {
public:
  OCDependableFile(OCDirectory *directory, const string &filename);
  void update_from_cache(const vector<string> &words);
  void write_cache(ostream &out);

  OCDirectory *get_directory() const;
  const string &get_filename() const;
  string get_pathname() const;
  string get_dirpath() const;

  bool exists();
  time_t get_mtime();

  int get_num_dependencies();
  OCDependableFile *get_dependency(int n);

  void get_complete_dependencies(vector<OCDependableFile *> &files);
  void get_complete_dependencies(set<OCDependableFile *> &files);

  bool is_circularity();
  string get_circularity();

  bool was_examined() const;

private:
  void update_dependencies();
  OCDependableFile *compute_dependencies(string &circularity);
  void stat_file();

  OCDirectory *_directory;
  string _filename;

  enum Flags {
    F_updating    = 0x001,
    F_updated     = 0x002,
    F_circularity = 0x004,
    F_statted     = 0x008,
    F_exists      = 0x010,
    F_from_cache  = 0x020,
  };
  int _flags;
  string _circularity;
  time_t _mtime;

  class Dependency {
  public:
    OCDependableFile *_file;
    bool _okcircular;

    bool operator < (const Dependency &other) const;
  };

  typedef vector<Dependency> Dependencies;
  Dependencies _dependencies;

  typedef vector<string> ExtraIncludes;
  ExtraIncludes _extra_includes;
};

#endif
