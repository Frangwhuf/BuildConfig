// Filename: ocDependableFile.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "ocDependableFile.h"
#include "ocDirectory.h"
#include "ocDirectoryTree.h"
#include "filename.h"
#include "check_include.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <assert.h>
#include <sys/stat.h>
#include <algorithm>
#include <iterator>

class SortDependableFilesByName {
public:
  bool operator () (OCDependableFile *a, OCDependableFile *b) const {
    return a->get_filename() < b->get_filename();
  }
};

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::Ordering Operator
//       Access: Public
//  Description: We provide this function so we can sort the
//               dependency list into a consistent ordering, so that
//               the makefiles won't get randomly regenerated between
//               different sessions.
////////////////////////////////////////////////////////////////////
bool OCDependableFile::Dependency::
operator < (const Dependency &other) const {
  return _file->get_filename() < other._file->get_filename();
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCDependableFile::
OCDependableFile(OCDirectory *directory, const string &filename) :
  _directory(directory),
  _filename(filename)
{
  _flags = 0;
  _mtime = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::update_from_cache
//       Access: Public
//  Description: Called as the dependency cache file is being read,
//               this asks the file to update its information from the
//               cache file if appropriate.  This means comparing the
//               cached modification time against the file's actual
//               modification time, and storing the cached
//               dependencies if they match.
////////////////////////////////////////////////////////////////////
void OCDependableFile::
update_from_cache(const vector<string> &words) {
  // Shouldn't call this once the file has actually been read.
  assert((_flags & F_updated) == 0);
  assert((_flags & F_updating) == 0);
  assert((_flags & F_from_cache) == 0);
  assert(words.size() >= 2);

  // The second parameter is the cached modification time.
  time_t mtime = strtol(words[1].c_str(), (char **)NULL, 10);
  if (mtime == get_mtime()) {
    // The modification matches; preserve the cache information.
    OCDirectoryTree *tree = _directory->get_tree();

    _dependencies.clear();
    vector<string>::const_iterator wi;
    for (wi = words.begin() + 2; wi != words.end(); ++wi) {
      string dirpath = (*wi);

      Dependency dep;
      dep._okcircular = false;

      if (dirpath.length() > 1 && dirpath[0] == '/') {
        // If the first character is '/', it means that the file has
        // been marked okcircular.
        dep._okcircular = true;
        dirpath = dirpath.substr(1);
      }

      if (dirpath.length() > 2 && dirpath.substr(0, 2) == "*/") {
        // This is an extra include file, not a file in this source
        // tree.
        _extra_includes.push_back(dirpath.substr(2));

      } else {
        dep._file = 
          tree->get_dependable_file_by_dirpath(dirpath, false);
        if (dep._file != (OCDependableFile *)NULL) {
          _dependencies.push_back(dep);
        }
      }
    }

    _flags |= F_from_cache;
    _flags |= F_updated;
    sort(_dependencies.begin(), _dependencies.end());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::write_cache
//       Access: Public
//  Description: Writes the dependency information out as a single
//               line to the indicated dependency cache file.
////////////////////////////////////////////////////////////////////
void OCDependableFile::
write_cache(ostream &out) {
  out << _filename << " " << get_mtime();

  Dependencies::const_iterator di;
  for (di = _dependencies.begin(); di != _dependencies.end(); ++di) {
    out << " ";
    if ((*di)._okcircular) {
      out << "/";
    }
    out << (*di)._file->get_pathname();
  }

  // Also write out the extra includes--those #include directives
  // which do not reference a file within this source tree.  We need
  // those just for comparison's sake later, so we can tell whether
  // the cache line is still current (without having to know which
  // files are part of the tree).
  ExtraIncludes::const_iterator ei;
  for (ei = _extra_includes.begin(); ei != _extra_includes.end(); ++ei) {
    out << " */" << (*ei);
  }

  out << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_directory
//       Access: Public
//  Description: Returns the directory that this file can be found in.
////////////////////////////////////////////////////////////////////
OCDirectory *OCDependableFile::
get_directory() const {
  return _directory;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_filename
//       Access: Public
//  Description: Returns the local filename of this particular file
//               within the directory.
////////////////////////////////////////////////////////////////////
const string &OCDependableFile::
get_filename() const {
  return _filename;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_pathname
//       Access: Public
//  Description: Returns the relative pathname from the root of the
//               source tree to this particular filename.
////////////////////////////////////////////////////////////////////
string OCDependableFile::
get_pathname() const {
  return _directory->get_path() + "/" + _filename;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_dirpath
//       Access: Public
//  Description: Returns an abbreviated pathname to this file, in the
//               form dirname/filename.
////////////////////////////////////////////////////////////////////
string OCDependableFile::
get_dirpath() const {
  return _directory->get_dirname() + "/" + _filename;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::exists
//       Access: Public
//  Description: Returns true if the file exists, false if it does
//               not.
////////////////////////////////////////////////////////////////////
bool OCDependableFile::
exists() {
  stat_file();
  return ((_flags & F_exists) != 0);
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_mtime
//       Access: Public
//  Description: Returns the last modification time of the file.
////////////////////////////////////////////////////////////////////
time_t OCDependableFile::
get_mtime() {
  stat_file();
  return _mtime;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_num_dependencies
//       Access: Public
//  Description: Returns the number of files this file depends on.
////////////////////////////////////////////////////////////////////
int OCDependableFile::
get_num_dependencies() {
  update_dependencies();
  return static_cast<int>(_dependencies.size());
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_dependency
//       Access: Public
//  Description: Returns the nth file this file depends on.
////////////////////////////////////////////////////////////////////
OCDependableFile *OCDependableFile::
get_dependency(int n) {
  assert((_flags & F_updated) != 0);
  assert(n >= 0 && n < (int)_dependencies.size());
  return _dependencies[static_cast<unsigned>(n)]._file;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_complete_dependencies
//       Access: Public
//  Description: This flavor of get_complete_dependencies() works like
//               the one below, except it returns the results in a
//               consistently-ordered vector.  This allows us to keep
//               the dependencies in the same order between sessions
//               and prevent makefiles from being arbitrarily
//               regenerated.
////////////////////////////////////////////////////////////////////
void OCDependableFile::
get_complete_dependencies(vector<OCDependableFile *> &files) {
  set<OCDependableFile *> files_set;
  get_complete_dependencies(files_set);

  copy(files_set.begin(), files_set.end(), back_inserter(files));
  sort(files.begin(), files.end(), SortDependableFilesByName());
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_complete_dependencies
//       Access: Public
//  Description: Recursively determines the complete set of files this
//               file depends on.  It is the user's responsibility to
//               empty the set before calling this function; the
//               results will simply be added to the existing set.
////////////////////////////////////////////////////////////////////
void OCDependableFile::
get_complete_dependencies(set<OCDependableFile *> &files) {
  update_dependencies();

  Dependencies::const_iterator di;
  for (di = _dependencies.begin(); di != _dependencies.end(); ++di) {
    OCDependableFile *file = (*di)._file;
    if (files.insert(file).second) {
      if(file->get_directory()->recursive_depends_ok() &&
		 _directory->external_recursion_ok(file)) {
        file->get_complete_dependencies(files);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::is_circularity
//       Access: Public
//  Description: Returns true if a circular dependency exists between
//               this file and one or more other files.
////////////////////////////////////////////////////////////////////
bool OCDependableFile::
is_circularity() {
  update_dependencies();
  return (_flags & F_circularity) != 0;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::get_circularity
//       Access: Public
//  Description: If is_circularity() returns true, returns a string
//               describing the circular dependency path for the user.
////////////////////////////////////////////////////////////////////
string OCDependableFile::
get_circularity() {
  update_dependencies();
  return _circularity;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::was_examined
//       Access: Public
//  Description: Returns true if anyone ever asked this file for its
//               list of dependencies, or false otherwise.
////////////////////////////////////////////////////////////////////
bool OCDependableFile::
was_examined() const {
  return ((_flags & F_updated) != 0);
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::update_dependencies
//       Access: Private
//  Description: Builds up the dependency list--the list of files this
//               file depends on--if it hasn't already been built.  If
//               a circular dependency is detected during this
//               process, _circularity and _circularity_detected will
//               be updated accordingly.
////////////////////////////////////////////////////////////////////
void OCDependableFile::
update_dependencies() {
  if ((_flags & F_updated) != 0) {
    return;
  }  

  assert((_flags & F_updating) == 0);
  string circularity;
  compute_dependencies(circularity);
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::compute_dependencies
//       Access: Private
//  Description: Builds up the dependency list--the list of files this
//               file depends on--if it hasn't already been built.
//
//               If a circularity is detected, e.g. two files depend
//               on each other, a pointer to the offending file is
//               returned and the string is updated to indicate the
//               circularity.  Otherwise, if there is no circularity,
//               NULL is returned.
////////////////////////////////////////////////////////////////////
OCDependableFile *OCDependableFile::
compute_dependencies(string &circularity) {
  if ((_flags & F_updated) != 0) {
    return (OCDependableFile *)NULL;

  } else if ((_flags & F_updating) != 0) {
    // Oh oh, a circular dependency!
    circularity = get_dirpath();
    return this;
  }

  _flags |= F_updating;

  if ((_flags & F_from_cache) == 0) {
    // Now open the file and scan it for #include statements.
    ifstream in(get_pathname().c_str());
    if (!in) {
      // Can't read the file, or the file doesn't exist.  Interesting.
      if (exists()) {
        cerr << "Warning: dependent file " << get_pathname() 
             << " exists but cannot be read.\n";
      } else {
        cerr << "Warning: dependent file " << get_pathname() 
             << " does not exist.\n";
      }

    } else {
      OCDirectoryTree *tree = _directory->get_tree();
      
      vector<string> dirnames;
      _directory->get_subdirs(dirnames);
      dirnames.push_back(_directory->get_path());

      set<OCDirectory *> i_dep_on;
      _directory->get_complete_i_depend_on(i_dep_on);

      bool okcircular = false;
      string line;
      getline(in, line);
      while (!in.fail() && !in.eof()) {
        if (line.substr(0, 16) == "/* okcircular */") {
          okcircular = true;
        } else {
          string filename = check_include(line);
          if (!filename.empty() /*&& filename.find('/') == string::npos*/) {
            // if it starts with '../' discard it
            if (filename[0] == '.' && filename[1] == '.') {
              filename = filename.substr(3, filename.length());
            }

            Dependency dep;
            dep._okcircular = okcircular;
            dep._file = tree->find_dependable_file(filename);
            if (dep._file != (OCDependableFile *)NULL) {
              // All right!  Here's a file we depend on.  Add it to the
              // list.
              _dependencies.push_back(dep);
          
            // the file is not in our tree but prehaps it should be.
            // likely this file has not yet been examined
            } else {
              // check our own src tree to see if it exists
              bool found = false;
              if (!_directory->non_existing(filename)) {
                vector<string>::const_iterator ni;
                for (ni = dirnames.begin(); ni != dirnames.end(); ++ni) {
                  const string &dirname = (*ni);
                  Filename fn = dirname + "/" + filename;
                  if (fn.exists() && !fn.is_directory()) {
                    // All right!  Here's a file we depend on.  Add it to the
                    // tree and then our list.
                    string full = fn.get_fullpath();
                    string partial = full.substr(_directory->get_path().length()+1,
                                                full.length());
                    dep._file = _directory->get_dependable_file(partial, true);
                    _dependencies.push_back(dep);
                    found = true;
                    break;
                  }
                }
              }
              
              if (!found) {
                // add to not found list so we don't look again later
                _directory->add_to_not_found(filename);

                // check the dirs we depend on 
                set<OCDirectory *>::const_iterator de;
                for (de = i_dep_on.begin(); de != i_dep_on.end(); de++) {
                  OCDirectory *dir = (*de);
                  Filename fn = dir->get_path() + "/" + filename;
                  if (fn.exists() && !fn.is_directory()) {
                    dep._file = dir->get_dependable_file(filename, true);
                    _dependencies.push_back(dep);
                    found = true;
                    break;
                  } else {
                    // add to not found list so we don't look again later
                    dir->add_to_not_found(filename);
                  }
                }
              }

              if (!found) {
                // It's an include file from somewhere else, not from within
                // our source tree.  We don't care about it, but we do need
                // to record it so we can easily check later if the cache
                // file has gone stale.
                _extra_includes.push_back(filename);
              }
            }
          }
          okcircular = false;
        }
        getline(in, line);
      }
    }
  }

  // Now recursively expand all our dependent files, so we can check
  // for circularities.
  OCDependableFile *circ = (OCDependableFile *)NULL;

  Dependencies::iterator di;
  for (di = _dependencies.begin(); 
       di != _dependencies.end() && circ == (OCDependableFile *)NULL;
       ++di) {
    // Skip this file if the user specifically marked it
    // with an "okcircular" comment.
    // or the dir is marked as NO_RECURSIVE_DEPENDS
    if (!(*di)._okcircular && 
        ((*di)._file->get_directory()->recursive_depends_ok()) &&
		_directory->external_recursion_ok((*di)._file)) {
      circ = (*di)._file->compute_dependencies(circularity);
      if (circ != (OCDependableFile *)NULL) {
        // we are going to ignore files that include
        // themselves. assume the only time we can end up
        // with this case is when something like pit/Math.h
        // includes math.h from standard libs.
        if (circularity != get_dirpath()) {
          // Oops, a circularity.  Silly user.
          circularity = get_dirpath() + " => " + circularity;
    
          if (circ == this) {
            _flags |= F_circularity;
            _circularity = circularity;
          }
        }
      }
    }
  }

  _flags = (_flags & ~F_updating) | F_updated;
  sort(_dependencies.begin(), _dependencies.end());
  return circ;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDependableFile::stat_file
//       Access: Private
//  Description: Performs a stat() on the file, if it has not already
//               been performed, to check whether the file exists and
//               to get its last-modification time.
////////////////////////////////////////////////////////////////////
void OCDependableFile::
stat_file() {
  if ((_flags & F_statted) != 0) {
    // Already done.
    return;
  }

  _flags |= F_statted;
  struct stat st;
  Filename pathname(get_pathname());
  string ospath = pathname.to_os_specific();
  if (stat(ospath.c_str(), &st) < 0) {
    // The file doesn't exist!
    return;
  }

#ifdef S_ISREG
  if (!S_ISREG(st.st_mode)) {
    // The file exists, but it's not a regular file--we consider that
    // not existing.
    return;
  }
#endif  // S_ISREG

  _flags |= F_exists;
  _mtime = st.st_mtime;
}
