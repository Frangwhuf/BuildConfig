// Filename: ocDirectory.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "filename.h"
#include "ocDirectory.h"
#include "ocDirectoryTree.h"
#include "ocScope.h"
#include "ocNamedScopes.h"
#include "ocCommandFile.h"
#include "ocDependableFile.h"
#include "tokenize.h"

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <algorithm>
#include <assert.h>
#include <iterator>

#ifdef WIN32_VC
#include <direct.h>
#include <windows.h>
#endif

#include <string.h>

OCDirectory *current_output_directory = (OCDirectory *)NULL;

// An STL object to sort directories in order by dependency and then
// by name, used in get_child_dirnames().
class SortDirectoriesByDependencyAndName {
public:
  bool operator () (const OCDirectory *a, const OCDirectory *b) const {
    if (a->get_depends_index() != b->get_depends_index()) {
      return a->get_depends_index() < b->get_depends_index();
    }
    return a->get_dirname() < b->get_dirname();
  }
};

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::Constructor
//       Access: Public
//  Description: Creates the root directory.
////////////////////////////////////////////////////////////////////
OCDirectory::
OCDirectory(OCDirectoryTree *tree) {
  _scope = (OCScope *)NULL;
  _source = (OCCommandFile *)NULL;
  _parent = (OCDirectory *)NULL;
  _tree = tree;
  _depth = 0;
  _depends_index = 0;
  _computing_depends_index = false;
  _dirs_scanned = false;
  _recursive_depends_ok = true;

  _dirname = "";
  _tree->_dirnames.insert(OCDirectoryTree::Dirnames::value_type(_dirname, this));
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::Constructor
//       Access: Public
//  Description: Creates a new directory level that automatically adds
//               itself to its parent's children list.
////////////////////////////////////////////////////////////////////
OCDirectory::
OCDirectory(const string &dirname, OCDirectory *parent) :
  _dirname(dirname),
  _parent(parent)
{
  assert(_parent != (OCDirectory *)NULL);
  _scope = (OCScope *)NULL;
  _source = (OCCommandFile *)NULL;
  _parent->_children.push_back(this);
  _tree = _parent->_tree;
  _depth = _parent->_depth + 1;
  _depends_index = 0;
  _computing_depends_index = false;
  _dirs_scanned = false;
  _recursive_depends_ok = _parent->_recursive_depends_ok;

  bool inserted = 
    _tree->_dirnames.insert(OCDirectoryTree::Dirnames::value_type(get_path(), this)).second;
  if (!inserted) {
    cerr << "Warning: multiple directories encountered named "
         << get_path() << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::Destructor
//       Access: Public
//  Description: When a tree root destructs, all of its children are
//               also destroyed.
////////////////////////////////////////////////////////////////////
OCDirectory::
~OCDirectory() {
  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    delete (*ci);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_tree
//       Access: Public
//  Description: Returns the OCDirectoryTree object corresponding to
//               the source tree that this directory is a part of.
////////////////////////////////////////////////////////////////////
OCDirectoryTree *OCDirectory::
get_tree() const {
  return _tree;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::count_source_files
//       Access: Public
//  Description: Returns the number of directories within the tree
//               that actually have a Sources.pp file that was read.
////////////////////////////////////////////////////////////////////
int OCDirectory::
count_source_files() const {
  int count = 0;
  if (_source != (OCCommandFile *)NULL) {
    count++;
  }

  Children::const_iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    count += (*ci)->count_source_files();
  }

  return count;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_dirname
//       Access: Public
//  Description: Returns the name of this particular directory level.
////////////////////////////////////////////////////////////////////
const string &OCDirectory::
get_dirname() const {
  return _dirname;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_depends_index
//       Access: Public
//  Description: Returns the dependency index associated with this
//               directory.  It is generally true that if directory A
//               depends on B, then A.get_depends_index() >
//               B.get_depends_index().
////////////////////////////////////////////////////////////////////
int OCDirectory::
get_depends_index() const {
  return _depends_index;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_path
//       Access: Public
//  Description: Returns the relative path from the root to this
//               particular directory.  This does not include the root
//               name itself, and does not include a trailing slash.
////////////////////////////////////////////////////////////////////
string OCDirectory::
get_path() const {
  if (_parent == (OCDirectory *)NULL) {
    return ".";
  }
  if (_parent->_parent == (OCDirectory *)NULL) {
    return _dirname;
  }
  return _parent->get_path() + "/" + _dirname;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_rel_to
//       Access: Public
//  Description: Returns the relative path to the other directory from
//               this one.  This does not include a trailing slash.
////////////////////////////////////////////////////////////////////
string OCDirectory::
get_rel_to(const OCDirectory *other) const {
  const OCDirectory *a = this;
  const OCDirectory *b = other;

  if (a == b) {
    return ".";
  }

  string prefix, postfix;
  while (a->_depth > b->_depth) {
    prefix += "../";
    a = a->_parent;
    assert(a != (OCDirectory *)NULL);
  }

  while (b->_depth > a->_depth) {
    postfix = b->_dirname + "/" + postfix;
    b = b->_parent;
    assert(b != (OCDirectory *)NULL);
  }

  while (a != b) {
    prefix += "../";
    postfix = b->_dirname + "/" + postfix;
    a = a->_parent;
    b = b->_parent;
    assert(a != (OCDirectory *)NULL);
    assert(b != (OCDirectory *)NULL);
  }

  string result = prefix + postfix;
  assert(!result.empty());
  return result.substr(0, result.length() - 1);
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_source
//       Access: Public
//  Description: Returns the source file associated with this level
//               of the directory hierarchy.  This *might* be NULL.
////////////////////////////////////////////////////////////////////
OCCommandFile *OCDirectory::
get_source() const {
  return _source;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_num_children
//       Access: Public
//  Description: Returns the number of subdirectories below this
//               level.
////////////////////////////////////////////////////////////////////
int OCDirectory::
get_num_children() const {
  return static_cast<int>(_children.size());
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_child
//       Access: Public
//  Description: Returns the nth subdirectory below this level.
////////////////////////////////////////////////////////////////////
OCDirectory *OCDirectory::
get_child(int n) const {
  assert(n >= 0 && n < (int)_children.size());
  return _children[static_cast<unsigned>(n)];
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_child_dirnames
//       Access: Public
//  Description: Returns a single string listing the names of all the
//               subdirectories of this level, delimited by spaces.
//
//               The list is sorted in dependency order such that a
//               directory is listed after the other directories it
//               might depend on.
////////////////////////////////////////////////////////////////////
string OCDirectory::
get_child_dirnames() const {
  Children copy_children = _children;
  sort(copy_children.begin(), copy_children.end(),
       SortDirectoriesByDependencyAndName());

  vector<string> words;
  Children::const_iterator ci;
  for (ci = copy_children.begin(); ci != copy_children.end(); ++ci) {
    words.push_back((*ci)->get_dirname());
  }

  string result = repaste(words, " ");
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_complete_subtree
//       Access: Public
//  Description: Returns a single string listing the relative path
//               from the source root to each source directory at this
//               level and below, delimited by spaces.
////////////////////////////////////////////////////////////////////
string OCDirectory::
get_complete_subtree() const {
  Children copy_children = _children;
  sort(copy_children.begin(), copy_children.end(),
       SortDirectoriesByDependencyAndName());

  vector<string> words;
  words.push_back(get_path());

  Children::const_iterator ci;
  for (ci = copy_children.begin(); ci != copy_children.end(); ++ci) {
    words.push_back((*ci)->get_complete_subtree());
  }

  string result = repaste(words, " ");
  return result;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_dependable_file
//       Access: Public
//  Description: Returns a OCDependableFile object corresponding to
//               the named filename, creating one if it does not
//               already exist.  This can be used to determine the
//               inter-file dependencies between source files.
//
//               If is_header is true, then the file will be added to
//               the index at the top of the directory tree, so that
//               other directories may include this file.  In this
//               case, if the filename is not unique, a warning
//               message will be issued.
////////////////////////////////////////////////////////////////////
OCDependableFile *OCDirectory::
get_dependable_file(const string &fname, bool is_header) {
  Dependables::iterator di;
  di = _dependables.find(fname);
  if (di != _dependables.end()) {
    return (*di).second;
  }

  // No such file found; create a new definition.
  OCDependableFile *dependable = new OCDependableFile(this, fname);
  _dependables.insert(Dependables::value_type(fname, dependable));

  if (is_header) {
    bool unique = _tree->_dependables.insert
      (OCDirectoryTree::Dependables::value_type(fname, dependable)).second;
    
    if (!unique) {
	  Filename f1(dependable->get_pathname());
	  Filename f2(_tree->find_dependable_file(fname)->get_pathname());
	  f1.standardize();
	  f2.standardize();
	  if (strcmp(f1.c_str(), f2.c_str()) != 0) {
        cerr << "Warning: source file " << dependable->get_pathname()
             << " may be confused with "
             << _tree->find_dependable_file(fname)->get_pathname()
             << ".\n";
	  }
    }
  }

  return dependable;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_depends
//       Access: Public
//  Description: Get all the directories that the current directory
//               depends on.
////////////////////////////////////////////////////////////////////
string OCDirectory::
get_depends() const {
  if (_i_depend_on.empty()) {
    return "";
  } else {
	// Get the complete set of directories we depend on
	Depends dep;
	get_complete_i_depend_on(dep);

	string all = get_directories(_i_depend_on);
	all += " " + get_directories(dep);
	return all;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::report_depends
//       Access: Public
//  Description: Reports all the directories that the current
//               directory depends on.
////////////////////////////////////////////////////////////////////
void OCDirectory::
report_depends() const {
  if (_i_depend_on.empty()) {
    cerr << _dirname << " depends on no other directories.\n";

  } else {
    // Get the complete set of directories we depend on.
    Depends dep;
    get_complete_i_depend_on(dep);
    
    cerr << _dirname << " depends directly on the following directories:";
	cerr << "\n" << get_directories(_i_depend_on) << "\n";

    cerr << "and directly or indirectly on the following directories:";
	cerr << "\n" << get_directories(dep) << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::report_reverse_depends
//       Access: Public
//  Description: Reports all the directories that depend on the
//               current directory.
////////////////////////////////////////////////////////////////////
void OCDirectory::
report_reverse_depends() const {
  if (_depends_on_me.empty()) {
    cerr << _dirname << " is needed by no other directories.\n";

  } else {
    // Get the complete set of directories we depend on.
    Depends dep;
    get_complete_depends_on_me(dep);

    cerr << _dirname << " is needed directly by the following directories:";
	cerr << "\n" << get_directories(_depends_on_me) << "\n";

    cerr << "and directly or indirectly by the following directories:";
	cerr << "\n" << get_directories(dep) << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::write_dep_cache
//       Access: Public
//  Description: Write a dependency cache file out to a particular file
////////////////////////////////////////////////////////////////////
void OCDirectory::
write_dep_cache( const string& filename )
{
  if (dry_run) {
    // If this is just a dry run, just report circularities.
    Dependables::const_iterator di;
    for (di = _dependables.begin(); di != _dependables.end(); ++di) {
      OCDependableFile *file = (*di).second;
      if (file->was_examined()) {
        if (file->is_circularity()) {
          cerr << "Warning: circular #include directives:\n"
               << "  " << file->get_circularity() << "\n";
        }
      }
    }

  } else {
    // Open up the dependency cache file in the directory.
    Filename cache_pathname(filename);
    cache_pathname.set_text();
    cache_pathname.unlink();

    // If we have no files, don't bother writing the cache.
    if (!_dependables.empty()) {
      bool wrote_anything = false;
      
      ofstream out;
      if (!cache_pathname.open_write(out)) {
        cerr << "Cannot update cache dependency file " << cache_pathname << "\n";
        return;
      }
      
      // Walk through our list of dependable files, writing them out the
      // the cache file.
      Dependables::const_iterator di;
      for (di = _dependables.begin(); di != _dependables.end(); ++di) {
        OCDependableFile *file = (*di).second;
        if (file->was_examined()) {
          if (file->is_circularity()) {
            cerr << "Warning: circular #include directives:\n"
                 << "  " << file->get_circularity() << "\n";
          }
          file->write_cache(out);
          wrote_anything = true;
        }
      }
      
      out.close();
      
      if (!wrote_anything) {
        // Well, if we didn't write anything, remove the cache file
        // after all.
        cache_pathname.unlink();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::read_dep_cache
//       Access: Public
//  Description: Read a dependency cache file from a given filename
////////////////////////////////////////////////////////////////////
void OCDirectory::
read_dep_cache( const string& filename )
{
  Filename cache_file( filename );
  cache_file.set_text();
  ifstream in;
  if (!cache_file.open_read(in)) {
    // Can't read it.  Maybe it's not there.  No problem.
    return;
  }
  if (verbose) {
    cerr << "Reading (dep) \"" << cache_file.to_os_specific() << "\"\n";
  }

  string line;
  getline(in, line);
  while (!in.fail() && !in.eof()) {
    vector<string> words;
    tokenize_whitespace(line, words);
    if (words.size() >= 2) {
      OCDependableFile *file = get_dependable_file(words[0], false);
      file->update_from_cache(words);
    }
    getline(in, line);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_subdirs
//       Access: Private
//  Description: Gets all of the directories below the
//               root. This is different than r_scan in that we do
//               not care whether or not there is a SOURCE file in
//               the sub dirs. 
//               Sub dirs are pushed onto vector_string &subdirs
////////////////////////////////////////////////////////////////////
bool OCDirectory::
get_subdirs(vector<string> &subdirs) {
  // we only want to compute subdirs once
  if (_dirs_scanned) {
    vector<string>::const_iterator ni;
    for (ni = _subdirs.begin(); ni != _subdirs.end(); ++ni) {
      const string &dirname = (*ni);
      subdirs.push_back(dirname);
    }
    return true;
  }

  // otherwise we recursively scan for dirs
  if (r_subdirs(get_path(), subdirs)) {
    _dirs_scanned = true;
    // save items into _subdirs
    vector<string>::const_iterator ni;
    for (ni = subdirs.begin(); ni != subdirs.end(); ++ni) {
      const string &dirname = (*ni);
      _subdirs.push_back(dirname);
    }
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::add_to_not_found
//       Access: Public
//  Description: Add to the not found list. This list contains
//               files that were searched for in this directory
//               but not found. Used to prevent looking for the
//               same file over and over again.
////////////////////////////////////////////////////////////////////
void OCDirectory::
add_to_not_found(const string &filename) {
  _not_found.insert(filename);
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::non_existing
//       Access: Public
//  Description: Search this _not_found list for a filename.
//               If the file is on that list then it is not part
//               of this directory and true is returned. False otherwise.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
non_existing(const string &filename) {
  set<string>::iterator si = _not_found.find(filename);
  return (si != _not_found.end());
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::recursive_depends_ok
//       Access: Public
//  Description: Returns whether or not it is ok for this dir
//               to recursively determine it's dependency tree.
//               Things such as STL and boost would not be ok.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
recursive_depends_ok() {
  return _recursive_depends_ok;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::external_recursion_ok
//       Access: Public
//  Description: Returns whether or not it is ok for this dir to
//               recursively determine it's dependency tree for the
//               given file.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
external_recursion_ok(OCDependableFile * file) {
  Filename fname(file->get_pathname());
  fname.standardize();

  // check to see if we are in an external source tree that should
  // not be recursed
  vector<string>::const_iterator ni;
  for (ni=_no_recurse.begin(); ni!=_no_recurse.end(); ++ni) {
	string f = fname.c_str();
	string partial = f.substr(0, (strlen((*ni).c_str())));

	// This file is in a dir we should not recurse into
	if (strcmp((*ni).c_str(), partial.c_str()) == 0) {
	  return false;
	}
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_complete_i_depend_on
//       Access: Public
//  Description: Gets the transitive closure of i_depend_on.  This
//               fills the given set (which must have been empty
//               before this call) with the complete set of all
//               directories this directory depends on, directly or
//               indirectly.
////////////////////////////////////////////////////////////////////
void OCDirectory::
get_complete_i_depend_on(Depends &dep) const {
  Depends::const_iterator di;
  for (di = _i_depend_on.begin(); di != _i_depend_on.end(); ++di) {
    OCDirectory *dir = (*di);
    bool inserted = dep.insert(dir).second;
    if (inserted) {
      dir->get_complete_i_depend_on(dep);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_complete_depends_on_me
//       Access: Public
//  Description: Gets the transitive closure of depends_on_me.  This
//               fills the given set (which must have been empty
//               before this call) with the complete set of all
//               directories this that depend on this directory,
//               directly or indirectly.
////////////////////////////////////////////////////////////////////
void OCDirectory::
get_complete_depends_on_me(Depends &dep) const {
  Depends::const_iterator di;
  for (di = _depends_on_me.begin(); di != _depends_on_me.end(); ++di) {
    OCDirectory *dir = (*di);
    bool inserted = dep.insert(dir).second;
    if (inserted) {
      dir->get_complete_depends_on_me(dep);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::r_scan
//       Access: Private
//  Description: The recursive implementation of scan_source().
////////////////////////////////////////////////////////////////////
bool OCDirectory::
r_scan(const string &prefix) {
  Filename root_name = ".";
  if (!prefix.empty()) {
    root_name = prefix.substr(0, prefix.length() - 1);
  }

  // Collect all the filenames in the directory in this vector first,
  // so we can sort them.
  vector<string> filenames;
  if (!root_name.scan_directory(filenames)) {
    cerr << "Unable to scan directory " << root_name << "\n";
    return false;
  }

  vector<string>::const_iterator fi;
  for (fi = filenames.begin(); fi != filenames.end(); ++fi) {
    string filename = (*fi);

    if (!filename.empty() && filename[0] != '.') {
      // Is this possibly a subdirectory with its own Sources.pp
      // within it?
      string next_prefix = prefix + filename + "/";
      Filename source_filename = next_prefix + SOURCE_FILENAME;
      if (source_filename.exists()) {
        OCDirectory *subtree = new OCDirectory(filename, this);

        if (!subtree->r_scan(next_prefix)) {
          return false;
        }
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::r_subdirs
//       Access: Private
//  Description: The recursive implementation of get_subdirs().
////////////////////////////////////////////////////////////////////
bool OCDirectory::
r_subdirs(const string &prefix, vector<string> &subdirs) {
  Filename root_name = prefix;

  // Collect all the filenames in the directory in this vector first,
  // so we can sort them.
  vector<string> filenames;
  if (!root_name.scan_directory(filenames)) {
    cerr << "Unable to scan directory " << root_name << "\n";
    return false;
  }

  vector<string>::const_iterator fi;
  for (fi = filenames.begin(); fi != filenames.end(); ++fi) {
    string filename = (*fi);
    if (!filename.empty() && filename[0] != '.') {
      Filename fn(root_name.get_fullpath(), filename);
      if (fn.is_directory()) {
        string next_prefix;
        if(prefix.empty()) {
          next_prefix = filename;
        } else {
          next_prefix = prefix + "/" + filename;
        }
        subdirs.push_back(next_prefix);
        r_subdirs(next_prefix, subdirs);
      }
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::read_source_file
//       Access: Private
//  Description: Recursively reads in the source file at each level,
//               if defined.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
read_source_file(const string &prefix, OCNamedScopes *named_scopes) {
  string source_filename = prefix + SOURCE_FILENAME;

  ifstream in(source_filename.c_str());
  if (in) {
    if (verbose) {
        cerr << "Reading (dir) \"" << source_filename << "\"\n";
    }

    string path = get_path();
    named_scopes->set_current(path);
    _scope = named_scopes->make_scope("");
    
    _scope->define_variable("SOURCEFILE", SOURCE_FILENAME);
    _scope->define_variable("DIRNAME", _dirname);
    _scope->define_variable("DIRPREFIX", prefix);
    if (path == ".") {
      _scope->define_variable("PATH", "");
    } else {
      _scope->define_variable("PATH", path);
    }
    _scope->define_variable("SUBDIRS", get_child_dirnames());
    _scope->define_variable("SUBTREE", get_complete_subtree());
    _scope->set_directory(this);
    
    _source = new OCCommandFile(_scope);
    
    if (!_source->read_stream(in, source_filename)) {
      return false;
    }
  }

  if( ! _scope ) {
    return true;
  }
    
  string recursive_ok = _scope->get_variable("NO_RECURSIVE_DEPENDS");
  if(!recursive_ok.empty() && recursive_ok == "1") {
    _recursive_depends_ok = false;
  } else if(!recursive_ok.empty() && recursive_ok == "0") {
    _recursive_depends_ok = true;
  }

  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    (*ci)->_recursive_depends_ok = _recursive_depends_ok;
    if (!(*ci)->read_source_file(prefix + (*ci)->get_dirname() + "/",
                                  named_scopes)) {
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::read_depends_file
//       Access: Private
//  Description: Recursively reads in the dependency definition file
//               for each source file.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
read_depends_file(OCNamedScopes *named_scopes) {
  if (_scope != (OCScope *)NULL) {
    // Read the depends file, so we can determine the relationship
    // between this source file and all of the other source files.

    string depends_filename = _scope->expand_variable("DEPENDS_FILE");
    if (depends_filename.empty()) {
      cerr << "No definition given for $[DEPENDS_FILE], cannot process.\n";
      return false;
    }
    
    named_scopes->set_current(get_path());
    current_output_directory = this;
    OCCommandFile depends(_scope);
    if (!depends.read_file(depends_filename)) {
      cerr << "Error reading dependency definition file "
           << depends_filename << ".\n";
      return false;
    }
    
    // This should have defined the variable DEPEND_DIRS, which lists
    // the various dirnames this source file depends on.

    vector<string> dirnames;
    tokenize_whitespace(_scope->expand_variable("DEPEND_DIRS"), dirnames);

    vector<string>::const_iterator ni;
    for (ni = dirnames.begin(); ni != dirnames.end(); ++ni) {
      const string &dirname = (*ni);
      OCDirectory *dir = _tree->find_dirname(dirname);
      if (dir == (OCDirectory *)NULL) {
        cerr << "Could not find dependent dirname " << dirname << "\n";
      } else {
        if (dir != this) {
          _i_depend_on.insert(dir);
          dir->_depends_on_me.insert(this);
        }
      }
    }
   
    // add any external dependecy dirs (not part of this tree) 
    vector<string> external_dirs;
    tokenize_whitespace(_scope->expand_variable("EXTERNAL_DEPENDS"), external_dirs);
    vector<string>::const_iterator ei;
    for (ei = external_dirs.begin(); ei != external_dirs.end(); ++ei) {
      const string &dirname = (*ei);
      OCDirectory *dir = new OCDirectory(dirname, this);
      _i_depend_on.insert(dir);
      dir->_depends_on_me.insert(this);
    }

	// Also create list of external source trees that we will not recurse into
	tokenize_whitespace(_scope->expand_variable("NO_EXTERNAL_RECURSION"), _no_recurse);

    // This may also have defined the variable DEPENDABLE_HEADERS,
    // which lists the header files in this directory that C/C++
    // source files in this and other directories might be including
    // (and will therefore depend on).
    vector<string> headers;
    tokenize_whitespace(_scope->expand_variable("DEPENDABLE_HEADERS"), headers);
    for (ni = headers.begin(); ni != headers.end(); ++ni) {
      get_dependable_file(*ni, true);
    }
  }
    
  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    if (!(*ci)->read_depends_file(named_scopes)) {
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::scan_depends_cache
//       Access: Private
//  Description: Recursively reads in the dependency cache file for
//               each source file.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
scan_depends_cache(OCNamedScopes * named_scopes) {
  if (_scope != (OCScope *)NULL) {
	string depends_cache = _scope->expand_variable("DEPENDENCY_CACHE_FILENAME");
	if (depends_cache.empty()) {
	  cerr << "No definition given for $(DEPENDENCY_CACHE_FILENAME), cannot process.\n";
	  return false;
	}

	named_scopes->set_current(get_path());
	Filename cache_pathname(depends_cache);
	read_dep_cache(cache_pathname);
  }

  Children::iterator ci;
  for (ci=_children.begin(); ci!=_children.end(); ++ci) {
	if (!(*ci)->scan_depends_cache(named_scopes)) {
	  return false;
	}
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::resolve_dependencies
//       Access: Private
//  Description: Visits each directory and assigns a correct
//               _depends_index to each one, such that if directory A
//               depends on directory B then A._depends_index >
//               B._depends_index.
//
//               This also detects cycles in the directory dependency
//               graph.
////////////////////////////////////////////////////////////////////
bool OCDirectory::
resolve_dependencies() {
  if (!compute_depends_index()) {
    return false;
  }

  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    if (!(*ci)->resolve_dependencies()) {
      return false;
    }
  }

  // Now that we've resolved all of our children's dependencies,
  // redefine our SUBDIRS and SUBTREE variables to put things in the
  // right order.
  if (_scope != (OCScope *)NULL) {
    _scope->define_variable("SUBDIRS", get_child_dirnames());
    _scope->define_variable("SUBTREE", get_complete_subtree());
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::compute_depends_index
//       Access: Private
//  Description: Computes the dependency score for a particular
//               directory.  See resolve_dependencies().
////////////////////////////////////////////////////////////////////
bool OCDirectory::
compute_depends_index() {
  if (_depends_index != 0) {
    return true;
  }

  if (_i_depend_on.empty()) {
    _depends_index = 1;
    return true;
  }

  _computing_depends_index = true;
  int max_index = 0;

  Depends::iterator di;
  for (di = _i_depend_on.begin(); di != _i_depend_on.end(); ++di) {
    if ((*di)->_computing_depends_index) {
      // Oops, we have a cycle!
      cerr << "Cycle detected in inter-directory dependencies!\n"
           << _dirname << " depends on " << (*di)->_dirname << "\n";
      return false;
    }
      
    if (!(*di)->compute_depends_index()) {
      // Keep reporting the cycle as we unroll the recursion.
      cerr << _dirname << " depends on " << (*di)->_dirname << "\n";
      return false;
    }

    max_index = max(max_index, (*di)->_depends_index);
  }

  _computing_depends_index = false;
  _depends_index = max_index + 1;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::read_file_dependencies
//       Access: Private
//  Description: Before processing the source files, makes a pass and
//               reads in all of the dependency cache files so we'll
//               have a heads-up on which files depend on the others.
////////////////////////////////////////////////////////////////////
void OCDirectory::
read_file_dependencies(const string &cache_filename) {
  // Open up the dependency cache file in the directory.
  Filename cache_pathname(get_path(), cache_filename);
  read_dep_cache( cache_pathname );

  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    (*ci)->read_file_dependencies(cache_filename);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::update_file_dependencies
//       Access: Private
//  Description: After all source processing has completed, makes one
//               more pass through the directory hierarchy and writes
//               out the inter-file dependency cache.
////////////////////////////////////////////////////////////////////
void OCDirectory::
update_file_dependencies(const string &cache_filename) {
  Filename cache_pathname(get_path(), cache_filename);
  write_dep_cache( cache_pathname );

  Children::iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    (*ci)->update_file_dependencies(cache_filename);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::get_directories
//       Access: Private
//  Description: Get a set of dependency directory names
////////////////////////////////////////////////////////////////////
string OCDirectory::
get_directories(const OCDirectory::Depends &dep) const {
	// Copy the set into a vector, so we can sort it into a nice order
	// for the user's pleasure.
	vector<OCDirectory *> dirs;
	copy(dep.begin(), dep.end(),
		 back_insert_iterator<vector<OCDirectory *> >(dirs));

	sort(dirs.begin(), dirs.end(), SortDirectoriesByDependencyAndName());

	static const int max_col = 72;
	string all;
	vector<OCDirectory *>::const_iterator di;
	for (di=dirs.begin(); di!=dirs.end(); ++di) {
	  all += (*di)->_dirname + " ";
	}
	return all;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectory::show_directories
//       Access: Private
//  Description: Writes a set of dependency directory names to
//               standard error.  The output begins with a newline.
////////////////////////////////////////////////////////////////////
void OCDirectory::
show_directories(const OCDirectory::Depends &dep) const {
  // Copy the set into a vector, so we can sort it into a nice order
  // for the user's pleasure.
  vector<OCDirectory *> dirs;
  copy(dep.begin(), dep.end(),
       back_insert_iterator<vector<OCDirectory *> >(dirs));
  
  sort(dirs.begin(), dirs.end(), SortDirectoriesByDependencyAndName());
  
  static const int max_col = 72;
  int col = max_col;
  vector<OCDirectory *>::const_iterator di;
  for (di = dirs.begin(); di != dirs.end(); ++di) {
    const string &dirname = (*di)->_dirname;
    col += static_cast<int>(dirname.length() + 1);
    if (col >= max_col) {
      col = static_cast<int>(dirname.length() + 2);
      cerr << "\n  " << dirname;
    } else {
      cerr << " " << dirname;
    }
  }
  cerr << "\n";
}
