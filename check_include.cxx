// Filename: check_include.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "check_include.h"

////////////////////////////////////////////////////////////////////
//     Function: check_include
//  Description: Checks to see if the given line is a C/C++ #include
//               directive.  If it is, returns the named filename;
//               otherwise, returns the empty string.
////////////////////////////////////////////////////////////////////
string
check_include(const string &line) {
  // Skip initial whitespace on the line or comments that begin with
  // characters that are less than 0 such as the copyright symbol
  size_t p = 0;
  while (p < line.length() && line[p] >= 0 && isspace(line[p])) {
    p++;
  }

  if (p >= line.length() || line[p] != '#') {
    // No hash mark.
    return string();
  }
   
  // We have a hash mark!  Skip more whitespace.
  p++;
  while (p < line.length() && isspace(line[p])) {
    p++;
  }

  // Check for 'include ' (with a space) so that we don't get warnings from
  // things like include_next.
  if (p >= line.length() || !(line.substr(p, 8) == "include ")) {
    // Some other directive, not #include.
    return string();
  }

  // It's an #include directive!  Skip more whitespace.
  p += 7;
  while (p < line.length() && isspace(line[p])) {
    p++;
  }

  // note: oconfig cant expand cpp #define vars used as include targets yet

  if (p >= line.length() || (line[p] != '"' && line[p] != '<')) {
    // if it starts with an underscore and then a capital, or just a capatil
    // assume its a #define var used as include tgt, and dont print a warning
    if(!(((line[p]=='_')&&(line[p+1]>='A')&&(line[p+1]<='Z'))
	 || (line[p]>='A')&&(line[p]<='Z'))) {
      cerr << "Ignoring invalid #include directive: " << line << "\n";
    }
    return string();
  }

  char close = (line[p] == '"') ? '"' : '>';

  p++;
  // Now get the filename.
  size_t q = p;
  while (q < line.length() && line[q] != close) {
    q++;
  }

  if (q >= line.length()) {
    cerr << "Ignoring invalid #include directive: " << line << "\n";
    return string();
  }

  return line.substr(p, q - p);
}
