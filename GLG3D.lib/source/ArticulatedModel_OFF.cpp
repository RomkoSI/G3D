/**
 \file GLG3D/source/ArticulatedModel_OFF.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2012-07-24
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/


#include "GLG3D/ArticulatedModel.h"
#ifndef DISABLE_OFF
#include "G3D/FileSystem.h"

namespace G3D {

// There is no "ParseOFF" because OFF parsing is trivial--it has no subparts or materials,
// and is directly an indexed format.
void ArticulatedModel::loadOFF(const Specification& specification) {
    
    Part* part      = addPart(m_name);
    Geometry* geom  = addGeometry("geom");
    Mesh* mesh      = addMesh("mesh", part, geom);
    mesh->material = UniversalMaterial::create();
    
    TextInput::Settings s;
    s.cppBlockComments = false;
    s.cppLineComments = false;
    s.otherCommentCharacter = '#';
    TextInput ti(specification.filename, s);           
    
    ///////////////////////////////////////////////////////////////
    // Parse header
    String header = ti.readSymbol();
    bool hasTexCoords = false;
    bool hasColors = false;
    bool hasNormals = false;
    bool hasHomogeneous = false;
    bool hasHighDimension = false;

    if (beginsWith(header, "ST")) {
        hasTexCoords = true;
        header = header.substr(2);
    }
    if (beginsWith(header, "C")) {
        hasColors = true;
        header = header.substr(1);
    }
    if (beginsWith(header, "N")) {
        hasNormals = true;
        header = header.substr(1);
    }
    if (beginsWith(header, "4")) {
        hasHomogeneous = true;
        header = header.substr(1);
    }
    if (beginsWith(header, "n")) {
        hasHighDimension = true;
        header = header.substr(1);
    }
    
    geom->cpuVertexArray.hasTexCoord0   = hasTexCoords;
    geom->cpuVertexArray.hasTangent     = false;

    // Remaining header should be "OFF", but is not required according to the spec

    Token t = ti.peek();
    if ((t.type() == Token::SYMBOL) && (t.string() == "BINARY")) {
        throw String("BINARY OFF files are not supported by this version of G3D::ArticulatedModel");
    }

    int ndim = 3;
    if (hasHighDimension) {
        ndim = int(ti.readNumber());
    }
    if (hasHomogeneous) {
        ++ndim;
    }

    if (ndim < 3) {
        throw String("OFF files must contain at least 3 dimensions");
    }

    int nV = iFloor(ti.readNumber());
    int nF = iFloor(ti.readNumber());
    int nE = iFloor(ti.readNumber());
    (void)nE;

    ///////////////////////////////////////////////////

    Array<int>& index = mesh->cpuIndexArray;

    geom->cpuVertexArray.vertex.resize(nV);
    
    // Read the per-vertex data
    for (int v = 0; v < nV; ++v) {
        CPUVertexArray::Vertex& vertex = geom->cpuVertexArray.vertex[v];

        // Position 
        for (int i = 0; i < 3; ++i) {
            vertex.position[i] = float(ti.readNumber());
        }
        
        // Ignore higher dimensions
        for (int i = 3; i < ndim; ++i) {
            (void)ti.readNumber();
        }

        if (hasNormals) {
            // Normal (assume always 3 components)
            for (int i = 0; i < 3; ++i) {
                vertex.normal[i] = float(ti.readNumber());
            }
        } else {
            vertex.normal.x = fnan();
        }

        if (hasColors) {
            // Color (assume always 3 components)
            for (int i = 0; i < 3; ++i) {
                ti.readNumber();
            }
        }

        if (hasTexCoords) {
            // Texcoords (assume always 2 components)
            for (int i = 0; i < 2; ++i) {
                vertex.texCoord0[i] = float(ti.readNumber());
            }
        }
        // Skip to the end of the line.  If the file was corrupt we'll at least get the next vertex right
        ti.readUntilNewlineAsString();
    }

    // Faces

    // Convert arbitrary triangle fans to triangles
    Array<int> poly;
    for (int i = 0; i < nF; ++i) {
        poly.fastClear();
        int polySize = iFloor(ti.readNumber());
        debugAssert(polySize > 2);

        if (polySize == 3) {
            // Triangle (common case)
            for (int j = 0; j < 3; ++j) {
                index.append(iFloor(ti.readNumber()));
            }
        } else {
            poly.resize(polySize);
            for (int j = 0; j < polySize; ++j) {
                poly[j] = iFloor(ti.readNumber());
                debugAssertM(poly[j] < nV, 
                             "OFF file contained an index greater than the number of vertices."); 
            }

            // Expand the poly into triangles
            MeshAlg::toIndexedTriList(poly, PrimitiveType::TRIANGLE_FAN, index);
        }

        // Trim to the end of the line, except on the last line of the
        // file (where it doesn't matter)
        if (i != nF - 1) {
            // Ignore per-face colors
            ti.readUntilNewlineAsString();
        }
    }
}

} // namespace G3D
#endif //DISABLE_OFF
