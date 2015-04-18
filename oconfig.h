/*
// Filename: oconfig.h
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////
*/

#ifndef OCONFIG_H
#define OCONFIG_H

#ifdef _MSC_VER
  /* For Visual C, include the special config.h file. */
  #include "config_msvc.h"
#else
  /* Otherwise, include the normal automatically-generated file. */
  #include "config_linux.h"
#endif

#ifdef __cplusplus
#ifdef HAVE_IOSTREAM
#include <iostream>
#include <fstream>
#ifdef HAVE_SSTREAM
#include <sstream>
#else  /* HAVE_SSTREAM */
#include <strstream>
#endif  /* HAVE_SSTREAM */

typedef std::ios::openmode ios_openmode;
typedef std::ios::fmtflags ios_fmtflags;
typedef std::ios::iostate ios_iostate;
typedef std::ios::seekdir ios_seekdir;

#else  /* HAVE_IOSTREAM */
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>

typedef int ios_openmode;
typedef int ios_fmtflags;
typedef int ios_iostate;
/* Old iostream libraries used ios::seek_dir instead of ios::seekdir. */
typedef ios::seek_dir ios_seekdir;

#endif  /* HAVE_IOSTREAM */

#if defined(HAVE_CYGWIN) || defined(WIN32_VC)
/* Either Cygwin or Visual C++ is a Win32 environment. */
#if !defined(WIN32)
#define WIN32
#endif
#endif

#include <string>

#ifdef HAVE_NAMESPACE
using namespace std;
#endif
#endif /* __cplusplus */

#ifndef HAVE_ALLOCA_H
  /* If we don't have alloca.h, use malloc() to implement gnu_regex. */
  #define REGEX_MALLOC 1
#endif

#define PACKAGE_FILENAME "Depot.bld"
#define SOURCE_FILENAME "Sources.bld"
#define REDIRECT_FILENAME "Redirect.bld"

#define COMMAND_PREFIX '#'
#define VARIABLE_PREFIX '$'
#define VARIABLE_OPEN_BRACE '['
#define VARIABLE_CLOSE_BRACE ']'
#define PATTERN_WILDCARD '%'
#define BEGIN_COMMENT "//"

#define FUNCTION_PARAMETER_SEPARATOR ','

#define VARIABLE_OPEN_NESTED '('
#define VARIABLE_CLOSE_NESTED ')'
#define VARIABLE_PATSUBST ":"
#define VARIABLE_PATSUBST_DELIM "="

#define SCOPE_DIRNAME_SEPARATOR '/'
#define SCOPE_DIRNAME_WILDCARD "*"
#define SCOPE_DIRNAME_CURRENT "."

#ifdef __cplusplus
/* These are set from the similarly-named variables defined in
   System.pp. */
extern bool unix_platform;
extern bool windows_platform;

/* This is a command-line global parameter. */
extern bool dry_run;
extern bool verbose_dry_run;
extern int verbose; // 0..9 to set verbose level.  0 == off.
#endif

#define INLINE inline

#endif
