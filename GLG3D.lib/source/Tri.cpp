/**
  \file GLG3D.lib/source/Tri.cpp
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-05-25
  \edited  2016-06-26
 */ 
#include "GLG3D/Tri.h"
#include "G3D/Ray.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Surfel.h"
#include "GLG3D/Material.h"
#include "GLG3D/UniversalSurface.h"
#include "GLG3D/CPUVertexArray.h"

namespace G3D {

Tri::Tri(const int i0, const int i1, const int i2,
         const CPUVertexArray&       vertexArray,
         const lazy_ptr<ReferenceCountedObject>&   material,
         bool                        twoSided) :
    m_data(material) {
    index[0] = i0;
    index[1] = i1;
    index[2] = i2;

    m_area = e1(vertexArray).cross(e2(vertexArray)).length() * 0.5f;

    // Encode the twoSided flag into the sign bit of m_area
    m_area = m_area * (twoSided ? -1.0f : 1.0f);
}


Triangle Tri::toTriangle(const CPUVertexArray& vertexArray) const {
    return Triangle(position(vertexArray, 0), position(vertexArray, 1), position(vertexArray, 2));
}


shared_ptr<Surface> Tri::surface() const {
    return dynamic_pointer_cast<Surface>(m_data.resolve());
}


shared_ptr<Material> Tri::material() const {
    shared_ptr<Material> material = dynamic_pointer_cast<Material>(m_data.resolve());
    if (notNull(material)) {
        return material;
    } 

    shared_ptr<UniversalSurface> surface = dynamic_pointer_cast<UniversalSurface>(m_data.resolve());
    if (notNull(surface)) {
        return surface->material();
    }

    return shared_ptr<Material>();
}


bool Tri::intersectionAlphaTest(const CPUVertexArray& vertexArray, float u, float v, float threshold) const {
    const shared_ptr<Material>& material = this->material();

    if (isNull(material) || ! material->hasPartialCoverage()) {
        return true;
    }

    const CPUVertexArray::Vertex& vertex0 = vertex(vertexArray, 0);
    const CPUVertexArray::Vertex& vertex1 = vertex(vertexArray, 1);
    const CPUVertexArray::Vertex& vertex2 = vertex(vertexArray, 2);

    const float w = 1.0f - u - v;
    const Point2& texCoord =
        w * vertex0.texCoord0 +
        u * vertex1.texCoord0 +
        v * vertex2.texCoord0;

    return ! material->coverageLessThanEqual(threshold, texCoord);
}


bool Tri::hasPartialCoverage() const {
    const shared_ptr<Material>& m = material();
    return notNull(m) && m->hasPartialCoverage();
}


shared_ptr<Surfel> Tri::sample(float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backface) const {
    return material()->sample(*this, u, v, triIndex, vertexArray, backface);
}



} // namespace G3D
