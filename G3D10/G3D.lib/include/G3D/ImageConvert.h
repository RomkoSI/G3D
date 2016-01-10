/**
  \file G3D/ImageConvert.h

  \created 2012-05-24
  \edited  2012-05-24
*/

#ifndef G3D_ImageConvert_H
#define G3D_ImageConvert_H

#include "G3D/PixelTransferBuffer.h"

namespace G3D {


/**
    Image conversion utility methods
*/
class ImageConvert {
private:
    ImageConvert();
    
    typedef PixelTransferBuffer::Ref (*ConvertFunc)(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);
    static ConvertFunc findConverter(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);
    
    // Converters
    static PixelTransferBuffer::Ref convertRGBAddAlpha(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);
    static PixelTransferBuffer::Ref convertRGBA8toBGRA8(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);
    static PixelTransferBuffer::Ref convertFloatToUnorm8(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);
    static PixelTransferBuffer::Ref convertUnorm8ToFloat(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);

public:
    /** Converts image buffer to another format if supported, otherwise returns null ref.
        If no conversion is necessary then \a src reference is returned and a new buffer is NOT created. */
    static PixelTransferBuffer::Ref convertBuffer(const PixelTransferBuffer::Ref& src, const ImageFormat* dstFormat);
};

} // namespace G3D

#endif // GLG3D_ImageConvert_H
