//
// oconfig call times profiling object
//

#include "ocProfileInfo.h"

#include <iostream>
#include <sstream>

#include <sys/types.h>
#include <sys/timeb.h>

// in ocmain, new the object
ocProfileInfo *command_profile;
string profile_tag;

millis cur_time_millis() {
  millis cur_time;

#ifdef WIN32  
  struct _timeb tb;
  // note that linux prefers "gettimeofday" to _ftime .. but windows doesn't have it.
  _ftime( &tb );
  cur_time = (tb.time * 1000ll);
  cur_time += tb.millitm;
#else
  cur_time = 0;
#endif
  
  return ( cur_time );
}


ocProfileInfo::ocProfileInfo() {
  // initialize with just the root element
  string str = "ROOT";
  capturing = 0;
  TimesliceSample *ts = new TimesliceSample(str);
  
  slicestack.push_back( ts );
  callstack.push_back( str );
  linestack.push_back( str );
}


// ------------------------------
// handle the current "call stack" regarding oconfig
// ------------------------------
void ocProfileInfo::openCallstack( string filename, int line ) {
  includestack.push_front(new IncludeFrame(filename, line));
}

void ocProfileInfo::closeCallstack() {
  includestack.pop_front();
}

// ------------------------------
// return a multiline backtrace with the passed filename / linenumber
// listed as "frame 0"
// ------------------------------
string ocProfileInfo::getCallstackBacktrace(string filename, int linenumber) {
  ostringstream ret;
  ret << "0: " << filename << ":" << linenumber << "\n";
  IncludeFrame *iframe;

  int i = 1;
  for (includestack_iter_t iter = includestack.begin() ;
       iter != includestack.end() ;
       ++iter, ++i) {
    iframe = (IncludeFrame *)*iter;
    ret << i << ": " << iframe->filename << ":" << iframe->linenum << "\n";
  }
  return ret.str();
}

// ------------------------------
// open a new timeslice, returns the new timeslice start time
// ------------------------------
millis ocProfileInfo::openTimeslice( string id, string line ) {
  if (! capturing) { return 0; }
  TimesliceSample *ts = new TimesliceSample(id);
  string new_key = callstack.back() + "." + id;
  string new_line = linestack.back() + "\n" + line;

  linestack.push_back ( new_line );
  callstack.push_back( new_key );
  slicestack.push_back( ts )  ;
  return ( ts->start_time );
}


// ------------------------------
// close the timeslice, returns the number of millis elapsed in this timeslice
// ------------------------------
millis ocProfileInfo::closeTimeslice( ) {
  if (! capturing) { return 0; }
  TimesliceSample *ts = slicestack.back();
  string key = callstack.back();
  string line = linestack.back();

  slicestack.pop_back();
  callstack.pop_back();
  linestack.pop_back ();
  
  ts->stop();
  
  millis elapsed_time = ts->getElapsedTime();
  delete ts;

  ElapsedTime et = key_to_millis[key];
  et.samples += 1;
  et.elapsed += elapsed_time;
  key_to_millis[key] = et;

  // expect 73 samples, 63 millis
  if (key == profile_tag) {
    et = detail_to_millis[line];
    et.samples += 1;
    et.elapsed += elapsed_time;
    detail_to_millis[line] = et;
  }
  
  return ( elapsed_time );
}


// ------------------------------
// retrieve the number of millis elapsed for a particular key
// ------------------------------
millis ocProfileInfo::getMillis( string key ) {
  if (! capturing) { return 0; }
  ElapsedTime et = key_to_millis[key];
  
  return ( et.elapsed );
}

// ------------------------------
// Dump all timeslice info to standard out
// ------------------------------
void ocProfileInfo::setCaptureState( int capture ) {
  capturing = capture;
}

// ------------------------------
// Dump all timeslice info to standard out
// ------------------------------
void ocProfileInfo::dumpTimesliceInfo() {
  if (! capturing) { return; }
  timeslice_map_t::const_iterator iter;

  for ( iter = key_to_millis.begin(); iter != key_to_millis.end() ; iter++ ) {
    cout << iter->first << ": " << iter->second.elapsed
         << " (" << iter->second.samples << ")" << endl;
  }
}

// ------------------------------
// Dump all timeslice info to standard out
// ------------------------------
void ocProfileInfo::dumpTimesliceDetail() {
  if (! capturing) { return; }
  timeslice_map_t::const_iterator iter;

  for ( iter = detail_to_millis.begin(); iter != detail_to_millis.end() ; iter++ ) {
    cout << iter->first << ": " << iter->second.elapsed
         << " (" << iter->second.samples << ")" << endl << endl << endl;
  }
}
