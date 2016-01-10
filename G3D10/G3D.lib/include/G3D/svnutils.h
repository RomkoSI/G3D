#ifndef G3D_svnutils_h
#define G3D_svnutils_h
#include "G3D/G3DString.h"
namespace G3D {
/** 
    Returns the highest revision of svn version files under \param path
    Return -1 if no svn information is found
    */
int getSVNDirectoryRevision(const String& path);

/** 
    Fails if the path is over 4 parents away from the versioned part of the repository.
    Works by finding the revision of all parent directories of rawPath (and rawPath itself).
    Return -1 if no svn information is found. Caches reuslts.
    */
int getSVNRepositoryRevision(const String& rawPath);

}

#endif
