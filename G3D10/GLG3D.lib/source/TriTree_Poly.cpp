/**
  @file GLG3D/TriTree_Poly.cpp

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2009-06-10
  @edited  2009-06-20
*/

#include "GLG3D/TriTree.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

TriTree::Poly::Poly() : m_source(NULL), m_area(0) {}

TriTree::Poly::Poly(const CPUVertexArray& vertexArray, const Tri* tri) : 
    m_source(tri),
    m_low(Vector3::inf()),
    m_high(-Vector3::inf()),
    m_area(tri->area()) {

    m_vertex.resize(3);
    for (int v = 0; v < 3; ++v) {
        const Vector3& x = tri->position(vertexArray, v);
        m_vertex[v] = x;
        m_low  = m_low.min(x);
        m_high = m_high.max(x);
    }
}


void TriTree::Poly::draw(RenderDevice* rd, const CPUVertexArray& vertexArray) const {
	/*
    rd->beginPrimitive(PrimitiveType::TRIANGLE_FAN);
    rd->setNormal(m_source->normal(vertexArray));
    for (int i = 0; i < m_vertex.size(); ++i) {
        rd->sendVertex(m_vertex[i]);
    }
    rd->endPrimitive();
	*/
}


void TriTree::Poly::split
(Vector3::Axis axis,
 float         offset,
 float         minSpanArea,
 Array<Poly>&  lowArray,
 Array<Poly>&  highArray,
 Array<Poly>&  largeSpanArray) const {
    debugAssert(m_vertex.size() >= 3);

    //debugPrintf("     [%f, %f] @ %f: ", m_low[axis], m_high[axis], offset);
    // Detect case
    if (m_high[axis] <= offset) {
        lowArray.append(*this);
        //debugPrintf("low\n");
    } else if (m_low[axis] >= offset) {
        highArray.append(*this);
        //debugPrintf("HIGH\n");
    } else if (m_area >= minSpanArea) {
        //debugPrintf("--span--\n");
        largeSpanArray.append(*this);
    } else {
        //debugPrintf("split\n");
        Poly& L = lowArray.next();
        Poly& H = highArray.next();

        L.m_source     = m_source;
        L.m_low        = Vector3::inf();
        L.m_high       = -Vector3::inf();

        H.m_source     = m_source;
        H.m_low        = Vector3::inf();
        H.m_high       = -Vector3::inf();

        // Actually split
        bool inLow = m_vertex[0][axis] < offset;
        for (int i0 = 0; i0 < m_vertex.size(); ++i0) {
            const Vector3& v0 = m_vertex[i0];

            if (inLow) {
                L.addIfNewVertex(v0);
            } else {
                H.addIfNewVertex(v0);
            }

            const int i1 = (i0 + 1) % m_vertex.size();
            const Vector3& v1 = m_vertex[i1]; 
            bool nextInLow = v1[axis] < offset;

            if (inLow != nextInLow) {
                // Crossing over the splitting plane
                if (v0[axis] == offset) {
                    // Exactly on the splitting plane and coming
                    // from the high side.
                    L.addIfNewVertex(v0);
                } else if (v1[axis] == offset) {
                    // Exactly on the splitting plane and coming
                    // from the low side.
                    L.addIfNewVertex(v1);
                } else {
                    // Find intersection
                    const Vector3& delta = v1 - v0;
                    debugAssert(delta[axis] != 0);
                    const float alpha = (offset - v0[axis]) / delta[axis];
                    
                    const Vector3& v = v0 + (v1 - v0) * alpha;
                    
                    // Add this point to both
                    L.addIfNewVertex(v);
                    H.addIfNewVertex(v);
                }

                inLow = nextInLow;
            }
        }

        // Compute areas
        L.computeArea();
        H.computeArea();
        

        // Remove slivers and degenerates
        if (L.area() <= 0.0f) {
            lowArray.popDiscard();
            //debugPrintf("Warning: TriTree generated and removed zero area poly (low)\n");
        }
        
        if (H.area() <= 0.0f) {
            highArray.popDiscard();
            //debugPrintf("Warning: TriTree generated and removed zero area poly (high)\n");
        }
    }
}


void TriTree::Poly::computeArea() {
    m_area = 0.0f;
    // Compute area of triangles
    const int N = m_vertex.size();
    const Vector3& v = m_vertex[0];
    for (int i = 0; i < N - 2; ++i) {
        m_area += (m_vertex[(i + 1) % N] - v).cross(m_vertex[(i + 2) % N] - v).length();
    }
    m_area *= 0.5f;
}


AABox TriTree::Poly::computeBounds(const Array<Poly>& array) {
    if (array.size() == 0) {
        return AABox(Vector3::zero());
    }

    Vector3 L = array[0].m_low, H = array[0].m_high;
    for (int i = 1; i < array.size(); ++i) {
        L = L.min(array[i].m_low);
        H = H.max(array[i].m_high);
    }

    return AABox(L, H);
}

}
