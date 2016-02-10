/**
  \file G3D/ImageConvert.cpp

  \created 2012-05-24
  \edited  2012-05-24
*/

#include "G3D/ImageConvert.h"
#include "G3D/CPUPixelTransferBuffer.h"

namespace G3D {


shared_ptr<PixelTransferBuffer> ImageConvert::convertBuffer(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat) {
    // Early return for no conversion
    if (src->format() == dstFormat) {
        return src;
    }

    ConvertFunc converter = findConverter(src, dstFormat);
    if (converter) {
        return (*converter)(src, dstFormat);
    } else {
        return shared_ptr<PixelTransferBuffer>();
    }
}


ImageConvert::ConvertFunc ImageConvert::findConverter(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat) {
    debugAssert(notNull(src));

    // Only handle inter-RGB color space conversions for now
    if (src->format()->colorSpace != ImageFormat::COLOR_SPACE_RGB) {
        return NULL;
    }

    if (dstFormat->colorSpace != ImageFormat::COLOR_SPACE_RGB) {
        return NULL;
    }

    // Check for color order reversal
    if (src->format()->code == ImageFormat::CODE_RGBA8 && dstFormat->code == ImageFormat::CODE_BGRA8) {
        return convertRGBA8toBGRA8;
    }

    // Check for conversion that only adds alpha channel
    if (ImageFormat::getFormatWithAlpha(src->format()) == dstFormat) {
        return &ImageConvert::convertRGBAddAlpha;
    }

    if ((src->format()->numberFormat == ImageFormat::FLOATING_POINT_FORMAT) &&
        (dstFormat->numberFormat == ImageFormat::NORMALIZED_FIXED_POINT_FORMAT) &&
        (src->format()->colorSpace == dstFormat->colorSpace) &&
        (src->format()->cpuBitsPerPixel == 8 * (int)sizeof(float) * src->format()->numComponents) &&
        (dstFormat->cpuBitsPerPixel == 8 * (int)sizeof(unorm8) * dstFormat->numComponents) &&
        (src->format()->sameComponents(dstFormat))) {

        return &ImageConvert::convertFloatToUnorm8;
    }

    if ((src->format()->numberFormat == ImageFormat::NORMALIZED_FIXED_POINT_FORMAT) &&
        (dstFormat->numberFormat == ImageFormat::FLOATING_POINT_FORMAT) &&
        (src->format()->colorSpace == dstFormat->colorSpace) &&
        (src->format()->cpuBitsPerPixel == 8 * (int)sizeof(unorm8) * src->format()->numComponents) &&
        (dstFormat->cpuBitsPerPixel == 8 * (int)sizeof(float) * dstFormat->numComponents) &&
        (src->format()->sameComponents(dstFormat))) {

        return &ImageConvert::convertUnorm8ToFloat;
    }

    return NULL;
}


shared_ptr<PixelTransferBuffer> ImageConvert::convertFloatToUnorm8(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat) {
    shared_ptr<CPUPixelTransferBuffer> dst = CPUPixelTransferBuffer::create(src->width(), src->height(), dstFormat);
    
    const int     N = src->width() * src->height() * dstFormat->numComponents;
    const float*  srcPtr = static_cast<const float*>(src->mapRead());
    unorm8*       dstPtr = static_cast<unorm8*>(dst->buffer());

    for (int i = 0; i < N; ++i) {
        dstPtr[i] = unorm8(srcPtr[i]);
    }
    src->unmap(srcPtr);

    return dst;
}


shared_ptr<PixelTransferBuffer> ImageConvert::convertUnorm8ToFloat(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat) {
    shared_ptr<CPUPixelTransferBuffer> dst = CPUPixelTransferBuffer::create(src->width(), src->height(), dstFormat);
    
    const int     N = src->width() * src->height() * dstFormat->numComponents;
    const unorm8* srcPtr = static_cast<const unorm8*>(src->mapRead());
    float*        dstPtr = static_cast<float*>(dst->buffer());

    for (int i = 0; i < N; ++i) {
        dstPtr[i] = srcPtr[i];
    }
    src->unmap(srcPtr);

    return dst;
}


shared_ptr<PixelTransferBuffer> ImageConvert::convertRGBAddAlpha(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat) {
    debugAssert(src->rowAlignment() == 1);

    shared_ptr<CPUPixelTransferBuffer> dstImage = CPUPixelTransferBuffer::create(src->width(), src->height(), dstFormat);
    const unorm8* oldPixels = static_cast<const unorm8*>(src->mapRead());
    for (int pixelIndex = 0; pixelIndex < src->width() * src->height(); ++pixelIndex) {
        switch (dstFormat->code) {
            case ImageFormat::CODE_LA8:
            {
                unorm8* newPixels = static_cast<unorm8*>(dstImage->buffer());
                
                newPixels[pixelIndex * 2] = oldPixels[pixelIndex];
                newPixels[pixelIndex * 2 + 1] = unorm8::one();

                break;
            }

            case ImageFormat::CODE_RGBA8:
            case ImageFormat::CODE_BGRA8:
            {
                unorm8* newPixels = static_cast<unorm8*>(dstImage->buffer());
                
                newPixels[pixelIndex * 4] = oldPixels[pixelIndex * 3];
                newPixels[pixelIndex * 4 + 1] = oldPixels[pixelIndex * 3 + 1];
                newPixels[pixelIndex * 4 + 2] = oldPixels[pixelIndex * 3 + 2];
                newPixels[pixelIndex * 4 + 3] = unorm8::one();
                break;
            }
            default:
                debugAssertM(false, "Unsupported destination image format");
                break;
        }
    }

    src->unmap(oldPixels);

    return dstImage;
}


shared_ptr<PixelTransferBuffer> ImageConvert::convertRGBA8toBGRA8(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat) {
    int width = src->width();
    int height = src->height();

    shared_ptr<CPUPixelTransferBuffer> dstBuffer = CPUPixelTransferBuffer::create(width, height, dstFormat, AlignedMemoryManager::create());

    const unorm8* srcData = static_cast<const unorm8*>(src->mapRead());
    unorm8* dstData = static_cast<unorm8*>(dstBuffer->buffer());

    const int bytesPerPixel = 4;

    // From RGBA to BGRA, for every 4 bytes, first and third swapped, others remain in place
    for(int i = 0; i < (bytesPerPixel * width * height); i += 4) {
        dstData[i+0] = srcData[i+2];
        dstData[i+1] = srcData[i+1];
        dstData[i+2] = srcData[i+0];
        dstData[i+3] = srcData[i+3];
    }

    src->unmap(srcData);
    return dstBuffer;
}

} // namespace G3D
