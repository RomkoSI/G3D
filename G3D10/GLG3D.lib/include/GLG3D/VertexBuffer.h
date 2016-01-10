/**
  \file GLG3D/VertexBuffer.h
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2003-08-09
  \edited  2012-11-19
 
 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#ifndef GLG3D_VertexBuffer_h
#define GLG3D_VertexBuffer_h

#include "G3D/ReferenceCount.h"
#include "G3D/Table.h"
#include "GLG3D/glheaders.h"

namespace G3D {

// Forward-declaration because including RenderDevice.h would include VertexBuffer.h also
class RenderDevice;
class Milestone;

/**
 \brief A block of GPU memory within which G3D::AttributeArray%s for
 vertex, texcoord, normal, etc. arrays or index lists can be
 allocated.
 
 Allocate a VertexBuffer, then allocate AttributeArray%s and IndexStream%s within it.  For example:

 \code
    // Allocate on CPU
    Array<Vector3>   cpuVertex;
    Array<Vector2>   cpuTexCoord;
    Array<int>       cpuIndex;

    ... initialize ...

    // Upload to GPU
    shared_ptr<VertexBuffer> vbuffer = VertexBuffer::create((sizeof(Vector3) + sizeof(Vector2)) * cpuVertex.size() + sizeof(int) * cpuIndex.size());
    AttributeArray gpuVertex   = AttributeArray(cpuVertex, vbuffer);
    AttributeArray gpuTexCoord = AttributeArray(cpuTexCoord, vbuffer);
    IndexStream gpuIndex       = IndexStream(cpuIndex, vbuffer);
 \endcode

 VertexBuffer%s are garbage collected: when no pointers remain to
 AttributeArray%s inside it or the VertexBuffer itself, it will
 automatically be reclaimed by the system.

 You cannot mix pointers from different VertexBuffer%s when rendering.
 For example, if the vertex AttributeArray is in one VertexBuffer, the
 normal AttributeArray and color AttributeArray must come from the same
 area.

 This class corresponds closely to the OpenGL Vertex Buffer Object
 http://oss.sgi.com/projects/ogl-sample/registry/ARB/vertex_buffer_object.txt
 http://developer.nvidia.com/docs/IO/8230/GDC2003_OGL_BufferObjects.ppt
 */
class VertexBuffer : public ReferenceCountedObject {
public:
    typedef shared_ptr<class VertexBuffer> Ref;


    /**
     These values are <B>hints</B>. Your program will work correctly
     regardless of which you use, but using the appropriate value
     lets the renderer optimize for your useage patterns and can
     increase performance.

     Use WRITE_EVERY_FRAME if you write <I>at least</I> once per frame
     (e.g. software animation).

     Use WRITE_EVERY_FEW_FRAMES if you write to the area as part of
     the rendering loop, but not every frame (e.g. impostors, deformable
     data).

     Use WRITE_ONCE if you do not write to the area inside the rendering
     loop (e.g. rigid bodies loaded once at the beginning of a game level).  
     This does not mean you can't write multiple times
     to the area, just that writing might be very slow compared to rendering.

     Correspond to OpenGL hints: 
      WRITE_ONCE : GL_STATIC_DRAW_ARB
      WRITE_EVERY_FRAME : GL_STREAM_DRAW_ARB
      WRITE_EVERY_FEW_FRAMEs : DYNAMIC_DRAW_ARB
     */
    enum UsageHint {
        WRITE_ONCE,
        WRITE_EVERY_FEW_FRAMES,
        WRITE_EVERY_FRAME};
    
private:

    friend class AttributeArray;
    friend class RenderDevice;
    
    /** Number of bytes currently m_allocated out of m_size total. */
    size_t                              m_allocated;
    
    /**
       This count prevents vertex arrays that have been freed from
       accidentally being used-- it is incremented every time
       the VertexBuffer is reset.
    */
    uint64                              m_generation;
    
    /** The maximum m_size of this area that was ever used. */
    size_t                              m_peakAllocated;
    
