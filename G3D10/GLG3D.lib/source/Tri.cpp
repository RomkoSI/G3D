/**
  \file GLG3D.lib/source/Tri.cpp
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-05-25
  \edited  2016-09-09
 */ 
#include "GLG3D/Tri.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Surfel.h"
#include "GLG3D/Material.h"
#include "GLG3D/UniversalSurface.h"
#include "GLG3D/CPUVertexArray.h"

namespace G3D {

void Tri::setStorage(const Array<Tri>& triArray, ImageStorage newStorage) {
    if (newStorage != IMAGE_STORAGE_CURRENT) {
        for (int i = 0; i < triArray.size(); ++i) {
            const Tri& tri = triArray[i];
			// Handles the case where the data pointer refers to a surface as well
            const shared_ptr<Material>& material = tri.material();
            if (notNull(material)) {
                material->setStorage(newStorage);
            }
        }
    }
}


Tri::Tri(const int i0, const int i1, const int i2,
         const CPUVertexArray&       vertexArray,
         const shared_ptr<ReferenceCountedObject>&   data,
         bool                        twoSided) :
    m_data(data) {
    index[0] = i0;
    index[1] = i1;
    index[2] = i2;

    m_area = e1(vertexArray).cross(e2(vertexArray)).length() * 0.5f;

    m_flags |= twoSided ? TWO_SIDED : 0;

    const shared_ptr<Material>& material = this->material();
    m_flags |= (notNull(material) && material->hasPartialCoverage()) ? HAS_PARTIAL_COVERAGE : 0;
}


Tri::Tri(const int i0, const int i1, const int i2,
         const CPUVertexArray&       vertexArray,
         const shared_ptr<ReferenceCountedObject>&   data,
         bool                        twoSided,
	     bool                        partialCoverage) :
    m_data(data) {
    index[0] = i0;
    index[1] = i1;
    index[2] = i2;

    m_area = e1(vertexArray).cross(e2(vertexArray)).length() * 0.5f;

    m_flags |= twoSided ? TWO_SIDED : 0;
    m_flags |= partialCoverage ? HAS_PARTIAL_COVERAGE : 0;
}


Triangle Tri::toTriangle(const CPUVertexArray& vertexArray) const {
    return Triangle(position(vertexArray, 0), position(vertexArray, 1), position(vertexArray, 2));
}


shared_ptr<Surface> Tri::surface() const {
    return dynamic_pointer_cast<Surface>(m_data);
}


shared_ptr<Material> Tri::material() const {
    const shared_ptr<Material>& material = dynamic_pointer_cast<Material>(m_data);
    if (notNull(material)) {
        return material;
    } 

    const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(m_data);
    if (notNull(surface)) {
        return surface->material();
    }

    return nullptr;
}


bool Tri::intersectionAlphaTest(const CPUVertexArray& vertexArray, float u, float v, float threshold) const {
    if (! hasPartialCoverage()) {
        return true;
    }

    const shared_ptr<Material>& material = this->material();
	debugAssert(notNull(material));

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


void Tri::sample(float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backface, shared_ptr<Surfel>& surfel) const {
    // Avoid using the material() helper method because we don't want to increment the material's 
    // shared_ptr counter, which requires an atomic (incrementing the surfel's one is unavoidable)
    //
    // Avoid the atomic increment cost of a dynamic_ptr_cast, since we know the object can't be colleted during this call
    const UniversalSurface* surface = dynamic_cast<const UniversalSurface*>(m_data.get());
    if (surface) {
        // Most common case
        surface->material()->sample(*this, u, v, triIndex, vertexArray, backface, surfel);
    } else {
        const shared_ptr<Material>& material = dynamic_pointer_cast<Material>(m_data);
        if (notNull(material)) {
            material->sample(*this, u, v, triIndex, vertexArray, backface, surfel);
        } else {
            surfel = nullptr;
        }
    }
}



} // namespace G3D
