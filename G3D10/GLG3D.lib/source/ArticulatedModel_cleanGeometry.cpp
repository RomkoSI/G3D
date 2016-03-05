/**
 \file GLG3D/source/ArticulatedModel_cleanGeometry.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-18
 \edited  2015-09-03
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/Stopwatch.h"
#include "G3D/AreaMemoryManager.h"
#include "GLG3D/ArticulatedModel.h"
#include "G3D/FastPointHashGrid.h"

namespace G3D {
    
void ArticulatedModel::cleanGeometry(const CleanGeometrySettings& settings) {
    for (int g = 0; g < m_geometryArray.size(); ++g) {
        m_geometryArray[g]->cleanGeometry(settings, m_meshArray);
        debugAssertM((m_geometryArray[g]->cpuVertexArray.size() == 0) ||
            ! m_geometryArray[g]->cpuVertexArray.vertex[0].normal.isNaN(),
            "Undefined normal remained after cleanGeometry");
    }
}


void ArticulatedModel::computeBounds() {
    Array<Mesh*> affectedMeshes;
    
    // N.B. we loop through the mesh array for every geometry. 
    // This could be ameliorated with a table, though that might be slower in practice
    for (int g = 0; g < m_geometryArray.size(); ++g) {
        m_geometryArray[g]->getAffectedMeshes(m_meshArray, affectedMeshes);
        m_geometryArray[g]->computeBounds(affectedMeshes);
        affectedMeshes.fastClear();
    }
}


void ArticulatedModel::Geometry::getAffectedMeshes(const Array<Mesh*>& fullMeshArray, Array<Mesh*>& affectedMeshes) {
    for (int i = 0; i < fullMeshArray.size(); ++i) {
        if (fullMeshArray[i]->geometry == this) {
            affectedMeshes.append(fullMeshArray[i]);
        }
    }
}


static void generateFaceArray(Array<ArticulatedModel::Geometry::Face>& faceArray, const Array<ArticulatedModel::Mesh*>& affectedMeshes, const CPUVertexArray& cpuVertexArray) {
    int triangleCount = 0;
    for (int i = 0; i < affectedMeshes.size(); ++i) {
        triangleCount += affectedMeshes[i]->triangleCount();
    }
    faceArray.reserve(triangleCount);
    for (int m = 0; m < affectedMeshes.size(); ++m) {
        ArticulatedModel::Mesh* mesh = affectedMeshes[m];
        const Array<int>& indexArray = mesh->cpuIndexArray;

        // For every indexed triangle, create a Face
        for (int i = 0; i < indexArray.size(); i += 3) {
            ArticulatedModel::Geometry::Face& face = faceArray.next();
            face.mesh = mesh;

            // Copy each vertex, updating the adjacency table
            for (int v = 0; v < 3; ++v) {
                int index = indexArray[i + v];
                face.vertex[v] = ArticulatedModel::Geometry::Face::Vertex(cpuVertexArray.vertex[index], index);

                // Copy texCoord1s as well, if they exist
                if (cpuVertexArray.hasTexCoord1) {
                    face.vertex[v].texCoord1 = cpuVertexArray.texCoord1[index];
                }

                if (cpuVertexArray.hasVertexColors) {
                    face.vertex[v].vertexColor = cpuVertexArray.vertexColors[index];
                }

                if (cpuVertexArray.hasBones) {
                    face.vertex[v].boneWeights = cpuVertexArray.boneWeights[index];
                    face.vertex[v].boneIndices = cpuVertexArray.boneIndices[index];
                }
            }
            // Compute the non-unit and unit face normals
            face.normal = 
                (face.vertex[1].position - face.vertex[0].position).cross(
                    face.vertex[2].position - face.vertex[0].position);

            face.unitNormal = face.normal.directionOrZero();
        }
    }
}


static bool closeEnough(const CPUVertexArray::Vertex& v0, const CPUVertexArray::Vertex& v1, 
            const float positionEpsilon, const float normalAngleEpsilon, const float texCoordEpsilon) {
    bool positionCloseEnough    = (v0.position - v1.position).squaredMagnitude() <= positionEpsilon;
    bool normalCloseEnough      = (v0.normal - v1.normal).squaredMagnitude() <= normalAngleEpsilon;
    bool texCoordCloseEnough    = (v0.texCoord0 - v1.texCoord0).squaredLength() <= texCoordEpsilon;
    return positionCloseEnough && normalCloseEnough && texCoordCloseEnough;
}


class VertexIndexPair {
public:
    int index;
    CPUVertexArray::Vertex vertex;
    VertexIndexPair() : index(-1) {}
    VertexIndexPair(const CPUVertexArray::Vertex& v, int i) : index(i), vertex(v) {}
};


class VertexIndexPairPosFunc {
public:
    static void getPosition(const VertexIndexPair& d, Vector3& pos) {
        pos = d.vertex.position;
    }
};

typedef FastPointHashGrid<VertexIndexPair, VertexIndexPairPosFunc> PointGrid;

static void getMergeMapping(Array<int>& oldToNewIndexMapping, CPUVertexArray& newVertexArray, const CPUVertexArray& oldVertexArray, 
        const float positionEpsilon, const float normalAngleEpsilon, const float texCoordEpsilon) {
    oldToNewIndexMapping.resize(oldVertexArray.size());
    fprintf(stderr, "Beginning merge mapping\n");
    PointGrid hashGrid(positionEpsilon, 4);

    for (int i = 0; i < oldVertexArray.size(); ++i) {
        const CPUVertexArray::Vertex& v = oldVertexArray.vertex[i];
        bool foundMatch = false;
        int thisIndex = newVertexArray.size();
        
        for (PointGrid::SphereIterator it = hashGrid.begin(Sphere(v.position, positionEpsilon)); it.isValid(); ++it) {
            const CPUVertexArray::Vertex& potentialMatch = it->vertex;
            
            if (closeEnough(v, potentialMatch, positionEpsilon, normalAngleEpsilon, texCoordEpsilon)) {
                thisIndex = it->index;
                foundMatch = true;
            }
        }

        if (! foundMatch) {
            VertexIndexPair viPair(v, thisIndex);
            hashGrid.insert(viPair);
            newVertexArray.vertex.append(v);
            if (newVertexArray.hasTexCoord1) {
                newVertexArray.texCoord1.append(oldVertexArray.texCoord1[i]);
            }

            if (newVertexArray.hasVertexColors) {
                newVertexArray.vertexColors.append(oldVertexArray.vertexColors[i]);
            }

            if (newVertexArray.hasBones) {
                newVertexArray.boneIndices.append(oldVertexArray.boneIndices[i]);
                newVertexArray.boneWeights.append(oldVertexArray.boneWeights[i]);
            }
        }

        if (i % 100000 == 0) {
            fprintf(stderr, "Processed %d vertices\n", i);
            fprintf(stderr, "Compressed to %d vertices\n", newVertexArray.size());
        }
        oldToNewIndexMapping[i] = thisIndex;
    }

    fprintf(stderr, "Finished merge mapping\n");
}


static ArticulatedModel::Geometry::Face::Vertex lerpVertices(const ArticulatedModel::Geometry::Face::Vertex& v0, const ArticulatedModel::Geometry::Face::Vertex& v1, const float alpha = 0.5f) {
    ArticulatedModel::Geometry::Face::Vertex newVertex;
    newVertex.normal    = lerp(v0.normal,       v1.normal,      alpha).direction();
    newVertex.position  = lerp(v0.position,     v1.position,    alpha);
    newVertex.tangent   = Vector4(fnan(), fnan(), fnan(), fnan());
    newVertex.texCoord0 = lerp(v0.texCoord0,    v1.texCoord0,   alpha);
    newVertex.texCoord1 = Point2unorm16(lerp(v0.texCoord1,    v1.texCoord1,   alpha));
    newVertex.vertexColor = lerp(v0.vertexColor,    v1.vertexColor,   alpha);
    // TODO: don't ignore bones
    return newVertex;
}


static void subdivideSingleFaceUntilThreshold(const ArticulatedModel::Geometry::Face& face, Array<ArticulatedModel::Geometry::Face>& subdividedSingleFace, const float edgeLengthThreshold) {
    float edgeLength[3];
    float maxEdgeLength = 0.0f;
    int maxEdgeIndex = -1;
    static int nextIndex[3] = {1,2,0};
    for (int i = 0; i < 3; ++i) {
        edgeLength[i] = fabs((face.vertex[i].position - face.vertex[nextIndex[i]].position).length());
        if (maxEdgeLength < edgeLength[i]) {
            maxEdgeIndex    = i;
            maxEdgeLength   = edgeLength[i];
        }
    }

    const int i0 = maxEdgeIndex;
    const int i1 = nextIndex[maxEdgeIndex];
    const int i2 = nextIndex[i1];
    //       i2
    //       /|\
    //      / | \
    //     /  |  \
    //    /___|___\
    //  i0  midV   i1
    //
    if ( maxEdgeLength > edgeLengthThreshold ) {
        ArticulatedModel::Geometry::Face::Vertex midV = lerpVertices(face.vertex[i0], face.vertex[i1]);
        ArticulatedModel::Geometry::Face newFace0(face.mesh, face.vertex[i0], midV, face.vertex[i2]);
        ArticulatedModel::Geometry::Face newFace1(face.mesh, face.vertex[i2], midV, face.vertex[i1]);
        // Recursively subdivide
        subdivideSingleFaceUntilThreshold(newFace0, subdividedSingleFace, edgeLengthThreshold);
        subdivideSingleFaceUntilThreshold(newFace1, subdividedSingleFace, edgeLengthThreshold);
    } else {
        subdividedSingleFace.append(face); // base case
    }    
}


void ArticulatedModel::Geometry::subdivideUntilThresholdEdgeLength
   (const Array<Mesh*>& affectedMeshes,
    const float edgeLengthThreshold, 
    const float positionEpsilon,
    const float normalAngleEpsilon,
    const float texCoordEpsilon) {

    Array<Face> faceArray;

    generateFaceArray(faceArray, affectedMeshes, cpuVertexArray);

    // Clear all mesh index arrays
    for (int m = 0; m < affectedMeshes.size(); ++m) {
        Mesh* mesh = affectedMeshes[m];
        mesh->cpuIndexArray.fastClear();
        mesh->gpuIndexArray = IndexStream();
    }

    // Clear the CPU vertex array
    cpuVertexArray.vertex.fastClear();

    cpuVertexArray.texCoord1.fastClear();
    cpuVertexArray.vertexColors.fastClear();
    cpuVertexArray.boneIndices.fastClear();
    cpuVertexArray.boneWeights.fastClear();

    Array<Face> fullySubdividedFaceArray;
    Array<Face> subdividedSingleFace;
    fullySubdividedFaceArray.reserve(faceArray.size() * 2);
    for (int i = 0; i < faceArray.size(); ++i) {
        const Face& face = faceArray[i];
        subdividedSingleFace.fastClear();
        subdivideSingleFaceUntilThreshold(face, subdividedSingleFace, edgeLengthThreshold);
        fullySubdividedFaceArray.append(subdividedSingleFace);
    }

    CPUVertexArray explodedVertexArray;
    explodedVertexArray.hasTexCoord1    = cpuVertexArray.hasTexCoord1;
    explodedVertexArray.hasVertexColors  = cpuVertexArray.hasVertexColors;
    explodedVertexArray.hasBones        = cpuVertexArray.hasBones;
    
    // Iterate over all faces
    for (int f = 0; f < fullySubdividedFaceArray.size(); ++f) {
        const Face& face = fullySubdividedFaceArray[f];
        Mesh* mesh = face.mesh;
        for (int v = 0; v < 3; ++v) {
            const Face::Vertex& vertex = face.vertex[v];
            explodedVertexArray.vertex.append(vertex);

            if (explodedVertexArray.hasTexCoord1) {
                explodedVertexArray.texCoord1.append(vertex.texCoord1);
            }

            if (explodedVertexArray.hasVertexColors) {
                explodedVertexArray.vertexColors.append(vertex.vertexColor);
            }

            if (explodedVertexArray.hasBones) {
                explodedVertexArray.boneIndices.append(vertex.boneIndices);
                explodedVertexArray.boneWeights.append(vertex.boneWeights);
            }

        }
        const int vertexIndex = f * 3;
        mesh->cpuIndexArray.append(vertexIndex, vertexIndex + 1, vertexIndex + 2);
    }

    Array<int> oldIndicesToNewIndices;
    getMergeMapping(oldIndicesToNewIndices, cpuVertexArray, explodedVertexArray, positionEpsilon, normalAngleEpsilon, texCoordEpsilon);
    int sizeOfMergedVertexArray = 0;
    for(int i = 0; i < oldIndicesToNewIndices.size(); ++i) {
        sizeOfMergedVertexArray = max(sizeOfMergedVertexArray, oldIndicesToNewIndices[i]);
    }

    // Use merge mapping to write final index arrays
    for (int m = 0; m < affectedMeshes.size(); ++m) {
        Mesh* mesh = affectedMeshes[m];
        for (int i = 0; i < mesh->cpuIndexArray.size(); ++i) {
            mesh->cpuIndexArray[i] = oldIndicesToNewIndices[mesh->cpuIndexArray[i]];
        }
    }

    debugPrintf("Num vertices %d. Before compaction %d\n", cpuVertexArray.size(), explodedVertexArray.size());
}


void ArticulatedModel::Geometry::cleanGeometry(const CleanGeometrySettings& settings, const Array<Mesh*>& meshes) {
    Stopwatch timer;
    timer.setEnabled(false);
    clearAttributeArrays();
    // The meshes that use this geometry
    Array<Mesh*> affectedMeshes;
    getAffectedMeshes(meshes, affectedMeshes);
    if (isFinite(settings.maxEdgeLength)) {
        subdivideUntilThresholdEdgeLength(affectedMeshes, settings.maxEdgeLength);
    }

    if (settings.forceComputeNormals) {
        // Wipe the normal array
        for (int i = 0; i < cpuVertexArray.size(); ++i) {
            cpuVertexArray.vertex[i].normal = Vector3::nan();
        }
    }

    if (settings.forceComputeTangents) {
        // Wipe the tangent array
        for (int i = 0; i < cpuVertexArray.size(); ++i) {
            cpuVertexArray.vertex[i].tangent = Vector4::nan();
        }
    }

    bool computeSomeNormals  = settings.forceComputeNormals;
    bool computeSomeTangents = settings.forceComputeTangents;
    determineCleaningNeeds(computeSomeNormals, computeSomeTangents);
    timer.after("  determineCleaningNeeds");
    
    if (computeSomeNormals || (settings.forceVertexMerging && settings.allowVertexMerging)) {
        // Expand into an un-indexed triangle list.  This allows us to consider
        // each vertex's normal independently if needed.
        Array<Face> faceArray;
        Face::AdjacentFaceTable adjacentFaceTable;
        adjacentFaceTable.clearAndSetMemoryManager(AreaMemoryManager::create());

        buildFaceArray(faceArray, adjacentFaceTable, affectedMeshes);
        timer.after("  buildFaceArray");

        if (computeSomeNormals) {
            computeMissingVertexNormals(faceArray, adjacentFaceTable, settings.maxSmoothAngle);
            timer.after("  computeMissingVertexNormals");
        }
    
        // Merge vertices that have nearly equal normals, positions, and texcoords.
        // We no longer need adjacency information because tangents can be computed
        // solely from shared vertex information.
        if (settings.allowVertexMerging) {
            mergeVertices(faceArray, settings.maxNormalWeldAngle, affectedMeshes);
            timer.after("  mergeVertices");
        } else if (computeSomeNormals) {
            // Write the vertex normal data from the face array back to the vertex
            // array. This is needed because we aren't merging geometry.
            for (int f = 0; f < faceArray.size(); ++f) {
                const Face& face = faceArray[f];
                for (int v = 0; v < 3; ++v) {
                    const Face::Vertex& vertex = face.vertex[v];
                    cpuVertexArray.vertex[vertex.indexInSourceGeometry] = vertex;
                }
            }
        }
    }
    timer.after("  deallocation of adjacentFaceTable");

    if (computeSomeTangents) {
        // Compute tangent space
        computeMissingTangents(affectedMeshes);
        timer.after("  computeMissingTangents");
    }

    computeBounds(affectedMeshes);
}


void ArticulatedModel::Geometry::clearAttributeArrays() {
    gpuPositionArray    = AttributeArray();
    gpuNormalArray      = AttributeArray();
    gpuTexCoord0Array   = AttributeArray();
    gpuTangentArray     = AttributeArray();
    gpuTexCoord1Array   = AttributeArray();
    gpuVertexColorArray = AttributeArray();
    gpuBoneIndicesArray = AttributeArray();
    gpuBoneWeightsArray = AttributeArray();
}


void ArticulatedModel::Mesh::clearIndexStream() {
    gpuIndexArray = IndexStream();
}


void ArticulatedModel::Geometry::determineCleaningNeeds(bool& computeSomeNormals, bool& computeSomeTangents) {
    computeSomeTangents = false;
    computeSomeNormals = false;;

    // See if normals are needed
    for (int i = 0; i < cpuVertexArray.size(); ++i) {
        if (isNaN(cpuVertexArray.vertex[i].normal.x)) {
            computeSomeNormals = true;
            computeSomeTangents = true;
            // Wipe out the corresponding tangent vector
            cpuVertexArray.vertex[i].tangent.x = fnan();
        }
    }
    
    // See if tangents are needed
    if (! computeSomeTangents) {
        // Maybe there is a NaN tangent in there
        for (int i = 0; i < cpuVertexArray.size(); ++i) {
            if (isNaN(cpuVertexArray.vertex[i].tangent.x)) {
                computeSomeTangents = true;
                break;
            }
        }
    }    
}


void ArticulatedModel::Part::debugPrint() const {
    /*const Part* part = this;
    // Code for dumping the vertices
    debugPrintf("** Vertices:\n");
    for (int i = 0; i < part->cpuVertexArray.vertex.size(); ++i) {
        const CPUVertexArray::Vertex& vertex = part->cpuVertexArray.vertex[i];
        debugPrintf(" %d: %s %s %s %s\n", i, 
            vertex.position.toString().c_str(), vertex.normal.toString().c_str(),
            vertex.tangent.toString().c_str(), vertex.texCoord0.toString().c_str());
    }
    debugPrintf("\n");

    // Code for dumping the indices
    debugPrintf("** Indices:\n");
    for (int m = 0; m < part->m_meshArray.size(); ++m) {
        const Mesh* mesh = part->m_meshArray[m];
        debugPrintf(" Mesh %s\n", mesh->name.c_str());
        for (int i = 0; i < mesh->cpuIndexArray.size(); i += 3) {
            debugPrintf(" %d-%d: %d %d %d\n", i, i + 2, mesh->cpuIndexArray[i], mesh->cpuIndexArray[i + 1], mesh->cpuIndexArray[i + 2]);
        }
        debugPrintf("\n");
    }
    debugPrintf("\n");
    */
}


