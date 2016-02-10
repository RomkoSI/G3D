/**
 \file   G3D/lazy_ptr.h
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \date   2012-03-16
 \edited 2016-02-10
*/
#ifndef G3D_lazy_ptr_h
#define G3D_lazy_ptr_h

#include "G3D/platform.h"
#include <functional>
#include "G3D/ReferenceCount.h"
#include "G3D/GMutex.h"

using std::function;

namespace G3D {

/** 
  \brief Provides a level of indirection for accessing objects to allow computing them on
  demand or extending them with metadata without subclassing the object itself. For example,
  lazy loading of files.

  The G3D::Material and G3D::UniversalMaterial together comprise an example of using
  lazy_ptr for abstracting lazy loading and breaking dependency in subclasses.

  Analogous to shared_ptr and weak_ptr. Copies of lazy_ptrs retain the same underlying object,
  so it will only be resolved once.

  Threadsafe.
*/
template<class T>
class lazy_ptr {
private:

    class Proxy : public ReferenceCountedObject {
    private:
        mutable bool                    m_resolved;
        const function<shared_ptr<T>()> m_resolve;
        mutable shared_ptr<T>           m_object;
        mutable GMutex                  m_mutex;
    public:

        Proxy(const function<shared_ptr<T>()>& resolve, const shared_ptr<T>& object, bool resolved) : 
            m_resolved(resolved), m_resolve(resolve), m_object(object) {}

        shared_ptr<T> resolve() const {
            m_mutex.lock();
            if (! m_resolved) {
                debugAssert(m_resolve != nullptr);
                m_object = m_resolve();
                m_resolved = true;
            }
            m_mutex.unlock();
            return m_object;
        }


        bool operator==(const Proxy& other) const {
            // Pre-resolved case
            if ((m_resolve == nullptr) || (other.m_resolve == nullptr)) {
                return m_object == other.m_object;
            }

            // Check for same lock before locking the mutex to avoid deadlock
            if (&other == this) { return true; }             

            m_mutex.lock();
            other.m_mutex.lock();

            // Same object after resolution, or same resolution function
            const bool result = (m_resolved && other.m_resolved && (m_object.get() == other.m_object.get()));

            other.m_mutex.unlock();
            m_mutex.unlock();
            return result;
        }
    };

    shared_ptr<Proxy>  m_proxy;

public:

    /** Creates a NULL lazy pointer */
    lazy_ptr() {}

    lazy_ptr(nullptr_t) {}

    /** Creates a lazy_ptr from a function that will create the object */
    lazy_ptr(const function<shared_ptr<T> (void)>& resolve) : m_proxy(new Proxy(resolve, nullptr, false)) {}

    /** Creates a lazy_ptr for an already-resolved object */
    lazy_ptr(const shared_ptr<T>& object) : m_proxy(new Proxy(nullptr, object, true)) {}

    /** Creates a lazy_ptr for an already-resolved object */
    template <class S>
    lazy_ptr(const shared_ptr<S>& object) : m_proxy(new Proxy(nullptr, dynamic_pointer_cast<T>(object), true)) {}

    /** Is the proxy itself a null pointer */
    bool isNull() const {
        return (m_proxy.get() == nullptr);
    }

    /** Returns a pointer to a T or a NULL pointer. If there
       are multiple levels of proxies, then this call resolves all of them. */
    const shared_ptr<T> resolve() const { 
        return isNull() ? shared_ptr<T>() : m_proxy->resolve();
    }

    /** \copydoc resolve */
    shared_ptr<T> resolve() {         
        return isNull() ? shared_ptr<T>() : m_proxy->resolve();
    }

    bool operator==(const lazy_ptr<T>& other) const {
        return (m_proxy == other.m_proxy) || 
            (! isNull() && ! other.isNull() &&
                (*m_proxy == *other.m_proxy));
    }
};

} // namespace G3D

#endif // G3D_lazy_ptr_h
