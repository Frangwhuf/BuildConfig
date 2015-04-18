// Filename: ocSearchPath.cxx
// Created by:  frang (03June04)
//
////////////////////////////////////////////////////////////////////

#include "ocSearchPath.h"
#include "filename.h"

#include <algorithm>
#include <iterator>

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCSearchPath::Results::Results() {
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::Copy Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCSearchPath::Results::Results(const OCSearchPath::Results &copy) :
  _files(copy._files)
{
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::Copy Assignment Operator
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void OCSearchPath::Results::operator = (const OCSearchPath::Results &copy) {
  _files = copy._files;
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCSearchPath::Results::~Results() {
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::clear
//       Access: Public
//  Description: Removes all the files from the list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::Results::clear() {
  _files.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::get_num_files
//       Access: Public
//  Description: Returns the number of files on the result list.
////////////////////////////////////////////////////////////////////
int OCSearchPath::Results::get_num_files() const {
  return static_cast<int>(_files.size());
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::get_file
//       Access: Public
//  Description: Returns the nth file on the result list.
////////////////////////////////////////////////////////////////////
const Filename &OCSearchPath::Results::get_file(int n) const {
  assert(n >= 0 && n < (int)_files.size());
  return _files[static_cast<unsigned>(n)];
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Results::add_file
//       Access: Public
//  Description: Adds a new file to the result list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::Results::add_file(const Filename &file) {
  _files.push_back(file);
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Default Constructor
//       Access: Public
//  Description: Creates an empty search path.
////////////////////////////////////////////////////////////////////
OCSearchPath::OCSearchPath() {
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCSearchPath::OCSearchPath(const string &path, const string &delimiters) {
  append_path(path, delimiters);
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Copy Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCSearchPath::OCSearchPath(const OCSearchPath &copy) :
  _directories(copy._directories)
{
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Copy Assignment Operator
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void OCSearchPath::operator = (const OCSearchPath &copy) {
  _directories = copy._directories;
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
OCSearchPath::~OCSearchPath() {
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::clear
//       Access: Public
//  Description: Removes all the directories from the search list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::clear() {
  _directories.clear();
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::append_directory
//       Access: Public
//  Description: Adds a new directory to the end of the search list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::append_directory(const Filename &directory) {
  _directories.push_back(directory);
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::prepend_directory
//       Access: Public
//  Description: Adds a new directory to the front of the search list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::prepend_directory(const Filename &directory) {
  _directories.insert(_directories.begin(), directory);
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::append_path
//       Access: Public
//  Description: Adds all of the directories listed in the search path
//               to the end of the search list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::append_path(const string &path, const string &delimiters) {
  size_t p = 0;
  while (p < path.length()) {
    size_t q = path.find_first_of(delimiters, p);
    if (q == string::npos) {
      _directories.push_back(path.substr(p));
      return;
    }
    if (q != p) {
      _directories.push_back(path.substr(p, q - p));
    }
    p = q + 1;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::append_path
//       Access: Public
//  Description: Adds all of the directories listed in the search path
//               to the end of the search list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::append_path(const OCSearchPath &path) {
  copy(path._directories.begin(), path._directories.end(),
       back_inserter(_directories));
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::prepend_path
//       Access: Public
//  Description: Adds all of the directories listed in the search path
//               to the beginning of the search list.
////////////////////////////////////////////////////////////////////
void OCSearchPath::prepend_path(const OCSearchPath &path) {
  if (!path._directories.empty()) {
    Directories new_directories = path._directories;
    copy(_directories.begin(), _directories.end(),
         back_inserter(new_directories));
    _directories.swap(new_directories);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::is_empty
//       Access: Public
//  Description: Returns true if the search list is empty, false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool OCSearchPath::is_empty() const {
  return _directories.empty();
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::get_num_directories
//       Access: Public
//  Description: Returns the number of directories on the search list.
////////////////////////////////////////////////////////////////////
int OCSearchPath::get_num_directories() const {
  return static_cast<int>(_directories.size());
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::get_directory
//       Access: Public
//  Description: Returns the nth directory on the search list.
////////////////////////////////////////////////////////////////////
const Filename &OCSearchPath::get_directory(int n) const {
  assert(n >= 0 && n < (int)_directories.size());
  return _directories[static_cast<unsigned>(n)];
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::find_file
//       Access: Public
//  Description: Searches all the directories in the search list for
//               the indicated file, in order.  Returns the full
//               matching pathname of the first match if found, or the
//               empty string if not found.
////////////////////////////////////////////////////////////////////
Filename OCSearchPath::find_file(const Filename &filename) const {
  if (filename.is_local()) {
    Directories::const_iterator di;
    for (di = _directories.begin(); di != _directories.end(); ++di) {
      Filename match((*di), filename);
      if (match.exists()) {
        if ((*di) == "." && filename.is_fully_qualified()) {
          // A special case for the "." directory: to avoid prefixing
          // an endless stream of ./ in front of files, if the
          // filename already has a ./ prefixed
          // (i.e. is_fully_qualified() is true), we don't
          // prefix another one.
          return filename;
        } else {
          return match;
        }
      }
    }
  }

  return string();
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::find_all_files
//       Access: Public
//  Description: Searches all the directories in the search list for
//               the indicated file, in order.  Fills up the results
//               list with *all* of the matching filenames found, if
//               any.  Returns the number of matches found.
//
//               It is the responsibility of the the caller to clear
//               the results list first; otherwise, the newly-found
//               files will be appended to the list.
////////////////////////////////////////////////////////////////////
int OCSearchPath::find_all_files(const Filename &filename,
                               OCSearchPath::Results &results) const {
  int num_added = 0;

  if (filename.is_local()) {
    Directories::const_iterator di;
    for (di = _directories.begin(); di != _directories.end(); ++di) {
      Filename match((*di), filename);
      if (match.exists()) {
        if ((*di) == "." && filename.is_fully_qualified()) {
          // A special case for the "." directory: to avoid prefixing
          // an endless stream of ./ in front of files, if the
          // filename already has a ./ prefixed
          // (i.e. is_fully_qualified() is true), we don't
          // prefix another one.
          results.add_file(filename);
        } else {
          results.add_file(match);
        }
        num_added++;
      }
    }
  }

  return num_added;
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::output
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void OCSearchPath::output(ostream &out, const string &separator) const {
  if (!_directories.empty()) {
    Directories::const_iterator di = _directories.begin();
    out << (*di);
    ++di;
    while (di != _directories.end()) {
      out << separator << (*di);
      ++di;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::write
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void OCSearchPath::write(ostream &out, int indent_level) const {
  Directories::const_iterator di;
  for (di = _directories.begin(); di != _directories.end(); ++di) {
    for (int i = 0; i < indent_level; i++) {
      out << ' ';
    }
    out << (*di) << "\n";
  }
}
