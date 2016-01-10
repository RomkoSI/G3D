/**
 \file GLG3D/source/ArticulatedModel_PLY.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/


#include "GLG3D/ArticulatedModel.h"
#ifndef DISABLE_PLY
#include "G3D/ParsePLY.h"
#include "G3D/FileSystem.h"
#include "G3D/MeshAlg.h"

namespace G3D {

void ArticulatedModel::loadPLY(const Specification& specification) {

    // Read the data in
    Part*       part = addPart(m_name);
    Geometry*   geom = addGeometry("geom");
    Mesh*       mesh = addMesh("mesh", part, geom);
    mesh->material = UniversalMaterial::create();
    
    ParsePLY parseData;
    {
        BinaryInput bi(specification.filename, G3D_LITTLE_ENDIAN);
        parseData.parse(bi);
    }

    // Convert the format
    
    geom->cpuVertexArray.vertex.resize(parseData.numVertices);
    geom->cpuVertexArray.hasTangent = false;
    geom->cpuVertexArray.hasTexCoord0 = false;
     
    // The PLY format is technically completely flexible, so we have
    // to search for the location of the X, Y, and Z fields within each
    // vertex.
    int axisIndex[3];
    const String axisName[3] = {"x", "y", "z"};
    const int numVertexProperties = parseData.vertexProperty.size();
    for (int a = 0; a < 3; ++a) {
        axisIndex[a] = 0;
        for (int p = 0; p < numVertexProperties; ++p) {
            if (parseData.vertexProperty[p].name == axisName[a]) {
                axisIndex[a] = p;
                break;
            }
        }
    }

    for (int v = 0; v < parseData.numVertices; ++v) {
        CPUVertexArray::Vertex& vertex = geom->cpuVertexArray.vertex[v];

        // Read the position
        for (int a = 0; a < 3; ++a) {
            vertex.position[a] = parseData.vertexData[v * numVertexProperties + axisIndex[a]];
        }

        // Flag the normal as undefined 
        vertex.normal.x = fnan();
    }

    if (parseData.numFaces > 0) {
        // Read faces
        for (int f = 0; f < parseData.numFaces; ++f) {
            const ParsePLY::Face& face = parseData.faceArray[f];
        
            // Read and tessellate into triangles, assuming convex polygons
            for (int i = 2; i < face.size(); ++i) {
                mesh->cpuIndexArray.append(face[0], face[1], face[i]);
            }
        }

    } else {
        // Read tristrips
        for (int f = 0; f < parseData.numTriStrips; ++f) {
            const ParsePLY::TriStrip& triStrip = parseData.triStripArray[f];

            // Convert into an indexed triangle list and append to the end of the 
            // index array.
            bool clockwise = false;
            for (int i = 2; i < triStrip.size(); ++i) {
                if (triStrip[i] == -1) {
                    // Restart
                    clockwise = false;
                    // Skip not only this element, but the next two
                    i += 2;
                } else if (clockwise) {  // clockwise face
                    debugAssert(triStrip[i - 1] >= 0 && triStrip[i - 2] >= 0  && triStrip[i] >= 0);
                    mesh->cpuIndexArray.append(triStrip[i - 1], triStrip[i - 2], triStrip[i]);
                    clockwise = ! clockwise;
                } else { // counter-clockwise face
                    debugAssert(triStrip[i - 1] >= 0 && triStrip[i - 2] >= 0  && triStrip[i] >= 0);
                    mesh->cpuIndexArray.append(triStrip[i - 2], triStrip[i - 1], triStrip[i]);
                    clockwise = ! clockwise;
                }
            }
        }
    }
}

} // namespace G3D
#endif //DISABLE_PLY
