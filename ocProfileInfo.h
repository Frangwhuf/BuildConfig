//
// time slice code... assumes only one in existence, uses a static list
//

#ifndef _ocProfileInfo_
#define _ocProfileInfo_
#include "oconfig.h"

#include <map>
#include <list>
#include <string>
#include <time.h>

using namespace std;

class ocProfileInfo;
class ElapsedTime;
class IncludeFrame;

extern ocProfileInfo *command_profile;
extern string profile_tag;

typedef long long millis;

typedef list<string> keystack_t;
typedef list<string> callstack_t;
typedef list<IncludeFrame *> includestack_t;
typedef list<IncludeFrame *>::const_iterator includestack_iter_t;

// maybe the map should be a "timeslice" which is the millis and the samples
typedef map<string, ElapsedTime> timeslice_map_t;

millis cur_time_millis();

//
// encapsulator for a 'stack frame' for the debugger
//
class IncludeFrame {
public:
  int linenum;
  string filename;
  IncludeFrame() : linenum(0), filename("undef") {}
  IncludeFrame(string fn, int ln) : linenum(ln), filename(fn) {}
};

// ------------------------------
// strcture to hold the elapsed time for all the timeslices
// which match a particular "call stack"
// ------------------------------
class ElapsedTime {
 public:
  int samples;
  millis elapsed;
  
  ElapsedTime() {
    samples = 0;
    elapsed = 0LL;
  }
};

class TimesliceSample {
 public:
  TimesliceSample(string k) :
    key(k)
  {
    start_time = cur_time_millis();
  }

  millis stop() { end_time = cur_time_millis(); return end_time; }
  millis getElapsedTime() { return (end_time - start_time); }
  
  string key;
  millis start_time;
  millis end_time;
};


class ocProfileInfo {
 public:
  ocProfileInfo();

  // handle 'include stacks' for the debugger
  void openCallstack(string filename, int line);
  void closeCallstack();
  string getCallstackBacktrace(string filename, int linenumber);
  
  // open a new timeslice
  millis openTimeslice(string id, string line = "default");

  // the string id is "redundant" here, but we use it to assert we're closing the same slice we're opening
  // it adds the number of millis elapsed since the open of the slice to the "elapsed", returns number
  // of millis elapsed in the slice
  millis closeTimeslice();

  // set the capture state to "on" (nonzero) or "off" (zero)
  // a new object defaults to 'off'
  void setCaptureState(int state);
  
  // dump the list of known timeslice keys and associated millis to standard out
  void dumpTimesliceInfo();
  void dumpTimesliceDetail();

  // retrieve the total number of millis spent in this key
  millis getMillis(string key);

 private:
  int capturing;
  // map the keyword path to the timeslice info
  timeslice_map_t key_to_millis;
  // map the full line detail information to timeslice info
  timeslice_map_t detail_to_millis;
  // this is a hack to avoid recomputing the key every time we pop from the stack
  includestack_t includestack;
  keystack_t callstack;
  keystack_t linestack;
  list<TimesliceSample *> slicestack;
};

#endif


