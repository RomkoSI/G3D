/**
 \file Profiler.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2009-01-01
 \edited  2013-07-21

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/stringutils.h"
#include "G3D/Log.h"
#include "GLG3D/Profiler.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {
    
__thread shared_ptr<Profiler::ThreadInfo>*  Profiler::s_threadInfo = NULL;
__thread int                                Profiler::s_level = 0;

Array< shared_ptr<Profiler::ThreadInfo> >   Profiler::s_threadInfoArray;
GMutex                                      Profiler::s_profilerMutex;
uint64                                      Profiler::s_frameNum = 0;
bool                                        Profiler::s_enabled = false;
bool                                        Profiler::s_timeShaderLaunches = true;

void Profiler::set_LAUNCH_SHADER_timingEnabled(bool enabled) {
    s_timeShaderLaunches = enabled;
}

   
bool Profiler::LAUNCH_SHADER_timingEnabled() {
    return s_timeShaderLaunches;
}

GLuint Profiler::ThreadInfo::newQueryID() {
    if (queryFreelist.length() == 0) {
        queryFreelist.resize(queryFreelist.length() + 10);
        glGenQueries(10, &queryFreelist[queryFreelist.length() - 10]);
        debugAssertGLOk();
    }
    return queryFreelist.pop(false);
}


Profiler::ThreadInfo::~ThreadInfo() {
    glDeleteQueries(queryFreelist.length(), queryFreelist.getCArray());
    queryFreelist.clear();
    debugAssertGLOk();
}


void Profiler::ThreadInfo::beginEvent(const String& name, const String& file, int line, const size_t baseHash, const String& hint) {
    Event event; //eventTree.next();
    event.m_hash = HashTrait<String>::hashCode(name) ^ HashTrait<String>::hashCode(file) ^ size_t(line) ^ HashTrait<String>::hashCode(hint);
    if (ancestorStack.length() == 0) {
        event.m_parentIndex = -1;
    } else {
        event.m_parentIndex = ancestorStack.last();
        if (eventTree[event.m_parentIndex].m_numChildren == 0) {
            ++eventTree[event.m_parentIndex].m_numChildren;
            Event dummy;
            dummy.m_name = "other";
            dummy.m_file = eventTree[event.m_parentIndex].m_file;
            dummy.m_line = eventTree[event.m_parentIndex].m_line;
            dummy.m_level = s_level;
            dummy.m_openGLStartID = GL_NONE;
            dummy.m_openGLEndID = GL_NONE;
            dummy.m_hash = eventTree[event.m_parentIndex].hash() ^ HashTrait<String>::hashCode(dummy.m_name);
            dummy.m_parentIndex = ancestorStack.last();
            eventTree.append(dummy);
        }
        ++eventTree[event.m_parentIndex].m_numChildren;
        event.m_hash = eventTree[event.m_parentIndex].m_hash ^ event.m_hash;
        const Event& prev = eventTree.last();
        if (prev.m_name == name && prev.m_file == file && prev.m_line == line && prev.m_hint == hint) {
            event.m_hash = prev.m_hash + 1;
        }
    }
    ancestorStack.push(eventTree.length());

    if (RenderDevice::current) {
        // Take a GPU sample
        event.m_openGLStartID = newQueryID();
        glQueryCounter(event.m_openGLStartID, GL_TIMESTAMP);
        debugAssertGLOk();
    }

    // Take a CPU sample
    event.m_name = name;
    event.m_file = file;
    event.m_line = line;
    event.m_hint = hint;
    event.m_cpuStart = System::time();
    event.m_level = s_level;
    ++s_level;
    eventTree.append(event);
}


void Profiler::ThreadInfo::endEvent() {
    const int i = ancestorStack.pop();

    Event& event(eventTree[i]);

    if (RenderDevice::current) {
        // Take a GPU sample
        event.m_openGLEndID = newQueryID();
        glQueryCounter(event.m_openGLEndID, GL_TIMESTAMP);
        debugAssertGLOk();
    }

    // Take a CPU sample
    event.m_cpuEnd = System::time();
    --s_level;
}

//////////////////////////////////////////////////////////////////////////

int Profiler::calculateUnaccountedTime(Array<Event>& eventTree, const int index, RealTime& cpuTime, RealTime& gpuTime) {
    const Event& event = eventTree[index];
    
    if (event.m_numChildren == 0) {
        cpuTime = event.cpuDuration();
        gpuTime = event.gfxDuration();
        return index + 1;    
    }        
    
    int childIndex = index + 2;
    RealTime totalChildCpu = 0;
    RealTime totalChildGfx = 0;
    for (int child = 1; child < event.m_numChildren; ++child) {
        RealTime childCpu = 0;
        RealTime childGfx = 0;
        childIndex = calculateUnaccountedTime(eventTree, childIndex, childCpu, childGfx);
        totalChildCpu += childCpu;
        totalChildGfx += childGfx;
    }

    Event& dummy = eventTree[index + 1];
    dummy.m_cpuStart = 0;
    dummy.m_cpuEnd = event.cpuDuration() - totalChildCpu;
    dummy.m_gfxStart = 0;
    dummy.m_gfxEnd = event.gfxDuration() - totalChildGfx;

    cpuTime = event.cpuDuration();
    gpuTime = event.gfxDuration();
    return childIndex;
}

void Profiler::threadShutdownHook() {
    GMutexLock lock(&s_profilerMutex);
    const int i = s_threadInfoArray.findIndex(*s_threadInfo);
    alwaysAssertM(i != -1, "Could not find thread info during thread destruction for Profiler");
    s_threadInfoArray.remove(i);

    delete s_threadInfo;
    s_threadInfo = NULL;
}


void Profiler::beginEvent(const String& name, const String& file, int line, const size_t baseHash, const String& hint) {
    if (! s_enabled) { return; }

    if (isNull(s_threadInfo)) {
        // First time that this thread invoked beginEvent--intialize it
        s_threadInfo = new shared_ptr<ThreadInfo>(new ThreadInfo());
        GMutexLock lock(&s_profilerMutex);
        s_threadInfoArray.append(*s_threadInfo);
    }

    (*s_threadInfo)->beginEvent(name, file, line, baseHash, hint);
}


void Profiler::endEvent() {
    if (! s_enabled) { return; }
    (*s_threadInfo)->endEvent();
}


void Profiler::setEnabled(bool e) {
    s_enabled = e;
}


void Profiler::nextFrame() {
    if (! s_enabled) { return; }

    GMutexLock lock(&s_profilerMutex);
    debugAssertGLOk();

    // For each thread
    for (int t = 0; t < s_threadInfoArray.length(); ++t) {
        shared_ptr<ThreadInfo> info = s_threadInfoArray[t];

        for (int e = 0; e < info->eventTree.length(); ++e) {
            Event& event = info->eventTree[e];
            if (event.m_openGLStartID != GL_NONE) {
                #ifdef G3D_OSX
                // Attempt to get OS X profiling working:

                // Wait for the result if necessary
                const int maxCount = 1000000;
                int count;
                GLint available = GL_FALSE;
                glGetQueryObjectiv(event.m_openGLStartID, GL_QUERY_RESULT_AVAILABLE, &available); 
                for (count = 0; (available != GL_TRUE) && (count < maxCount); ++count) {
                    glGetQueryObjectiv(event.m_openGLStartID, GL_QUERY_RESULT_AVAILABLE, &available);
                }
                
                if (count > maxCount) {
                    debugPrintf("Timed out on GL_QUERY_RESULT_AVAILABLE\n");
                }
                #endif

                // Take the GFX times now
                // Get the time, in nanoseconds
                GLuint64EXT ns = 0;
                glGetQueryObjectui64v(event.m_openGLStartID, GL_QUERY_RESULT, &ns);
                event.m_gfxStart = ns * 1e-9;
                debugAssertGLOk();

                glGetQueryObjectui64v(event.m_openGLEndID, GL_QUERY_RESULT, &ns);
                event.m_gfxEnd = ns * 1e-9;
                debugAssertGLOk();

                // Free the OpenGL query objects to our buffer pool
                info->queryFreelist.append(event.m_openGLStartID, event.m_openGLEndID);
                event.m_openGLStartID = GL_NONE;
                event.m_openGLEndID   = GL_NONE;
            } // if GFX
        } // e

        RealTime a, b;
        for (int e = 0; e < info->eventTree.length(); e = calculateUnaccountedTime(info->eventTree, e, a, b));

        // Swap pointers (to avoid a massive array copy)
        Array<Event>::swap(info->previousEventTree, info->eventTree);

        // Clear (but retain the underlying storage to avoid allocation next frame)
        info->eventTree.fastClear();
    } // t

    ++s_frameNum;
}

void Profiler::getEvents(Array<const Array<Event>*>& eventTrees) {
    eventTrees.fastClear();
    GMutexLock lock(&s_profilerMutex);
    
    for (int t = 0; t < s_threadInfoArray.length(); ++t) {
        eventTrees.append(&s_threadInfoArray[t]->previousEventTree);
    }
}

} // namespace G3D
