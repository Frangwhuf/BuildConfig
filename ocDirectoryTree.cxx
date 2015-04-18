// Filename: ocDirectoryTree.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "ocDirectoryTree.h"
#include "ocDirectory.h"

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCDirectoryTree::
OCDirectoryTree() {
  _root = new OCDirectory(this);
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCDirectoryTree::
~OCDirectoryTree() {
  delete _root;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::scan_source
//       Access: Public
//  Description: Reads in the complete hierarchy of source files,
//               beginning at the current directory.
////////////////////////////////////////////////////////////////////
bool OCDirectoryTree::
scan_source(OCNamedScopes *named_scopes) {
  if (!_root->r_scan("")) {
    return false;
  }

  if (!_root->read_source_file("", named_scopes)) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::scan_depends
//       Access: Public
//  Description: Reads in the depends file and the dependency cache
//               for each source file, and than sorts the files into
//               dependency order.
////////////////////////////////////////////////////////////////////
bool OCDirectoryTree::
scan_depends(OCNamedScopes *named_scopes) {
  if (!_root->read_depends_file(named_scopes)) {
    return false;
  }

  if (!_root->scan_depends_cache(named_scopes)) {
	return false;
  }

  if (!_root->resolve_dependencies()) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::count_source_files
//       Access: Public
//  Description: Returns the number of directories within the tree
//               that actually have a Sources.pp file that was read.
////////////////////////////////////////////////////////////////////
int OCDirectoryTree::
count_source_files() const {
  return _root->count_source_files();
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::get_root
//       Access: Public
//  Description: Returns the root directory of the tree.
////////////////////////////////////////////////////////////////////
OCDirectory *OCDirectoryTree::
get_root() const {
  return _root;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::get_complete_tree
//       Access: Public
//  Description: Returns a single string listing the relative path
//               from the source root to each source directory in the
//               tree, delimited by spaces.
////////////////////////////////////////////////////////////////////
string OCDirectoryTree::
get_complete_tree() const {
  return _root->get_complete_subtree();
}


////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::find_dirname
//       Access: Public
//  Description: Searches for the a source directory with the
//               matching dirname.  This is just the name of the
//               directory itself, not the relative path to the
//               directory.
////////////////////////////////////////////////////////////////////
OCDirectory *OCDirectoryTree::
find_dirname(const string &dirname) const {
  Dirnames::const_iterator di;
  di = _dirnames.find(dirname);
  if (di != _dirnames.end()) {
    return (*di).second;
  }

  // No such dirname; too bad.
  return (OCDirectory *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::find_dependable_file
//       Access: Public
//  Description: Returns a OCDependableFile object corresponding to
//               the named filename, searching all of the known source
//               subdirectories.  This can only find files marked by a
//               previous call to get_dependable_file() with is_header
//               set to true.  Unlike
//               get_dependable_file_by_pathname() or
//               OCDirectory::get_dependable_file(), this does not
//               create an entry if it does not exist; instead, it
//               returns NULL if no matching file can be found.
////////////////////////////////////////////////////////////////////
OCDependableFile *OCDirectoryTree::
find_dependable_file(const string &filename) const {
  Dependables::const_iterator di;
  di = _dependables.find(filename);
  if (di != _dependables.end()) {
    return (*di).second;
  }

  return (OCDependableFile *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::get_dependable_file_by_pathname
//       Access: Public
//  Description: Given a dirname/filename for a particular dependable
//               filename, return (or create and return) the
//               corresponding OCDirectoryTree.  This is different
//               from find_dependable_file() in that an explicit
//               dirname is given, and the entry will be created if
//               it does not already exist.  However, if the directory
//               name does not exist, nothing is created, and NULL is
//               returned.
////////////////////////////////////////////////////////////////////
OCDependableFile *OCDirectoryTree::
get_dependable_file_by_dirpath(const string &dirpath, bool is_header) {
  // Start a the lowest level directory and work our way up to the top
  size_t slash = dirpath.find_last_of('/');
  while (slash != string::npos) {
	string dirname = dirpath.substr(0, slash);
	string filename = dirpath.substr(slash + 1);

	OCDirectory * dir = find_dirname(dirname);
	if (dir != (OCDirectory *)NULL) {
	  return dir->get_dependable_file(filename, is_header);
	}
	slash = dirname.find_last_of('/');
  }
  
  // No valid directory name.
  return (OCDependableFile *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::read_file_dependencies
//       Access: Public
//  Description: Before processing the source files, makes a pass and
//               reads in all of the dependency cache files so we'll
//               have a heads-up on which files depend on the others.
////////////////////////////////////////////////////////////////////
void OCDirectoryTree::
read_file_dependencies(const string &cache_filename) {
  _root->read_file_dependencies(cache_filename);
}


////////////////////////////////////////////////////////////////////
//     Function: OCDirectoryTree::update_file_dependencies
//       Access: Public
//  Description: After all source processing has completed, makes one
//               more pass through the directory hierarchy and writes
//               out the inter-file dependency cache.
////////////////////////////////////////////////////////////////////
void OCDirectoryTree::
update_file_dependencies(const string &cache_filename) {
  _root->update_file_dependencies(cache_filename);
}

