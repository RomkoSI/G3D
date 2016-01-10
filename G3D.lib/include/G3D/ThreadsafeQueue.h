/** 
  \file G3D/ThreadSafeQueue.h
 
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2013-03-25
  \edited  2013-03-25
 */

#ifndef G3D_ThreadsafeQueue_h
#define G3D_ThreadsafeQueue_h

#include "G3D/platform.h"
#include "G3D/GMutex.h"
#include "G3D/Queue.h"

namespace G3D {

/** A queue whose methods are synchronized with respect to each other.
    \sa Queue, GMutex, Spinlock */
template<class T>
class ThreadsafeQueue {
private:
    mutable Spinlock    m_mutex;
    Queue<T>            m_data;

public:

    void clear() {
        m_mutex.lock();
        m_data.clear();
        m_mutex.unlock();
    }

    void pushBack(const T& v) {
        m_mutex.lock();
        m_data.pushBack(v);
        m_mutex.unlock();
    }

    void pushFront(const T& v) {
        m_mutex.lock();
        m_data.pushFront(v);
        m_mutex.unlock();
    }

    /** Returns true if v was actually read */
    bool popFront(T& v) {
        bool read = false;
        m_mutex.lock();
        if (m_data.size() > 0) {
            v = m_data.popFront();
            read = true;
        }
        m_mutex.unlock();
        return read;
    }

    /** Returns true if v was actually read */
    bool popBack(T& v) {
        bool read = false;
        m_mutex.lock();
        if (m_data.size() > 0) {
            v = m_data.popBack();
            read = true;
        }
        m_mutex.unlock();
        return read;
    }

    /** Note that by the time the method has returned, the value may be incorrect. */
    int size() const {
        m_mutex.lock();
        int i = m_data.size();
        m_mutex.unlock();
        return i;
    }

    bool empty() const {
        return size() == 0;
    }
};
}

#endif
