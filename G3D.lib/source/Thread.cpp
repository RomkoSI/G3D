/**
 @file Thread.cpp

 Thread class.

 @created 2005-09-24
 @edited  2016-08-26
 */

#include "G3D/Thread.h"
#include "G3D/System.h"
#include "G3D/debugAssert.h"
#include "G3D/GMutex.h"

#define TBB_IMPLEMENT_CPP0X 0
#define __TBB_NO_IMPLICIT_LINKAGE 1
#define __TBBMALLOC_NO_IMPLICIT_LINKAGE 1
#include "../../tbb/include/tbb/tbb.h"

namespace G3D {

namespace _internal {

class BasicThread: public Thread {
public:
    BasicThread(const String& name, void (*proc)(void*), void* param):
        Thread(name), m_wrapperProc(proc), m_param(param) { }
protected:
    virtual void threadMain() {
        m_wrapperProc(m_param);
    }

private:
    void (*m_wrapperProc)(void*);

    void* m_param;
};

} // namespace _internal


Thread::Thread(const String& name):
    m_status(STATUS_CREATED),
    m_name(name) {

#ifdef G3D_WINDOWS
    m_event = NULL;
#endif

    // system-independent clear of handle
    System::memset(&m_handle, 0, sizeof(m_handle));
}

Thread::~Thread() {
#ifdef _MSC_VER
#   pragma warning( push )
#   pragma warning( disable : 4127 )
#endif
    alwaysAssertM(m_status != STATUS_RUNNING, "Deleting thread while running.");
#ifdef _MSC_VER
#   pragma warning( pop )
#endif

#ifdef G3D_WINDOWS
    if (m_event) {
        ::CloseHandle(m_event);
    }
#endif
}


shared_ptr<Thread> Thread::create(const String& name, void (*proc)(void*), void* param) {
    return std::make_shared<_internal::BasicThread>(name, proc, param);
}


bool Thread::started() const {
    return m_status != STATUS_CREATED;
}


int Thread::numCores() {
    return System::numCores();
}


#ifdef G3D_WINDOWS
// From http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO {
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadName(DWORD dwThreadID, const char* threadName) {
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try {
      RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
   } __except(EXCEPTION_EXECUTE_HANDLER) {}
}
#endif


bool Thread::start(SpawnBehavior behavior) {
    
    debugAssertM(! started(), "Thread has already executed.");
    if (started()) {
        return false;
    }

    m_status = STATUS_STARTED;

    if (behavior == USE_CURRENT_THREAD) {
        // Run on this thread
        m_status = STATUS_RUNNING;
        threadMain();
        m_status = STATUS_COMPLETED;
        return true;
    }

#   ifdef G3D_WINDOWS
        DWORD threadId;

        m_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        debugAssert(m_event);

        m_handle = ::CreateThread(NULL, 0, &internalThreadProc, this, 0, &threadId);

        if (m_handle == NULL) {
            ::CloseHandle(m_event);
            m_event = NULL;
        }

        SetThreadName(threadId, m_name.c_str());

        return (m_handle != NULL);
#   else
        if (!pthread_create(&m_handle, NULL, &internalThreadProc, this)) {
            return true;
        } else {
            // system-independent clear of handle
            System::memset(&m_handle, 0, sizeof(m_handle));

            return false;
        }
#   endif
}


void Thread::terminate() {
    if (m_handle) {
#       ifdef G3D_WINDOWS
        ::TerminateThread(m_handle, 0);
#       else
        pthread_kill(m_handle, SIGSTOP);
#       endif
        // system-independent clear of handle
        System::memset(&m_handle, 0, sizeof(m_handle));
    }
}


bool Thread::running() const{
    return (m_status == STATUS_RUNNING);
}


bool Thread::completed() const {
    return (m_status == STATUS_COMPLETED);
}


void Thread::waitForCompletion() {
    if (m_status == STATUS_COMPLETED) {
        // Must be done
        return;
    }

#   ifdef G3D_WINDOWS
        debugAssert(m_event);
        ::WaitForSingleObject(m_event, INFINITE);
#   else
        debugAssert(m_handle);
        pthread_join(m_handle, NULL);
#   endif
}


#ifdef G3D_WINDOWS
DWORD WINAPI Thread::internalThreadProc(LPVOID param) {
    Thread* current = reinterpret_cast<Thread*>(param);
    debugAssert(current->m_event);
    current->m_status = STATUS_RUNNING;
    current->threadMain();
    current->m_status = STATUS_COMPLETED;
    ::SetEvent(current->m_event);
    return 0;
}
#else
void* Thread::internalThreadProc(void* param) {
    Thread* current = reinterpret_cast<Thread*>(param);
    current->m_status = STATUS_RUNNING;
    current->threadMain();
    current->m_status = STATUS_COMPLETED;
    return (void*)NULL;
}
#endif

// Below this size, we use parallel_for on individual elements
static const int TASKS_PER_BATCH = 32;

void Thread::runConcurrently
   (const Point3int32& start, 
    const Point3int32& stopBefore, 
    const std::function<void (Point3int32)>& callback,
    bool singleThread) {

    const Point3int32 extent = stopBefore - start;
    const int numTasks = extent.x * extent.y * extent.z;
    const int numRows = extent.y * extent.z;

    if (singleThread) {
        for (Point3int32 coord(start); coord.z < stopBefore.z; ++coord.z) {
            for (coord.y = start.y; coord.y < stopBefore.y; ++coord.y) {
                for (coord.x = start.x; coord.x < stopBefore.x; ++coord.x) {
                    callback(coord);
                }
            }
        }
    } else if (extent.x > TASKS_PER_BATCH) {
        // Group tasks into batches by row (favors Y; blocks would be better)
        tbb::parallel_for(0, numRows, [&](size_t r) {
            for (Point3int32 coord(start.x, (int(r) % extent.y) + start.y, (int(r) / extent.y) + start.z); coord.x < stopBefore.x; ++coord.x) {
				callback(coord);
            }
        });
    } else if (extent.x * extent.y > TASKS_PER_BATCH) {
        // Group tasks into batches by groups of rows (favors Z; blocks would be better)
        tbb::parallel_for(tbb::blocked_range<size_t>(0, numRows, TASKS_PER_BATCH), [&](const tbb::blocked_range<size_t>& block) {
            for (size_t r = block.begin(); r < block.end(); ++r) {
                for (Point3int32 coord(start.x, (int(r) % extent.y) + start.y, (int(r) / extent.y) + start.z); coord.x < stopBefore.x; ++coord.x) {
                    callback(coord);
                }
            }
        });
    } else {
        // Process individual tasks as their own batches
        const int tasksPerPlane = extent.x * extent.y;
        tbb::parallel_for(0, numTasks, 1, [&](size_t i) {
            const int t = (int(i) % tasksPerPlane); 
            callback(Point3int32((t % extent.x) + start.x, (t / extent.x) + start.y, (int(i) / tasksPerPlane) + start.z));
        });
    }
}


void Thread::runConcurrently
   (const Point2int32& start,
    const Point2int32& stopBefore, 
    const std::function<void (Point2int32)>& callback,
    bool singleThread) {

    const Point2int32 extent = stopBefore - start;
    const int numTasks = extent.x * extent.y;
    
    if (singleThread) {
        for (Point2int32 coord(start); coord.y < stopBefore.y; ++coord.y) {
            for (coord.x = start.x; coord.x < stopBefore.x; ++coord.x) {
                callback(coord);
            }
        }
    } else if (extent.y > TASKS_PER_BATCH) {
        // Group tasks into batches by row (favors Y; blocks would be better)
        tbb::parallel_for(start.y, stopBefore.y, 1, [&](size_t y) {
            for (Point2int32 coord(start.x, int(y)); coord.x < stopBefore.x; ++coord.x) {
                callback(coord);
            }
        });
    } else if (extent.x > TASKS_PER_BATCH) {
        // Group tasks into batches by column
        tbb::parallel_for(start.x, stopBefore.x, 1, [&](size_t x) {
            for (Point2int32 coord(int(x), start.y); coord.y < stopBefore.y; ++coord.y) {
                callback(coord);
            }
        });
    } else {
        // Process individual tasks as their own batches
        tbb::parallel_for(0, numTasks, 1, [&](size_t i) {
            callback(Point2int32((int(i) % extent.x) + start.x, (int(i) / extent.x) + start.y));
        });
    }
}


void Thread::runConcurrently
   (const int& start, 
    const int& stopBefore, 
    const std::function<void (int)>& callback,
    bool singleThread) {

    if (singleThread) {
        for (int i = start; i < stopBefore; ++i) {
            callback(i);
        }
    } else if (stopBefore - start > TASKS_PER_BATCH) {
        // Group tasks into batches
        tbb::parallel_for(tbb::blocked_range<size_t>(start, stopBefore, TASKS_PER_BATCH), [&](const tbb::blocked_range<size_t>& block) {
            for (size_t i = block.begin(); i < block.end(); ++i) {
                callback(int(i));
            }
        });
    } else {
        // Process individual tasks as their own batches
        tbb::parallel_for(start, stopBefore, 1, [&](size_t i) {
            callback(int(i));
        });
    }
}


class _internalThreadWorkerNew : public Thread {
public:
    /** Start for this thread, which differs from the others */
    const int                 threadID;
    const Vector3int32        start;
    const Vector3int32        upTo;
    const Vector3int32        stride;
    std::function<void(int, int, int, int)> callback;
        
