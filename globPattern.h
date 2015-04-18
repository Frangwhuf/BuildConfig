// Filename: globPattern.h
// Created by:  frang (03June04)
//
////////////////////////////////////////////////////////////////////

#ifndef GLOBPATTERN_H
#define GLOBPATTERN_H

#include "oconfig.h"
#include "filename.h"
#include "vector_string.h"

////////////////////////////////////////////////////////////////////
//       Class : GlobPattern
// Description : This class can be used to test for string matches
//               against standard Unix-shell filename globbing
//               conventions.  It serves as a portable standin for the
//               Posix fnmatch() call.
//
//               A GlobPattern is given a pattern string, which can
//               contain operators like *, ?, and [].  Then it can be
//               tested against any number of candidate strings; for
//               each candidate, it will indicate whether the string
//               matches the pattern or not.  It can be used, for
//               example, to scan a directory for all files matching a
//               particular pattern.
////////////////////////////////////////////////////////////////////
class GlobPattern {
public:
  INLINE GlobPattern(const string &pattern = string());
  INLINE GlobPattern(const GlobPattern &copy);
  INLINE void operator = (const GlobPattern &copy);

  INLINE void set_pattern(const string &pattern);
  INLINE const string &get_pattern() const;

  INLINE bool matches(const string &candidate) const;

  INLINE void output(ostream &out) const;

  bool has_glob_characters() const;
  int match_files(vector_string &results, const Filename &cwd = Filename());

private:
  bool matches_substr(string::const_iterator pi,
                      string::const_iterator pend,
                      string::const_iterator ci,
                      string::const_iterator cend) const;

  bool matches_set(string::const_iterator &pi,
                   string::const_iterator pend,
                   char ch) const;

  int r_match_files(const Filename &prefix, const string &suffix,
                    vector_string &results, const Filename &cwd);

  string _pattern;
};

INLINE ostream &operator << (ostream &out, const GlobPattern &glob) {
  glob.output(out);
  return out;
}


#include "globPattern.I"

#endif