void ArticulatedModel::Geometry::computeBounds(const Array<Mesh*>& affectedMeshes) {
    const CPUVertexArray::Vertex* vertexArray = cpuVertexArray.vertex.getCArray();

    boxBounds = AABox::empty();

    // Iterate over the meshes, computing *their* bounds, and then accumulate them
    // for the geometry.  This is slower than just computing the part's bound, but is the
    // only way to get the meshes to have correct bounds as well.
    for (int m = 0; m < affectedMeshes.size(); ++m) {
        Mesh* mesh = affectedMeshes[m];
        const Array<int>& indexArray = mesh->cpuIndexArray;
        const int* index = indexArray.getCArray();

        AABox meshBounds;
        for (int i = 0; i < indexArray.size(); ++i) {            
            meshBounds.merge(vertexArray[index[i]].position);
        }

        mesh->boxBounds = meshBounds;
        meshBounds.getBounds(mesh->sphereBounds);
        boxBounds.merge(meshBounds);
    }

    boxBounds.getBounds(sphereBounds);
}


void ArticulatedModel::Geometry::computeMissingTangents(const Array<Mesh*> affectedMeshes) {

    if ( !cpuVertexArray.hasTexCoord0 ) {
        cpuVertexArray.hasTangent = false;
        // If we have no texture coordinates, we are unable to compute tangents.   
        for (int v = 0; v < cpuVertexArray.size(); ++v) {
            cpuVertexArray.vertex[v].tangent = Vector4::zero();
        }
        return;
    }

    cpuVertexArray.hasTangent = true;
    alwaysAssertM(cpuVertexArray.hasTexCoord0, "Cannot compute tangents without some texture coordinates.");
 
    // Compute all tangents, but only extract those that we need at the bottom.

    // See http://www.terathon.com/code/tangent.html for a derivation of the following code
    Array<Vector3>  tangent1;
    Array<Vector3>  tangent2;
    tangent1.resize(cpuVertexArray.size());
    tangent2.resize(cpuVertexArray.size());
    Vector3* tan1 = tangent1.getCArray();
    Vector3* tan2 = tangent2.getCArray();
    CPUVertexArray::Vertex* vertexArray = cpuVertexArray.vertex.getCArray();
    debugAssertM(tan1[0].x == 0, "This implementation assumes that new Vector3 values are initialized to zero.");

    // For each face
    for (int m = 0; m < affectedMeshes.size(); ++m) {
        const Mesh* mesh = affectedMeshes[m];        
        const Array<int>& cpuIndexArray = mesh->cpuIndexArray;
        const int* indexArray = cpuIndexArray.getCArray();

        for (int i = 0; i < cpuIndexArray.size(); i += 3) {
            const int i0 = indexArray[i];
            const int i1 = indexArray[i + 1];
            const int i2 = indexArray[i + 2];
        
            const CPUVertexArray::Vertex& vertex0 = vertexArray[i0];
            const CPUVertexArray::Vertex& vertex1 = vertexArray[i1];
            const CPUVertexArray::Vertex& vertex2 = vertexArray[i2];

            const Point3& v0 = vertex0.position;
            const Point3& v1 = vertex1.position;
            const Point3& v2 = vertex2.position;
        
            const Point2& w0 = vertex0.texCoord0;
            const Point2& w1 = vertex1.texCoord0;
            const Point2& w2 = vertex2.texCoord0;
        
            // triangle edge vectors
            const float x0 = v1.x - v0.x;
            const float x1 = v2.x - v0.x;
            const float y0 = v1.y - v0.y;
            const float y1 = v2.y - v0.y;
            const float z0 = v1.z - v0.z;
            const float z1 = v2.z - v0.z;
        
            // texcoord directional derivatives along triangle edge vectors
            const float s0 = w1.x - w0.x;
            const float s1 = w2.x - w0.x;
            const float t0 = w1.y - w0.y;
            const float t1 = w2.y - w0.y;
        
            const float r = 1.0f / (s0 * t1 - s1 * t0);
            
            const Vector3 sdir
                ((t1 * x0 - t0 * x1) * r, 
                 (t1 * y0 - t0 * y1) * r,
                 (t1 * z0 - t0 * z1) * r);

            const Vector3 tdir
                ((s0 * x1 - s1 * x0) * r, 
                 (s0 * y1 - s1 * y0) * r,
                 (s0 * z1 - s1 * z0) * r);
        
            tan1[i0] += sdir;
            tan1[i1] += sdir;
            tan1[i2] += sdir;
        
            tan2[i0] += tdir;
            tan2[i1] += tdir;
            tan2[i2] += tdir;        
        } // For each triangle
    } // For each mesh

    for (int v = 0; v < cpuVertexArray.size(); ++v) {
        CPUVertexArray::Vertex& vertex = vertexArray[v];

        if (isNaN(vertex.tangent.x)) {
            // This tangent needs to be overriden
            const Vector3& n = vertex.normal;
            const Vector3& t1 = tan1[v];
            const Vector3& t2 = tan2[v];
        
            // Gram-Schmidt orthogonalize
            const Vector3& T = (t1 - n * n.dot(t1)).directionOrZero();

            if ( T.isZero() ) {
                Vector3 tan1, tan2;
                n.direction().getTangents(tan1, tan2);
                const Vector3& tan = tan1.direction();
                vertex.tangent.x = tan.x;
                vertex.tangent.y = tan.y;
                vertex.tangent.z = tan.z;
            } else {
                vertex.tangent.x = T.x;
                vertex.tangent.y = T.y;
                vertex.tangent.z = T.z;
            }

            // Calculate handedness
            vertex.tangent.w = (n.cross(t1).dot(t2) < 0.0f) ? 1.0f : -1.0f;
        } // if this must be updated
    } // for each vertex
    
 }

 