    _internalThreadWorkerNew
           (int                 threadID,
            const Vector3int32& start, 
            const Vector3int32& upTo, 
            const std::function<void(int, int, int, int)>& callback,
            const Vector3int32& stride) : 
        Thread("runConcurrently worker"),
        threadID(threadID),
        start(start),
        upTo(upTo), 
        stride(stride),
        callback(callback) {}
        
    virtual void threadMain() {
        for (int z = start.z; z < upTo.z; z += stride.z) {
            for (int y = start.y; y < upTo.y; y += stride.y) {
                for (int x = start.x; x < upTo.x; x += stride.x) {
                    callback(x, y, z, threadID);
                }
            }
        }
    }
};


#if 0 // Old implementation
void Thread::runConcurrently
   (const Vector3int32& start, 
    const Vector3int32& upTo, 
    const std::function<void (int, int, int, int)>& callback,
    int                 maxThreads) {

    // Create a group of threads
    if (maxThreads == Thread::NUM_CORES) {
        maxThreads = Thread::numCores();
    }

    ThreadSet threadSet;
    if ((upTo.y - start.y == 1) && (upTo.z - start.z == 1)) {
        // 1D iteration
        const int numElements = upTo.x - start.x;
        const int numThreads = min(maxThreads, numElements);
        const Vector3int32 stride(1, 1, 1);
        const int runLength = numElements / numThreads;
        for (int t = 0; t < numThreads; ++t) {
            Vector3int32 A = start + Vector3int32(t * runLength, 0, 0);
            Vector3int32 B(min(upTo.x, A.x + runLength), upTo.y, upTo.z);
            if (t == numThreads - 1) {
                // On the last iteration, run all of the way to the end even if we rounded down in the division
                B.x = upTo.x;
            }
            threadSet.insert(shared_ptr<_internalThreadWorkerNew >(new _internalThreadWorkerNew(t, A, B, callback, stride)));
        }
    } else {
        // 2D or 3D iteration
        const int numRows = upTo.y - start.y;
        const int numThreads = min(maxThreads, numRows);
        const Vector3int32 stride(1, numThreads, 1);
        for (int t = 0; t < numThreads; ++t) {
            threadSet.insert(shared_ptr<_internalThreadWorkerNew >(new _internalThreadWorkerNew(t, start + Vector3int32(0, t, 0), upTo, callback, stride)));
        }
    }

    // Run the threads, reusing the current thread and blocking until
    // all complete
    threadSet.start(USE_CURRENT_THREAD);
    threadSet.waitForCompletion();
}
#endif



//GMutex implementation
GMutex::GMutex() {
#ifdef G3D_WINDOWS
    ::InitializeCriticalSection(&m_handle);
#else
    int ret = pthread_mutexattr_init(&m_attr);
    debugAssert(ret == 0);
    ret = pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
    debugAssert(ret == 0);
    ret = pthread_mutex_init(&m_handle, &m_attr);
    debugAssert(ret == 0);
#endif
}

GMutex::~GMutex() {
#ifdef G3D_WINDOWS
    ::DeleteCriticalSection(&m_handle);
#else
    int ret = pthread_mutex_destroy(&m_handle);
    debugAssert(ret == 0);
    ret = pthread_mutexattr_destroy(&m_attr);
    debugAssert(ret == 0);
#endif
}

bool GMutex::tryLock() {
#ifdef G3D_WINDOWS
    return (::TryEnterCriticalSection(&m_handle) != 0);
#else
    return (pthread_mutex_trylock(&m_handle) == 0);
#endif
}

void GMutex::lock() {
#ifdef G3D_WINDOWS
    ::EnterCriticalSection(&m_handle);
#else
    pthread_mutex_lock(&m_handle);
#endif
}

void GMutex::unlock() {
#ifdef G3D_WINDOWS
    ::LeaveCriticalSection(&m_handle);
#else
    pthread_mutex_unlock(&m_handle);
#endif
}

} // namespace G3D
