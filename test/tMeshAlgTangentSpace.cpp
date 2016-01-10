#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testMeshAlgTangentSpace() {
    printf("MeshAlg::computeTangentSpaceBasis ");

    MeshAlg::Geometry geometry;
    Array<MeshAlg::Face> face;
    Array<MeshAlg::Edge> edge;
    Array<MeshAlg::Vertex> vertex;

	Array<Vector2> texCoord;
    Array<Vector3> tangent;
    Array<Vector3> binormal;
	Array<int>	   index;

    // Create a quad
	float s = 2.0f;
	geometry.vertexArray.append(Vector3(-s, -s, 0));
	geometry.vertexArray.append(Vector3( s, -s, 0));
	geometry.vertexArray.append(Vector3( s,  s, 0));
	geometry.vertexArray.append(Vector3(-s,  s, 0));

    texCoord.append(Vector2(0, 1));
	texCoord.append(Vector2(1, 1));
	texCoord.append(Vector2(1, 0));
	texCoord.append(Vector2(0, 0));

	index.append(0, 1, 2);
	index.append(0, 2, 3);

    MeshAlg::computeAdjacency(geometry.vertexArray, index, face, edge, vertex);
    MeshAlg::computeNormals(geometry, index);

    for (int i = 0; i < 4; ++i) {
        testAssert(geometry.normalArray[i].fuzzyEq(Vector3::unitZ()));
    }

    MeshAlg::computeTangentSpaceBasis(geometry.vertexArray, texCoord, geometry.normalArray, face, tangent, binormal);

    for (int i = 0; i < 4; ++i) {
        testAssert(tangent[i].fuzzyEq(Vector3::unitX()));
        testAssert(binormal[i].fuzzyEq(Vector3::unitY()));
    }

    printf("passed\n");
}
