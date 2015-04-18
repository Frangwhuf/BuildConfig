// Filename: ocCommandFile.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "ocCommandFile.h"
#include "ocScope.h"
#include "ocNamedScopes.h"
#include "ocSubroutine.h"
#include "tokenize.h"
#include "ocDirectory.h"
#include "ocProfileInfo.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif

#include <ctype.h>
#include <stdio.h>  // for tempnam()
#include <sys/types.h>
#include <assert.h>

static const string begin_comment(BEGIN_COMMENT);

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::IfNesting::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::IfNesting::
IfNesting(IfState state) :
  _state(state)
{
  _block = (OCCommandFile::BlockNesting *)NULL;
  _next = (OCCommandFile::IfNesting *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::IfNesting::push
//       Access: Public
//  Description: Adds this IfNesting object to the top of the
//               nesting stack.
////////////////////////////////////////////////////////////////////
void OCCommandFile::IfNesting::
push(OCCommandFile *file) {
  _block = file->_block_nesting;
  _next = file->_if_nesting;
  file->_if_nesting = this;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::IfNesting::pop
//       Access: Public
//  Description: Removes this IfNesting object from the top of the
//               nesting stack, and restores the command file's
//               nesting state.
////////////////////////////////////////////////////////////////////
void OCCommandFile::IfNesting::
pop(OCCommandFile *file) {
  assert(file->_if_nesting == this);
  file->_if_nesting = _next;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::WriteState::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::WriteState::
WriteState() {
  _out = NULL;
  _format = WF_collapse;
  _last_blank = true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::WriteState::Copy Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::WriteState::
WriteState(const WriteState &copy) :
  _out(copy._out),
  _format(copy._format),
  _last_blank(copy._last_blank)
{
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::WriteState::write_line
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
bool OCCommandFile::WriteState::
write_line(const string &line) {
  if (_out == (ostream *)NULL || _format == WF_error) {
    if (!line.empty()) {
      cerr << "Ignoring: " << line << "\n";
    }
    return true;

  } else {
    switch (_format) {
    case WF_straight:
      (*_out) << line << "\n";
      return true;
      
    case WF_collapse:
      return write_collapse_line(line);
      
    case WF_makefile:
      return write_makefile_line(line);
	case WF_error:
    default:
      // fall through to error case below
      break;
    }

    cerr << "Unsupported write format: " << (int)_format << "\n";
    return false;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::WriteState::write_collapse_line
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
bool OCCommandFile::WriteState::
write_collapse_line(const string &line) {
  if (line.empty()) {
    if (!_last_blank) {
      (*_out) << "\n";
      _last_blank = true;
    }
    
  } else {
    _last_blank = false;
    (*_out) << line << "\n";
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::WriteState::write_makefile_line
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
bool OCCommandFile::WriteState::
write_makefile_line(const string &line) {
  if (line.length() <= 72) {
    return write_collapse_line(line);
  }
  _last_blank = false;

  // In makefile mode, long variable assignment lines are folded after
  // the assignment.
  vector<string> words;
  tokenize_whitespace(line, words);

  if (words.size() > 2 && (words[1] == "=" || words[1] == ":")) {
    // This appears to be a variable assignment or a dependency rule;
    // fold it.
    (*_out) << words[0] << " " << words[1];
    vector<string>::const_iterator wi;
    int col = 80;
    wi = words.begin() + 2;
    while (wi != words.end()) {
      col += static_cast<int>((*wi).length() + 1);
      if (col > 72) {
        (*_out) << " \\\n   ";
        col = static_cast<int>(4 + (*wi).length());
      }
      (*_out) << " " << (*wi);
      ++wi;
    }
    (*_out) << "\n";

  } else {
    // This is not a variable assignment, so just write it out.
    (*_out) << line << "\n";
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::BlockNesting::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::BlockNesting::
BlockNesting(BlockState state, const string &name) :
  _state(state),
  _name(name)
{
  _if = (OCCommandFile::IfNesting *)NULL;
  _write_state = (OCCommandFile::WriteState *)NULL;
  _scope = (OCScope *)NULL;
  _flags = 0;
  _next = (OCCommandFile::BlockNesting *)NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::BlockNesting::push
//       Access: Public
//  Description: Adds this BlockNesting object to the top of the
//               nesting stack.
////////////////////////////////////////////////////////////////////
void OCCommandFile::BlockNesting::
push(OCCommandFile *file) {
  _if = file->_if_nesting;
  _write_state = file->_write_state;
  _scope = file->_scope;
  _next = file->_block_nesting;
  file->_block_nesting = this;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::BlockNesting::pop
//       Access: Public
//  Description: Removes this BlockNesting object from the top of the
//               nesting stack, and restores the command file's
//               nesting state.
////////////////////////////////////////////////////////////////////
void OCCommandFile::BlockNesting::
pop(OCCommandFile *file) {
  assert(file->_block_nesting == this);

  if (file->_write_state != _write_state) {
    delete file->_write_state;
    file->_write_state = _write_state;
  }
  file->_scope = _scope;
  file->_block_nesting = _next;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::
OCCommandFile(OCScope *scope) {
  _native_scope = scope;
  _scope = scope;
  _got_command = false;
  _in_for = false;
  _if_nesting = (IfNesting *)NULL;
  _block_nesting = (BlockNesting *)NULL;
  _write_state = new WriteState;
  _debugging = false;
  _cur_linenumber = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::
~OCCommandFile() {
  delete _write_state;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::set_output
//       Access: Public
//  Description: Changes the main output stream that will be written
//               to when text appears outside of a #output .. #end
//               block.  This is cout by default.
////////////////////////////////////////////////////////////////////
void OCCommandFile::
set_output(ostream *out) {
  _write_state->_out = out;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::set_scope
//       Access: Public
//  Description: Changes the command file to use the indicated scope.
//               This scope will *not* be deleted when the command
//               file destructs.
////////////////////////////////////////////////////////////////////
void OCCommandFile::
set_scope(OCScope *scope) {
  _native_scope = scope;
  _scope = scope;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::get_scope
//       Access: Public
//  Description: Returns the current scope associated with the command
//               file.  This may change as the command file is
//               processed (e.g. between #begin .. #end sequences),
//               and it may or may not be tied to the life of the
//               OCCommandFile itself.
////////////////////////////////////////////////////////////////////
OCScope *OCCommandFile::
get_scope() const {
  return _scope;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::read_file
//       Access: Public
//  Description: Reads input from the given filename.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
read_file(Filename filename) {
  filename.set_text();
  ifstream in;

  if (!filename.open_read(in)) {
    cerr << "Unable to open " << filename.to_os_specific() << ".\n";
    return false;
  }
  if (verbose) {
    cerr << "Reading (cmd) \"" << filename.to_os_specific() << "\"\n";
  }

  return read_stream(in, filename);
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::read_stream
//       Access: Public
//  Description: Reads input from the given stream.  Each line is
//               read, commands are processed, variables are expanded,
//               and the resulting output is sent to write_line()
//               one line at a time.  The return value is true if the
//               entire file is read with no errors, false if there is
//               some problem.
//
//               The filename is just informational; it is used to
//               update the variables like THISFILENAME and
//               THISDIRPREFIX as appropriate, and to report errors to
//               the user.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
read_stream(istream &in, const string &filename) {
  PushFilename pushed(_scope, filename, _cur_linenumber);

  if (!read_stream(in)) {
    if (!in.eof()) {
      cerr << "Error reading " << filename << ".\n";
    }
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::read_stream
//       Access: Public
//  Description: Reads input from the given stream.  Each line is
//               read, commands are processed, variables are expanded,
//               and the resulting output is sent to write_line()
//               one line at a time.  The return value is true if the
//               entire file is read with no errors, false if there is
//               some problem.
//
//               This flavor of read_stream() does not take a
//               filename.  It does not, therefore, adjust
//               THISFILENAME and THISDIRPREFIX.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
read_stream(istream &in) {
  string line;
  getline(in, line);
  _cur_linenumber = 1;
  begin_read();
  while ((!in.fail() && !in.eof()) || (!line.empty())) {
    if (!read_line(line)) {
      return false;
    }
    line.clear();
    getline(in, line);
	_cur_linenumber += 1;
  }

  if (!end_read()) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::begin_read
//       Access: Public
//  Description: Resets to the beginning-of-the-stream state, in
//               preparation for a sequence of read_line() calls.
////////////////////////////////////////////////////////////////////
void OCCommandFile::
begin_read() {
  assert(_if_nesting == (IfNesting *)NULL);
  assert(_block_nesting == (BlockNesting *)NULL);
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::read_line
//       Access: Public
//  Description: Reads one line at a time, as if from the input
//               stream.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
read_line(string line) {
  // First things first: strip off any comment in the line.

  // We only recognize comments that are proceeded by whitespace, or
  // that start at the beginning of the line.
  size_t comment = line.find(begin_comment);
  while (comment != string::npos && 
         !(comment == 0 || isspace(line[comment - 1]))) {
    comment = line.find(begin_comment, comment + begin_comment.length());
  }

  if (comment != string::npos) {
    // Also strip any whitespace leading up to the comment.
    while (comment > 0 && isspace(line[comment - 1])) {
      comment--;
    }
    line = line.substr(0, comment);
  }

  // If the comment was at the beginning of the line, ignore the whole
  // line, including its whitespace.
  if (comment != 0) {
    // We also strip off whitespace at the end of the line, since this
    // is generally invisible and almost always just leads to trouble.
    size_t eol = line.length();
    while (eol > 0 && (isspace(line[eol - 1]) || line[eol - 1] == '\r')) {
      eol--;
    }
    line = line.substr(0, eol);

    if (_in_for) {
      // Save up the lines for later execution if we're within a #forscopes.
      _saved_lines.push_back(line);
    }
    
    if (_got_command) {
      return handle_command(line);
      
    } else {
      // Find the beginning of the line--skip initial whitespace.
      size_t p = 0;
      while (p < line.length() && isspace(line[p])) {
        p++;
      }
      
      if (p == line.length()) {
        // The line is empty.  Make it truly empty.
        line = "";
      } else {
        if (((p+1) < line.length()) && (line[p] == COMMAND_PREFIX) && 
            isalpha(line[p + 1])) {
          // This is a special command.
          return handle_command(line.substr(p + 1));
        }
      }

      if (!_in_for && !failed_if()) {
        if(!line.empty() && line[p+1]==COMMAND_PREFIX) {
          // double prefix at start of line indicates echo single prefix, like '\\' in C
          line.erase(0,1);
        }
        return _write_state->write_line(_scope->expand_string(line));
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::end_read
//       Access: Public
//  Description: Finishes up the input stream, after a sequence of
//               read_line() calls.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
end_read() {
  bool okflag = true;

  if (_if_nesting != (IfNesting *)NULL) {
    cerr << "Unclosed if\n";
    _if_nesting = (IfNesting *)NULL;
    okflag = false;
  }

  if (_block_nesting != (BlockNesting *)NULL) {
    switch (_block_nesting->_state) {
    case BS_begin:
      cerr << "Unclosed begin " << _block_nesting->_name << "\n";
      break;

    case BS_while:
    case BS_nested_while:
      cerr << "Unclosed while " << _block_nesting->_name << "\n";
      break;

    case BS_forscopes:
    case BS_nested_forscopes:
      cerr << "Unclosed forscopes " << _block_nesting->_name << "\n";
      break;

    case BS_foreach:
    case BS_nested_foreach:
      cerr << "Unclosed foreach " << _block_nesting->_name << "\n";
      break;

    case BS_formap:
    case BS_nested_formap:
      cerr << "Unclosed formap " << _block_nesting->_name << "\n";
      break;

    case BS_defsub:
      cerr << "Unclosed defsub " << _block_nesting->_name << "\n";
      break;

    case BS_defun:
      cerr << "Unclosed defun " << _block_nesting->_name << "\n";
      break;

    case BS_output:
      cerr << "Unclosed output " << _block_nesting->_name << "\n";
      break;
	case BS_nested_for:
	case BS_for:
    default:
      break;
    }
    _block_nesting = (BlockNesting *)NULL;
    okflag = false;
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_command
//       Access: Protected
//  Description: wraps handle_command_direct, takes care of anything
//               that debugging might care about
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_command(const string &line) {
	if (_debugging) {
		if (_got_command) {
			cout << "- ";   // - means 'reading from file'
		} else {
			cout << "- #";  // put the command prefix back on
		}
		cout << line << "\n";
		bool ret;
		ret = handle_command_direct(line);
		if (!_got_command) {
			handle_break_command(ret);
		}
		return ret;
	} else {
		return (handle_command_direct(line));
	}
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_command_direct
//       Access: Protected
//  Description: process a macro command without checking for debugging
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_command_direct(const string &line) {
  if (_got_command) {
    // If we were still processing a command from last time, keep
    // going; this line is just a continuation.  But skip any initial
    // whitespace.
    size_t p = 0;
    while (p < line.length() && isspace(line[p])) {
      p++;
    }
    _params += ' ';
    _params += line.substr(p);

  } else {
    // This is the first line of a new command.

    // Pull off the first word and the rest of the line.
    size_t p = 0;
    while (p < line.length() && !isspace(line[p])) {
      p++;
    }
    _command = line.substr(0, p);
  
    // Skip whitespace between the command and its arguments.
    while (p < line.length() && isspace(line[p])) {
      p++;
    }
    _params = line.substr(p);
  }

  if (!_params.empty() && _params[_params.length() - 1] == '\\') {
    // If the line ends with a backslash, there's more to come before
    // we can process the command.
    _got_command = true;

    // Truncate off the backslash, and any whitespace before it.
    size_t p = _params.length() - 1;
    while (p > 0 && isspace(_params[p - 1])) {
      p--;
    }
    _params = _params.substr(0, p);
    return true;
  }

  // We're completely done scanning the command now.
  _got_command = false;
  bool ret;
  // create a copy of command

  if (_command == "if") {
    command_profile->openTimeslice(_command, line);
    ret = handle_if_command();
    return ret;

  } else if (_command == "elif") {
    command_profile->closeTimeslice();
    command_profile->openTimeslice(_command, line);
    ret = handle_elif_command();
    return ret;
  
  } else if (_command == "else") {
    command_profile->closeTimeslice();
    command_profile->openTimeslice(_command, line);
    ret = handle_else_command();
    return ret;

  } else if (_command == "endif") {
    command_profile->closeTimeslice();
    ret = handle_endif_command();
    return ret;

  } else if (failed_if()) {
    // If we're in the middle of a failed #if, we ignore all commands
    // except for the if-related commands, above.
    return true;

  } else if (_command == "begin") {
    command_profile->openTimeslice(_command, line);
    ret = handle_begin_command();
    return ret;

  } else if (_command == "while") {
    command_profile->openTimeslice(_command, line);
    ret = handle_while_command();
    return ret;

  } else if (_command == "for") {
    command_profile->openTimeslice(_command, line);
    ret = handle_for_command();
    return ret;

  } else if (_command == "forscopes") {
    command_profile->openTimeslice(_command, line);
    ret = handle_forscopes_command();
    return ret;

  } else if (_command == "foreach") {
    command_profile->openTimeslice(_command, line);
    ret = handle_foreach_command();
    return ret;

  } else if (_command == "formap") {
    command_profile->openTimeslice(_command, line);
    ret = handle_formap_command();
    return ret;

  } else if (_command == "defsub") {
    command_profile->openTimeslice(_command, line);
    ret = handle_defsub_command(true);
    return ret;

  } else if (_command == "defun") {
    command_profile->openTimeslice(_command, line);
    ret = handle_defsub_command(false);
    return ret;

  } else if (_command == "output") {
    command_profile->openTimeslice(_command, line);
    ret = handle_output_command();
    return ret;

  } else if (_command == "end") {
    ret = handle_end_command();
    command_profile->closeTimeslice();

    return ret;

  } else if (_command == "dprint") {
	// special "debug print" so that we can output in the middle of
	// of blocks, etc.
	return handle_print_command();
  } else if (_in_for) {
    // If we're currently saving up lines within a block sequence, we
    // ignore all commands except for the block-related commands,
    // above.
    return true;

  } else if (_command == "format") {
    command_profile->openTimeslice(_command, line);
    ret = handle_format_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "print") {
    command_profile->openTimeslice(_command, line);
    ret = handle_print_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "printvar") {
    command_profile->openTimeslice(_command, line);
    ret = handle_printvar_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "include") {
    command_profile->openTimeslice(_command, line);
    ret = handle_include_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "sinclude") {
    command_profile->openTimeslice(_command, line);
    ret = handle_sinclude_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "call") {
    command_profile->openTimeslice(_command, line);
    ret = handle_call_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "error") {
    command_profile->openTimeslice(_command, line);
    ret = handle_error_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "mkdir") {
    command_profile->openTimeslice(_command, line);
    ret = handle_mkdir_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "defer") {
    command_profile->openTimeslice(_command, line);
    ret = handle_defer_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "define") {
    command_profile->openTimeslice(_command, line);
    ret = handle_define_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "set") {
    command_profile->openTimeslice(_command, line);
    ret = handle_set_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "map") {
    command_profile->openTimeslice(_command, line);
    ret = handle_map_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "addmap") {
    command_profile->openTimeslice(_command, line);
    ret = handle_addmap_command();
    command_profile->closeTimeslice();
    return ret;

  } else if (_command == "push") {
    command_profile->openTimeslice(_command, line);
    ret = handle_push_command();
    command_profile->closeTimeslice();
    return ret;
  } else if (_command == "debugscopes") {
    command_profile->openTimeslice(_command, line);
    ret = handle_debugscopes_command();
    command_profile->closeTimeslice();
    return ret;
  } else if (_command == "depsave") {
    command_profile->openTimeslice(_command, line);
    ret = handle_depsave_command();
    command_profile->closeTimeslice();
    return ret;
  } else if (_command == "depload") {
    command_profile->openTimeslice(_command, line);
    ret = handle_depload_command();
    command_profile->closeTimeslice();
    return ret;
  } else if (_command == "getincludes") {
	command_profile->openTimeslice(_command, line);
	ret = handle_getincludes_command();
	command_profile->closeTimeslice();
	return ret;
  } else if (_command == "break") {
	// begin a debugging session on stdin, always true
	cout << "Breakpoint encountered\n";
	ret = handle_break_command(true);
	return ret;
  }

  cerr << "Invalid command: " << COMMAND_PREFIX << _command << "\n";
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_if_command
//       Access: Protected
//  Description: Handles the #if command: conditionally evaluate the
//               following code.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_if_command() {
  if (failed_if()) {
    // If we're *already* inside a failed if, we don't have to
    // evaluate this one, but we do need to record the nesting level.

    IfNesting *nest = new IfNesting(IS_done);
    nest->push(this);

  } else {

    // If the parameter string evaluates to empty, the case is false.
    // Otherwise the case is true.  However, if we're currently
    // scanning #forscopes or something, we don't evaluate this at
    // all, because it doesn't matter.

    bool is_empty = true;
    if (!_in_for) {
      _params = _scope->expand_string(_params);
      string::const_iterator si;
      for (si = _params.begin(); si != _params.end() && is_empty; ++si) {
        is_empty = isspace(*si) != 0;
      }
    }
    
    IfState state = is_empty ? IS_off : IS_on;
    IfNesting *nest = new IfNesting(state);
    nest->push(this);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_elif_command
//       Access: Protected
//  Description: Handles the #elif command: conditionally evaluate
//               the following code, following a failed #if command.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_elif_command() {
  if (_if_nesting == (IfNesting *)NULL) {
    cerr << "elif encountered without if.\n";
    return false;
  }
  if (_if_nesting->_state == IS_else) {
    cerr << "elif encountered after else.\n";
    return false;
  }
  if (_if_nesting->_state == IS_on || _if_nesting->_state == IS_done) {
    // If we passed the #if above, we don't need to evaluate the #elif.
    _if_nesting->_state = IS_done;
    return true;
  }

  // If the parameter string evaluates to empty, the case is false.
  // Otherwise the case is true.
  bool is_empty = true;
  if (!_in_for) {
    _params = _scope->expand_string(_params);
    string::const_iterator si;
    for (si = _params.begin(); si != _params.end() && is_empty; ++si) {
      is_empty = isspace(*si) != 0;
    }
  }

  _if_nesting->_state = is_empty ? IS_off : IS_on;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_else_command
//       Access: Protected
//  Description: Handles the #else command: evaluate the following
//               code following a failed #if command.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_else_command() {
  if (_if_nesting == (IfNesting *)NULL) {
    cerr << "else encountered without if.\n";
    return false;
  }
  if (_if_nesting->_state == IS_else) {
    cerr << "else encountered after else.\n";
    return false;
  }
  if (_if_nesting->_state == IS_on || _if_nesting->_state == IS_done) {
    _if_nesting->_state = IS_done;
    return true;
  }

  _if_nesting->_state = IS_else;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_endif_command
//       Access: Protected
//  Description: Handles the #endif command: close a preceeding #if
//               command.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_endif_command() {
  if (_if_nesting == (IfNesting *)NULL) {
    cerr << "endif encountered without if.\n";
    return false;
  }

  IfNesting *nest = _if_nesting;
  nest->pop(this);

  if (nest->_block != _block_nesting) {
    if (nest->_block != (BlockNesting *)NULL) {
      cerr << "If block not closed within scoping block " << nest->_block->_name << ".\n";
    } else {
      cerr << "If block not closed within scoping block " << _block_nesting->_name << ".\n";
    }
    return false;
  }

  delete nest;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_begin_command
//       Access: Protected
//  Description: Handles the #begin command: begin a named scope
//               block.  The variables defined between this command
//               and the corresponding #end command will be local to
//               this named scope.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_begin_command() {
  string name = trim_blanks(_params);
  BlockNesting *nest = new BlockNesting(BS_begin, name);

  if (contains_whitespace(name)) {
    cerr << "Attempt to define scope named \"" << name 
         << "\".\nScope names may not contain whitespace.\n";
    return false;
  }

  if (name.find(SCOPE_DIRNAME_SEPARATOR) != string::npos) {
    cerr << "Attempt to define scope named \"" << name 
         << "\".\nScope names may not contain the '"
         << SCOPE_DIRNAME_SEPARATOR << "' character.\n";
    return false;
  }

  nest->push(this);

  OCScope *named_scope = _scope->get_named_scopes()->make_scope(name);
  named_scope->set_parent(_scope);
  _scope = named_scope;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_while_command
//       Access: Protected
//  Description: Handles the #while command: repeat a block of
//               commands while a condition is true (nonempty).
//               Unlike many of the other block commands, this does
//               not begin a new scope.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_while_command() {
  BlockState state = _in_for ? BS_nested_while : BS_while;
  string name = trim_blanks(_params);
  BlockNesting *nest = new BlockNesting(state, name);
  nest->push(this);

  if (!_in_for) {
    _in_for = true;
    _saved_lines.clear();
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_for_command
//       Access: Protected
//  Description: Handles the #for command: repeat a block of
//               commands with a loop variable iterating over a range
//               of numeric values.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_for_command() {
  // We get the name of the variable out first.
  size_t p = _scope->scan_to_whitespace(_params);
  string name = _params.substr(0, p);

  if (name.empty()) {
    cerr << "#for without varname\n";
    return false;
  }

  // The rest is the comma-delimited range of values.
  vector<string> words;
  _scope->tokenize_params(_params.substr(p), words, true);
  if (words.size() != 2 && words.size() != 3) {
    cerr << "Invalid numeric range: '" << _params.substr(p) 
         << "' for #for " << name << "\n";
    return false;
  }

  BlockState state = _in_for ? BS_nested_for : BS_for;
  BlockNesting *nest = new BlockNesting(state, name);
  nest->push(this);

  if (!_in_for) {
    _in_for = true;
    _saved_lines.clear();

    nest->_words.swap(words);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_forscopes_command
//       Access: Protected
//  Description: Handles the #forscopes command: interpret all the lines
//               between this command and the corresponding #end
//               command once for each occurrence of a named scope
//               with the given name.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_forscopes_command() {
  BlockState state = _in_for ? BS_nested_forscopes : BS_forscopes;
  string name = trim_blanks(_params);
  BlockNesting *nest = new BlockNesting(state, name);
  nest->push(this);

  if (!_in_for) {
    _in_for = true;
    _saved_lines.clear();
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_foreach_command
//       Access: Protected
//  Description: Handles the #foreach command: interpret all the lines
//               between this command and the corresponding #end
//               command once for each word in the argument.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_foreach_command() {
  // Get the parameters of the foreach command.  If we are in a foreach
  // then don't expand, this could lead to functions being called
  // unexpectedly.
  vector<string> words;
  if (_in_for) {
	  tokenize_whitespace(_params, words);
  } else {
	  tokenize_whitespace(_scope->expand_string(_params), words);
  }

  // The first word is the name of the variable to substitute in (and
  // which should appear on the matching #end command), and the remaining
  // words are the values to substitute in.

  if (words.empty()) {
    cerr << "#foreach requires at least one parameter.\n";
    return false;
  }

  string variable_name = words.front();

  BlockState state = _in_for ? BS_nested_foreach : BS_foreach;
  BlockNesting *nest = new BlockNesting(state, variable_name);
  nest->push(this);

  // We insert in all but the first word in the words vector.
  nest->_words.insert(nest->_words.end(), words.begin() + 1, words.end());

  if (!_in_for) {
    _in_for = true;
    _saved_lines.clear();
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_formap_command
//       Access: Protected
//  Description: Handles the #formap command: interpret all the lines
//               between this command and the corresponding #end
//               command once for each key in the map, and also within
//               the corresponding scope of that particular key.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_formap_command() {
  // Get the parameters of the formap command.  The first word is the
  // name of the variable to substitute in (and which should appear on
  // the matching #end command), and the second word is the name of
  // the map variable.
  vector<string> words;
  tokenize_whitespace(_scope->expand_string(_params), words);

  if (words.size() != 2) {
    cerr << "#formap requires exactly two parameters.\n";
    return false;
  }

  string variable_name = words.front();

  BlockState state = _in_for ? BS_nested_formap : BS_formap;
  BlockNesting *nest = new BlockNesting(state, words[0]);
  nest->push(this);

  nest->_words.push_back(words[1]);

  if (!_in_for) {
    _in_for = true;
    _saved_lines.clear();
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_defsub_command
//       Access: Protected
//  Description: Handles the #defsub (or #defun) command: save all the
//               lines between this command and the matching #end as a
//               callable subroutine to be invoked by a later #call
//               command.  If is_defsub is false, it means this
//               subroutine was actually defined via a #defun command,
//               so it is to be invoked by a later variable reference,
//               instead of by a #call command.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_defsub_command(bool is_defsub) {
  string command = (is_defsub) ? "#defsub" : "#defun";

  // The first word of the parameter list is the subroutine name; the
  // rest is the comma-separated list of formal parameter names.

  // Pull off the first word and the rest of the params.
  size_t p = 0;
  while (p < _params.length() && !isspace(_params[p])) {
    p++;
  }
  string subroutine_name = trim_blanks(_params.substr(0, p));

  if (subroutine_name.empty()) {
    cerr << command << " requires at least one parameter.\n";
    return false;
  }

  vector<string> formals;
  _scope->tokenize_params(_params.substr(p), formals, false);

  vector<string>::const_iterator fi;
  for (fi = formals.begin(); fi != formals.end(); ++fi) {
    if (!is_valid_formal(*fi)) {
      cerr << command << " " << subroutine_name
           << ": invalid formal parameter name '" << (*fi) << "'\n";
      return false;
    }
  }

  if (_in_for) {
    cerr << command << " may not appear within another block scoping command like\n"
         << "#forscopes, #foreach, #formap, #defsub, or #defun.\n";
    return false;
  }

  BlockState state = is_defsub ? BS_defsub : BS_defun;
  BlockNesting *nest = new BlockNesting(state, subroutine_name);

  nest->push(this);
  nest->_words.swap(formals);

  _in_for = true;
  _saved_lines.clear();

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_output_command
//       Access: Protected
//  Description: Handles the #output command: all text between this
//               command and the corresponding #end command will be
//               sent to the indicated output file.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_output_command() {
  // We must get the filename out (the first parameter) without
  // expanding it, because it might involve a string that has to be
  // expanded later.
  size_t p = _scope->scan_to_whitespace(_params);
  string name = _params.substr(0, p);

  if (name.empty()) {
    cerr << "#output command requires one parameter.\n";
    return false;
  }

  // Now get the remaining parameters out; these we will expand immediately.
  vector<string> words;
  tokenize_whitespace(_scope->expand_string(_params.substr(p)), words);

  BlockNesting *nest = new BlockNesting(BS_output, name);

  // Also check the output flags.
  for (unsigned i = 0; i < words.size(); i++) {
    if (words[i] == "notouch") {
      nest->_flags |= OF_notouch;
    } else if (words[i] == "notest") {
      nest->_flags |= OF_notest;
    } else {
      cerr << "Invalid output flag: " << words[i] << "\n";
    }
  }

  nest->push(this);

  if (!_in_for) {
    Filename filename = trim_blanks(_scope->expand_string(nest->_name));
    if (filename.empty()) {
      cerr << "Attempt to output to empty filename\n";
      return false;
    }
    
    if (filename.is_local()) {
      string prefix = _scope->expand_variable("DIRPREFIX");
      filename = Filename(prefix, filename);
    }

    nest->_params = filename;

    // Generate an in-memory copy of the file first.
    _write_state = new WriteState(*_write_state);
    _write_state->_out = &nest->_output;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_end_command
//       Access: Protected
//  Description: Handles the #end command.  This closes a number of
//               different kinds of blocks, like #begin and #forscopes.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_end_command() {
  if (_block_nesting == (BlockNesting *)NULL) {
    cerr << "Unmatched end " << _params << ".\n";
    return false;
  }

  // We don't expand the closing name string, because we didn't expand
  // the opening string.  (This is especially true for an #output
  // statement.  On some of the other statements, we might have
  // inadvertently expanded this string, but probably that was a
  // mistake; and there's no reason for programmers to take advantage
  // of an expansion there.)
  string name = trim_blanks(_params);
  if (name != _block_nesting->_name) {
    cerr << "end " << name << " encountered where end "
         << _block_nesting->_name << " expected.\n";
    return false;
  }

  BlockNesting *nest = _block_nesting;
  nest->pop(this);

  if (nest->_if != _if_nesting) {
    cerr << "If block not closed within scoping block " << name << ".\n";
    return false;
  }

  if (nest->_state == BS_while) {
    // Now replay all of the saved lines.
    _in_for = false;
    if (!replay_while(nest->_name)) {
      return false;
    }

  } else if (nest->_state == BS_for) {
    // Now replay all of the saved lines.
    _in_for = false;
    if (!replay_for(nest->_name, nest->_words)) {
      return false;
    }

  } else if (nest->_state == BS_forscopes) {
    // Now replay all of the saved lines.
    _in_for = false;
    if (!replay_forscopes(nest->_name)) {
      return false;
    }

  } else if (nest->_state == BS_foreach) {
    // Now replay all of the saved lines.
    _in_for = false;
    if (!replay_foreach(nest->_name, nest->_words)) {
      return false;
    }

  } else if (nest->_state == BS_formap) {
    // Now replay all of the saved lines.
    _in_for = false;
    assert(nest->_words.size() == 1);
    if (!replay_formap(nest->_name, nest->_words[0])) {
      return false;
    }

  } else if (nest->_state == BS_defsub || nest->_state == BS_defun) {
    // Save all of the saved lines as a named subroutine.
    _in_for = false;
    OCSubroutine *sub = new OCSubroutine;
    sub->_formals.swap(nest->_words);
    sub->_lines.swap(_saved_lines);

    // Remove the #end command.  This will fail if someone makes an
    // #end command that spans multiple lines.  Don't do that.
    assert(!sub->_lines.empty());
    sub->_lines.pop_back();

    if (nest->_state == BS_defsub) {
      OCSubroutine::define_sub(nest->_name, sub);
    } else {
      OCSubroutine::define_func(nest->_name, sub);
    }

  } else if (nest->_state == BS_output) {
    if (!_in_for) {
      if (!nest->_output) {
        cerr << "Error while writing " << nest->_params << "\n";
        return false;
      }

      // Now compare the file we generated to the file that's already
      // there, if there is one.

#ifdef HAVE_SSTREAM
      string generated_file = nest->_output.str();
#else
      nest->_output << ends;
      char *c_str = nest->_output.str();
      string generated_file = c_str;
      delete[] c_str;
#endif  // HAVE_SSTREAM

      if (!compare_output(generated_file, nest->_params,
                          (nest->_flags & OF_notouch) != 0,
                          (nest->_flags & OF_notest) != 0)) {
        return false;
      }
    }
  }


  delete nest;

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_format_command
//       Access: Protected
//  Description: Handles the #format command: change the formatting
//               mode of lines as they are output.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_format_command() {
  _params = trim_blanks(_scope->expand_string(_params));
  if (_params == "straight") {
    _write_state->_format = WF_straight;

  } else if (_params == "collapse") {
    _write_state->_format = WF_collapse;

  } else if (_params == "makefile") {
    _write_state->_format = WF_makefile;

  } else {
    cerr << "Ignoring invalid write format: " << _params << "\n";
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_print_command
//       Access: Protected
//  Description: Handles the #print command: immediately output the
//               arguments to this line to standard error.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_print_command() {
  cerr << _scope->expand_string(_params) << "\n";
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_printvar_command
//       Access: Protected
//  Description: Writes the literal contents of the named variable(s)
//               (the variables are named directly without enclosing
//               $[ ... ] syntax) to cerr, for debugging.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_printvar_command() {
  size_t p = 0;

  while (p < _params.length()) {
    // Pull off the next varname.
    size_t q = _scope->scan_to_whitespace(_params, p);
    string varname = trim_blanks(_scope->expand_string(_params.substr(p, q - p)));

    cerr << varname << " = \"" << _scope->get_variable(varname)
         << "\" ";
    p = q;
    while (p < _params.length() && isspace(_params[p])) {
      p++;
    }
  }
  cerr << "\n";
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_include_command
//       Access: Protected
//  Description: Handles the #include command: the indicated file is
//               read and processed at this point.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_include_command() {
  string filename = trim_blanks(_scope->expand_string(_params));

  // We allow optional quotation marks around the filename.
  if (filename.length() >= 2 &&
      filename[0] == '"' && 
      filename[filename.length() - 1] == '"') {
    filename = filename.substr(1, filename.length() - 2);
  }

  return include_file(filename);
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_sinclude_command
//       Access: Protected
//  Description: Handles the #sinclude command: the indicated file is
//               read and processed at this point.  This is different
//               from #include only in that if the file does not
//               exist, there is no error; instead, nothing happens.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_sinclude_command() {
  string filename = trim_blanks(_scope->expand_string(_params));

  // We allow optional quotation marks around the filename.
  if (filename.length() >= 2 &&
      filename[0] == '"' && 
      filename[filename.length() - 1] == '"') {
    filename = filename.substr(1, filename.length() - 2);
  }

  Filename fn(filename);

  if (!fn.exists()) {
    // No such file; no error.
    return true;
  }

  return include_file(filename);
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_call_command
//       Access: Protected
//  Description: Handles the #call command: the indicated named
//               subroutine is read and processed at this point.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_call_command() {
  // The first word is the name of the subroutine; the rest is the
  // comma-separated list of expressions to substitute in for the
  // subroutine's formal parameters.

  // Pull off the first word and the rest of the params.
  size_t p = 0;
  while (p < _params.length() && !isspace(_params[p])) {
    p++;
  }
  string subroutine_name = trim_blanks(_params.substr(0, p));
  string params = _params.substr(p);

  if (subroutine_name.empty()) {
    cerr << "#call requires at least one parameter.\n";
    return false;
  }

  const OCSubroutine *sub = OCSubroutine::get_sub(subroutine_name);
  if (sub == (const OCSubroutine *)NULL) {
    cerr << "Attempt to call undefined subroutine " << subroutine_name << "\n";
  }

  OCScope *old_scope = _scope;
  // We used to push _scope on the scope stack.  But this caused trouble evaluating
  // certain map variables.  If we need to do this again, we'll need to work that
  // out.  If you add that back in, be certain to pop the stack before either of
  // the returns below.
  OCScope nested_scope(_scope->get_named_scopes());
  nested_scope.set_parent(_scope);
  _scope = &nested_scope;
  nested_scope.define_formals(subroutine_name, sub->_formals, params);

  vector<string>::const_iterator li;
  for (li = sub->_lines.begin(); li != sub->_lines.end(); ++li) {
    if (!read_line(*li)) {
      _scope = old_scope;
      return false;
    }
  }

  _scope = old_scope;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_error_command
//       Access: Protected
//  Description: Handles the #error command: terminate immediately
//               with the given error message.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_error_command() {
  string message = trim_blanks(_scope->expand_string(_params));
  
  if (!message.empty()) {
    cerr << message << "\n";
  }
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_break_command
//       Access: Protected
//  Description: Handles a 'break' in an oconfig file which allows
//               the user to input commands by hand to view current
//               oconfig state.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_break_command(bool last_cmd_ok) {
  string line;
  string trimmed_line;
  bool ok = true;
  _debugging = false;
  int count = 0;

  while (1) {
    ++count;
	cout << "oconfig"
		 << " (" << _scope->get_variable("THISFILENAME") << ":"
		 << _cur_linenumber << " " << count << ")";
	if (ok) {
		cout << ">";
	} else {
		cout << "!";
	}
	getline(cin, line);

	// if EOF, assume they meant continue instead
	if (cin.eof()) {
	  cout << "cont\n";
	  read_line(line);
	  cin.clear();
	  _debugging = false;
	  return true;
	}
	trimmed_line = trim_blanks(line);

	if (trimmed_line == "bt") {
	  string fname;
	  fname.append(_scope->get_variable("THISFILENAME"));
	  cout << command_profile->getCallstackBacktrace(fname, _cur_linenumber);
	  line.clear();
	} else if (trimmed_line == "cont") {
	  read_line("");
	  // continue execution
	  _debugging = false;
	  return true;
	} else if (trimmed_line == "n") {
	  // read one more 'command' of input, then auto-break back to here
	  _debugging = true;
	  if (last_cmd_ok) {
	    return true;
	  } else {
		cout << "Cannot continue ... last command was in error\n";
	  }
	} else if (trimmed_line == "quit") {
	  cout << "Terminating oconfig\n";
	  exit(0);
	} else if ((trimmed_line == "?") || (trimmed_line == "h") || (trimmed_line == "help")) {
	  cout << "oconfig debug help\n"
		   << "------------------\n"
		   << "bt   - back trace (stack)\n"
		   << "cont - continue execution\n"
		   << "n    - next line / step forward\n"
		   << "quit - terminate oconfig\n"
		   << "?    - this help\n";
	  line.clear();
	}
	_debugging = false; // trun off the 'next' handling if we did that
	ok = read_line(line);
	line.clear();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_mkdir_command
//       Access: Protected
//  Description: Handles the #mkdir command: create a directory or
//               directories.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_mkdir_command() {
  vector<string> words;
  tokenize_whitespace(_scope->expand_string(_params), words);

  vector<string>::const_iterator wi;
  for (wi = words.begin(); wi != words.end(); ++wi) {
    Filename dirname(*wi);
    
    if (dirname.is_local()) {
      string prefix = _scope->expand_variable("DIRPREFIX");
      dirname = Filename(prefix, dirname);
    }
    
    Filename filename(dirname, "file");
    if (!filename.make_dir()) {
      if (!dirname.is_directory()) {
        cerr << "Unable to create directory " << dirname << "\n";
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_defer_command
//       Access: Protected
//  Description: Handles the #defer command: define a new variable or
//               change the definition of an existing variable.  This
//               is different from #define in that the variable
//               definition is not immediately expanded; it will be
//               expanded when the variable is later used.  This
//               allows the definition of variables that depend on
//               other variables whose values have not yet been
//               defined.  This is akin to GNU make's = assignment.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_defer_command() {
  // Pull off the first word and the rest of the params.
  size_t p = _scope->scan_to_whitespace(_params);
  string varname = trim_blanks(_scope->expand_string(_params.substr(0, p)));

  if (OCSubroutine::get_func(varname) != (const OCSubroutine *)NULL) {
    cerr << "Warning: variable " << varname
         << " shadowed by function definition.\n";
  }
  
  // Skip whitespace between the variable name and its definition.
  while (p < _params.length() && isspace(_params[p])) {
    p++;
  }
  string def = _params.substr(p);

  // We don't expand the variable's definition immediately; it will be
  // expanded when the variable is referenced later.  However, we
  // should expand any simple self-reference immediately, to allow for
  // recursive definitions.
  def = _scope->expand_self_reference(def, varname);
  _scope->define_variable(varname, def);

  if (verbose>=2) {
    cerr<<"#defer "<<varname<<" = "<<_params.substr(p)<<endl;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_define_command
//       Access: Protected
//  Description: Handles the #define command: define a new variable or
//               change the definition of an existing variable.  The
//               variable definition is immediately expanded.  This is
//               akin to GNU make's := assignment.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_define_command() {
  // Pull off the first word and the rest of the params.
  size_t p = _scope->scan_to_whitespace(_params);
  string varname = trim_blanks(_scope->expand_string(_params.substr(0, p)));

  if (OCSubroutine::get_func(varname) != (const OCSubroutine *)NULL) {
    cerr << "Warning: variable " << varname
         << " shadowed by function definition.\n";
  }
  
  // Skip whitespace between the variable name and its definition.
  while (p < _params.length() && isspace(_params[p])) {
    p++;
  }
  string def = _scope->expand_string(_params.substr(p));
  _scope->define_variable(varname, def);

  if (verbose>=2) {
    cerr<<"#define "<<varname<<" = "<<_params.substr(p)<<"\n          \""<<def<<"\""<<endl;
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_set_command
//       Access: Protected
//  Description: Handles the #set command: change the definition of an
//               existing variable.
//
//               This is different from #defer in two ways: (1) the
//               variable in question must already have been #defined
//               elsewhere, (2) if the variable was #defined in some
//               parent scope, this will actually change the variable
//               in the parent scope, rather than shadowing it in the
//               local scope.  Like #define and unlike #defer, the
//               variable definition is expanded immediately, similar
//               to GNU make's := operator.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_set_command() {
  // Pull off the first word and the rest of the params.
  size_t p = _scope->scan_to_whitespace(_params);
  string varname = trim_blanks(_scope->expand_string(_params.substr(0, p)));

  if (OCSubroutine::get_func(varname) != (const OCSubroutine *)NULL) {
    cerr << "Warning: variable " << varname
         << " shadowed by function definition.\n";
  }
  
  // Skip whitespace between the variable name and its definition.
  while (p < _params.length() && isspace(_params[p])) {
    p++;
  }
  string def = _scope->expand_string(_params.substr(p));

  if (!_scope->set_variable(varname, def)) {
    cerr << "Attempt to set undefined variable " << varname << "\n";
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_map_command
//       Access: Protected
//  Description: Handles the #map command: define a new map variable.
//               This is a special kind of variable declaration that
//               creates a variable that can be used as a function to
//               look up variable expansions within a number of
//               different named scopes, accessed by keyword.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_map_command() {
  // Pull off the first word and the rest of the params.
  size_t p = _scope->scan_to_whitespace(_params);
  string varname = trim_blanks(_scope->expand_string(_params.substr(0, p)));
  
  // Skip whitespace between the variable name and its definition.
  while (p < _params.length() && isspace(_params[p])) {
    p++;
  }
  string def = trim_blanks(_params.substr(p));
  _scope->define_map_variable(varname, def);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_addmap_command
//       Access: Protected
//  Description: Handles the #addmap command: add a new key/scope pair
//               to an existing map variable.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_addmap_command() {
  // Pull off the first word and the rest of the params.
  size_t p = _scope->scan_to_whitespace(_params);
  string varname = trim_blanks(_scope->expand_string(_params.substr(0, p)));
  
  // Skip whitespace between the variable name and the key.
  while (p < _params.length() && isspace(_params[p])) {
    p++;
  }
  string key = trim_blanks(_scope->expand_string(_params.substr(p)));

  _scope->add_to_map_variable(varname, key, _scope);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_push_command
//       Access: Protected
//  Description: Handles the #push command: push a variable definition
//               out to the enclosing scope.  Useful for defining
//               variables within a #forscopes block that you want to
//               persist longer than the block itself.
//
//               Syntax is:
//
//               #push n varname [varname2 ... ]
//
//               Where n is the number of levels out to push.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_push_command() {
  // The first word is the number of levels.
  size_t p = _scope->scan_to_whitespace(_params);
  string levels_str = trim_blanks(_scope->expand_string(_params.substr(0, p)));

  // Validate the number.
  const char *param = levels_str.c_str();
  char *n;
  int levels = strtol(param, &n, 10);
  if (n == param || levels < 0) {
    // Invalid integer.
    cerr << "#push with invalid level count: " << levels_str << "\n";
    return false;
  }

  OCScope *enclosing_scope = _scope;
  if (levels > 0) {
    enclosing_scope = _scope->get_enclosing_scope(static_cast<unsigned>(levels - 1));
  }

  // Skip whitespace to the first variable name.
  while (p < _params.length() && isspace(_params[p])) {
    p++;
  }

  while (p < _params.length()) {
    // Pull off the next varname.
    size_t q = _scope->scan_to_whitespace(_params, p);
    string varname = trim_blanks(_scope->expand_string(_params.substr(p, q - p)));
    string def = _scope->get_variable(varname);
    enclosing_scope->define_variable(varname, def);

    p = q;
    while (p < _params.length() && isspace(_params[p])) {
      p++;
    }
  }
  return true;
}


////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_debugscopes_command
//       Access: Protected
//  Description: Handles the #debugscopes command: dumps to cerr the
//               tree of known scopes.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_debugscopes_command() {
  OCNamedScopes *named_scopes = _scope->get_named_scopes();
  OCNamedScopes::Scopes scopes;
  named_scopes->get_scopes("*/", scopes);
  named_scopes->get_scopes("*/*", scopes);
  OCNamedScopes::Scopes::const_iterator si;
  cerr << "Debugging scopes:\n";
  for (si=scopes.begin(); si!=scopes.end(); ++si) {
    cerr << "Scope name: '" << named_scopes->get_scope_name(*si) << "'";
    OCScope *parent = (*si)->get_parent();
    if (parent!=NULL) {
      cerr << " (parented to '" << named_scopes->get_scope_name(parent) << "')\n";
    } else {
      cerr << "\n";
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_depsave_command
//       Access: Protected
//  Description: Handles the #depsave command; dumps the dependency
//               cache to a (given) file.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_depsave_command() {
  string filename = trim_blanks(_scope->expand_string(_params));
  OCDirectory * dir = _scope->get_directory();
  dir->write_dep_cache( filename );
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_depload_command
//       Access: Protected
//  Description: Handles the #depload command; reads the dependency
//               cache from a (given) file.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_depload_command() {
  string filename = trim_blanks(_scope->expand_string(_params));
  OCDirectory * dir = _scope->get_directory();
  dir->read_dep_cache( filename );
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::handle_getincludes_command
//       Access: Protected
//  Description: Handles the 'getincludes' command, get the dirs
//               that should be in the include path
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
handle_getincludes_command() {
  size_t p = _scope->scan_to_whitespace(_params);
  string varname = trim_blanks(_scope->expand_string(_params.substr(0, p)));

  OCDirectory * dir = _scope->get_directory();
  string def = dir->get_depends();
  _scope->define_variable(varname, def);

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::include_file
//       Access: Protected
//  Description: The internal implementation of #include: includes a
//               particular named file at this point.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
include_file(Filename filename) {
  filename.set_text();

  ifstream in;
  if (!filename.open_read(in)) {
    cerr << "Unable to open include file " << filename.to_os_specific() << ".\n";
    return false;
  }
  if (verbose) {
    cerr << "Reading (inc) \"" << filename.to_os_specific() << "\"\n";
  }

  int oldlineno = _cur_linenumber;
  // Need to pass the old line number becase it can't look it up
  PushFilename pushed(_scope, filename, oldlineno);

  string line;
  getline(in, line);
  _cur_linenumber = 1;
  while ((!in.fail() && !in.eof()) || (!line.empty())) {
    if (!read_line(line)) {
      return false;
    }
    line.clear();
    getline(in, line);
	_cur_linenumber += 1;
  }

  if (!in.eof()) {
    cerr << "Error reading " << filename.to_os_specific() << ".\n";
    return false;
  }

  // restore the old line number
  _cur_linenumber = oldlineno;
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::replay_while
//       Access: Protected
//  Description: Replays all the lines that were saved during a
//               previous #while..#end block.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
replay_while(const string &name) {
  assert(!_in_for);

  bool okflag = true;

  vector<string> lines;
  lines.swap(_saved_lines);

  // Remove the #end command.  This will fail if someone makes an #end
  // command that spans multiple lines.  Don't do that.
  assert(!lines.empty());
  lines.pop_back();

  // Now replay all of the saved lines.
  BlockNesting *saved_block = _block_nesting;
  IfNesting *saved_if = _if_nesting;

  while (!_scope->expand_string(name).empty()) {
    vector<string>::const_iterator li;
    for (li = lines.begin(); li != lines.end() && okflag; ++li) {
      okflag = read_line(*li);
    }
  }

  if (saved_block != _block_nesting || saved_if != _if_nesting) {
    cerr << "Misplaced #end or #endif.\n";
    okflag = false;
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::replay_for
//       Access: Protected
//  Description: Replays all the lines that were saved during a
//               previous #for..#end block.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
replay_for(const string &name, const vector<string> &words) {
  assert(!_in_for);

  bool okflag = true;

  vector<string> lines;
  lines.swap(_saved_lines);

  // Remove the #end command.  This will fail if someone makes an #end
  // command that spans multiple lines.  Don't do that.
  assert(!lines.empty());
  lines.pop_back();

  // Expand the variable name.
  string varname = _scope->expand_string(name);

  // Get out the numeric range.
  int range[3] = {0, 0, 1};
  assert(words.size() <= 3);
  for (unsigned i = 0; i < words.size(); i++) {
    const char *param = words[i].c_str();
    char *n;
    range[i] = strtol(param, &n, 10);
    if (n == param) {
      cerr << "Invalid integer in #for: " << param << "\n";
      return false;
    }
  }

  if (range[2] == 0) {
    cerr << "Step by zero in #for " << name << "\n";
    return false;
  }

  // Now replay all of the saved lines.
  BlockNesting *saved_block = _block_nesting;
  IfNesting *saved_if = _if_nesting;
  int index_var;

  if (range[2] > 0) {
    for (index_var = range[0]; index_var <= range[1]; index_var += range[2]) {
      _scope->define_variable(varname, _scope->format_int(index_var));
      vector<string>::const_iterator li;
      for (li = lines.begin(); li != lines.end() && okflag; ++li) {
        okflag = read_line(*li);
      }
    }
  } else {
    for (index_var = range[0]; index_var >= range[1]; index_var += range[2]) {
      _scope->define_variable(varname, _scope->format_int(index_var));
      vector<string>::const_iterator li;
      for (li = lines.begin(); li != lines.end() && okflag; ++li) {
        okflag = read_line(*li);
      }
    }
  }
  _scope->define_variable(varname, _scope->format_int(index_var));

  if (saved_block != _block_nesting || saved_if != _if_nesting) {
    cerr << "Misplaced #end or #endif.\n";
    okflag = false;
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::replay_forscopes
//       Access: Protected
//  Description: Replays all the lines that were saved during a
//               previous #forscopes..#end block.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
replay_forscopes(const string &name) {
  assert(!_in_for);

  bool okflag = true;

  vector<string> lines;
  lines.swap(_saved_lines);

  // Remove the #end command.  This will fail if someone makes an #end
  // command that spans multiple lines.  Don't do that.
  assert(!lines.empty());
  lines.pop_back();

  OCNamedScopes *named_scopes = _scope->get_named_scopes();

  // Extract out the scope names from the #forscopes .. #end name.  This
  // is a space-delimited list of scope names.
  vector<string> words;
  tokenize_whitespace(_scope->expand_string(name), words);

  // Now build up the list of scopes with these names.
  OCNamedScopes::Scopes scopes;
  vector<string>::const_iterator wi;
  for (wi = words.begin(); wi != words.end(); ++wi) {
    named_scopes->get_scopes(*wi, scopes);
  }
  OCNamedScopes::sort_by_dependency(scopes);

  // And finally, replay all of the saved lines.
  BlockNesting *saved_block = _block_nesting;
  IfNesting *saved_if = _if_nesting;

  OCNamedScopes::Scopes::const_iterator si;
  for (si = scopes.begin(); si != scopes.end() && okflag; ++si) {
    OCScope::push_scope(_scope);
    _scope = (*si);
    
    vector<string>::const_iterator li;
    for (li = lines.begin(); li != lines.end() && okflag; ++li) {
      okflag = read_line(*li);
    }
    _scope = OCScope::pop_scope();
  }

  if (saved_block != _block_nesting || saved_if != _if_nesting) {
    cerr << "Misplaced #end or #endif.\n";
    okflag = false;
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::replay_foreach
//       Access: Protected
//  Description: Replays all the lines that were saved during a
//               previous #foreach..#end block.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
replay_foreach(const string &varname, const vector<string> &words) {
  assert(!_in_for);

  bool okflag = true;

  vector<string> lines;
  lines.swap(_saved_lines);

  // Remove the #end command.  This will fail if someone makes an #end
  // command that spans multiple lines.  Don't do that.
  assert(!lines.empty());
  lines.pop_back();

  // Now traverse through the saved words.
  BlockNesting *saved_block = _block_nesting;
  IfNesting *saved_if = _if_nesting;

  vector<string>::const_iterator wi;
  for (wi = words.begin(); wi != words.end() && okflag; ++wi) {
    _scope->define_variable(varname, (*wi));
    vector<string>::const_iterator li;
    for (li = lines.begin(); li != lines.end() && okflag; ++li) {
      okflag = read_line(*li);
    }
  }

  if (saved_block != _block_nesting || saved_if != _if_nesting) {
    cerr << "Misplaced #end or #endif.\n";
    okflag = false;
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::replay_formap
//       Access: Protected
//  Description: Replays all the lines that were saved during a
//               previous #formap..#end block.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
replay_formap(const string &varname, const string &mapvar) {
  assert(!_in_for);

  bool okflag = true;

  vector<string> lines;
  lines.swap(_saved_lines);

  // Remove the #end command.  This will fail if someone makes an #end
  // command that spans multiple lines.  Don't do that.
  assert(!lines.empty());
  lines.pop_back();

  // Now look up the map variable.
  OCScope::MapVariableDefinition &def = _scope->find_map_variable(mapvar);
  if (&def == &OCScope::_null_map_def) {
    cerr << "Undefined map variable: #formap " << varname << " " 
         << mapvar << "\n";
    return false;
  }

  // Now traverse through the map definition.
  BlockNesting *saved_block = _block_nesting;
  IfNesting *saved_if = _if_nesting;

  OCScope::MapVariableDefinition::const_iterator di;
  for (di = def.begin(); di != def.end() && okflag; ++di) {
    _scope->define_variable(varname, (*di).first);

    OCScope::push_scope(_scope);
    _scope = (*di).second;

    vector<string>::const_iterator li;
    for (li = lines.begin(); li != lines.end() && okflag; ++li) {
      okflag = read_line(*li);
    }

    _scope = OCScope::pop_scope();
  }

  if (saved_block != _block_nesting || saved_if != _if_nesting) {
    cerr << "Misplaced #end or #endif.\n";
    okflag = false;
  }

  return okflag;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::compare_output
//       Access: Protected
//  Description: After a file has been written to a (potentially
//               large) string via an #output command, compare the
//               results to the original file.  If they are different,
//               remove the original file and replace it with the new
//               contents; otherwise, leave the original alone.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
compare_output(const string &new_contents, Filename filename,
               bool notouch, bool notest) {
  filename.set_text();
  bool exists = filename.exists();
  bool differ = notest;

  if (exists && !notest) {
    size_t len = new_contents.length();
    size_t want_bytes = len + 1;

    char *orig_contents = new char[want_bytes];
    ifstream in;
    if (!filename.open_read(in)) {
      cerr << "Cannot read existing " << filename.to_os_specific() << ", regenerating.\n";
      differ = true;
    } else {
      if (verbose) {
        cerr << "Reading (cmp) \"" << filename.to_os_specific() << "\"\n";
      }
      in.read(orig_contents, want_bytes);

      if (static_cast<size_t>(in.gcount()) != len) {
        // The wrong number of bytes.
        differ = true;
        
      } else {
        differ = !(new_contents == string(orig_contents, len));
      }
    }
  }

  if (differ || !exists) {
#ifndef WIN32_VC
    if (verbose_dry_run) {
      // Write our new contents to a file so we can run diff on both
      // of them.
      Filename temp_filename = filename.get_fullpath() + string(".ppd");
      temp_filename.set_text();
      ofstream out_b;
      if (!temp_filename.open_write(out_b)) {
        cerr << "Unable to open temporary file " << filename.to_os_specific() << " for writing.\n";
        return false;
      }
      
      out_b.write(new_contents.data(), new_contents.length());

      bool diff_ok = true;
      if (!out_b) {
        cerr << "Unable to write to temporary file " << filename.to_os_specific() << "\n";
        diff_ok = true;
      }
      out_b.close();
      string command = "diff -ub '" + filename.get_fullpath() + "' '" + 
        temp_filename.get_fullpath() + "'";
      int sys_result = system(command.c_str());
      if (sys_result < 0) {
        cerr << "Unable to invoke diff\n";
        diff_ok = false;
      }
      out_b.close();
      temp_filename.unlink();

      return diff_ok;
      
    } else
#endif
      if (dry_run) {
        cerr << "Would generate " << filename.to_os_specific() << "\n";
      } else {
        if (!notest) {
          // if we were told not to test, we can are assuming we will always
          // write this file.
          cerr << "Generating " << filename.to_os_specific() << "\n";
        }
        
        if (exists) {
          if (!filename.unlink()) {
            cerr << "Unable to remove old " << filename.to_os_specific() << "\n";
            return false;
          }
        }
        
        ofstream out_b;
        if (!filename.open_write(out_b)) {
          cerr << "Unable to open file " << filename.to_os_specific() << " for writing.\n";
          return false;
        }
        
        out_b.write(new_contents.data(), new_contents.length());
        
        if (!out_b) {
          cerr << "Unable to write to file " << filename.to_os_specific() << "\n";
          return false;
        }
        out_b.close();
      }
      
  } else {
    // Even though the file is unchanged, unless the "notouch" flag is
    // set, we want to update the modification time.  This helps the
    // makefiles know we did something.
    if (!notouch && !dry_run) {
      if (!filename.touch()) {
        cerr << "Warning: unable to update timestamp for " << filename.to_os_specific() << "\n";
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::failed_if
//       Access: Protected
//  Description: Returns true if we are currently within a failed #if
//               block (or in an #else block for a passed #if block),
//               or false otherwise.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
failed_if() const {
  return (_if_nesting != (IfNesting *)NULL && 
          (_if_nesting->_state == IS_off || _if_nesting->_state == IS_done));
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::is_valid_formal_parameter_name
//       Access: Protected
//  Description: Returns true if the indicated name is an acceptable
//               name for a formal parameter.  This means it includes
//               no whitespace or crazy punctuation.  Mainly this is
//               to protect the user from making some stupid syntax
//               mistake.
////////////////////////////////////////////////////////////////////
bool OCCommandFile::
is_valid_formal(const string &formal_parameter_name) const {
  if (formal_parameter_name.empty()) {
    return false;
  }
  
  string::const_iterator si;
  for (si = formal_parameter_name.begin();
       si != formal_parameter_name.end();
       ++si) {
    switch (*si) {
    case ' ':
    case '\n':
    case '\t':
    case '$':
    case '[':
    case ']':
    case ',':
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::PushFilename::Constructor
//       Access: Public
//  Description: This special class changes the current filename of
//               the OCCommandFile.  The idea is to create one of
//               these when the filename is changed (for instance, to
//               read in a new file via an #include directive); when
//               the variable then goes out of scope, it will restore
//               the previous filename.
//
//               This updates the scope with the appropriate
//               variables.
////////////////////////////////////////////////////////////////////
OCCommandFile::PushFilename::
PushFilename(OCScope *scope, const string &filename, int cur_linenumber) {
  _scope = scope;
  _old_thisdirprefix = _scope->get_variable("THISDIRPREFIX");
  _old_thisfilename = _scope->get_variable("THISFILENAME");

  if (cur_linenumber == 0) {
    OCDirectory * ocd = _scope->get_directory();
	if (ocd != NULL) {
	  // put the 'path we're loading from' as the top level.
	  _old_thisfilename = _scope->get_directory()->get_path();
	}
  }

  command_profile->openCallstack(_old_thisdirprefix + "/" + _old_thisfilename, cur_linenumber);

  string thisfilename = filename;
  string thisdirprefix;

  size_t slash = filename.rfind('/');
  if (slash == string::npos) {
    thisdirprefix = string();
  } else {
    thisdirprefix = filename.substr(0, slash + 1);
  }

  _scope->define_variable("THISFILENAME", thisfilename);
  _scope->define_variable("THISDIRPREFIX", thisdirprefix);
}

////////////////////////////////////////////////////////////////////
//     Function: OCCommandFile::PushFilename::Destructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCCommandFile::PushFilename::
~PushFilename() {
  command_profile->closeCallstack();

  _scope->define_variable("THISFILENAME", _old_thisfilename);
  _scope->define_variable("THISDIRPREFIX", _old_thisdirprefix);
}
