/** 
  @file Thread.h
 
  @created 2005-09-22
  @edited  2016-08-26

  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */

#pragma once

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ThreadSet.h"
#include "G3D/Vector2int32.h"
#include "G3D/Vector3int32.h"
#include "G3D/SpawnBehavior.h"
#include "G3D/G3DString.h"
#include <functional>
#include <thread>

#ifndef G3D_WINDOWS
#   include <pthread.h>
#   include <signal.h>
#endif


namespace G3D {

/**
 Platform independent thread implementation.  You can either subclass and 
 override Thread::threadMain or call the create method with a method.

 Beware of reference counting and threads.  If circular references exist between
 Thread subclasses then neither class will ever be deallocated.  Also, 
 dropping all pointers (and causing deallocation) of a Thread does NOT 
 stop the underlying process.

 \sa G3D::GMutex, G3D::Spinlock, G3D::AtomicInt32, G3D::ThreadSet
*/
class Thread : public ReferenceCountedObject {
private:
    // "Status" is a reserved word on FreeBSD
    enum GStatus {STATUS_CREATED, STATUS_STARTED, STATUS_RUNNING, STATUS_COMPLETED};

    // Not implemented on purpose, don't use
    Thread(const Thread &);
    Thread& operator=(const Thread&);
    bool operator==(const Thread&);

#ifdef G3D_WINDOWS
    static DWORD WINAPI internalThreadProc(LPVOID param);
#else
    static void* internalThreadProc(void* param);
#endif //G3D_WINDOWS

    volatile GStatus     m_status;

    // Thread handle to hold HANDLE and pthread_t
#ifdef G3D_WINDOWS
    HANDLE              m_handle;
    HANDLE              m_event;
#else
    pthread_t           m_handle;
#endif //G3D_WINDOWS

    String              m_name;

protected:

    /** Overriden by the thread implementor */
    virtual void threadMain() = 0;

public:

    /** Returns System::numCores(); put here to break a dependence on System.h */
    static int numCores();

    Thread(const String& name);

    virtual ~Thread();

    /** Constructs a basic Thread without requiring a subclass.

        @param proc The global or static function for the threadMain() */
    static shared_ptr<Thread> create(const String& name, void (*proc)(void*), void* param = NULL);

    /** Starts the thread and executes threadMain().  Returns false if
       the thread failed to start (either because it was already started
       or because the OS refused).
       
       @param behavior If USE_CURRENT_THREAD, rather than spawning a new thread, this routine
       runs threadMain on the current thread.
       */
    bool start(SpawnBehavior behavior = USE_NEW_THREAD);

    /** Terminates the thread without notifying or
        waiting for a cancelation point. */
    void terminate();

    /**
        Returns true if threadMain is currently executing.  This will
        only be set when the thread is actually running and might not
        be set when start() returns. */
    bool running() const;

    /** True after start() has been called, even through the thread
        may have already completed(), or be currently running().*/
    bool started() const;

    /** Returns true if the thread has exited. */
    bool completed() const;

    /** Waits for the thread to finish executing. */
    void waitForCompletion();

    /** Returns thread name */
    const String& name() {
        return m_name;
    }

#if 0 // Old code
    /** For backwards compatibility to G3D 8.xx */
    static const SpawnBehavior USE_CURRENT_THREAD = G3D::USE_CURRENT_THREAD;
    /** For backwards compatibility to G3D 8.xx */
    static const SpawnBehavior USE_NEW_THREAD = G3D::USE_NEW_THREAD;


    enum {
        /** Tells Thread::runConcurrently() and Thread::runConcurrently2D() to
            use System::numCores() threads.*/
        NUM_CORES = -100
    };
    
    /** 
        \brief Iterates over a 2D region using multiple threads and
        blocks until all threads have completed. 
        
        <p> Evaluates \a
        object->\a method(\a x, \a y) for every <code>start.x <= x <
        upTo.x</code> and <code>start.y <= y < upTo.y</code>.
        Iteration is row major, so each thread can expect to see
        successive x values.  </p> 

        \param maxThreads
        Maximum number of threads to use.  By default at most one
        thread per processor core will be used.

        Example:

        \code
        class RayTracer {
        public:
            void trace(const Vector2int32& pixel) {
               ...
            }

            void traceAll() {
               Thread::runConcurrently2D(Point2int32(0,0), Point2int32(w,h), this, &RayTracer::trace);
            }
        };
        \endcode

        \deprecated Use runConcurrently
    */
    template<class Class>
    static void runConcurrently2D
    (const Vector2int32& start, 
     const Vector2int32& upTo, 
     Class*              object,
     void (Class::*method)(int x, int y),
     int                 maxThreads = NUM_CORES) {
        _internal_runConcurrently2DHelper(start, upTo, object, method, static_cast<void (Class::*)(int, int, int)>(nullptr), maxThreads);
    }

