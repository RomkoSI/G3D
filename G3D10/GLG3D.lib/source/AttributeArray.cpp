/**
 \file AttributeArray.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 \created 2003-04-08
 \edited  2012-07-26
 */

#include "G3D/Log.h"
#include "GLG3D/AttributeArray.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

AttributeArray::AttributeArray() : 
    m_pointer(NULL), m_elementSize(0), 
    m_numElements(0), m_stride(0), m_generation(0), 
    m_underlyingRepresentation(GL_NONE), m_maxSize(0), m_normalizedFixedPoint(false) {
}


AttributeArray::AttributeArray(size_t numBytes, const shared_ptr<VertexBuffer>& area) : 
    m_pointer(NULL), m_elementSize(0), 
    m_numElements(0), m_stride(0), m_generation(0), 
    m_underlyingRepresentation(GL_NONE), m_maxSize(0), m_normalizedFixedPoint(false) {

    init(NULL, (int)numBytes, area, GL_NONE, 1, false);
}


bool AttributeArray::valid() const {
    return notNull(m_area) && 
        (m_area->currentGeneration() == m_generation);
}


void AttributeArray::init
(AttributeArray& dstPtr,
 size_t       dstOffset,
 GLenum       glformat,
 size_t       eltSize, 
 int          numElements,
 size_t       dstStride,
 bool         normalizedFixedPoint) {

    m_area = dstPtr.m_area;
    alwaysAssertM(m_area, "Bad VertexBuffer");

    m_numElements              = numElements;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_stride                   = dstStride;
    m_maxSize                  = dstPtr.m_maxSize / dstStride;
    m_normalizedFixedPoint     = normalizedFixedPoint;

    m_generation = m_area->currentGeneration();
    m_pointer = (uint8*)dstPtr.m_pointer + dstOffset;
}


void AttributeArray::init
(const void* srcPtr,
 int         numElements, 
 size_t      srcStride,      
 GLenum      glformat, 
 size_t      eltSize,
 AttributeArray dstPtr,
 size_t      dstOffset, 
 size_t      dstStride,
 bool        normalizedFixedPoint) {

    m_area = dstPtr.m_area;
    alwaysAssertM(m_area, "Bad VertexBuffer");

    m_numElements              = numElements;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_stride                   = dstStride;
    m_maxSize                  = dstPtr.m_maxSize / dstStride;
    m_normalizedFixedPoint     = normalizedFixedPoint;

    debugAssertM(
        (m_elementSize % sizeOfGLFormat(m_underlyingRepresentation)) == 0,
        "Sanity check failed on OpenGL data format; you may"
        " be using an unsupported type in a vertex array.");

    m_generation = m_area->currentGeneration();

    m_pointer = (uint8*)dstPtr.m_pointer + dstOffset;

    // Upload the data
    if ((numElements > 0) && (srcPtr != NULL)) {
        uploadToCardStride(srcPtr, numElements, eltSize, srcStride, 0, dstStride);
    }
}


void AttributeArray::init
(const void*          sourcePtr,
 int                  numElements,
 shared_ptr<VertexBuffer> area,
 GLenum               glformat,
 size_t               eltSize,
 bool                 normalizedFixedPoint) {
    
    alwaysAssertM(notNull(area), "Bad VertexBuffer");

    m_numElements              = numElements;
    m_area                     = area;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_stride                   = eltSize;
    m_normalizedFixedPoint     = normalizedFixedPoint;

    size_t size                = m_elementSize * m_numElements;
    m_maxSize                  = size;

    debugAssertM(
                 (sourcePtr == NULL) ||
                 (m_elementSize % sizeOfGLFormat(m_underlyingRepresentation)) == 0,
                 "Sanity check failed on OpenGL data format; you may"
                 " be using an unsupported type in a vertex array.");

    m_generation = m_area->currentGeneration();

    m_pointer = (uint8*)m_area->openGLBasePointer() + m_area->allocatedSize();

    // Align to the nearest multiple of this many bytes.
    const size_t alignment = 4;

    // Ensure that the next memory address is aligned.  This has 
    // a significant (up to 25%!) performance impact on some GPUs
    size_t pointerOffset = (size_t)((alignment - (intptr_t)m_pointer % alignment) % alignment);

    if (numElements == 0) {
        pointerOffset = 0;
    }

    // Adjust pointer to new alignment
    m_pointer = (uint8*)m_pointer + pointerOffset;
    
    const size_t newAlignedSize = size + pointerOffset;

    alwaysAssertM(newAlignedSize <= m_area->freeSize(),
                  "VertexBuffer too small to hold new AttributeArray (possibly due to rounding"
                  " to the nearest dword boundary).");

    // Update VertexBuffer values
    m_area->updateAllocation(newAlignedSize);

    // Upload the data
    if ((size > 0) && notNull(sourcePtr)) {
        uploadToCard(sourcePtr, 0, size);
    }
}