void ArticulatedModel::Geometry::mergeVertices(const Array<Face>& faceArray, float maxNormalWeldAngle, const Array<Mesh*> affectedMeshes) {
    // Clear all mesh index arrays
    for (int m = 0; m < affectedMeshes.size(); ++m) {
        Mesh* mesh = affectedMeshes[m];
        mesh->cpuIndexArray.fastClear();
        mesh->gpuIndexArray = IndexStream();
    }

    // Clear the CPU vertex array
    cpuVertexArray.vertex.fastClear();
    cpuVertexArray.texCoord1.fastClear();
    cpuVertexArray.vertexColors.fastClear();
    cpuVertexArray.boneIndices.fastClear();
    cpuVertexArray.boneWeights.fastClear();

    Stopwatch timer;
    timer.setEnabled(false);

    // Track the location of vertices in cpuVertexArray by their exact texcoord and position.
    // The vertices in the list may have differing normals.
    typedef int VertexIndex;
    typedef SmallArray<VertexIndex, 4> VertexIndexList;
    Table<Face::Vertex, VertexIndexList, Face::AMFaceVertexHash, Face::AMFaceVertexHash> vertexIndexTable;

    // Almost all of the time in this method is spent deallocating the table at
    // the end, so use an AreaMemoryManager to directly dump the allocated memory
    // without freeing individual objects.
    vertexIndexTable.clearAndSetMemoryManager(AreaMemoryManager::create());

    // Conservative estimate of the size (overallocation here is bad for large models on low RAM systems (such as San Miguel on a standard 8GB RAM computer)
    vertexIndexTable.setSizeHint(faceArray.size() / 6); 

    const float normalClosenessThreshold = cos(maxNormalWeldAngle);

    // Iterate over all faces
    int longestListLength = 0;
    for (int f = 0; f < faceArray.size(); ++f) {
        const Face& face = faceArray[f];
        Mesh* mesh = face.mesh;
        int vertexIndex[3];
        for (int v = 0; v < 3; ++v) {
            const Face::Vertex& vertex = face.vertex[v];

            // Find the location of this vertex in cpuVertexArray...or add it.
            // The texture coordinates and vertices must exactly match.
            // The normals may be slightly off, since the order of computation can affect them
            // even if we wanted no normal welding.
            VertexIndexList& list = vertexIndexTable.getCreate(vertex);

            int index = -1;
            for (int i = 0; i < list.size(); ++i) {
                int j = list[i];
                // See if the normals are close (we know that the texcoords and positions match exactly)
                const Vector3& otherNormal = cpuVertexArray.vertex[j].normal;
                if ((otherNormal.dot(vertex.normal) >= normalClosenessThreshold) 
                    || otherNormal.isZero() || vertex.normal.isZero()) { 
                    // Reuse this vertex
                    index = j;
                    break;
                }
            }

            if (index == -1) {
                // This must be a new vertex, so add it
                index = cpuVertexArray.size();
                cpuVertexArray.vertex.append(vertex);
                if (cpuVertexArray.hasTexCoord1) {
                    cpuVertexArray.texCoord1.append(vertex.texCoord1);
                }
                if (cpuVertexArray.hasVertexColors) {
                    cpuVertexArray.vertexColors.append(vertex.vertexColor);
                }
                if (cpuVertexArray.hasBones) {
                    cpuVertexArray.boneIndices.append(vertex.boneIndices);
                    cpuVertexArray.boneWeights.append(vertex.boneWeights);
                }
                list.append(index);
                longestListLength = max(longestListLength, list.size());
            }

            // Add this vertex index to the mesh
            vertexIndex[v] = index;
        }

        // Add only non-degenerate triangles
        if ((vertexIndex[0] != vertexIndex[1]) && (vertexIndex[1] != vertexIndex[2]) && (vertexIndex[2] != vertexIndex[0])) {
            mesh->cpuIndexArray.append(vertexIndex[0], vertexIndex[1], vertexIndex[2]);
        }
    }

   // debugPrintf("average bucket size = %f\n", vertexIndexTable.debugGetAverageBucketSize());
   // debugPrintf("deepest bucket size = %d\n", vertexIndexTable.debugGetDeepestBucketSize());
   // debugPrintf("longestListLength = %d\n", longestListLength);
}


