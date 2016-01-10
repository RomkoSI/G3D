/**
 \file GLG3D/Shape.cpp

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2005-08-30
 \edited  2014-07-27
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/ParseOBJ.h"
#include "G3D/Random.h"
#include "GLG3D/Shape.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"
#include "GLG3D/SlowMesh.h"

namespace G3D {

MeshShape::MeshShape
   (const Array<Vector3>& vertex, 
    const Array<int>& index) : 
    _vertexArray(vertex), 
    _indexArray(index),
    _hasTree(false) {
    
    debugAssert(index.size() % 3 == 0);
    debugAssert(index.size() >= 0);
	computeArea();
}


MeshShape::MeshShape(const class ParseOBJ& parseObj) : _hasTree(false) {
    _vertexArray = parseObj.vertexArray;
    Array<int>    index;

    // Extract the mesh
    for (ParseOBJ::GroupTable::Iterator git = parseObj.groupTable.begin(); git.isValid(); ++git) {
        const shared_ptr<ParseOBJ::Group>& group = git->value;
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin(); mit.isValid(); ++mit) {
            const shared_ptr<ParseOBJ::Mesh>& mesh = mit->value;
            for (int f = 0; f < mesh->faceArray.size(); ++f) {
                const ParseOBJ::Face& face = mesh->faceArray[f];
                //add all tris as a triangle fan
                for (int offset = 0; offset + 3 <= face.size(); ++offset) {
                    _indexArray.append(face[0].vertex);
                    _indexArray.append(face[1 + offset].vertex);
                    _indexArray.append(face[2 + offset].vertex);
                }
            }  // face
        } // mesh
    } // group

	computeArea();
}


MeshShape::MeshShape(const CPUVertexArray& vertexArray,
    const Array<Tri>& tri) : 
    _hasTree(false) {

    _vertexArray.resize(tri.size() * 3);
    _indexArray.resize(tri.size() * 3);

    int i = 0;
    for (int t = 0; t < tri.size(); ++t) {
        for (int v = 0; v < 3; ++v, ++i) {
            _indexArray[i] = i;
            _vertexArray[i] = tri[t].position(vertexArray, v);
        }
    }

	computeArea();
}


void MeshShape::computeArea() {
	_area = 0;
	_triangleArea.resize(_indexArray.size() / 3);
    for (int i = 0, j = 0; i < _indexArray.size(); i += 3, ++j) {
        const int v0 = _indexArray[i + 0];
        const int v1 = _indexArray[i + 1];
        const int v2 = _indexArray[i + 2];

        Triangle tri(_vertexArray[v0], _vertexArray[v1], _vertexArray[v2]);
        const float a = tri.area();
		_triangleArea[j] = a;
        _area += a;
    }
}


void MeshShape::buildBSP() {
    debugAssert(! _hasTree);
    _area = 0;

    // These arrays are built for use in getRandomSurfacePoint()
    _triangleAreaSumArray.resize(_indexArray.size() / 3);
    _orderedBSPTris.resize(_indexArray.size() / 3);
    
    CPUVertexArray cpuVertexArray;
    cpuVertexArray.hasBones = false;
    cpuVertexArray.hasTangent = false;
    cpuVertexArray.hasTexCoord0 = false;
    cpuVertexArray.hasTexCoord1 = false;
    cpuVertexArray.hasVertexColors = false;

    //cpuVertexArray.vertex.resize(_vertexArray.size());
    Array<Tri> trisArray;    

    for (int i = 0; i < _vertexArray.size(); ++i) {
        CPUVertexArray::Vertex vertex;
        vertex.position = _vertexArray[i];

        cpuVertexArray.vertex.append(vertex);
    }

    for (int i = 0, j = 0; i < _indexArray.size(); i += 3) {
        const int v0 = _indexArray[i];
        const int v1 = _indexArray[i + 1];
        const int v2 = _indexArray[i + 2];
        
        trisArray.append(Tri(v0, v1, v2, cpuVertexArray));
        Triangle tri(_vertexArray[v0], _vertexArray[v1], _vertexArray[v2]);

        _area += tri.area();
        _orderedBSPTris[j] = tri;

        if (j == 0) {
            _triangleAreaSumArray[j] = tri.area();
        } else {
            _triangleAreaSumArray[j] = _triangleAreaSumArray[j - 1] + tri.area();
        }
        ++j;
    }
    _bspTree.setContents(trisArray, cpuVertexArray);
     MeshAlg::computeBounds(_vertexArray, _indexArray, _boundingAABox, _boundingSphere);
    _hasTree = true;
}


float MeshShape::area() const {
    return (float)_area;
}


float MeshShape::volume() const {
    return 0;
}


Vector3 MeshShape::center() const {
    if (! _hasTree) {
        const_cast<MeshShape*>(this)->buildBSP();
    }

    return _boundingAABox.center();
}


Sphere MeshShape::boundingSphere() const {
    if (! _hasTree) {
        const_cast<MeshShape*>(this)->buildBSP();
    }
    return _boundingSphere;
}


AABox MeshShape::boundingAABox() const {
    if (! _hasTree) {
        const_cast<MeshShape*>(this)->buildBSP();
    }
    return _boundingAABox;
}


void MeshShape::getRandomSurfacePoint(
    Vector3& P,
    Vector3& N,
    Random& rnd) const {
    int ignoreIndex;
    Vector3 ignoreWeights;
    getRandomSurfacePoint(P, N, ignoreIndex, ignoreWeights, rnd);
}


void MeshShape::getRandomSurfacePoint(
    Vector3& P,
    Vector3& N,
    int& triangleStartIndex,
    Vector3& barycentricWeights,
    Random& rnd) const {

    if (! _hasTree) {
        const_cast<MeshShape*>(this)->buildBSP();
    }
    
    // Choose uniformly at random based on surface area
    const float sum = rnd.uniform(0, (float)_area);;
    int bottom = 0;
    int mid = _orderedBSPTris.length() / 2;
    int top = _orderedBSPTris.length() - 1;

    // Binary search, for log-runtime in the number of tris
    while (top > bottom + 1) {
        if (_triangleAreaSumArray[mid] < sum) {
            bottom = mid;
            mid = (mid + top) / 2;
        } else {
            top = mid;
            mid = (bottom + mid) / 2;
        }
    }

    int target = bottom;
    // Udjust up a little if the search undershoots
    while ((_triangleAreaSumArray[target] < sum) && (target < _orderedBSPTris.length() - 1)) { ++target; }

    // Get the triangle from a pre-ordered array
    const Triangle& tri = _orderedBSPTris[target];
    N = tri.normal();
    P = tri.randomPoint(rnd);

    triangleStartIndex = target * 3;
    barycentricWeights = tri.barycentric(P);
}


Vector3 MeshShape::randomInteriorPoint(Random& rnd) const {
    if (!_hasTree) {
        const_cast<MeshShape*>(this)->buildBSP();
    } 

    const float BUMP_DISTANCE = 0.00005f;

    for (int attempt = 0; attempt < MAX_ATTEMPTS_RANDOM_INTERIOR_POINT; ++attempt) {
        const Vector3 origin = _boundingAABox.randomInteriorPoint();
        const Vector3 direction = (origin - _boundingAABox.center()).direction();

        int isInterior = 0;

        Ray r(origin, direction);

        float distance = finf();
        Tri::Intersector intersector;

        while (_bspTree.intersectRay(r, intersector, distance, false, true)) {
            isInterior += intersector.backside ? 1 : -1;
            r = r.bumpedRay(distance += BUMP_DISTANCE);
            distance = finf();
        }

        if (isInterior > 0) {
            return origin;
        } 
    }

    //the maximum number of attempts have been made and no intersection was found
    return Vector3::nan();
}


void MeshShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();

    rd->pushState(); {
        rd->setObjectToWorldMatrix(cframe0 * cframe);
        if (solidColor.a < 1.0) {
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        }

        {
            SlowMesh mesh(PrimitiveType::TRIANGLES);
            mesh.setColor(solidColor);
            for (int i = 0; i < _indexArray.size(); i += 3) {
                for (int j = 0; j < 3; ++j) {
                    mesh.makeVertex(_vertexArray[_indexArray[i + j]]);
                }
            }
            mesh.render(rd);
        }

        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        if (wireColor.a > 0) {
            SlowMesh mesh(PrimitiveType::LINES);
            mesh.setColor(wireColor);
            for (int i = 0; i < _indexArray.size(); i += 3) {
                const Vector3& v0 = _vertexArray[_indexArray[i + 0]];
                const Vector3& v1 = _vertexArray[_indexArray[i + 1]];
                const Vector3& v2 = _vertexArray[_indexArray[i + 2]];
                mesh.makeVertex(v0);
                mesh.makeVertex(v1);
                mesh.makeVertex(v1);
                mesh.makeVertex(v2);
                mesh.makeVertex(v2);
                mesh.makeVertex(v0);
            }
            mesh.render(rd);
        }
    } rd->popState();
}

////////////////////////////////////////////////////////////////////////

void BoxShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::box(geometry, rd, solidColor, wireColor);
    rd->setObjectToWorldMatrix(cframe0);
}


void TriangleShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->pushState(); {
        rd->setObjectToWorldMatrix(cframe0 * cframe);

        if (wireColor.a > 0.0f) {
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
            rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
            SlowMesh mesh(PrimitiveType::TRIANGLES);
            mesh.setColor(wireColor);
            mesh.setNormal(geometry.normal());
            for (int i = 0; i < 3; ++i) {
                mesh.makeVertex(geometry.vertex(i));
            }
            mesh.render(rd);
            rd->setPolygonOffset(-0.2f);
        }
        rd->setRenderMode(RenderDevice::RENDER_SOLID);

        if (solidColor.a < 1.0f) {
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        }
        rd->setCullFace(CullFace::NONE);

        SlowMesh mesh(PrimitiveType::TRIANGLES);
        mesh.setColor(solidColor);
        mesh.setNormal(geometry.normal());
        for (int i = 0; i < 3; ++i) {
            mesh.makeVertex(geometry.vertex(i));
        }
        mesh.render(rd);
    } rd->popState();
}


void SphereShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::sphere(geometry, rd, solidColor, wireColor);
    rd->setObjectToWorldMatrix(cframe0);
}


void CylinderShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::cylinder(geometry, rd, solidColor, wireColor);
    rd->setObjectToWorldMatrix(cframe0);
}


void CapsuleShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::capsule(geometry, rd, solidColor, wireColor);
    rd->setObjectToWorldMatrix(cframe0);
}


void RayShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    (void)wireColor;
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::ray(geometry, rd, solidColor);
    rd->setObjectToWorldMatrix(cframe0);
}


void ArrowShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    (void)wireColor;
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::arrow(m_point, m_vector, rd, solidColor, m_scale);
    rd->setObjectToWorldMatrix(cframe0);
}


void AxesShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    (void)wireColor;
    (void)solidColor;
    const CoordinateFrame& cframe0 = rd->objectToWorldMatrix();
    rd->setObjectToWorldMatrix(cframe0 * cframe);
    Draw::axes(geometry, rd, Color3::red(), Color3::green(), Color3::blue(), m_axisLength * 0.5f);
    rd->setObjectToWorldMatrix(cframe0);
}


void PointShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    (void)wireColor;
    Draw::sphere(Sphere(geometry, 0.1f), rd, solidColor, Color4::clear());
}


void PlaneShape::render(RenderDevice* rd, const CoordinateFrame& cframe, Color4 solidColor, Color4 wireColor) {
    CoordinateFrame cframe0 = rd->objectToWorldMatrix();

    rd->setObjectToWorldMatrix(cframe0 * cframe);

    Draw::plane(geometry, rd, solidColor, wireColor);
    
    rd->setObjectToWorldMatrix(cframe0);
}

}

