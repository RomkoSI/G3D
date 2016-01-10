/**
  \file GLG3D/GLPixelTransferBuffer.h
 
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef GLG3D_GLPixelTransferBuffer_h
#define GLG3D_GLPixelTransferBuffer_h

#include "G3D/PixelTransferBuffer.h"
#include "GLG3D/glheaders.h"

namespace G3D {

class ImageFormat;
class Milestone;

/** Abstraction of OpenGL Pixel Buffer Object, an efficient way of transferring
    data to or from a GPU. 
    
   \sa PixelTransferBuffer, CPUPixelTransferBuffer, Image, Texture, VideoInput, VideoOutput, ImageFormat, VertexBuffer, Material, UniversalMaterial, Texture::toPixelTransferBuffer

*/
class GLPixelTransferBuffer : public PixelTransferBuffer {
protected:

    /** If this was created from data on the GPU, then m_milestone is the milestone that must be 
        reached before the data can be memory mapped on the CPU. */
    shared_ptr<Milestone>       m_milestone;
    unsigned int                m_glBufferID;

    GLPixelTransferBuffer(const ImageFormat* format, int width, int height, int depth, const void* data, uint32 glUsageHint);

    void map(unsigned int access) const;

public:

    /** Calls glDeleteBuffers on the free list */
    static void deleteAllBuffers();

    /** The underlying OpenGL buffer ID */
    unsigned int glBufferID() const {
        return m_glBufferID;
    }

    /** Bind this as the current OpenGL GL_PIXEL_PACK_BUFFER so that OpenGL can write to it.*/
    void bindWrite();

    /** Bind this as the current OpenGL GL_PIXEL_UNPACK_BUFFER so that OpenGL can read from it.*/
    void bindRead();

    /** Unbind the current OpenGL GL_PIXEL_PACK_BUFFER.
    
        Sets the G3D::Milestone on this buffer; it will not be readyToMap() until all GPU commands
        issued prior to unbind() have completed execution.*/
    void unbindWrite();

    /** Unbind the current OpenGL GL_PIXEL_UNPACK_BUFFER.
    
        Sets the G3D::Milestone on this buffer; it will not be readyToMap() until all GPU commands
        issued prior to unbind() have completed execution.*/
    void unbindRead();

    /** Creates a buffer backed by an OpenGL PBO of uninitialized values. 
        \param data If non-NULL, copy this data to the GPU as the initial value of the buffer.
        The pointer is not returned and the data may be deallocated as soon as the method returns.*/
    static shared_ptr<GLPixelTransferBuffer> create
        (int                            width, 
         int                            height, 
         const ImageFormat*             format,
         const void*                    data = NULL,
         int                            depth = 1,
         uint32                         glUsageHint = GL_STREAM_COPY);

    virtual ~GLPixelTransferBuffer();

    /** Obtain a pointer for general access.
        \sa mapRead, mapWrite, unmap
    */
    virtual void* mapReadWrite() override;

    /** Obtain a pointer for write-only access.
        \sa mapRead, mapReadWrite, unmap
        */
    virtual void* mapWrite() override;

    /** Obtain a pointer for read-only access.
    \sa mapReadWrite, mapWrite, unmap */
    virtual const void* mapRead() const override;

    virtual void unmap() const override;

    /** Returns true if calls to mapX() will not block the CPU on the GPU. */
    virtual bool readyToMap() const override;

    virtual bool requiresGPUContext() const override {
        return true;
    }

    /** Overwrite the current contents with \a data.  Cannot call while mapped. */
    virtual void setData(const void* data) override;

    /** Read back the current contents to \a data.  Cannot call while mapped. */
    virtual void getData(void* data) const override;

    /**
        \param srcUpperLeftPixelIndex Starting pixel index, in row-major order
        \param srcSizePixels -1 means size of src
    */ 
    static void copy
        (const shared_ptr<GLPixelTransferBuffer>&   src, 
         const shared_ptr<GLPixelTransferBuffer>&   dst, 
         int                               srcSizePixels = -1, 
         int                               srcUpperLeftPixelIndex = 0, 
         int                               dstUpperLeftPixelIndex = 0);


	/** CC: NVIDIA-only bindless GPU pointers */
	uint64 getGPUAddress(GLenum access = GL_READ_WRITE) const;
};

} // namespace G3D

#endif
