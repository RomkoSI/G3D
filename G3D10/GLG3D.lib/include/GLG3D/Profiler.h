/**
  @file Profiler.h
  @author Morgan McGuire, http://graphics.cs.williams.edu
  
 @created 2009-01-01
 @edited  2013-07-14

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_Profiler_h
#define G3D_Profiler_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/G3DGameUnits.h"
#include "G3D/Table.h"
#include "G3D/GThread.h"

typedef int GLint;
typedef unsigned int GLuint;
#ifndef GL_NONE
#   define GL_NONE (0)
#   define DEFINED_GL_NONE 1
#endif

namespace G3D {

/** 
    \brief Measures execution time of CPU and GPU events across multiple threads.

    \beta

 */
class Profiler {
public:

    /**
      May have child Events.
     */
    class Event {
    private:
        friend class Profiler;

        String			m_name;
        String			m_file;
        String          m_hint;
        int             m_line;
        /** a unique identifier that is the events parent hash plus the hash of its hint and the hash of the its shader file and line number */
        size_t          m_hash;

        /** Relative to an arbitrary baseline */
        RealTime        m_gfxStart;
        RealTime        m_gfxEnd;

        /** Unix time */
        RealTime        m_cpuStart;
        RealTime        m_cpuEnd;

        int             m_numChildren;
        int             m_parentIndex;

        /** GL counter query IDs */
        GLint           m_openGLStartID;
        GLint           m_openGLEndID;

        int             m_level;

    public:

        /** For the root's parent */
        enum { NONE = -1 };

        Event() : m_line(0), m_hash(0), m_gfxStart(nan()), m_gfxEnd(nan()), m_cpuStart(nan()), m_cpuEnd(nan()), m_numChildren(0),
            m_parentIndex(NONE), m_openGLStartID(GL_NONE), m_openGLEndID(GL_NONE),   m_level(0) {
        }

        /** Tree level, 0 == root.  This information can be inferred from the tree structure but
            is easiest to directly query. */
        int level() const {
            return m_level;
        }

        /** Number of child events.  Descendents are expanded in depth-first order. */
        int numChildren() const {
            return m_numChildren;
        }

        /** Index in the event tree of this node's parent, NONE if this is the root. */
        int parentIndex() const {
            return m_parentIndex;
        }

        /** The name provided for this event when it began. For auto-generated shader events from LAUNCH_SHADER,
            this will be the name of the shader.

            Note that event names are not necessarily unique. The location of an event within the tree is the
            only unique identification.
            */
        const String& name() const {
            return m_name;
        }

        /** The name of the C++ file in which the event began. */
        const String& file() const {
            return m_file;
        }

        const String& hint() const {
            return m_hint;
        }

        const size_t hash() const {
            return m_hash;
        }

        /** The line number in file() at which the event began. */
        int line() const {
            return m_line;
        }

        /** Unix time at which Profiler::beginEvent() was called to create this event. Primarily useful for ordering events on a timeline.
            \see cpuDuration, gfxDuration, endTime */
        RealTime startTime() const {
            return m_cpuStart;
        }

        /** Unix time at which Profiler::endEvent() was called to create this event. */
        RealTime endTime() const {
            return m_cpuEnd;
        }

        /** Time elapsed between when the GPU began processing this task and when it completed it,
            including the time consumed by its children. The GPU may have been idle for some of that
            time if it was blocked on the CPU or the event began before significant GPU calls were
            actually issued by the program.*/
        RealTime gfxDuration() const {
            return m_gfxEnd - m_gfxStart;
        }

        /** Time elapsed between when the CPU began processing this task and when it completed it,
            including the time consumed by its children. */
        RealTime cpuDuration() const {
            return m_cpuEnd - m_cpuStart;
        }

		bool operator==(const String& name) const {
			return m_name == name;
		}

		bool operator!=(const String& name) const {
			return m_name != name;
		}
    };

private:
    
    /** Per-thread profiling information */
    class ThreadInfo {
    public:
        /** GPU query objects available for use.*/
        Array<GLuint>                       queryFreelist;
    
        /** Full tree of all events for the current frame on the current thread */
        Array<Event>                        eventTree;

        /** Indices of the ancestors of the current event, in m_taskTree */
        Array<int>                          ancestorStack;

        /** Full tree of events for the previous frame */
        Array<Event>                        previousEventTree;

        void beginEvent(const String& name, const String& file, int line, const size_t baseHash, const String& hint = "");

