/**
 \file   G3D/lazy_ptr.h
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \date   2012-03-16
 \edited 2016-02-10
*/
#ifndef G3D_lazy_ptr_h
#define G3D_lazy_ptr_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"

namespace G3D {

/** 
  \brief Provides a level of indirection for accessing objects to allow computing them on
  demand or extending them with metadata without subclassing the object itself. For example,
  a proxy is useful for implementing lazy loading of files.

  The G3D::Material and G3D::UniversalMaterial together comprise an example of using
  lazy_ptr for abstracting lazy loading and breaking dependency in subclasses.

  It is sometimes useful to have a non-NULL proxy to a NULL object, for example, when
  attaching data or reporting an error.

  Analogous to shared_ptr and weak_ptr.
*/
template<class T>
class lazy_ptr : public ReferenceCountedObject {
public:

    /** Returns a pointer to a T or a NULL pointer. If there
       are multiple levels of proxies, then this call resolves all of them. */
    virtual const shared_ptr<T> resolve() const { return shared_ptr<T>(); }

    /** \copydoc resolve 
    */
    virtual shared_ptr<T> resolve() { return shared_ptr<T>(); }

    /** \brief Handles the resolve for the case where the proxy itself is NULL.
      
       \code
         shared_ptr<lazy_ptr<Foo>> p = ...;

         const shared_ptr<Foo>& f = lazy_ptr<Foo>::resolve(p);
       \endcode
    */
    static shared_ptr<T> resolve(const shared_ptr<lazy_ptr<T>>& r) {
        return isNull(r) ? shared_ptr<T>() : r->resolve();
    }
};

} // namespace G3D

#endif // G3D_Proxy_h
