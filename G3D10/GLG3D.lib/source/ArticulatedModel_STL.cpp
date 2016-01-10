/**
 \file GLG3D/source/ArticulatedModel_STL.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2013-01-03
 \edited  2013-01-03
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/


#include "GLG3D/ArticulatedModel.h"
#ifndef DISABLE_STL
#include "G3D/FileSystem.h"

namespace G3D {

/**
Example (from http://orion.math.iastate.edu/burkardt/data/stl/stl.html)

solid cube_corner
        facet normal 0.0 -1.0 0.0
        outer loop
            vertex 0.0 0.0 0.0
            vertex 1.0 0.0 0.0
            vertex 0.0 0.0 1.0
        endloop
        endfacet
        ...
endsolid
*/
static void parseASCII
   (const ArticulatedModel::Specification&  specification, 
    String&                            name, 
    Array<Point3>&                          vertexArray) {

    TextInput ti(specification.filename);

    ti.readSymbol("solid");
    name = trimWhitespace(ti.readUntilNewlineAsString());
    if (name.empty()) {
        name = "root";
    }

    while (true) {
        Token t = ti.read();
        if (t.type() != Token::SYMBOL) {
            throw ParseError(ti.filename(), t.line(), t.character(), "Expected a symbol");
        }

        const String& s = t.string();
        if (s == "facet") {
            // Parse one triangle

            // Maybe read the normal
            String v = ti.readSymbol();
            if (v == "normal") {
                // Read (and ignore) the normal
                for (int i = 0; i < 3; ++i) { ti.readNumber(); }
                v = ti.readSymbol();
            }

            if (v != "outer") {
                throw ParseError(ti.filename(), t.line(), t.character(), "Expected 'outer', found " + v);
            }
            ti.readSymbol("loop");

            for (int i = 0; i < 3; ++i) {
                ti.readSymbol("vertex");
                Point3& vertex = vertexArray.next();
                for (int a = 0; a < 3; ++a) {
                    vertex[a] = float(ti.readNumber());
                }
            }

            ti.readSymbol("endloop");

            ti.readSymbol("endfacet");

        } else if (s == "endsolid") {
            // Done
            break;
        } else {
            throw ParseError(ti.filename(), t.line(), t.character(), "Illegal symbol: " + s);
        }
    } // while true
} // parseASCII


/**
 Specification at 
 http://orion.math.iastate.edu/burkardt/data/stl/stl.html
 http://en.wikipedia.org/wiki/STL_(file_format)

 A binary STL file has the following structure: 

An 80 byte ASCII header that can be used as a title. 
A 4 byte unsigned long integer, the number of facets. 
For each facet, a facet record of 50 bytes. 

The facet record has the form: 

The normal vector, 3 floating values of 4 bytes each; 
Vertex 1 XYZ coordinates, 3 floating values of 4 bytes each; 
Vertex 2 XYZ coordinates, 3 floating values of 4 bytes each; 
Vertex 3 XYZ coordinates, 3 floating values of 4 bytes each; 
An unsigned integer, of 2 bytes, that should be zero; 
*/
static void parseBinary
   (const ArticulatedModel::Specification&  specification, 
    String&                            name, 
    Array<Point3>&                          vertexArray) {

    BinaryInput* bi = new BinaryInput(specification.filename, G3D_LITTLE_ENDIAN);

    name = trimWhitespace(bi->readFixedLengthString(80));
    if (beginsWith(name, "solid ")) {
        // This is actually an ASCII file.  We could pass the existing BinaryInput
        // to the TextInput...so long as the file is not so large that it has to
        // be read in chunks.  For simplicity, we simply close and re-open the file.
        delete bi;
        bi = NULL;
        parseASCII(specification, name, vertexArray);
        return;
    }

    const uint32 numFacets = bi->readUInt32();
    alwaysAssertM(numFacets < 1e8, "Unreasonable number of facets");

    vertexArray.resize(numFacets * 3);
    int n = 0;
    for (uint32 i = 0; i < numFacets; ++i) {
        bi->readVector3(); // Ignore the normal
        for (int v = 0; v < 3; ++v) {
            vertexArray[n].deserialize(*bi);
            ++n;
        }
        bi->skip(2);
    }

    delete bi;
    bi = NULL;
}


void ArticulatedModel::loadSTL(const Specification& specification) {
    const String& ext = toLower(FilePath::ext(specification.filename));

    // PArse
    Array<Point3> positionArray;
    String partName;
    if (ext == "stla") {
        parseASCII(specification, partName, positionArray);
    } else {
        parseBinary(specification, partName, positionArray);
    }

    // Create the geometry
    Part* part = addPart(partName);
    Geometry* geom = addGeometry(partName + "_geom");

    Array<CPUVertexArray::Vertex>& vertex = geom->cpuVertexArray.vertex;
    vertex.resize(positionArray.size());
    for (int v = 0; v < vertex.size(); ++v) {
        CPUVertexArray::Vertex& vtx = vertex[v];
        vtx.position  = positionArray[v];
        vtx.normal    = Vector3::nan();
        vtx.texCoord0 = Vector2::zero();
        vtx.tangent   = Vector4::nan();
    }
    geom->cpuVertexArray.hasTangent   = false;
    geom->cpuVertexArray.hasTexCoord0 = false;
    geom->cpuVertexArray.hasTexCoord1 = false;

    Mesh* mesh = addMesh("mesh", part, geom);
    MeshAlg::createIndexArray(vertex.size(), mesh->cpuIndexArray);
    mesh->twoSided = false;
    mesh->primitive = PrimitiveType::TRIANGLES;
    mesh->material = UniversalMaterial::createDiffuse(Color3::one() * 0.99);
}

} // namespace G3D
#endif //DISABLE_STL
