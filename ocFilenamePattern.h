// Filename: ocFilenamePattern.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef OCFILENAMEPATTERN_H
#define OCFILENAMEPATTERN_H

#include "oconfig.h"

///////////////////////////////////////////////////////////////////
//       Class : OCFilenamePattern
// Description : This is a string that represents a filename, or a
//               family of filenames, using the make convention that a
//               wildcard sign (PATTERN_WILDCARD, typically '%') in
//               the filename represents any sequence of characters.
////////////////////////////////////////////////////////////////////
class OCFilenamePattern {
public:
  OCFilenamePattern(const string &pattern);
  OCFilenamePattern(const OCFilenamePattern &copy);
  void operator = (const OCFilenamePattern &copy);

  bool has_wildcard() const;
  string get_pattern() const;
  const string &get_prefix() const;
  const string &get_suffix() const;

  bool matches(const string &filename) const;
  string extract_body(const string &filename) const;
  string transform(const string &filename,
                   const OCFilenamePattern &transform_from) const;

private:
  bool _has_wildcard;
  string _prefix;
  string _suffix;
};

inline ostream &
operator << (ostream &out, const OCFilenamePattern &pattern) {
  return out << pattern.get_pattern();
}

#endif