    /** Set by RenderDevice */
    RenderDevice*                       m_renderDevice;
    
    /** Total  number of bytes in this area.  May be zero if resources have been freed.*/
    size_t                              m_size;
    
    /**
       The OpenGL buffer object associated with this area
    */
    uint32                              m_glbuffer;
    
    /** Pointer to the memory (NULL when
        the VBO extension is not present). */
    void*                               m_basePointer;

    /** The usage hint the buffer was created with */
    UsageHint                           m_usageHint;
        
    /** Updates allocation and peakAllocation based off of new allocation. */
    inline void updateAllocation(size_t newAllocation) {
        m_allocated += newAllocation;
        m_peakAllocated = max(m_peakAllocated, m_allocated);
    }

    static size_t                       m_sizeOfAllVertexBuffersInMemory;

    static Array< bool >                          s_vertexBufferUsedThisFrame;
    static Array<shared_ptr<VertexBuffer> >       s_allVertexBuffers;
    
    /** Removes elements of s_allVertexBuffers that are not externally referenced. */
    static void cleanCache();

    /** Returns a pointer to a non-externally-referenced vertex buffer of size at least minSize with \param usageHint.
        if there is one in the cache, else returns null. */
    static shared_ptr<VertexBuffer> getUnusedVertexBuffer(size_t minSize, UsageHint usageHint);
    
    VertexBuffer(size_t _size, UsageHint h);

public:

    static void resetCacheMarkers();
    
    /**
       You should always create your VertexBuffers at least 8 bytes larger
       than needed for each individual AttributeArray because VertexBuffer tries to 
       align AttributeArray starts in memory with dword boundaries.
     */
    static shared_ptr<VertexBuffer> create(size_t s, UsageHint h = WRITE_EVERY_FRAME);

    ~VertexBuffer();
    
    inline UsageHint usageHint() const {
        return m_usageHint;
    }

    inline size_t totalSize() const {
        return m_size;
    }

    inline size_t freeSize() const {
        return m_size - m_allocated;
    }

    inline size_t allocatedSize() const {
        return m_allocated;
    }

    inline size_t peakAllocatedSize() const {
        return m_peakAllocated;
    }

    inline uint64 currentGeneration() const {
        return m_generation;
    }

    /**
     Provided for breaking the VertexBuffer abstraction; use G3D::AttributeArray and 
     G3D::RenderDevice in general.

     When using the OpenGL vertex buffer API, this is the underlying 
     OpenGL vertex buffer object.  It is zero when using system memory.
     The caller cannot control whether VBO is used or not; G3D selects
     the best method automatically.
     */
    inline uint32 openGLVertexBufferObject() const {
        return m_glbuffer;
    }

    /**
     Provided for breaking the VertexBuffer abstraction; use G3D::AttributeArray and 
     G3D::RenderDevice in general.

     When using system memory, this is a pointer to the beginning of 
     the system memory block in which data is stored.  Null when using VBO.
     */
    inline void* openGLBasePointer() const {
        return m_basePointer;
    }

    /**
     Blocks the CPU until all rendering calls referencing 
     this area have completed.
     */
    void finish();

    /** Finishes, then frees all AttributeArray memory inside this area.*/ 
    void reset();

    /** Returns the total m_size of all VertexBuffers allocated.  Note that not all
        will be in video memory, and some will be backed by main memory 
        even if nominally stored in video memory, so the total size may
        exceed the video memory size.*/
    static size_t sizeOfAllVertexBuffersInMemory() {
        return m_sizeOfAllVertexBuffersInMemory;
    }

    /** Releases all VertexBuffers. Called before shutdown by RenderDevice. */
    static void cleanupAllVertexBuffers();

};

} // namespace G3D

template <> struct HashTrait<G3D::VertexBuffer*> {
    static size_t hashCode(const G3D::VertexBuffer* key) { return reinterpret_cast<size_t>(key); }
};

#endif
