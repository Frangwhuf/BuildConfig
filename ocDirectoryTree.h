// Filename: ocDirectoryTree.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCDIRECTORYTREE_H
#define OCDIRECTORYTREE_H

#include "oconfig.h"

#include <map>

class OCNamedScopes;
class OCDirectory;
class OCDependableFile;

///////////////////////////////////////////////////////////////////
//       Class : OCDirectoryTree
// Description : Stores the entire directory hierarchy relationship of the
//               source tree.  This is the root of a tree of
//               OCDirectory objects, each of which corresponds to a
//               particular directory.
////////////////////////////////////////////////////////////////////
class OCDirectoryTree {
public:
  OCDirectoryTree();
  ~OCDirectoryTree();

  bool scan_source(OCNamedScopes *named_scopes);
  bool scan_depends(OCNamedScopes *named_scopes);

  int count_source_files() const;
  OCDirectory *get_root() const;

  string get_complete_tree() const;

  OCDirectory *find_dirname(const string &dirname) const;

  OCDependableFile *find_dependable_file(const string &filename) const;
  OCDependableFile *get_dependable_file_by_dirpath(const string &dirpath,
                                                   bool is_header);

  void read_file_dependencies(const string &cache_filename);
  void update_file_dependencies(const string &cache_filename);

private:
  OCDirectory *_root;

  typedef map<string, OCDirectory *> Dirnames;
  Dirnames _dirnames;

  typedef map<string, OCDependableFile *> Dependables;
  Dependables _dependables;

  friend class OCDirectory;
};

#endif
