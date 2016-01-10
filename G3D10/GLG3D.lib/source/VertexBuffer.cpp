/**
 @file VertexBuffer.cpp
 
 Implementation of the vertex array system used by RenderDevice.

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2003-01-08
 @edited  2011-11-24
 */

#include "G3D/Log.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

Array< shared_ptr<VertexBuffer> >    VertexBuffer::s_allVertexBuffers;
Array< bool >    VertexBuffer::s_vertexBufferUsedThisFrame;

size_t VertexBuffer::m_sizeOfAllVertexBuffersInMemory = 0;

void VertexBuffer::resetCacheMarkers() {
    for (int i = 0; i < s_vertexBufferUsedThisFrame.size(); ++i) {
        s_vertexBufferUsedThisFrame[i] = false;
    }
}

shared_ptr<VertexBuffer>  VertexBuffer::getUnusedVertexBuffer(size_t minSize, UsageHint usageHint) {
    shared_ptr<VertexBuffer> result;
    int resultIndex = -1;
    for( int i = 0; i < s_allVertexBuffers.size(); ++i) {
        const shared_ptr<VertexBuffer>& candidate = s_allVertexBuffers[i];
        if (candidate.unique() && 
                (!s_vertexBufferUsedThisFrame[i]) && 
                candidate->totalSize() >= minSize && 
                candidate->usageHint() == usageHint && 
                (isNull(result) || result->totalSize() > candidate->totalSize()) ) {
            result = s_allVertexBuffers[i];
            resultIndex = i;
        }
    }
    if (resultIndex >= 0) {
        s_vertexBufferUsedThisFrame[resultIndex] = true;
    }
    return result;
}

shared_ptr<VertexBuffer> VertexBuffer::create(size_t s, UsageHint h) {
    shared_ptr<VertexBuffer> vertexBuffer = getUnusedVertexBuffer(s, h);
    if (isNull(vertexBuffer)) {
        vertexBuffer = shared_ptr<VertexBuffer>(new VertexBuffer(s, h));
        s_allVertexBuffers.push(vertexBuffer);
        s_vertexBufferUsedThisFrame.push(true);
    } else {
        vertexBuffer->reset();
    }
    return vertexBuffer;
}


VertexBuffer::VertexBuffer(size_t _size, UsageHint hint) :  m_size(_size), m_usageHint(hint) {
    m_renderDevice = NULL;
    debugAssertGLOk();

    m_sizeOfAllVertexBuffersInMemory += m_size;
    glGenBuffers(1, &m_glbuffer);

    // GL allows us to reserve space using any target type; we can later switch to index
    glBindBuffer(GL_ARRAY_BUFFER, m_glbuffer);
            
    GLenum usage;
            
    switch (hint) {
    case WRITE_EVERY_FRAME:
        usage = GL_STREAM_DRAW;
        break;
                
    case WRITE_ONCE:
        usage = GL_STATIC_DRAW;
        break;
                
    case WRITE_EVERY_FEW_FRAMES:
        usage = GL_DYNAMIC_DRAW;
        break;
                
    default:
        usage = GL_STREAM_DRAW;
        debugAssertM(false, "Fell through switch");
    }
            
    // Load some (undefined) data to initialize the buffer
    glBufferData(GL_ARRAY_BUFFER, m_size, NULL, usage);
    debugAssertGLOk();    
            
    // The m_basePointer is always NULL for a VBO
    m_basePointer = NULL;
            
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    debugAssertGLOk();

    m_allocated     = 0;
    m_generation    = 1;
    m_peakAllocated = 0;
}


VertexBuffer::~VertexBuffer() {
    m_sizeOfAllVertexBuffersInMemory -= m_size;

    if (m_size == 0) {
        // Already freed
        return;
    }

    // Delete the vertex buffer
    glDeleteBuffers(1, &m_glbuffer);
    m_glbuffer = 0;

    m_size = 0;
}


void VertexBuffer::finish() {
   
}


void VertexBuffer::reset() {
    finish();
    ++m_generation;
    m_allocated = 0;
}


void VertexBuffer::cleanCache() {
    int i = 0;
    while (i < s_allVertexBuffers.size()) {
        if (s_allVertexBuffers[i].unique()) {
            s_allVertexBuffers.fastRemove(i);
            s_vertexBufferUsedThisFrame.fastRemove(i);
        } else {
            ++i;
        }
    }
}


void VertexBuffer::cleanupAllVertexBuffers() {
    // Intentionally empty
    for (int i = 0; i < s_allVertexBuffers.size(); ++i) {
        //debugAssert((void*)s_allVertexBuffers[i]->m_renderDevice != (void*)0xcdcdcdcd);

        s_allVertexBuffers[i]->reset();
    }
    s_allVertexBuffers.clear();
    s_vertexBufferUsedThisFrame.clear();
}

} // namespace
