// Filename: ocMain.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "ocMain.h"
#include "ocScope.h"
#include "ocCommandFile.h"
#include "ocDirectory.h"
#include "ocProfileInfo.h"
#include "tokenize.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h> // for perror

#ifdef WIN32_VC
#include <direct.h>  // Windows requires this for getcwd()
#define getcwd _getcwd
#endif  // WIN32_VC

Filename OCMain::_root;

////////////////////////////////////////////////////////////////////
//     Function: OCMain::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCMain::
OCMain(OCScope *global_scope) {
  _global_scope = global_scope;
  OCScope::push_scope(_global_scope);

  _def_scope = (OCScope *)NULL;
  _defs = (OCCommandFile *)NULL;

  // save current working directory name, so that "oconfig ." can map
  // to the current directory.
  Filename dirpath = get_cwd();
  _original_working_dir = dirpath.get_basename();
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::Destructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCMain::
~OCMain() {
  if (_def_scope != (OCScope *)NULL) {
    delete _def_scope;
  }
  if (_defs != (OCCommandFile *)NULL) {
    delete _defs;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::read_source
//       Access: Public
//  Description: Reads the directory hierarchy of Sources.pp files, at
//               the indicated directory and below.
////////////////////////////////////////////////////////////////////
bool OCMain::
read_source(const string &root) {
  // First, find the top of the source tree, as indicated by the
  // presence of a Package.pp file.
  Filename trydir = root;

  Filename package_file(trydir, PACKAGE_FILENAME);
  bool any_source_files_found = false;

  while (!package_file.exists()) {
    // We continue to walk up directories as long as we see a source
    // file in each directory.  When we stop seeing source files, we
    // stop walking upstairs.
    Filename source_file(trydir, SOURCE_FILENAME);
    if (!source_file.exists()) {
      Filename redirect_file = Filename::text_filename(trydir.operator const string &() + "/" + REDIRECT_FILENAME);
      if (redirect_file.exists()) {
        ifstream in;
        redirect_file.open_read(in);
        string line;
        getline(in, line);
        bool foundTop = false;
        while (!in.fail() && !in.eof()) {
          vector<string> parts;
          tokenize(line, parts, " ");
          if (parts[0] == "#define") {
            if (parts[1] == "TOPDIR") {
              trydir = Filename(parts[2]);
              package_file = Filename(trydir, PACKAGE_FILENAME);
              foundTop = true;
              break;
            }
          }
          getline(in, line);
        }
        if (!foundTop) {
          cerr << "Could not find oconfig source file " << SOURCE_FILENAME
               << ", depot file " << PACKAGE_FILENAME
               << ", or redirect file " << REDIRECT_FILENAME
               << ".\n\n"
               << "One of these should be present wherever you launch oconfig.";
          return false;
        }
      } else {
        if (!any_source_files_found) {
          // If we never saw a single Sources.pp file, complain about that.
          cerr << "Could not find oconfig source file " << SOURCE_FILENAME
               << ".\n\n"
               << "This file should be present at each level of the source directory tree;\n"
               << "it defines how each directory should be processed by oconfig.\n\n";
        } else {
          // If we found at least one Sources.pp file, but didn't find
          // the Package.pp file at the top of the tree, complain about
          // *that*.
          cerr << "Could not find oconfig package file " << PACKAGE_FILENAME
               << ".\n\n"
               << "This file should be present in the top of the source directory tree;\n"
               << "it defines implementation-specific variables to control the output\n"
               << "of oconfig, as well as pointing out the installed location of\n"
               << "important oconfig config files.\n\n";
        }
        return false;
      }
    } else {
      any_source_files_found = true;
      trydir = Filename(trydir, "..");
      package_file = Filename(trydir, PACKAGE_FILENAME);
    }
  }

  // Now cd to the source root and get the actual path.
  string osdir;
#ifdef HAVE_CYGWIN
  osdir = trydir;
#else
  osdir = trydir.to_os_specific();
#endif
  if (OCONFIG_CHDIR(osdir.c_str()) < 0) {
    perror("chdir");
    return false;
  }

  _root = get_cwd();
  cerr << "Root is " << _root.to_os_specific() << "\n";

  _def_scope = new OCScope(&_named_scopes);
  _def_scope->define_variable("PACKAGEFILE", package_file);
  _def_scope->define_variable("TOPDIR", _root);
  _def_scope->define_variable("THISSCOPENAME", "");
  _def_scope->define_variable("PARENTSCOPENAME", "");
  _defs = new OCCommandFile(_def_scope);

  if (!_defs->read_file(PACKAGE_FILENAME)) {
    return false;
  }

  // Now check the *_PLATFORM variables that System.pp was supposed to
  // set.
  if (!trim_blanks(_def_scope->expand_string("$[UNIX_PLATFORM]")).empty()) {
    unix_platform = true;
  }
  if (!trim_blanks(_def_scope->expand_string("$[WINDOWS_PLATFORM]")).empty()) {
    windows_platform = true;
  }

  OCScope::push_scope(_def_scope);

  if (!_tree.scan_source(&_named_scopes)) {
    return false;
  }

  _def_scope->define_variable("TREE", _tree.get_complete_tree());

  if (_tree.count_source_files() == 0) {
    cerr << "Could not find any source definition files named " << SOURCE_FILENAME
     << ".\n\n"
     << "A file by this name should be present in each directory of the source\n"
     << "hierarchy; it defines the source files and targets that should be\n"
     << "built in each directory, as well as the relationships between the\n"
     << "directories.\n\n";
    return false;
  }

  cerr << "Read " << _tree.count_source_files() << " " << SOURCE_FILENAME
       << " files.\n";

  if (!read_global_file()) {
    return false;
  }

  if (!_tree.scan_depends(&_named_scopes)) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::process_all
//       Access: Public
//  Description: Does all the processing on all known directories.
//               See process().
////////////////////////////////////////////////////////////////////
bool OCMain::
process_all() {
  string cache_filename = _def_scope->expand_variable("DEPENDENCY_CACHE_FILENAME");

  if (cache_filename.empty()) {
    if (verbose > 3) {
      cerr << "Note: no definition given for $[DEPENDENCY_CACHE_FILENAME].\n";
    }
  } else {
    _tree.read_file_dependencies(cache_filename);
  }

  if (!r_process_all(_tree.get_root())) {
    return false;
  }

  if (!cache_filename.empty()) {
    _tree.update_file_dependencies(cache_filename);
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::process
//       Access: Public
//  Description: Does the processing associated with the source file
//               in the indicated subdirectory name.  This involves
//               reading in the template file and generating whatever
//               output the template file indicates.
////////////////////////////////////////////////////////////////////
bool OCMain::
process(string dirname) {
  string cache_filename = _def_scope->expand_variable("DEPENDENCY_CACHE_FILENAME");
  if (cache_filename.empty()) {
    if (verbose > 3) {
      cerr << "Note: no definition given for $[DEPENDENCY_CACHE_FILENAME].\n";
    }
  } else {
    _tree.read_file_dependencies(cache_filename);
  }

  if (dirname == ".") {
    dirname = _original_working_dir;
  }
  
  OCDirectory *dir = _tree.find_dirname(dirname);
  if (dir == (OCDirectory *)NULL) {
    cerr << "Unknown directory: " << dirname << "\n";
    return false;
  }

  if (dir->get_source() == (OCCommandFile *)NULL) {
    cerr << "No source file in " << dirname << "\n";
    return false;
  }

  if (!p_process(dir)) {
    return false;
  }

  if (!cache_filename.empty()) {
    _tree.update_file_dependencies(cache_filename);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::report_depends
//       Access: Public
//  Description: Reports all the directories that the named directory
//               depends on.
////////////////////////////////////////////////////////////////////
void OCMain::
report_depends(const string &dirname) const {
  OCDirectory *dir = _tree.find_dirname(dirname);
  if (dir == (OCDirectory *)NULL) {
    cerr << "Unknown directory: " << dirname << "\n";
    return;
  }

  dir->report_depends();
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::report_reverse_depends
//       Access: Public
//  Description: Reports all the directories that depend on (need) the
//               named directory.
////////////////////////////////////////////////////////////////////
void OCMain::
report_reverse_depends(const string &dirname) const {
  OCDirectory *dir = _tree.find_dirname(dirname);
  if (dir == (OCDirectory *)NULL) {
    cerr << "Unknown directory: " << dirname << "\n";
    return;
  }

  dir->report_reverse_depends();
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::get_root
//       Access: Public, Static
//  Description: Returns the full path to the root directory of the
//               source hierarchy; this is the directory in which the
//               runs most of the time.
////////////////////////////////////////////////////////////////////
string OCMain::
get_root() {
  return _root.get_fullpath();
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::chdir_root
//       Access: Public, Static
//  Description: Changes the current directory to the root directory
//               of the source hierarchy.  This should be executed
//               after a temporary change to another directory, to
//               restore the current directory to a known state.
////////////////////////////////////////////////////////////////////
void OCMain::
chdir_root() {
  if (OCONFIG_CHDIR(_root.c_str()) < 0) {
    perror("chdir");
    // This is a real error!  We can't get back to our starting
    // directory!
    cerr << "Error!  Source directory is invalid!\n";
    exit(1);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::r_process_all
//       Access: Private
//  Description: The recursive implementation of process_all().
////////////////////////////////////////////////////////////////////
bool OCMain::
r_process_all(OCDirectory *dir) {
  if (dir->get_source() != (OCCommandFile *)NULL) {
    if (!p_process(dir)) {
      return false;
    }
  }

  int num_children = dir->get_num_children();
  for (int i = 0; i < num_children; i++) {
    if (!r_process_all(dir->get_child(i))) {
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::p_process
//       Access: Private
//  Description: The private implementation of process().
////////////////////////////////////////////////////////////////////
bool OCMain::
p_process(OCDirectory *dir) {
  current_output_directory = dir;
  _named_scopes.set_current(dir->get_path());
  OCCommandFile *source = dir->get_source();
  assert(source != (OCCommandFile *)NULL);

  OCScope *scope = source->get_scope();

  string template_filename = scope->expand_variable("TEMPLATE_FILE");
  if (template_filename.empty()) {
    cerr << "No definition given for $[TEMPLATE_FILE], cannot process.\n";
    return false;
  }

  OCCommandFile template_file(scope);
  if (!template_file.read_file(template_filename)) {
    cerr << "Error reading template file " << template_filename << ".\n";
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::read_global_file
//       Access: Private
//  Description: Reads in the Global.pp file after all sources files
//               have been read and sorted into dependency order.
////////////////////////////////////////////////////////////////////
bool OCMain::
read_global_file() {
  assert(_def_scope != (OCScope *)NULL);

  string global_filename = _def_scope->expand_variable("GLOBAL_FILE");
  if (global_filename.empty()) {
    cerr << "No definition given for $[GLOBAL_FILE], cannot process.\n";
    return false;
  }
  
  OCCommandFile global(_def_scope);
  if (!global.read_file(global_filename)) {
    cerr << "Error reading global definition file "
     << global_filename << ".\n";
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCMain::get_cwd
//       Access: Private, Static
//  Description: Calls the system getcwd(), automatically allocating a
//               large enough string.
////////////////////////////////////////////////////////////////////
Filename OCMain::
get_cwd() {
  static size_t bufsize = 1024;
  static char *buffer = NULL;

  if (buffer == (char *)NULL) {
    buffer = new char[bufsize];
  }

  while (getcwd(buffer, static_cast<int>(bufsize)) == (char *)NULL) {
    if (errno != ERANGE) {
      perror("getcwd");
      return string();
    }
    delete[] buffer;
    bufsize = bufsize * 2;
    buffer = new char[bufsize];
    assert(buffer != (char *)NULL);
  }

  return Filename::from_os_specific(buffer);
}
