// Filename: ocSearchPath.I
// Created by:  frang (03June04)
//
////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////
//     Function: OCSearchPath::search_path
//       Access: Public, Static
//  Description: A quick-and-easy way to search a searchpath for a
//               file when you don't feel like building or keeping
//               around a OCSearchPath object.  This simply
//               constructs a temporary OCSearchPath based on the
//               indicated path string, and searches that.
////////////////////////////////////////////////////////////////////
INLINE Filename OCSearchPath::search_path(const Filename &filename,
                                          const string &path,
                                          const string &delimiters) {
  OCSearchPath search(path, delimiters);
  return search.find_file(filename);
}
