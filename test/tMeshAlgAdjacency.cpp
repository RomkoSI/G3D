#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testAdjacency() {
    printf("MeshAlg::computeAdjacency\n");

    {
        //          2
        //        /|
        //       / |
        //      /  |
        //     /___|
        //    0     1
        //
        //


        MeshAlg::Geometry       geometry;
        Array<int>              index;
        Array<MeshAlg::Face>    faceArray;
        Array<MeshAlg::Edge>    edgeArray;
        Array<MeshAlg::Vertex>  vertexArray;

        geometry.vertexArray.append(Vector3(0,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));
        geometry.vertexArray.append(Vector3(1,1,0));

        index.append(0, 1, 2);

        MeshAlg::computeAdjacency(
            geometry.vertexArray,
            index,
            faceArray,
            edgeArray,
            vertexArray);

        testAssert(faceArray.size() == 1);
        testAssert(edgeArray.size() == 3);

        testAssert(faceArray[0].containsVertex(0));
        testAssert(faceArray[0].containsVertex(1));
        testAssert(faceArray[0].containsVertex(2));

        testAssert(faceArray[0].containsEdge(0));
        testAssert(faceArray[0].containsEdge(1));
        testAssert(faceArray[0].containsEdge(2));

        testAssert(edgeArray[0].inFace(0));
        testAssert(edgeArray[1].inFace(0));
        testAssert(edgeArray[2].inFace(0));

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        // Severely weld, creating a degenerate face
        MeshAlg::weldAdjacency(geometry.vertexArray, faceArray, edgeArray, vertexArray, 1.1f);
        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);
        testAssert(! faceArray[0].containsVertex(0));
    }

    {
        //      
        //    0====1
        //  (degenerate face)
        //

        MeshAlg::Geometry       geometry;
        Array<int>              index;
        Array<MeshAlg::Face>    faceArray;
        Array<MeshAlg::Edge>    edgeArray;
        Array<MeshAlg::Vertex>  vertexArray;

        geometry.vertexArray.append(Vector3(0,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));

        index.append(0, 1, 0);

        MeshAlg::computeAdjacency(
            geometry.vertexArray,
            index,
            faceArray,
            edgeArray,
            vertexArray);

        testAssert(faceArray.size() == 1);
        testAssert(edgeArray.size() == 2);

        testAssert(faceArray[0].containsVertex(0));
        testAssert(faceArray[0].containsVertex(1));

        testAssert(faceArray[0].containsEdge(0));
        testAssert(faceArray[0].containsEdge(1));

        testAssert(edgeArray[0].inFace(0));
        testAssert(edgeArray[1].inFace(0));
        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);
    }

    {
        //          2                       .
        //        /|\                       .
        //       / | \                      .
        //      /  |  \                     . 
        //     /___|___\                    . 
        //    0     1    3
        //
        //


        MeshAlg::Geometry       geometry;
        Array<int>              index;
        Array<MeshAlg::Face>    faceArray;
        Array<MeshAlg::Edge>    edgeArray;
        Array<MeshAlg::Vertex>  vertexArray;

        geometry.vertexArray.append(Vector3(0,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));
        geometry.vertexArray.append(Vector3(1,1,0));
        geometry.vertexArray.append(Vector3(2,0,0));

        index.append(0, 1, 2);
        index.append(1, 3, 2);

        MeshAlg::computeAdjacency(
            geometry.vertexArray,
            index,
            faceArray,
            edgeArray,
            vertexArray);

        testAssert(faceArray.size() == 2);
        testAssert(edgeArray.size() == 5);
        testAssert(vertexArray.size() == 4);

        testAssert(faceArray[0].containsVertex(0));
        testAssert(faceArray[0].containsVertex(1));
        testAssert(faceArray[0].containsVertex(2));

        testAssert(faceArray[1].containsVertex(3));
        testAssert(faceArray[1].containsVertex(1));
        testAssert(faceArray[1].containsVertex(2));

        // The non-boundary edge must be first
        testAssert(! edgeArray[0].boundary());
        testAssert(edgeArray[1].boundary());
        testAssert(edgeArray[2].boundary());
        testAssert(edgeArray[3].boundary());
        testAssert(edgeArray[4].boundary());

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        MeshAlg::weldAdjacency(geometry.vertexArray, faceArray, edgeArray, vertexArray);

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        testAssert(faceArray.size() == 2);
        testAssert(edgeArray.size() == 5);
        testAssert(vertexArray.size() == 4);
    }


    {
        // Test Welding


        //         2                  .
        //        /|\                 .
        //       / | \                .
        //      /  |  \               .
        //     /___|___\              .
        //    0   1,4   3
        //


        MeshAlg::Geometry       geometry;
        Array<int>              index;
        Array<MeshAlg::Face>    faceArray;
        Array<MeshAlg::Edge>    edgeArray;
        Array<MeshAlg::Vertex>  vertexArray;

        geometry.vertexArray.append(Vector3(0,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));
        geometry.vertexArray.append(Vector3(1,1,0));
        geometry.vertexArray.append(Vector3(2,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));

        index.append(0, 1, 2);
        index.append(2, 4, 3);

        MeshAlg::computeAdjacency(
            geometry.vertexArray,
            index,
            faceArray,
            edgeArray,
            vertexArray);

        testAssert(faceArray.size() == 2);
        testAssert(edgeArray.size() == 6);
        testAssert(vertexArray.size() == 5);

        testAssert(edgeArray[0].boundary());
        testAssert(edgeArray[1].boundary());
        testAssert(edgeArray[2].boundary());
        testAssert(edgeArray[3].boundary());
        testAssert(edgeArray[4].boundary());
        testAssert(edgeArray[5].boundary());

        testAssert(faceArray[0].containsVertex(0));
        testAssert(faceArray[0].containsVertex(1));
        testAssert(faceArray[0].containsVertex(2));

        testAssert(faceArray[1].containsVertex(2));
        testAssert(faceArray[1].containsVertex(3));
        testAssert(faceArray[1].containsVertex(4));

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        MeshAlg::weldAdjacency(geometry.vertexArray, faceArray, edgeArray, vertexArray);

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        testAssert(faceArray.size() == 2);
        testAssert(edgeArray.size() == 5);
        testAssert(vertexArray.size() == 5);

        testAssert(! edgeArray[0].boundary());
        testAssert(edgeArray[1].boundary());
        testAssert(edgeArray[2].boundary());
        testAssert(edgeArray[3].boundary());
        testAssert(edgeArray[4].boundary());
    }
    {
        // Test Welding


        //        2,5 
        //        /|\         . 
        //       / | \        .
        //      /  |  \       .
        //     /___|___\      .
        //    0   1,4   3
        //


        MeshAlg::Geometry       geometry;
        Array<int>              index;
        Array<MeshAlg::Face>    faceArray;
        Array<MeshAlg::Edge>    edgeArray;
        Array<MeshAlg::Vertex>  vertexArray;

        geometry.vertexArray.append(Vector3(0,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));
        geometry.vertexArray.append(Vector3(1,1,0));
        geometry.vertexArray.append(Vector3(2,0,0));
        geometry.vertexArray.append(Vector3(1,0,0));
        geometry.vertexArray.append(Vector3(1,1,0));

        index.append(0, 1, 2);
        index.append(5, 4, 3);

        MeshAlg::computeAdjacency(
            geometry.vertexArray,
            index,
            faceArray,
            edgeArray,
            vertexArray);

        testAssert(faceArray.size() == 2);
        testAssert(edgeArray.size() == 6);
        testAssert(vertexArray.size() == 6);

        testAssert(edgeArray[0].boundary());
        testAssert(edgeArray[1].boundary());
        testAssert(edgeArray[2].boundary());
        testAssert(edgeArray[3].boundary());
        testAssert(edgeArray[4].boundary());
        testAssert(edgeArray[5].boundary());

        testAssert(faceArray[0].containsVertex(0));
        testAssert(faceArray[0].containsVertex(1));
        testAssert(faceArray[0].containsVertex(2));

        testAssert(faceArray[1].containsVertex(5));
        testAssert(faceArray[1].containsVertex(3));
        testAssert(faceArray[1].containsVertex(4));

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        MeshAlg::weldAdjacency(geometry.vertexArray, faceArray, edgeArray, vertexArray);

        MeshAlg::debugCheckConsistency(faceArray, edgeArray, vertexArray);

        testAssert(faceArray.size() == 2);
        testAssert(edgeArray.size() == 5);
        testAssert(vertexArray.size() == 6);

        testAssert(! edgeArray[0].boundary());
        testAssert(edgeArray[1].boundary());
        testAssert(edgeArray[2].boundary());
        testAssert(edgeArray[3].boundary());
        testAssert(edgeArray[4].boundary());

    }
    
}
