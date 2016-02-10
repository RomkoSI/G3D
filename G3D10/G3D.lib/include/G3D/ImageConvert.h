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
    
    typedef shared_ptr<PixelTransferBuffer> (*ConvertFunc)(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);
    static ConvertFunc findConverter(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);
    
    // Converters
    static shared_ptr<PixelTransferBuffer> convertRGBAddAlpha(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);
    static shared_ptr<PixelTransferBuffer> convertRGBA8toBGRA8(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);
    static shared_ptr<PixelTransferBuffer> convertFloatToUnorm8(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);
    static shared_ptr<PixelTransferBuffer> convertUnorm8ToFloat(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);

public:
    /** Converts image buffer to another format if supported, otherwise returns null ref.
        If no conversion is necessary then \a src reference is returned and a new buffer is NOT created. */
    static shared_ptr<PixelTransferBuffer> convertBuffer(const shared_ptr<PixelTransferBuffer>& src, const ImageFormat* dstFormat);
};

} // namespace G3D

#endif // GLG3D_ImageConvert_H
