/**
  \file G3D/PixelTransferBuffer.h
 
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_PixelTransferBuffer_h
#define G3D_PixelTransferBuffer_h

#include "G3D/platform.h"
#include "G3D/MemoryManager.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"

namespace G3D {

/**
  \brief Base class for transfering arrays of pixels between major classes, generalized
  over CPU arrays, memory-mapped files, and OpenGL Pixel Buffer Objects.

  Beware that because the memory accessed through mapRead mapWrite mapReadWrite may be
  memory mapped, it may not be cached in the same was as general CPU memory, and thus
  random access and mixed read-write may have unexpected performance characteristics.

  \sa GLPixelTransferBuffer, CPUPixelTransferBuffer, Image, Texture, VideoInput, VideoOutput, ImageFormat, VertexBuffer, Material, UniversalMaterial
  
 */
class PixelTransferBuffer : public ReferenceCountedObject {
protected:

    /** NULL if not currently mapped. */
    mutable void*       m_mappedPointer;

    const ImageFormat*  m_format;
    size_t              m_rowAlignment;
    size_t              m_rowStride;

    int                 m_width;
    int                 m_height;
    int                 m_depth;

    PixelTransferBuffer(const ImageFormat* format, int width, int height, int depth, int rowAlignment);

public:

    virtual ~PixelTransferBuffer();

    const ImageFormat* format() const   { return m_format; }

    /** Returns entire size of pixel data in bytes. */
    size_t size() const                 { return m_height * m_depth * m_rowStride; }

    /** Returns alignment of each row of pixel data in bytes. */
    size_t rowAlignment() const         { return m_rowAlignment; }

    /** Returns size of each row of pixel data in bytes. */
    size_t stride() const               { return m_rowStride; }

    int width() const                   { return m_width; }
    int height() const                  { return m_height; }
    int depth() const                   { return m_depth; }
    int pixelCount() const              { return m_width * m_height * m_depth; }

    /** Obtain a pointer for general access.
        \sa mapRead, mapWrite, unmap
    */
    virtual void* mapReadWrite() = 0;

    /** Obtain a pointer for write-only access.
        \sa mapRead, mapReadWrite, unmap
        */
    virtual void* mapWrite() = 0;

    /** Obtain a pointer for read-only access.
    \sa mapReadWrite, mapWrite, unmap */
    virtual const void* mapRead() const = 0;

    /** \sa G3D::BufferUnmapper */
    virtual void unmap() const = 0;

    template<class T> void unmap(const T*& setToNull) const {
        unmap();
        setToNull = NULL;
    }

    /** If true, mapReadWrite, mapRead, and mapWrite will return immediately.  
    
        This is always true for the base class but subclasses that map Pixel Buffer Objects 
        and files may have a delay between when they are constructed and when they are available
        for mapping. The standard usage in those cases is to have the GL thread check if data
        is ready to map, and if not, then wait until the next frame to perform the mapping.
      */
    virtual bool readyToMap() const = 0;

    /** If true, then readyToMap(), mapRead(), mapWrite(), mapReadWrite(), and
        unmap() can only be invoked on a thread that currently has an active OpenGL
        context.  Always returns false for the base class. */
    virtual bool requiresGPUContext() const = 0;


    /** Overwrite the current contents with \a data.  Cannot call while mapped. */
    virtual void setData(const void* data) = 0;

    /** Read back the current contents to \a data.  Cannot call while mapped. */
    virtual void getData(void* data) const = 0;
};


/** "Frees" mapped memory by unmapping it. Useful when passing a mapped PixelTransferBuffer to a NetConnection
    or other API that can use a memory manager for deallocation. */
class BufferUnmapper : public MemoryManager {
protected:
    shared_ptr<PixelTransferBuffer> m_buffer;

    BufferUnmapper(const shared_ptr<PixelTransferBuffer>& b) : m_buffer(b) {}
    
public:
    static shared_ptr<BufferUnmapper> create(const shared_ptr<PixelTransferBuffer>& b) { return shared_ptr<BufferUnmapper>(new BufferUnmapper(b)); }
    virtual void* alloc(size_t s) override { debugAssert(false); return NULL; }
    virtual void free(void *ptr) override { m_buffer->unmap(); m_buffer.reset(); }
    virtual bool isThreadsafe() const override { return false; }
};

} // namespace G3D

#endif // G3D_PixelTransferBuffer_h
