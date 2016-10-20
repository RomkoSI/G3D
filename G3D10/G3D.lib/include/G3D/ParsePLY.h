/**
 \file G3D/ParsePLY.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2011-07-23
 \edited  2011-07-23

 Copyright 2002-2011, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_ParsePLY_h
#define G3D_ParsePLY_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "G3D/SmallArray.h"
#include "G3D/Table.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"

namespace G3D {

class BinaryInput;

/** \brief Parses PLY geometry files to extract face and vertex information.

The input file is required to contain only vertex and (face or triStrip) elements, in that order.
Each may have any number of properties.

\cite http://paulbourke.net/dataformats/ply/

\sa G3D::ParseMTL, G3D::ParseOBJ, G3D::ArticulatedModel
*/
class ParsePLY {
public:

    /**
    The order must be maintained

    char       character                 1
    uchar      unsigned character        1
    short      short integer             2
    ushort     unsigned short integer    2
    int        integer                   4
    uint       unsigned integer          4
    float      single-precision float    4
    double     double-precision float    8
    */
    enum DataType {
        char_type,
        uchar_type,
        short_type,
        ushort_type,
        int_type,
        uint_type,
        float_type,
        double_type,
        list_type,
        none_type};

    static DataType parseDataType(const char* t);
    static size_t byteSize(DataType d);

    class Property {
    public:
        DataType        type;

        String     name;

        /** Only used for type = LIST */
        DataType        listLengthType;

        /** Only used for type = LIST */
        DataType        listElementType;

        Property() : type(none_type) {}
    };

    typedef SmallArray<int, 6> Face;

    /** A -1 inside the triStrip means "restart" */
    typedef Array<int> TriStrip;
    
    int             numVertices;
    int             numFaces;
    int             numTriStrips;

    Array<Property> vertexProperty;

    /** Face or tristrip properties */
    Array<Property> faceOrTriStripProperty;    

    /** 
        vertexData[v*vertexPropertyArray.size() + p] is a float representing property p
        for vertex v.  If property p is a list type,
        the value is zero.
    */
    float*          vertexData;

    /** Only one of faceArray and triStripArray will be non-NULL */
    Face*           faceArray;

    /** Only one of faceArray and triStripArray will be non-NULL */
    TriStrip*       triStripArray;

private:

    static void parseProperty(const String& s, Property& prop);
    static float readAsFloat(const Property& prop, BinaryInput& bi);

    void readHeader(BinaryInput& bi);
    void readVertexList(BinaryInput& bi);
    void readFaceList(BinaryInput& bi);

public:
    
    ParsePLY();

    void clear();

    ~ParsePLY();

    void parse(BinaryInput& bi);

};


} // namespace G3D




#endif // G3D_ParsePLY_h
