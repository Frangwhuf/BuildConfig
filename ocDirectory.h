// Filename: ocDirectory.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCDIRECTORY_H
#define OCDIRECTORY_H

#include "oconfig.h"

#include <vector>
#include <map>
#include <set>

class OCCommandFile;
class OCScope;
class OCNamedScopes;
class OCDirectoryTree;
class OCDependableFile;

///////////////////////////////////////////////////////////////////
//       Class : OCDirectory
// Description : Represents a single directory in the source
//               hierarchy.  Each OCDirectory object is one-to-one
//               associated with a OCCommandFile object, that
//               corresponds to the source file found within this
//               directory.
////////////////////////////////////////////////////////////////////
class OCDirectory {
public:
  OCDirectory(OCDirectoryTree *tree);
  OCDirectory(const string &dirname, OCDirectory *parent);
  ~OCDirectory();

  OCDirectoryTree *get_tree() const;
  int count_source_files() const;

  const string &get_dirname() const;
  int get_depends_index() const;
  string get_path() const;
  string get_rel_to(const OCDirectory *other) const;

  OCCommandFile *get_source() const;

  int get_num_children() const;
  OCDirectory *get_child(int n) const;

  string get_child_dirnames() const;
  string get_complete_subtree() const;

  OCDependableFile *get_dependable_file(const string &filename, 
                                        bool is_header);

  string get_depends() const;
  void report_depends() const;
  void report_reverse_depends() const;

  void write_dep_cache( const string& filename );
  void read_dep_cache( const string& filename );

  bool get_subdirs(vector<string> &subdirs);
  void add_to_not_found(const string &filename);
  bool non_existing(const string &filename);
  bool recursive_depends_ok();
  bool external_recursion_ok(OCDependableFile *file);
  
  typedef set<OCDirectory *> Depends;
  void get_complete_i_depend_on(Depends &dep) const;
  void get_complete_depends_on_me(Depends &dep) const;

private:

  bool r_scan(const string &prefix);
  bool r_subdirs(const string &prefix, vector<string> &subdirs);
  bool read_source_file(const string &prefix, OCNamedScopes *named_scopes);
  bool read_depends_file(OCNamedScopes *named_scopes);
  bool scan_depends_cache(OCNamedScopes *named_scopes);
  bool resolve_dependencies();
  bool compute_depends_index();
  void read_file_dependencies(const string &cache_filename);
  void update_file_dependencies(const string &cache_filename);

  string get_directories(const Depends &dep) const;
  void show_directories(const Depends &dep) const;

  string _dirname;
  OCScope *_scope;
  OCCommandFile *_source;
  OCDirectory *_parent;
  OCDirectoryTree *_tree;
  typedef vector<OCDirectory *> Children;
  Children _children;
  int _depth;
  bool _dirs_scanned;
  vector<string> _subdirs;
  set<string> _not_found;

  vector<string> _no_recurse;

  Depends _i_depend_on;
  Depends _depends_on_me;
  int _depends_index;
  bool _computing_depends_index;
  bool _recursive_depends_ok;

  typedef map<string, OCDependableFile *> Dependables;
  Dependables _dependables;

  friend class OCDirectoryTree;
};

extern OCDirectory *current_output_directory;

#endif

