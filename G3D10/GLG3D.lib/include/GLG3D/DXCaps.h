/**
 \file GLG3D/DXCaps.h

 \created 2006-04-06
 \edited  2006-05-06

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_DXCAPS_H
#define G3D_DXCAPS_H

#include "G3D/platform.h"
#include "G3D/g3dmath.h"


namespace G3D {

/** 
    Provides very basic DirectX detection and information support
*/
class DXCaps {

public:
    /**
        Returns 0 if not installed otherwise returns the major and minor number
        in the form (major * 100) + minor.  eg. 900 is 9.0 and 901 is 9.1
    */
    static uint32 version();

    /**
        Returns the amount of video memory detected by Direct3D in bytes. 
        This may not be the true amount of physical RAM; some graphics cards
        over or under report.
    */
    static uint64 videoMemorySize();
};

} // namespace G3D

#endif // G3D_DXCAPS_H
