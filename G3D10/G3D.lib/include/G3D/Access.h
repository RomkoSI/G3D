/**
  \file G3D/Access.h

  \maintainer Michael Mara, http://www.illuminationcodified.com
  \created 2013-05-17
  \edited  2013-05-17
*/
#ifndef G3D_Access_h
#define G3D_Access_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"

namespace G3D {
/** 
    \brief Access specifier for a buffer of data. Has identical values to the corresponding GL pragmas
 */
class Access {
public:

    enum Value {
        READ = 0x88B8, 
        WRITE = 0x88B9,
        READ_WRITE = 0x88BA
    } value;


    static const char* toString(int i, Value& v) {
        static const char* str[] = {"READ", "WRITE", "READ_WRITE", NULL};
        static const Value val[] = {READ, WRITE, READ_WRITE};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }
    G3D_DECLARE_ENUM_CLASS_METHODS(Access);
};


} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::Access);

#endif
