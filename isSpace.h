#ifndef ISSPACE_H
#define ISSPACE_H

extern bool gSpaceTable[ 256 ];
#define isSpace( x ) ( gSpaceTable[ (unsigned char)x ] )

#define isspace isSpace

#endif
