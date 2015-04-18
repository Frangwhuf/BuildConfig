// Filename: ocSearchPath.h
// Created by:  frang (03June04)
//
////////////////////////////////////////////////////////////////////

#ifndef SEARCHPATH_H
#define SEARCHPATH_H

#include "oconfig.h"

#include "filename.h"
#include <vector>

#define ocvector std::vector

///////////////////////////////////////////////////////////////////
//       Class : OCSearchPath
// Description : This class stores a list of directories that can be
//               searched, in order, to locate a particular file.  It
//               is normally constructed by passing it a traditional
//               searchpath-style string, e.g. a list of directory
//               names delimited by spaces or colons, but it can also
//               be built up explicitly.
////////////////////////////////////////////////////////////////////
class OCSearchPath {
public:
  class Results {
  public:
    Results();
    Results(const Results &copy);
    void operator = (const Results &copy);
    ~Results();

    void clear();
    int get_num_files() const;
    const Filename &get_file(int n) const;
    void add_file(const Filename &file);

  private:
    typedef ocvector<Filename> Files;
    Files _files;
  };

public:
  OCSearchPath();
  OCSearchPath(const string &path, const string &delimiters = ": \n\t");
  OCSearchPath(const OCSearchPath &copy);
  void operator = (const OCSearchPath &copy);
  ~OCSearchPath();

  void clear();
  void append_directory(const Filename &directory);
  void prepend_directory(const Filename &directory);
  void append_path(const string &path,
                   const string &delimiters = ": \n\t");
  void append_path(const OCSearchPath &path);
  void prepend_path(const OCSearchPath &path);

  bool is_empty() const;
  int get_num_directories() const;
  const Filename &get_directory(int n) const;

  Filename find_file(const Filename &filename) const;
  int find_all_files(const Filename &filename, Results &results) const;

  INLINE static Filename
  search_path(const Filename &filename, const string &path,
              const string &delimiters = ": \n\t");

  void output(ostream &out, const string &separator = ":") const;
  void write(ostream &out, int indent_level = 0) const;

private:
  typedef ocvector<Filename> Directories;
  Directories _directories;
};

INLINE ostream &operator << (ostream &out, const OCSearchPath &sp) {
  sp.output(out);
  return out;
}

#include "ocSearchPath.I"

#endif
