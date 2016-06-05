/**
 \file G3D/Parse3DS.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2011-08-12
 \edited  2011-08-12

 Copyright 2002-2016, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_Parse3DS_h
#define G3D_Parse3DS_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Color3.h"
#include "G3D/Matrix4.h"
#include "G3D/Matrix3.h"

namespace G3D {

class TextInput;

/** \brief Parses 3DS model files.

 This class maps the 3DS file format to a runtime object, which is then
 converted to a more useful runtime format and stored in G3D::ArticulatedModel.

 \cite Keyframe chunks from http://www.republika.pl/design3d/delphi/004.html
 \cite MLI chunks from http://www.programmersheaven.com/zone10/cat454/941.htm

 \sa G3D::ParseMTL, G3D::ParsePLY, G3D::ParseOBJ, G3D::ArticulatedModel
*/
class Parse3DS {
protected:

    // Indenting in this section describes sub-chunks.  Do not reformat.
    enum ChunkHeaderID {
        MAIN3DS       = 0x4d4d,
        M3D_VERSION   = 0x0002,

        EDIT3DS       = 0x3D3D,
            // Subchunks of EDIT3DS
            MESH_VERSION  = 0x3D3E,
            EDITMATERIAL  = 0xAFFF,
                // Subchunks of EDITMATERIAL
                MATNAME       = 0xA000,
                MATAMBIENT    = 0xA010,
                MATDIFFUSE    = 0xA020,
                MATSPECULAR   = 0xA030,
                MATSHININESS  = 0xA040,

                MATSHIN2PCT     = 0xA041,
                MATSHIN3PC      = 0xA042, 
                MATTRANSPARENCY = 0xA050, 
                MATXPFALL       = 0xA052,
                MATREFBLUR      = 0xA053, 
                MATSELFILLUM    = 0xA080,
                MATTWOSIDE      = 0xA081,
                MATDECAL        = 0xA082,
                MATADDITIVE     = 0xA083,
                MATSELFILPCT    = 0xA084,
                MATWIRE         = 0xA085,
                MATSUPERSMP     = 0xA086,
                MATWIRESIZE     = 0xA087,
                MATFACEMAP      = 0xA088,
                MATXPFALLIN     = 0xA08A,
                MATPHONG        = 0xA08C,
                MATWIREABS      = 0xA08E,
                MATSHADING      = 0xA100,

                MATTEXTUREMAP1              = 0xA200,
                    MAT_MAP_FILENAME        = 0xA300,
                    MAT_MAP_TILING          = 0xA351,  
                    MAT_MAP_USCALE          = 0xA354,
                    MAT_MAP_VSCALE          = 0xA356,
                    MAT_MAP_UOFFSET         = 0xA358,
                    MAT_MAP_VOFFSET         = 0xA35A,

                MATTEXTUREMAP2  = 0xA33A,
                MATOPACITYMAP   = 0xA210,
                MATBUMPMAP      = 0xA230,
                MATGLOSSYMAP  = 0xA204,
                MATSHININESSMAP = 0xA33C,
                MATEMISSIVEMAP  = 0xA33D,
                MATREFLECTIONMAP= 0xA220,

            EDIT_CONFIG1 = 0x0100,
            EDIT_CONFIG2 = 0x3E3D,
            EDIT_VIEW_P1 = 0x7012,
            EDIT_VIEW_P2 = 0x7011,
            EDIT_VIEW_P3 = 0x7020,
            EDIT_VIEW1   = 0x7001, 
            EDIT_BACKGR  = 0x1200, 
            EDIT_AMBIENT = 0x2100,

            EDITOBJECT    = 0x4000,

            OBJTRIMESH    = 0x4100,
                // Subchunks of OBJTRIMESH
                TRIVERT       = 0x4110,
                TRIFACE       = 0x4120,
                TRIFACEMAT    = 0x4130,
                TRI_TEXCOORDS = 0x4140,
                TRISMOOTH     = 0x4150,
                TRIMATRIX     = 0x4160,

        EDITKEYFRAME  = 0xB000,
            // Subchunks of EDITKEYFRAME
            KFAMBIENT     = 0xB001,
            KFMESHINFO    = 0xB002,
                KFNAME        = 0xB010,
                KFPIVOT       = 0xB013,
                KFMORPHANGLE  = 0xB015,
                KFTRANSLATION = 0xB020,
                KFROTATION    = 0xB021,
                KFSCALE       = 0xB022,
            KFCAMERA      = 0xB003,
            KFCAMERATARGET= 0xB004,
            KFOMNILIGHT   = 0xB005,
            KFSPOTTARGET  = 0xB006,
            KFSPOTLIGHT   = 0xB007,
            KFFRAMES      = 0xB008,
            KFFOV         = 0xB023,
            KFROLL        = 0xB024,
            KFCOLOR       = 0xB025,
            KFMORPH       = 0xB026,
            KFHOTSPOT     = 0xB027,
            KFFALLOFF     = 0xB028,
            KFHIDE        = 0xB029,
            KFHIERARCHY   = 0xB030,

        // float32 color
        RGBF   = 0x0010,

        // G3D::uint8 color
        RGB24  = 0x0011,

        // Scalar percentage
        INT_PCT    = 0x0030,
        FLOAT_PCT    = 0x0031
    };

    struct ChunkHeader {
        ChunkHeaderID       id;

        /** In bytes, includes the size of the header itself. */
        int                 length;

        /** Absolute start postion */
        int                 begin;

        /** Absolute last postion + 1 */
        int                 end;

    };

public:

    /** A texture map */
    class Map {
    public:
        String         filename;
        Vector2             scale;
        Vector2             offset;

