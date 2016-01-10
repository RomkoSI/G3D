/**
 \file G3D/source/ParsePLY.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/ParsePLY.h"
#include "G3D/BinaryInput.h"
#include "G3D/FileSystem.h"
#include "G3D/stringutils.h"
#include "G3D/ParseError.h"

namespace G3D {
    
ParsePLY::ParsePLY() : vertexData(NULL), faceArray(NULL), triStripArray(NULL) {}


void ParsePLY::clear() {
    delete[] vertexData;
    vertexData = NULL;
    delete[] faceArray;
    faceArray = NULL;
    delete[] triStripArray;
    triStripArray = NULL;
    numVertices = numFaces = numTriStrips = 0;
}


ParsePLY::~ParsePLY() {
    clear();
}


void ParsePLY::parse(BinaryInput& bi) {
    const G3DEndian oldEndian = bi.endian();

    clear();
    readHeader(bi);

    vertexData = new float[numVertices * vertexProperty.size()];
    faceArray = new Face[numFaces];
    triStripArray = new TriStrip[numTriStrips];

    readVertexList(bi);
    readFaceList(bi);

    bi.setEndian(oldEndian);
}


ParsePLY::DataType ParsePLY::parseDataType(const char* t) {
    static const char* names[] = {"char", "uchar", "short", "ushort", "int", "uint", "float", "double", "list", NULL};

    for (int i = 0; names[i] != NULL; ++i) {
        if (strcmp(t, names[i]) == 0) {
            return DataType(i);
        }
    }

    throw String("Illegal type specifier: ") + t;
    return none_type;
}


void ParsePLY::parseProperty(const String& s, Property& prop) {
    // Examples:
    //
    // property float x
    // property list uchar int vertex_index

    char temp[100], name[100];

    sscanf(s.c_str(), "%*s %100s", temp);
    prop.type = parseDataType(temp);

    if (prop.type == list_type) {
        char temp2[100];
        // Read the index and element types
        sscanf(s.c_str(), "%*s %*s %100s %100s %100s", temp, temp2, name);
        prop.listLengthType = parseDataType(temp);
        prop.listElementType = parseDataType(temp2);
    } else {
        sscanf(s.c_str(), "%*s %*s %100s", name);
    }

    prop.name = name;
}


size_t ParsePLY::byteSize(DataType d) {
    const size_t sz[] = {1, 1, 2, 2, 4, 4, 4, 8};
    alwaysAssertM((int)d >= 0 && (int)d < 8, "Illegal data type");
    return sz[d];
}


void ParsePLY::readHeader(BinaryInput& bi) {
    const String& hdr = bi.readStringNewline();
    if (hdr != "ply") {
        throw ParseError(bi.getFilename(), bi.getPosition(), format("Bad PLY header: \"%s\"", hdr.c_str()));
    }

    const String& fmt = bi.readStringNewline();
    
    if (fmt == "format binary_little_endian 1.0") {
        // Default format, nothing to do
        bi.setEndian(G3D_LITTLE_ENDIAN);
    } else if (fmt == "format binary_big_endian 1.0") {
        // Flip the endian
        bi.setEndian(G3D_BIG_ENDIAN);
    } else if (fmt == "format ascii 1.0") {
        throw ParseError(bi.getFilename(), bi.getPosition(), "ASCII PLY format is not supported in this release.");
    } else {
        throw ParseError(bi.getFilename(), bi.getPosition(), "Unsupported PLY format: " + fmt);
    }

    // Set to true when these fields are read
    bool readVertex = false;
    bool readFace = false;

    String s =  bi.readStringNewline();
    while (s != "end_header") {
        if (beginsWith(s, "comment ")) {

            // Ignore this line
            s = bi.readStringNewline();

        } if (beginsWith(s, "element vertex ")) {
            if (readVertex) {
                throw String("Already defined vertex.");
            }

            // Read the properties
            sscanf(s.c_str(), "%*s %*s %d", &numVertices);

            s = bi.readStringNewline();
            while (beginsWith(s, "property ")) {
                parseProperty(s, vertexProperty.next());
                s = bi.readStringNewline();
            }

            readVertex = true;

        } else if (beginsWith(s, "element face ") || beginsWith(s, "element tristrips ")) {
            if (! readVertex) {
                throw String("This implementation only supports faces and tristrips following vertices.");
            }

            if (readFace) {
                throw String("Already defined faces.");
            }

            // Read the properties
            if (beginsWith(s, "element tristrips ")) {
                sscanf(s.c_str(), "%*s %*s %d", &numTriStrips);
            } else {
                sscanf(s.c_str(), "%*s %*s %d", &numFaces);
            }

            s = bi.readStringNewline();
            while (beginsWith(s, "property ")) {
                parseProperty(s, faceOrTriStripProperty.next());
                s = bi.readStringNewline();
            }

            readFace = true;

        } else {
            throw String("Unsupported PLY2 header command: ") + s;
        }
    }
}


template<class T>
static T readAs(ParsePLY::DataType type, BinaryInput& bi) {
    switch (type) {
    case ParsePLY::char_type:
        return (T)bi.readInt8();

    case ParsePLY::uchar_type:
        return (T)bi.readUInt8();

    case ParsePLY::short_type:
        return (T)bi.readInt16();

    case ParsePLY::ushort_type:
        return (T)bi.readUInt16();

    case ParsePLY::int_type:
        return (T)bi.readInt32();

    case ParsePLY::uint_type:
        return (T)bi.readUInt32();

    case ParsePLY::float_type:
        return (T)bi.readFloat32();

    case ParsePLY::double_type:
        return (T)bi.readFloat64();

    case ParsePLY::list_type:
        throw String("Tried to read a list as a value type");
        return (T)0;

    case ParsePLY::none_type:
        throw String("Tried to read an undefined type as a value type");
        return (T)0;
    }
    return (T)0;
}


float ParsePLY::readAsFloat(const Property& prop, BinaryInput& bi) {
    switch (prop.type) {
    case char_type:
    case uchar_type:
    case short_type:
    case ushort_type:
    case int_type:
    case uint_type:
    case float_type:
    case double_type:
        return readAs<float>(prop.type, bi);

    case list_type:
        {
            // Consume the list values
            const int n = readAs<int>(prop.listLengthType, bi);
            for (int i = 0; i < n; ++i) {
                readAs<float>(prop.listElementType, bi);
            }
        }
        return 0.0f;

    case none_type:
        throw String("Tried to read an undefined property");
        return 0.0f;
    };

    return 0.0f;
}


void ParsePLY::readVertexList(BinaryInput& bi) {
    const int N = vertexProperty.size();
    int i = 0;
    for (int v = 0; v < numVertices; ++v) {
        for (int p = 0; p < N; ++p) {
            vertexData[i] = readAsFloat(vertexProperty[p], bi);
            ++i;
        }
    }
}


void ParsePLY::readFaceList(BinaryInput& bi) {
    // How many properties are there before and after
    // the vertex_index list?
    int numBefore = 0, numAfter = faceOrTriStripProperty.size() - 2;

    bool found = false;
    for (int p = 0; p < faceOrTriStripProperty.size(); ++p) {
        if ((faceOrTriStripProperty[p].name == "vertex_index") || 
            (faceOrTriStripProperty[p].name == "vertex_indices")) {
            found = true;
        } else {
            ++numBefore;
            --numAfter;
        }
    }

    if (! found) {
        throw ParseError(bi.getFilename(), bi.getPosition(), "No vertex_index or vertex_indices property on faces in this PLY file");
    }

    // Only one of these is nonzero
    const int num = max(numFaces, numTriStrips);

    for (int f = 0; f < num; ++f) {
        int p = 0;
        // Ignore properties before.  Each one might contain lists and therefore
        // have variable length, so we actually have to parse this data even
        // though we throw it away.
        for (int i = 0; i < numBefore; ++i) {
            (void)readAsFloat(faceOrTriStripProperty[p], bi);
            ++p;
        }

        // Now read the index list
        const Property& prop = faceOrTriStripProperty[p];
        const int len = readAs<int>(prop.listLengthType, bi);

        if (numFaces > 0) {
            // Read one face
            Face& face = faceArray[f];
            for (int i = 0; i < len; ++i) {
                const int index = readAs<int>(prop.listElementType, bi);
                debugAssert(index >= 0 && index < numVertices);
                face.append(index);
            }
        } else {
            // Read one tristrip
            TriStrip& triStrip = triStripArray[f];
            for (int i = 0; i < len; ++i) {
                const int index = readAs<int>(prop.listElementType, bi);
                debugAssert(index >= -1 && index < numVertices);  // -1 = "restart tristrip"
                triStrip.append(index);
            }
        }

        // Ignore properties after
        for (int i = 0; i < numAfter; ++i) {
            (void)readAsFloat(faceOrTriStripProperty[p], bi);
            ++p;
        }
    }
}

} // G3D

