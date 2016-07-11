/**
 \file G3D/ParseVOX.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2016-07-09
 \edited  2016-07-09

 G3D Innovation Engine
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
*/
#pragma once

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Color4unorm8.h"
#include "G3D/Vector3uint8.h"
#include "G3D/Vector3int32.h"

namespace G3D {

class BinaryInput;

/** Parser for the MagicaVoxel sparse voxel format
    http://ephtracy.github.io/index.html?page=mv_vox_format */
class ParseVOX {
public:

    class Voxel {
    public:
        Point3uint8             position;

        /** Index into palette */
        uint8                   index;
    };
#if 0
    typedef FastPODTable<Point3uint8, uint8, HashTrait<Point3uint8>, EqualsTrait<Point3uint8>, true> VoxelTable;

    /** voxel[xyz] is an index into the palette. Non-zero voxels can be obtained by voxel.begin().
        Use voxel.getPointer(pos) to test whether a voxel is present without creating a new empty
        value at that location.*/
    VoxelTable                  voxel;
#endif

    Array<Voxel>                voxel;

    /** These are shifted from the file format, so that palette[0] 
        is always transparent black and palette[1] is the first value
        from the palette file.*/
    Color4unorm8                palette[256];

    Vector3int32                size;

    void parse(const char* ptr, size_t len);

    void parse(BinaryInput& bi);
};

}