        /** 
        bits 4 and 0: 00 tile (default) 11 decal  01 both
        bit 1: mirror
        bit 2: not used? (0)
        bit 3: negative
        bit 5: summed area map filtering (instead of pyramidal)
        bit 6: use alpha  (toggles RGBluma/alpha. For masks RGB means RGBluma)
        bit 7: there is a one channel tint (either RGBluma or alpha)
        bit 8: ignore alpha (take RGBluma even if an alpha exists (?))
        bit 9: there is a three channel tint (RGB tint)
        */
        G3D::uint16         flags;

        /** Brightness (?) */
        float               pct;

        Map() : scale(Vector2(1,1)), pct(1) {}
    };

    class UniversalMaterial {
    public:
        /** The FaceMat inside an object will reference a material by name */
        String            name;

        bool                twoSided;
        Color3                diffuse;
        Color3                specular;

        // "Self illumination"
        float               emissive;

        float                shininess;
        float                shininessStrength;
        float                transparency;
        float                transparencyFalloff;
        float               reflection;
        float                reflectionBlur;

        Map                 texture1;
        Map                 texture2;
        Map                 bumpMap;

        /** 1 = flat, 2 = gouraud, 3 = phong, 4 = metal */
        int                    materialType;

        inline UniversalMaterial() : twoSided(false), diffuse(Color3::white()), specular(Color3::white()), 
            emissive(0), shininess(0.8f),  shininessStrength(0.25f), transparency(0),
            transparencyFalloff(0), reflection(0), reflectionBlur(0), materialType(3) {
        }
    };

    class FaceMat {
    public:
        /** Indices into triples in an Object indexArray that share a material. */
        Array<int>                  faceIndexArray;

        /** Name of the UniversalMaterial */
        String                 materialName;
    };

    class Object {
    public:
        /** Loaded from the TRIVERTEX chunk (transformed to G3D coordinates).
            In World Space.*/
        Array<Point3>               vertexArray;
        Array<Point2>               texCoordArray;

        /** Triangle list indices (loaded from the TRIFACE chunk) */
        Array<int>                  indexArray;

        /** Part of the EDITOBJECT chunk */
        String                 name;

        /** From KFNAME.
            // The object hierarchy is a bit complex but works like this. 
            // Each Object in the scene is given a number to identify its
            // order in the tree. Also each object is orddered in the 3ds
            // file as it would appear in the tree. The root object is 
            // given the number -1 ( FFFF ). As the file is read a counter
            // of the object number is kept. Is the counter increments the
            // objects are children of the previous objects. But when the 
            // pattern is broken by a number what will be less than the 
            // current counter the hierarchy returns to that level.
        */
        int                         hierarchyIndex;

        int                         nodeID;

        /** TRI_LOCAL chunk (transformed to G3D coordinates).  
            In the file, this has <B>already been applied</B> to the
            vertices.
          */
        Matrix4                     cframe;

        /** Unused. */
        Vector3                     pivot;

        /** The center of the local reference frame */
        Matrix4                     keyframe;

        /** Mapping of face indices to materials */
        Array<FaceMat>              faceMatArray;

        inline Object() : pivot(Vector3::zero()), keyframe(Matrix4::identity()) {}
    };

    /** Index into objectArray of the object addressed by the
       current keyframe chunk.
      */
    int                         currentObject;

    int                         currentMaterial;

    Array<Object>               objectArray;
    Array<UniversalMaterial>        materialArray;

    /** Maps material names to indices into the MaterialArray */
    Table<String, int>     materialNameToIndex;

    /** Animation start and end frames from KFFRAMES chunk */
    G3D::uint32                 startFrame;
    G3D::uint32                 endFrame;

    /** Used in Keyframe chunk */
    Matrix3                     currentRotation;
    Vector3                     currentScale;
    Vector3                     currentTranslation;
    Vector3                     currentPivot;

    BinaryInput*                b;

    /** Version number of the file */
    int fileVersion;
    int meshVersion;

protected:

    /**
     Reads the next chunk from the file and returns it.
     */
    ChunkHeader readChunkHeader();

    /**
     Reads a vector in the 3DS coordinate system and converts it to
     the G3D coordinate system.
     */
    Vector3 read3DSVector();

    /**
     Read either of the 3DS color chunk types and return the result.
     */
    Color3 read3DSColor();

    /** Read a percentage chunk */
    float read3DSPct();

    /**
     Reads (and ignores) TCB information from a track part of 
     a keyframe chunk.
     */
    void readTCB();

    /**
     The translation and scale information in a keyframe is packed
     with additional interpolation information.  This reads all of
     it, then throws away everything except the 3D vector.
     */
    Vector3 readLin3Track();
    Matrix3 readRotTrack();

    /**
     Reads the next chunk from a file and processes it.
     */
    void processChunk(const ChunkHeader& prevChunkHeader);

    /** Called from processChunk */
    void processMaterialChunk
    (UniversalMaterial&                   material,
     const ChunkHeader&          materialChunkHeader);

    /** Called from processMaterialChunk */
    void processMapChunk
    (Map&                        map,
     const ChunkHeader&          materialChunkHeader);

    /** Called from processChunk */
    void processObjectChunk
    (Object&                     object,
     const ChunkHeader&          objectChunkHeader);

    /** Called from processObjectChunk */
    void processTriMeshChunk
    (Object&                     object,
     const ChunkHeader&          objectChunkHeader);

public:

    void parse(BinaryInput& bi, const String& basePath = "<AUTO>");

};


} // namespace G3D


#endif // G3D_Parse3DS_h