        void endEvent();

        GLuint newQueryID();

        ~ThreadInfo();
    };

    /** Information about the current thread. Initialized by beginEvent */
    static __thread shared_ptr<ThreadInfo>* s_threadInfo;

    static __thread int                     s_level;

    /** Stores information about all threads for the current frame */
    static Array<shared_ptr<ThreadInfo> >   s_threadInfoArray;

    static GMutex                           s_profilerMutex;

    /** Whether to make profile events in every LAUNCH_SHADER call. Default is true. */
    static bool                             s_timeShaderLaunches;

    /** Updated on every call to nextFrame() to ensure that events from different frames are never mixed. */
    static uint64                           s_frameNum;

    static bool                             s_enabled;

    static int calculateUnaccountedTime(Array<Event>& eventTree, const int index, RealTime& cpuTime, RealTime& gpuTime);

    /** Prevent allocation using this private constructor */
    Profiler() {}

public:

    /** Do not call directly if using GThread. Registered with GThread to deallocate the ThreadInfo for a thread.
        Must be explicitly invoked if you use a different thread API. */
    static void threadShutdownHook();

    /** Notify the profiler to latch the current event tree. 
        Events are always presented one frame late so that 
        that information is static and independent of when
        the caller requests it within the frame.
    
        Invoking nextFrame may stall the GPU and CPU by blocking in
        the method, causing your net frame time to appear to increase.
        This is (correctly) not reflected in the values returned by event timers.

        GApp calls this automatically.  Note that this may cause OpenGL errors and race conditions
        in programs that use multiple GL contexts if there are any outstanding
        events on any thread at the time that it is invoked. It is the programmer's responsibility to ensure that that 
        does not happen.
    */
    static void nextFrame();

    /** When disabled no profiling occurs (i.e., beginCPU and beginGFX
        do nothing).  Since profiling can affect performance
        (nextFrame() may block), top framerate should be measured with
        profiling disabled. */
    static bool enabled() {
        return s_enabled;
    }

    /** \copydoc enabled() */
    static void setEnabled(bool e);

    /** Calls to beginEvent may be nested on a single thread. Events on different
        threads are tracked independently.*/
    static void beginEvent(const String& name, const String& file, int line, const size_t baseHash, const String& hint = "");
    
    /** Ends the most recent pending event on the current thread. */
    static void endEvent();

    /** Return all events from the previous frame, one array per thread. The underlying
        arrays will be mutated when nextFrame() is invoked.
		
		The result has the form: <code>const Profiler::Event& e = (*(eventTrees[threadIndex]))[eventIndex]</code>
		The events are stored as the depth-first traversal of the event tree.
		See the Profiler::Event documentation for information about identifying
		the roots and edges within each tree. 
    */
    static void getEvents(Array<const Array<Event>*>& eventTrees);

    /** Set whether to make profile events in every LAUNCH_SHADER call. 
        Useful for when you only want to time a small amount of thing, or just the aggregate of many launches. */
    static void set_LAUNCH_SHADER_timingEnabled(bool enabled);

    /** Whether to make profile events in every LAUNCH_SHADER call. Default is true. */
    static bool LAUNCH_SHADER_timingEnabled();
};

} // namespace G3D 

/** \def BEGIN_PROFILER_EVENT 

   Defines the beginning of a profilable event.  Example:
    \code
   BEGIN_PROFILER_EVENT("MotionBlur");
   ...
   END_PROFILER_EVENT("MotionBlur");
   \endcode

   The event name must be a compile-time constant char* or String.

   \sa END_PROFILER_EVENT, Profiler, Profiler::beginEvent
 */

#define BEGIN_PROFILER_EVENT_WITH_HINT(eventName, hint) { static const String& _ProfilerEventName = (eventName); static const int _profilerHashBase = int(HashTrait<String>::hashCode(__FILE__)) + __LINE__; Profiler::beginEvent(_ProfilerEventName, __FILE__, __LINE__, _profilerHashBase, hint); }
#define BEGIN_PROFILER_EVENT(eventName) { BEGIN_PROFILER_EVENT_WITH_HINT(eventName, "") }
/** \def END_PROFILER_EVENT 
    \sa BEGIN_PROFILER_EVENT, Profiler, Profiler::endEvent
    */
#define END_PROFILER_EVENT() Profiler::endEvent()

#ifdef DEFINED_GL_NONE
#   undef GL_NONE
#   undef DEFINED_GL_NONE
#endif

#endif