void ArticulatedModel::Geometry::computeMissingVertexNormals
 (Array<Face>&                      faceArray, 
  const Face::AdjacentFaceTable&    adjacentFaceTable, 
  const float                       maximumSmoothAngle) {

    const float smoothThreshold = cos(maximumSmoothAngle);

    // Compute vertex normals as needed
    for (int f = 0; f < faceArray.size(); ++f) {
        Face& face = faceArray[f];

        for (int v = 0; v < 3; ++v) {
            CPUVertexArray::Vertex& vertex = face.vertex[v];

            // Only process vertices with normals that have been flagged as NaN
            if (isNaN(vertex.normal.x)) {
                // This normal needs to be computed
                vertex.normal = Vector3::zero();
                const Face::IndexArray& faceIndexArray = adjacentFaceTable.get(vertex.position);

                // Did we arrive at this vertex by considering a denegerate face?
                if (face.unitNormal.isZero()) {
                    // This face has no normal (presumably this is a degenerate face formed by three collinear points), 
                    // so just average adjacent ones directly.
                    for (int i = 0; i < faceIndexArray.size(); ++i) {
                        vertex.normal += faceArray[faceIndexArray[i]].normal;
                    }

                    if (vertex.normal.isZero()) {
                        // All adjacent faces are degenerate--choose an arbitrary normal, since it won't matter.
                        vertex.normal = Vector3::unitY();
                    }

                } else {
                    // The face containing this vertex has a valid normal.  Consider all adjacent
                    // faces and the angles that they subtend around the vertex.
                    for (int i = 0; i < faceIndexArray.size(); ++i) {
                        const Face& adjacentFace = faceArray[faceIndexArray[i]];
                        const float cosAngle = face.unitNormal.dot(adjacentFace.unitNormal);

                        // Only process if within the cutoff angle
                        if (cosAngle >= smoothThreshold) {
                            // These faces are close enough to be considered part of a
                            // smooth surface.  Add the non-unit normal.
                            vertex.normal += adjacentFace.normal;
                        }
                    }

                    if (vertex.normal.isZero()) {
                        // The faces must have been exactly opposed.  Revert to the face's normal.
                        vertex.normal = face.unitNormal;
                    }
                }

                // Make the vertex normal unit length
                vertex.normal = vertex.normal.directionOrZero();
                debugAssertM(! vertex.normal.isNaN() && ! vertex.normal.isZero(),
                    "computeMissingVertexNormals() produced an illegal value--"
                    "the adjacent face normals were probably corrupt"); 
            }
        }
    }
}


