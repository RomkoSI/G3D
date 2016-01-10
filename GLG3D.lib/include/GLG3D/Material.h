#ifndef GLG3D_Material_h
#define GLG3D_Material_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Proxy.h"
#include "GLG3D/Component.h"
#include "GLG3D/Tri.h"

namespace G3D {

class Surfel;

/** 
    Base class for materials in G3D, mostly useful as an interface for
    ray tracing since hardware rasterization rendering needs to be specialized
    for each Surface and Material subclass.

  \section Proxy
  Material is a Proxy subclass so that classes using it  mayassociate arbitrary data with UniversalMaterial%s 
  or computing Materials on demand without having to subclass UniversalMaterial itself. 
  
  Subclassing UniversalMaterial is often undesirable because
  that class has complex initialization and data management routines.
  Note that UniversalMaterial itself implements Proxy<UniversalMaterial>, so you can simply use a UniversalMaterial with any API
  (such as Tri) that requires a proxy.  


    \see UniversalMaterial
    \beta
 */
class Material : public Proxy<Material> {
public:

    // Inherited from Proxy
    virtual const shared_ptr<Material> resolve() const override {
        return dynamic_pointer_cast<Material>(const_cast<Material*>(this)->shared_from_this());
    }

    // Inherited from Proxy
    virtual shared_ptr<Material> resolve() override {
        return dynamic_pointer_cast<Material>(shared_from_this());
    }

    /** Returns true if this material has an alpha value less than \a alphaThreshold at texCoord. */
    virtual bool coverageLessThan(const float alphaThreshold, const Point2& texCoord) const = 0;

    virtual void setStorage(ImageStorage s) const = 0;
    virtual const String& name() const = 0;
    virtual shared_ptr<Surfel> sample(const Tri::Intersector& intersector) const = 0;
};

} // namespace G3D

#endif

