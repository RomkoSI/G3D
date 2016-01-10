/**
 \file GLG3D/source/ArticulatedModel_PLY2.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-23
 \edited  2011-07-23
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/


#include "GLG3D/ArticulatedModel.h"
#ifndef DISABLE_PLY2
#include "G3D/FileSystem.h"

namespace G3D {

// There is no "ParsePLY2" because PLY2 parsing is trivial--it has no subparts or materials,
// and is directly an indexed format.
void ArticulatedModel::loadPLY2(const Specification& specification) {
    
    Part*       part = addPart(m_name);
    Geometry*   geom = addGeometry("geom");
    Mesh*       mesh = addMesh("mesh", part, geom);
    mesh->material = UniversalMaterial::create();
    
    TextInput ti(specification.filename);
        
    const int nV = iFloor(ti.readNumber());
    const int nF = iFloor(ti.readNumber());
    
    geom->cpuVertexArray.vertex.resize(nV);
    mesh->cpuIndexArray.resize(3 * nF);
    geom->cpuVertexArray.hasTangent = false;
    geom->cpuVertexArray.hasTexCoord0 = false;
    
        
    for (int i = 0; i < nV; ++i) {
        geom->cpuVertexArray.vertex[i].normal = Vector3::nan();
        Vector3& v = geom->cpuVertexArray.vertex[i].position;
        for (int a = 0; a < 3; ++a) {
            v[a] = float(ti.readNumber());
        }
    }
                
    for (int i = 0; i < nF; ++i) {
        const int three = ti.readInteger();
        alwaysAssertM(three == 3, "Ill-formed PLY2 file");
        for (int j = 0; j < 3; ++j) {
            mesh->cpuIndexArray[3*i + j] = ti.readInteger();
        }
    }
}

} // namespace G3D
#endif //DISABLE_PLY2