void ArticulatedModel::Geometry::buildFaceArray
   (Array<Face>&             faceArray, 
    Face::AdjacentFaceTable& adjacentFaceTable, 
    const Array<Mesh*>&      affectedMeshes) {

    faceArray.fastClear();

    int triangleCount = 0;
    for (int i = 0; i < affectedMeshes.size(); ++i) {
        triangleCount += affectedMeshes[i]->triangleCount();
    }
    faceArray.reserve(triangleCount);
    adjacentFaceTable.setSizeHint(triangleCount / 2); // low ball estimate

    // Maps positions to the faces adjacent to that position.  The valence of the average vertex in a closed mesh is 6, so
    // allocate slightly more indices so that we rarely need to allocate extra heap space.

    for (int m = 0; m < affectedMeshes.size(); ++m) {
        Mesh* mesh = affectedMeshes[m];
        const Array<int>& indexArray = mesh->cpuIndexArray;

        // For every indexed triangle, create a Face
        for (int i = 0; i < indexArray.size(); i += 3) {
            const Face::Index faceIndex = faceArray.size();
            Face& face = faceArray.next();
            face.mesh = mesh;

            // Copy each vertex, updating the adjacency table
            for (int v = 0; v < 3; ++v) {
                int index = indexArray[i + v];
                face.vertex[v] = Face::Vertex(cpuVertexArray.vertex[index], index);

                // Copy texCoord1s as well, if they exist
                if (cpuVertexArray.hasTexCoord1) {
                    face.vertex[v].texCoord1 = cpuVertexArray.texCoord1[index];
                }
                if (cpuVertexArray.hasVertexColors) {
                    face.vertex[v].vertexColor = cpuVertexArray.vertexColors[index];
                }
                if (cpuVertexArray.hasBones) {
                    face.vertex[v].boneWeights = cpuVertexArray.boneWeights[index];
                    face.vertex[v].boneIndices = cpuVertexArray.boneIndices[index];
                }

                // Record that this face is next to this vertex
                adjacentFaceTable.getCreate(face.vertex[v].position).append(faceIndex);
            }
            

            // Compute the non-unit and unit face normals
            face.normal = 
                (face.vertex[1].position - face.vertex[0].position).cross(
                    face.vertex[2].position - face.vertex[0].position);

            face.unitNormal = face.normal.directionOrZero();
        }
    }
}

} // namespace G3D
