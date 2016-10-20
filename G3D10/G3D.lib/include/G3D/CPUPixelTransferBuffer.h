/**
  \file G3D/CPUPixelTransferBuffer.h
 
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_CPUPixelTransferBuffer_h
#define G3D_CPUPixelTransferBuffer_h

#include "G3D/platform.h"
#include "G3D/PixelTransferBuffer.h"

namespace G3D {

/** A PixelTransferBuffer in main memory.
    \sa GLPixelTransferBuffer, Image */
class CPUPixelTransferBuffer : public PixelTransferBuffer {
protected:

    shared_ptr<MemoryManager>   m_memoryManager;
    void*                       m_buffer;

    void allocateBuffer(shared_ptr<MemoryManager> memoryManager);
    void freeBuffer();

    CPUPixelTransferBuffer(const ImageFormat* format, int width, int height, int depth, int rowAlignment);

public:

    /** Creates a buffer backed by a CPU array of uninitialized contents. */
    static shared_ptr<CPUPixelTransferBuffer> create
        (int                            width, 
         int                            height, 
         const ImageFormat*             format, 
         shared_ptr<MemoryManager>      memoryManager   = MemoryManager::create(), 
         int                            depth           = 1, 
         int                            rowAlignment    = 1);

    /** Creates a buffer backed by a CPU array of existing data that will be managed by the caller.  That data must stay
        allocated while the buffer is in use.*/
    static shared_ptr<CPUPixelTransferBuffer> fromData
        (int                            width, 
         int                            height, 
         const ImageFormat*             format, 
         void*                          data,
         int                            depth           = 1, 
         int                            rowAlignment    = 1);

    virtual ~CPUPixelTransferBuffer();

    /** Returns pointer to raw pixel data */
    void* buffer()                      { return m_buffer; }

    /** Returns pointer to raw pixel data */
    const void* buffer() const          { return m_buffer; }

    /** Return row to raw pixel data at start of row \a y of depth \a d. 
    */
    void* row(int y, int d = 0) {
        debugAssert(y < m_height && d < m_depth);
        return static_cast<uint8*>(m_buffer) + (d * m_height * m_rowStride) + (y * m_rowStride); 
    }

    /** Return row to raw pixel data at start of row \a y of depth \a d.
    */
    const void* row(int y, int d = 0) const {
        debugAssert(y < m_height && d < m_depth);
        return static_cast<uint8*>(m_buffer) + (d * m_height * m_rowStride) + (y * m_rowStride); 
    }

    virtual void* mapReadWrite() override;

    virtual void* mapWrite() override;

    virtual const void* mapRead() const override;

    virtual void unmap() const override;

    virtual bool readyToMap() const override {
        return true;
    }

    virtual bool requiresGPUContext() const override {
        return false;
    }

    /** Overwrite the current contents with \a data.  Cannot call while mapped. */
    virtual void setData(const void* data) override;

    /** Read back the current contents to \a data.  Cannot call while mapped. */
    virtual void getData(void* data) const override;
};

} // namespace G3D

#endif // G3D_PixelTransferBuffer_h