    /** Like the other version of runConcurrently2D, but tells the
        method the thread index that it is running on.  That enables
        the caller to manage per-thread state.

        \deprecated Use runConcurrently
    */
    template<class Class>
    static void runConcurrently2D
    (const Vector2int32& start, 
     const Vector2int32& upTo, 
     Class*              object,
     void (Class::*method)(int x, int y, int threadID),
     int                 maxThreads = NUM_CORES) {
        _internal_runConcurrently2DHelper(start, upTo, object, static_cast<void (Class::*)(int, int, int)>(nullptr), method, maxThreads);
    }
#endif


    /** 
        \brief Iterates over a 3D region using multiple threads and
        blocks until all threads have completed. Has highest coherence
        per thread in x, and then in blocks of y.
        
        <p> Evaluates \a
        object->\a method(\a x, \a y) for every <code>start.x <= x <
        upTo.x</code> and <code>start.y <= y < upTo.y</code>.
        Iteration is row major, so each thread can expect to see
        successive x values.  </p> 

        \param singleThread If true, force all computation to run on the
        calling thread. Helpful when debugging

        Example:

        \code
        class RayTracer {
        public:
            void trace(const Vector2int32& pixel) {
               ...
            }

            void traceAll() {
               Thread::runConcurrently(Point2int32(0,0), Point2int32(w,h), [this](pixel) { trace(pixel); );
            }
        };
        \endcode
    */
    static void runConcurrently
    (const Point3int32& start, 
     const Point3int32& stopBefore, 
     const std::function<void (Point3int32)>& callback,
     bool singleThread = false);

    static void runConcurrently
    (const Point2int32& start,
     const Point2int32& stopBefore, 
     const std::function<void (Point2int32)>& callback,
     bool singleThread = false);

    static void runConcurrently
    (const int& start, 
     const int& stopBefore, 
     const std::function<void (int)>& callback,
     bool singleThread = false);
};



// Can't use an inherited class inside of its parent on g++ 4.2.1 if it is later a template parameter

/** For use by runConcurrently2D. Designed for arbitrary iteration, although only used for
    interlaced rows in the current implementation. */
template<class Class>
class _internalThreadWorker : public Thread {
public:
    /** Start for this thread, which differs from the others */
    const int                 threadID;
    const Vector2int32        start;
    const Vector2int32        upTo;
    const Vector2int32        stride;
    Class*                    object;
    void             (Class::*method1)(int x, int y);
    void             (Class::*method2)(int x, int y, int threadID);
        
    _internalThreadWorker(int                 threadID,
            const Vector2int32& start, 
            const Vector2int32& upTo, 
            Class*              object,
            void (Class::*method1)(int x, int y),
            void (Class::*method2)(int x, int y, int threadID),
            const Vector2int32& stride) : 
        Thread("runConcurrently2D worker"),
        threadID(threadID),
        start(start),
        upTo(upTo), 
        stride(stride),
        object(object), 
        method1(method1),
        method2(method2) {}
        
    virtual void threadMain() {
        for (int y = start.y; y < upTo.y; y += stride.y) {
            // Run whichever method was provided
            if (method1) {
                for (int x = start.x; x < upTo.x; x += stride.x) {
                    (object->*method1)(x, y);
                }
            } else {
                for (int x = start.x; x < upTo.x; x += stride.x) {
                    (object->*method2)(x, y, threadID);
                }
            }
        }
    }
};

#if 0
template<class Class>
void _internal_runConcurrently2DHelper
   (const Vector2int32& start, 
    const Vector2int32& upTo, 
    Class*              object,
    void (Class::*method1)(int x, int y),
    void (Class::*method2)(int x, int y, int threadID),
    int                 maxThreads) {
        
    // Create a group of threads
    if (maxThreads == Thread::NUM_CORES) {
        maxThreads = Thread::numCores();
    }

    const int numRows = upTo.y - start.y;
    const int numThreads = min(maxThreads, numRows);
    const Vector2int32 stride(1, numThreads);
    ThreadSet threadSet;
    for (int t = 0; t < numThreads; ++t) {
        threadSet.insert(shared_ptr<_internalThreadWorker<Class> >(new _internalThreadWorker<Class>(t, start + Vector2int32(0, t), upTo, object, method1, method2, stride)));
    }

    // Run the threads, reusing the current thread and blocking until
    // all complete
    threadSet.start(USE_CURRENT_THREAD);
    threadSet.waitForCompletion();
}

#endif

} // namespace G3D

