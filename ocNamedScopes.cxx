// Filename: ocNamedScopes.cxx
// Created by:  frang (03June04)
// 
////////////////////////////////////////////////////////////////////

#include "ocNamedScopes.h"
#include "ocScope.h"
#include "ocDirectory.h"

#include <assert.h>
#include <algorithm>

// An STL object to sort named scopes in order by dependency and then
// by directory name, used in sort_by_dependency().
class SortScopesByDependencyAndName {
public:
  bool operator () (OCScope *a, OCScope *b) const {
    OCDirectory *da = a->get_directory();
    OCDirectory *db = b->get_directory();

    // Scopes without associated directories appear first in the list.
    bool da_is_null = (da == (OCDirectory *)NULL);
    bool db_is_null = (db == (OCDirectory *)NULL);

    if (da_is_null != db_is_null) {
      return da_is_null > db_is_null;

    } else if (da_is_null) {
      // If two scopes have no associated directories (!) they are
      // considered equivalent.
      return false;

    } else {
      // Otherwise, both scopes have associated directories, and we
      // can properly put them in order by dependencies.
      assert(da != (OCDirectory *)NULL);
      assert(db != (OCDirectory *)NULL);
      if (da->get_depends_index() != db->get_depends_index()) {
        return da->get_depends_index() < db->get_depends_index();
      }
      return da->get_dirname() < db->get_dirname();
    }
  }
};

////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCNamedScopes::
OCNamedScopes() {
}

////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::Destructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
OCNamedScopes::
~OCNamedScopes() {
}

////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::make_scope
//       Access: Public
//  Description: Creates a new scope in the current directory name
//               with the indicated scope name.
////////////////////////////////////////////////////////////////////
OCScope *OCNamedScopes::
make_scope(const string &name) {
  OCScope *scope = new OCScope(this);
  _directories[_current][name].push_back(scope);
  scope->define_variable("THISSCOPENAME", get_scope_name(scope));
  return scope;
}
 
////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::get_scopes
//       Access: Public
//  Description: Returns a list of all the named scopes matching the
//               given scope name.  The scope name may be of the form
//               "dirname/scopename", in which case the dirname may be
//               another directory name, or "." or "*".  If the
//               dirname is ".", it is the same as the current
//               directory name; if it is "*", it represents all
//               directory names.  If omitted, the current directory
//               name is implied.
//
//               It is the responsibility of the user to ensure that
//               scopes is empty before calling this function; this
//               will append to the existing vector without first
//               clearing it.
////////////////////////////////////////////////////////////////////
void OCNamedScopes::
get_scopes(const string &name, Scopes &scopes) const {
  string dirname = _current;
  string scopename = name;

  size_t slash = name.rfind(SCOPE_DIRNAME_SEPARATOR);
  if (slash != string::npos) {
    dirname = name.substr(0, slash);
    scopename = name.substr(slash + 1);
    if (dirname == SCOPE_DIRNAME_CURRENT) {
      dirname = _current;
    }
  }

  Directories::const_iterator di;

  if (dirname == SCOPE_DIRNAME_WILDCARD) {
    for (di = _directories.begin(); di != _directories.end(); ++di) {
      p_get_scopes((*di).second, scopename, scopes);
    }

  } else {
    di = _directories.find(dirname);
    if (di != _directories.end()) {
      p_get_scopes((*di).second, scopename, scopes);
    }
  }
}
 
////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::get_scope_name
//       Access: Public
//  Description: Returns the name of the given scope
////////////////////////////////////////////////////////////////////
string OCNamedScopes::
get_scope_name(OCScope* scope) const {
  Directories::const_iterator di;
  for (di=_directories.begin(); di!=_directories.end(); ++di) {
    Named::const_iterator ni;
    for (ni=(*di).second.begin(); ni!=(*di).second.end(); ++ni) {
      Scopes::const_iterator si;
      for (si=(*ni).second.begin(); si!=(*ni).second.end(); ++si) {
        if ((*si) == scope) {
          if ((*di).first.empty()) {
            return (*ni).first;
          } else {
            return (*di).first + SCOPE_DIRNAME_SEPARATOR + (*ni).first;
          }
        }
      }
    }
  }
  return string();
}

////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::sort_by_dependency
//       Access: Public, Static
//  Description: Sorts the previously-generated list of scopes into
//               order such that the later scopes depend on the
//               earlier scopes.
////////////////////////////////////////////////////////////////////
void OCNamedScopes::
sort_by_dependency(OCNamedScopes::Scopes &scopes) {
  sort(scopes.begin(), scopes.end(), SortScopesByDependencyAndName());
}

////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::set_current
//       Access: Public
//  Description: Changes the currently-active directory, i.e. the
//               dirname represented by ".".
////////////////////////////////////////////////////////////////////
void OCNamedScopes::
set_current(const string &dirname) {
  if (dirname == SCOPE_DIRNAME_CURRENT) {
    _current = "";
  } else {
    _current = dirname;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: OCNamedScopes::p_get_scopes
//       Access: Private
//  Description: Adds the scopes from the given directory with the
//               matching name into the returned vector.
////////////////////////////////////////////////////////////////////
void OCNamedScopes::
p_get_scopes(const OCNamedScopes::Named &named, const string &name,
             Scopes &scopes) const {
  Named::const_iterator ni;
  if (name == "*") {
    // Scope name "*" means all nested scopes in this directory,
    // except for the empty-name scope (which is the outer scope).
    for (ni = named.begin(); ni != named.end(); ++ni) {
      if (!(*ni).first.empty()) {
        const Scopes &s = (*ni).second;
        scopes.insert(scopes.end(), s.begin(), s.end());
      }
    }

  } else {
    ni = named.find(name);
    if (ni != named.end()) {
      const Scopes &s = (*ni).second;
      scopes.insert(scopes.end(), s.begin(), s.end());
    }
  }
}
