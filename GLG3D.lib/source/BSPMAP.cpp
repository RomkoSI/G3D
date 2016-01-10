/**
 \file BSPMAP.cpp
    
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2003-05-22
 \edited  2012-07-29
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */ 

#include "GLG3D/BSPMAP.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Camera.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

namespace _BSPMAP {

BitSet::BitSet(): size(0), bits(NULL) {
}

BitSet::~BitSet() {
    System::alignedFree(bits);
}

void BitSet::resize(int count) {
    size = 0;
    // Delete any previous bits
    System::alignedFree(bits);

    size = iCeil(count / 32.0);

    bits = (G3D::uint32*)System::alignedMalloc(sizeof(G3D::uint32) * size, 16);
    clearAll();
}


Map::Map(): 
    lightVolumesCount(0),
    lightVolumes(NULL) {
    
    visData.clustersCount      = 0;
    visData.bytesPerCluster    = 0;
    visData.bitsets            = NULL;
    
    static const uint8 half[]  = {128, 128, 128, 128}; 
    static const uint8* arry[] = {half};
    

    defaultLightMap =
        Texture::fromMemory("Default Light Map", arry, ImageFormat::RGB8(), 1, 1, 1, 1,
                            ImageFormat::RGB8(), Texture::DIM_2D);
}


Map::~Map() {
    delete lightVolumes;
    delete visData.bitsets;
    
    faceArray.invokeDeleteOnAllElements();
}


int Map::findLeaf(const Vector3& pt) const {
    
    double distance = 0;
    int    index    = 0;

    while (index >= 0) {
        const BSPNode& node   = nodeArray[index];
        const BSPPlane& plane = planeArray[node.plane];

        // Distance from point to a plane
        distance = plane.normal.dot(pt) - plane.distance;

        if (distance >= 0) {
            index = node.front;
        } else {
            index = node.back;
        }
    }

    return -(index + 1);
}



void Map::checkCollision(Vector3& pos, Vector3& vel, const Vector3& extent) {
    if (vel.squaredLength() > 0) {
        collide(pos, vel, extent);
    }
}


void Map::slideCollision(Vector3& pos, Vector3& vel, const Vector3& extent) {

    if (vel.squaredLength() == 0) {
        return;
    }
    
    Vector3 startPos  = pos;
    Vector3 startVel  = vel;
    Vector3 normalPos = pos;
    Vector3 normalVel = vel;

    slide(normalPos, normalVel, extent);
    
    Vector3 up = startPos;

    // For going up stairs
    const double STEP_SIZE = 22 * 0.03; // G3D_LOAD_SCALE
    up.y += (float)STEP_SIZE;
    Vector3 up2 = up;
    BSPCollision collision = checkMove(up, up2, extent);
    
    if (collision.isSolid)  {
        pos = normalPos;
        return;
    }

    Vector3 upVel = startVel;
    collide(up, upVel, extent);
    Vector3 tmp = up;
    Vector3 down = up;
    down.y -= (float)STEP_SIZE;
    collision = checkMove(up, down, extent);

    if (! collision.isSolid) {
        tmp = collision.end;
    }

    up = tmp;
    float downStep = (normalPos.xz() - startPos.xz()).squaredLength();
    float upStep = (up.xz() - startPos.xz()).squaredLength();
    const float MIN_STEP_NORMAL = 0.7f;
    
    if ((downStep > upStep) || (collision.normal.y < MIN_STEP_NORMAL)) {
        vel = normalVel;
        pos = normalPos;
    } else {
        pos = up;
    }
}


void Map::collide(Vector3& pos, Vector3& vel, const Vector3& extent) {
    BSPCollision collision;
    collision.fraction = 0;
    Vector3 planeArray[5];
    float fractionLeft = 1;
    Vector3 dir;
    int planesCount = 0;
    
    for (int plane = 0; plane < 4; ++plane) {
        fractionLeft -= collision.fraction;
        Vector3 startPoint = pos;
        Vector3 endPoint = startPoint+vel;
        collision = checkMove(startPoint,endPoint,extent);
        
        if (collision.isSolid) {
            vel.y = 0;
            return;
        }

        if (collision.fraction > 0) {
            pos = collision.end;
            planesCount = 0;
            if (collision.fraction == 1) {
                break;
            }
        }

        if (planesCount >= 5) {
            break;
        }

        planeArray[planesCount] = collision.normal.direction();
        ++planesCount;
    }
}


void Map::slide(Vector3& pos, Vector3& vel, const Vector3& extent) {
    BSPCollision collision;
    collision.fraction = 0;
    Vector3 initVel = vel;
    Vector3 planeArray[5];

    float fractionLeft = 1.0f;

    Vector3 dir;
    int planesCount = 0;
    
    for (int plane = 0; plane < 4; ++plane) {
        
        fractionLeft -= collision.fraction;
        Vector3 startPoint = pos;
        Vector3 endPoint = startPoint + vel;
        collision = checkMove(startPoint, endPoint, extent);

        if (collision.isSolid) {
            vel.y = 0;
            return;
        }

        if (collision.fraction > 0) {
            pos = collision.end;
            planesCount = 0;
            if (collision.fraction == 1) {
                break;
            }
        }

        if (planesCount >= 5) {
            break;
        }

        planeArray[planesCount] = collision.normal.direction();
        ++planesCount;

        int i, j;
        for (i = 0; i < planesCount; ++i) {
            
            clipVelocity(vel, planeArray[i], vel, 1.01f);
            
            for (j = 0; j < planesCount; ++j) {
                if (j != i) {
                    if (vel.dot(planeArray[j]) < 0) {
                        break;
                    }
                }
            }

            if (j == planesCount) {
                break;
            }
        }

        if (i != planesCount) {

            clipVelocity(vel, planeArray[0], vel, 1.01f);

        } else {

            if (planesCount != 2) {
                vel = Vector3::zero();
                break;
            }

            dir = planeArray[0].cross(planeArray[1]);
            vel = dir * dir.dot(vel);
        }

        if (vel.dot(initVel) <= 0) {
            // We've run into a corner and are trying to slide away from
            // the intended movement direction; force velocity to zero.
            vel = Vector3::zero();
            break;
        }
    }
}


BSPCollision Map::checkMove(Vector3& start, Vector3& end, const Vector3& extent) {

    BSPCollision moveCollision;
    moveCollision.size      = extent;
    moveCollision.start     = start;
    moveCollision.end       = end;
    moveCollision.normal    = Vector3::zero();
    moveCollision.fraction  = 1;
    moveCollision.isSolid   = false;

    checkMoveNode(0, 1, start, end, 0, &moveCollision);

    if (moveCollision.fraction < 1.0f) {
        moveCollision.end = (start + (end - start) * (moveCollision.fraction));
    } else {
        moveCollision.end = end;
    }

    return moveCollision;
}


void Map::checkMoveNode(
    float               start,
    float               end,
    Vector3             startPos,
    Vector3             endPos,
    int                 node,
    BSPCollision*       moveCollision) const {
    
    if (moveCollision->fraction <= start) {
        return;
    }

    if (node < 0) {
        // The node index is really a leaf index
        checkMoveLeaf(~node, moveCollision);
        return;
    }

    int plane = nodeArray[node].plane;
    float t1 = planeArray[plane].normal.dot(startPos) - planeArray[plane].distance;
    float t2 = planeArray[plane].normal.dot(endPos) - planeArray[plane].distance;
    float offset =
        fabs(moveCollision->size.x*planeArray[plane].normal.x)+
        fabs(moveCollision->size.y*planeArray[plane].normal.y)+
        fabs(moveCollision->size.z*planeArray[plane].normal.z);

    if ((t1 >= offset) && (t2 >= offset)) {

        checkMoveNode(start, end, startPos, endPos, nodeArray[node].front, moveCollision);
        return;

    } else if ((t1 < -offset) && (t2 < -offset)) {
        
        checkMoveNode(start, end, startPos, endPos, nodeArray[node].back, moveCollision);
        return;
    }

    float frac;
    float frac2;
    const float DIST_EPSILON = 1.0f / 32;
    int frontNode, backNode;

    if (t1 < t2) {
        float invDist = 1 / (t1 - t2);

        backNode    = nodeArray[node].front;
        frontNode   = nodeArray[node].back;
        frac        = (t1 - offset - DIST_EPSILON) * invDist;
        frac2       = (t1 + offset + DIST_EPSILON) * invDist;

    } else if (t1 > t2) {
        float invDist = 1 / (t1 - t2);

        backNode    = nodeArray[node].back;
        frontNode   = nodeArray[node].front;
        frac        = (t1 + offset + DIST_EPSILON) * invDist;
        frac2       = (t1 - offset - DIST_EPSILON) * invDist;

    } else {

        backNode    = nodeArray[node].back;
        frontNode   = nodeArray[node].front;
        frac        = 1;
        frac2       = 0;
    }

    frac  = clamp(frac, 0.0f,  1.0f);
    frac2 = clamp(frac2, 0.0f, 1.0f);

    float mid = start + (end - start) * frac;
    Vector3 midPos = startPos + (endPos - startPos) * frac;

    checkMoveNode(start, mid, startPos, midPos, frontNode, moveCollision);

    mid    = start + (end - start) * frac2;
    midPos = startPos + (endPos - startPos) * frac2;

    checkMoveNode(mid, end, midPos, endPos, backNode, moveCollision);
}


void Map::checkMoveLeaf(int leaf, BSPCollision* moveCollision) const {
    int firstBrush = leafArray[leaf].firstBrush;

    for (int ct = 0; ct < leafArray[leaf].brushesCount; ++ct) {
        int brushIndex = leafBrushArray[firstBrush + ct];
        
        const Brush* brush = &brushArray[brushIndex];
        clipBoxToBrush(brush, moveCollision);
        
        if (! moveCollision->fraction) {
            return;
        }
    }
}


void Map::clipBoxToBrush(
    const Brush*        brush,
    BSPCollision*       moveCollision) const {

    if ((!textureIsHollow.isOn(brush->textureID)) ||
        (brush->brushSidesCount == 0)) {

        return;
    }

    const float DIST_EPSILON = 1.0f/32;
    float enter         = -1;
    float exit          = 1;
    bool startOut       = false;
    bool endOut         = false;
    int firstBrushSide  = brush->firstBrushSide;
    Vector3 hitNormal(0,0,0);
    
    for (int ct = 0; ct < brush->brushSidesCount; ++ct) {
        int planeIndex = brushSideArray[firstBrushSide+ct].plane;
        const BSPPlane& plane = planeArray[planeIndex];
        Vector3 offsets = moveCollision->size*(-1);
        
        if (plane.normal.x < 0) {
            offsets.x = -offsets.x;
        }

        if (plane.normal.y < 0) {
            offsets.y = -offsets.y;
        }

        if (plane.normal.z < 0) {
            offsets.z = -offsets.z;
        }

        float dist = plane.distance-offsets.dot(plane.normal);
        float d1 = moveCollision->start.dot(plane.normal)-dist;
        float d2 = moveCollision->end.dot(plane.normal)-dist;
        
        if (d1 > 0) {
            startOut = true;
        }

        if (d2 > 0) {
            endOut = true;
        }

        if ((d1 > 0) && (d2 >= d1)) {
            return;
        }

        if ((d1 <= 0) && (d2 <= 0)) {
            continue;
        }

        float f;
        if (d1 > d2) {
            f = (d1 - DIST_EPSILON) / (d1 - d2);
            if (f > enter) {
                enter = f;
                hitNormal = plane.normal;
            }
        } else {
            f = (d1 + DIST_EPSILON) / (d1 - d2);
            if (f < exit) {
                exit = f;
            }
        }
    }

    if (!startOut) {
        moveCollision->isSolid = !endOut;
        return;
    }

    if (enter < exit) {
        if ((enter > -1) && (enter < moveCollision->fraction)) {
            if (enter < 0) {
                enter = 0;
            }
            moveCollision->fraction = enter;
            moveCollision->normal = hitNormal;
        }
    }
}


void Map::clipVelocity(
    const Vector3&      in,
    const Vector3&      planeNormal,
    Vector3&            out,
    float               overbounce) const {

    const float STOP_EPSILON = 0.1f;
    
    out = in - planeNormal * (in.dot(planeNormal) * overbounce);
    
    if (fabs(out.x) < STOP_EPSILON) {
        out.x = 0;
    }

    if (fabs(out.y) < STOP_EPSILON) {
        out.y = 0;
    }

    if (fabs(out.z) < STOP_EPSILON) {
        out.z = 0;
    }
}


void Patch::Bezier2D::tessellate(int L) {
    level = L;

    // The number of vertices along a side is 1 + num edges
    const int L1 = L + 1;

    vertex.resize(L1 * L1);

    // Compute the vertices
    int i;

    for (i = 0; i <= L; ++i) {
        double a = (double)i / L;
        double b = 1 - a;

        vertex[i] = 
            controls[0] * float(b * b) + 
            controls[3] * float(2 * b * a) +
            controls[6] * float(a * a);
    }

    for (i = 1; i <= L; ++i) {
        double a = (double)i / L;
        double b = 1.0 - a;

        Vertex temp[3];

        int j;
        for (j = 0; j < 3; ++j) {
            int k = 3 * j;
            temp[j] = 
                controls[k + 0] * float(b * b) + 
                controls[k + 1] * float(2 * b * a) +
                controls[k + 2] * float(a * a);
        }

        for(j = 0; j <= L; ++j) {
            double a = (double)j / L;
            double b = 1.0 - a;

            vertex[i * L1 + j]= 
                temp[0] * float(b * b) + 
                temp[1] * float(2 * b * a) +
                temp[2] * float(a * a);
        }
    }

    // Compute the indices
    int row;
    indexes.resize(L * (L + 1) * 2);

    for (row = 0; row < L; ++row) {
        for(int col = 0; col <= L; ++col)    {
            indexes[(row * (L + 1) + col) * 2 + 1] = row       * L1 + col;
            indexes[(row * (L + 1) + col) * 2]     = (row + 1) * L1 + col;
        }
    }

    trianglesPerRow.resize(L);
    rowIndexes.resize(L);
    for (row = 0; row < L; ++row) {
        trianglesPerRow[row] = 2 * L1;
        rowIndexes[row]      = &indexes[row * 2 * L1];
    }
    
}


void Patch::updateSortKey(
    class Map*      map,
    const Vector3&  zAxis,
    Vector3&        origin) {

    if (bezierArray.size() > 0) {
        sortKey = (bezierArray[0].controls[0].position - origin).dot(zAxis);
    }
}
	

void Mesh::updateSortKey(
    class Map*      map,
    const Vector3&  zAxis,
    Vector3&        origin) {
    sortKey = (map->vertexArray[firstVertex].position - origin).dot(zAxis);
}


void Map::getTriangles(
    Array<Vector3>&     outVertexArray,
    Array<Vector3>&     outNormalArray,
    Array<int>&         outIndexArray,
    Array<Vector2>&     outTexCoordArray,
    Array<int>&         outTextureMapIndexArray,
    Array<Vector2>&     outLightCoordArray,
    Array<int>&         outLightMapIndexArray,
    Array<shared_ptr<Texture> >&  outTextureMapArray,
    Array<shared_ptr<Texture> >&  outLightMapArray) const {

    // Copy the textures
    outLightMapArray = lightMaps;
    outTextureMapArray = textures;

    // Resize the output arrays
    const int n = vertexArray.size();
    outVertexArray.resize(n);
    outLightCoordArray.resize(n);
    outTexCoordArray.resize(n);
    outNormalArray.resize(n);

    // Spread the input over the output arrays
    for (int i = 0; i < vertexArray.size(); ++i) {
        const Vertex& vertex  = vertexArray[i];

        outVertexArray[i]     = vertex.position;
        outLightCoordArray[i] = vertex.lightMapCoord;
        outTexCoordArray[i]   = vertex.textureCoord;
        outNormalArray[i]     = vertex.normal;
    }

    // Extract the indices
    for (int f = 0; f < faceArray.size(); ++f) {
        // Only render map faces (not entity faces), which have valid light maps
        if (faceArray[f]->lightMapID >= 0) {
            switch (faceArray[f]->type()) {
            case FaceSet::MESH:
                {
                    const Mesh* mesh = static_cast<const Mesh*>(faceArray[f]);
                    for (int t = 0; t < mesh->meshVertexesCount / 3; ++t) {
                        outLightMapIndexArray.append(mesh->lightMapID);
                        outTextureMapIndexArray.append(mesh->textureID);
                        debugAssert(mesh->lightMapID >= 0);

                        for (int v = 0; v < 3; ++v) {
                            // Compute the index into the index array
                            // Wind backwards
                            int i = mesh->firstMeshVertex + t * 3 + (2 - v);
                            // Compute the index into the vertex array
                            int index = meshVertexArray[i] + mesh->firstVertex;

                            outIndexArray.append(index);
                        }
                    }
                }
                break;

            case FaceSet::BILLBOARD:
                break;

            case FaceSet::PATCH:
                // Add new vertices at the end of the array
                {

                    const Patch* patch = static_cast<const Patch*>(faceArray[f]);
                    for (int b = 0; b < patch->bezierArray.size(); ++b) {
                        const Patch::Bezier2D& bezier = patch->bezierArray[b];

                        // Offet for bezier vertex indices
                        int index0 = outVertexArray.size();

                        // Append bezier's vertices to the array
                        for (int v = 0; v < bezier.vertex.size(); ++v) {
                            const Vertex& vertex = bezier.vertex[v];
                            outVertexArray.append(vertex.position);
                            outNormalArray.append(vertex.normal);
                            outLightCoordArray.append(vertex.lightMapCoord);
                            outTexCoordArray.append(vertex.textureCoord);
                        }

                        for (int row = 0; row < bezier.level; ++row) {
                            // This is a triangle strip.  Track every three vertices
                            // and alternate winding directions.
                            int v0 = bezier.rowIndexes[row][0];
                            int v1 = bezier.rowIndexes[row][1];
                            for (int v = 2; v < bezier.trianglesPerRow[row]; ++v) {
                                int v2 = bezier.rowIndexes[row][v];

                                outLightMapIndexArray.append(patch->lightMapID);
                                outTextureMapIndexArray.append(patch->textureID);

                                if (isOdd(v)) {
                                    outIndexArray.append(v0 + index0, v1 + index0, v2 + index0);
                                } else {
                                    outIndexArray.append(v2 + index0, v1 + index0, v0 + index0);
                                }

                                // Shift
                                v0 = v1;
                                v1 = v2;
                            }
                        }
                    }
                }
                break;

            default:
                debugAssertM(false, "Fell through switch");
            }
        }
    }

}

} // _BSPMAP

} // G3D