void AttributeArray::update
(const void*         sourcePtr,
 int                 numElements,
 GLenum              glformat,
 size_t              eltSize,
 bool                normalizedFixedPoint) {
    
    size_t size = eltSize * numElements;

    debugAssert(m_stride == 0 || m_stride == m_elementSize);
    alwaysAssertM(size <= m_maxSize,
        "A AttributeArray can only be updated with an array that is smaller "
        "or equal size (in bytes) to the original array.");

    alwaysAssertM(m_generation == m_area->currentGeneration(),
        "The VertexBuffer has been reset since this AttributeArray was created.");

    m_numElements              = numElements;
    m_underlyingRepresentation = glformat;
    m_elementSize              = eltSize;
    m_normalizedFixedPoint     = normalizedFixedPoint;

    debugAssertM((m_elementSize % sizeOfGLFormat(m_underlyingRepresentation)) == 0,
                 "Sanity check failed on OpenGL data format; you may"
                 " be using an unsupported type in a vertex array.");
    
    // Upload the data
    if (size > 0) {
        uploadToCard(sourcePtr, 0, size);
    }
    debugAssertGLOk();
}


void AttributeArray::set(int index, const void* value, GLenum glformat, size_t eltSize) {
    debugAssert(m_stride == 0 || m_stride == m_elementSize);
    (void)glformat;
    debugAssertM(index < m_numElements && index >= 0, 
        "Cannot call AttributeArray::set with out of bounds index");
    
    debugAssertM(glformat == m_underlyingRepresentation, 
        "Value argument to AttributeArray::set must match the intialization type.");

    debugAssertM((size_t)eltSize == m_elementSize, 
        "Value argument to AttributeArray::set must match the intialization type's memory footprint.");

    uploadToCard(value, index * eltSize, eltSize);
}


void* AttributeArray::mapBuffer(GLenum permissions) {
    // Map buffer
    glBindBuffer(openGLTarget(), m_area->m_glbuffer);
    return (uint8*)glMapBuffer(openGLTarget(), permissions) + (intptr_t)m_pointer;
}


void AttributeArray::unmapBuffer() {
    glUnmapBuffer(openGLTarget());
    glBindBuffer(openGLTarget(), GL_NONE);
}


void AttributeArray::uploadToCardStride
    (const void* srcPointer, size_t srcElements, size_t srcSize, size_t srcStride, 
    size_t dstPtrOffsetBytes, size_t dstStrideBytes) {
    
    if (srcStride == 0) {
        srcStride = srcSize;
    }

    if (dstStrideBytes == 0) {
        dstStrideBytes = srcSize;
    }

    uint8* dstPointer = (uint8*)mapBuffer(GL_WRITE_ONLY) + (int)dstPtrOffsetBytes;

    // Copy elements
    for (int i = 0; i < (int)srcElements; ++i) {
        System::memcpy(dstPointer, srcPointer, srcSize);
        srcPointer = (uint8*)srcPointer + srcStride;
        dstPointer = (uint8*)dstPointer + dstStrideBytes;
    }
    
    // Unmap buffer
    unmapBuffer();
    dstPointer = NULL;
}


void AttributeArray::uploadToCard(const void* sourcePtr, size_t dstPtrOffset, size_t size) {
    debugAssert(m_stride == 0 || m_stride == m_elementSize);

    void* ptr = (void*)(reinterpret_cast<intptr_t>(m_pointer) + dstPtrOffset);

    // Don't destroy any existing bindings; this call can
    // be made at any time and the program might also
    // use VBO on its own.

    {
        glBindBuffer(openGLTarget(), m_area->m_glbuffer);
        glBufferSubData(openGLTarget(), (GLintptr)ptr, size, sourcePtr);
        glBindBuffer(openGLTarget(), 0);
    }
}



static bool isIntegerType(GLenum byteFormat) {
    switch(byteFormat) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return true;
    default:
        return false;
    }
}


void AttributeArray::vertexAttribPointer(uint32 attribNum) const {
    debugAssert(valid());
    alwaysAssertM(m_stride < 0xFFFFFFFF, "Stride is too large for OpenGL");
    if (m_underlyingRepresentation == GL_DOUBLE) {
        glVertexAttribLPointer(attribNum, GLint(m_elementSize / sizeOfGLFormat(m_underlyingRepresentation)),
                               m_underlyingRepresentation, (GLsizei)m_stride, m_pointer);
    } else if (! m_normalizedFixedPoint && isIntegerType(m_underlyingRepresentation)) { 
        // Integer not acting as normalized fixed point
        const size_t sz = sizeOfGLFormat(m_underlyingRepresentation);
        glVertexAttribIPointer(attribNum, GLint(m_elementSize / sz),
                               m_underlyingRepresentation, (GLsizei)m_stride, m_pointer);
    } else {
        // Float or normalized fixed point 
        glVertexAttribPointer(attribNum, GLint(m_elementSize / sizeOfGLFormat(m_underlyingRepresentation)),
                              m_underlyingRepresentation, m_normalizedFixedPoint, (GLsizei)m_stride, m_pointer);
    }
    glEnableVertexAttribArray(attribNum);
}

}
