/**
  \file G3D/PixelTransferBuffer.cpp
 
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#include "G3D/PixelTransferBuffer.h"
#include "G3D/ImageFormat.h"

namespace G3D {


PixelTransferBuffer::PixelTransferBuffer(const ImageFormat* format, int width, int height, int depth, int rowAlignment) 
    : m_mappedPointer(NULL)
    , m_format(format)
    , m_rowAlignment(rowAlignment)
    , m_rowStride(0)
    , m_width(width)
    , m_height(height)
    , m_depth(depth) {

    debugAssert(m_format);
    debugAssert(isPow2(rowAlignment));
    debugAssert(m_width > 0);
    debugAssert(m_height > 0);
    debugAssert(m_depth > 0);

    m_rowStride = iCeil(width * format->cpuBitsPerPixel / 8.0);
    if (m_rowAlignment > 1) {
        // Round up to the nearest multiple of m_rowAlignment
        size_t remainder = m_rowStride % m_rowAlignment;
        if (remainder != 0) {
            m_rowStride += m_rowAlignment - remainder;
        }
    }
}


PixelTransferBuffer::~PixelTransferBuffer() {
    debugAssertM(isNull(m_mappedPointer), "Missing call to PixelTransferBuffer::unmap()");
}

} // namespace G3D
