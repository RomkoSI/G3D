/**
\file svnutils.h

\maintainer Mike Mara

\created 2014-10-01
\edited  2016-09-02

 G3D Innovation Engine
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
 */
#pragma once

#include "G3D/platform.h"
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
    Return -1 if no svn information is found. Caches results.
    */
int getSVNRepositoryRevision(const String& rawPath);

int svnAdd(const String& path);

bool hasCommandLineSVN();

}
