#pragma once

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/lazy_ptr.h"
#include "GLG3D/Component.h"
#include "GLG3D/Surfel.h"

namespace G3D {

class Surface;
class Tri;
    class CPUVertexArray;

/** 
    Base class for materials in G3D, mostly useful as an interface for
    ray tracing since hardware rasterization rendering needs to be specialized
    for each Surface and Material subclass.

  \section lazy_ptr
  Material is a lazy_ptr subclass so that classes using it  mayassociate arbitrary data with UniversalMaterial%s 
  or computing Materials on demand without having to subclass UniversalMaterial itself. 
  
  Subclassing UniversalMaterial is often undesirable because
  that class has complex initialization and data management routines.
  Note that UniversalMaterial itself implements lazy_ptr<UniversalMaterial>, so you can simply use a UniversalMaterial with any API
  (such as Tri) that requires a proxy.  


    \see UniversalMaterial
    \beta
 */
class Material : public ReferenceCountedObject {
public:

    /** Return true if coverageLessThanEqual(1) can ever return true. */
    virtual bool hasPartialCoverage() const = 0;

    /** Returns true if this material has an alpha value less than \a alphaThreshold at texCoord. */
    virtual bool coverageLessThanEqual(const float alphaThreshold, const Point2& texCoord) const = 0;
    virtual void setStorage(ImageStorage s) const = 0;
    virtual const String& name() const = 0;
    virtual shared_ptr<Surfel> sample(const Tri& tri, float u, float v, int triIndex, const CPUVertexArray& vertexArray, bool backside) const = 0;
};

} // namespace G3D
