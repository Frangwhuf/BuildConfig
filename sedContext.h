// Filename: sedContext.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#ifndef SEDCONTEXT_H
#define SEDCONTEXT_H

#include "oconfig.h"

///////////////////////////////////////////////////////////////////
//       Class : SedContext
// Description : This contains the current context of the sed process
//               as it is running: the pattern space, the hold space,
//               and the current line numbers, etc.  It is updated as
//               each line is read in and as each command is executed.
////////////////////////////////////////////////////////////////////
class SedContext {
public:
  SedContext(ostream &out);

  int _line_number;
  bool _is_last_line;
  string _pattern_space;
  string _hold_space;
  bool _deleted;

  ostream &_out;
};

#endif
