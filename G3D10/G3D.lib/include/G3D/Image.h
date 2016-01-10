/**
  \file G3D/Image.h 
  \author Corey Taylor
  \maintainer Morgan McGuire

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */
#ifndef G3D_Image_h
#define G3D_Image_h

#include "G3D/platform.h"
#include "G3D/Color4.h"
#include "G3D/Color4unorm8.h"
#include "G3D/Color1unorm8.h"
#include "G3D/GMutex.h"
#include "G3D/Rect2D.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Vector2int32.h"
#include "G3D/WrapMode.h"
#include "G3D/ImageFormat.h"

// non-G3D forward declarations
class fipImage;

namespace G3D {

// Forward declarations
class BinaryInput;
class ImageFormat;
class CPUPixelTransferBuffer;
class PixelTransferBuffer;

/**
    \brief Provides general image loading, saving, conversion and
    pixel access.

    Image allows you to load a variety of supported file formats in
    their native pixel format with very few exceptions.  Callers are
    responsible for converting pixel data to the desired format after
    loading and before saving.

    Image will also try to save directly to a file in the same pixel
    format as it is stored internally.  If a file format does not
    support that pixel format, then the caller is responsible for
    converting before saving.

    The Image::Error exception is thrown if a file cannot be loaded.
*/
class Image : public ReferenceCountedObject {
public:

    /* Copied from FreeImage.h and must be kept in sync with FREE_IMAGE_FORMAT */
    enum ImageFileFormat {
        UNKNOWN = -1,
        BMP     = 0,
        ICO     = 1,
        JPEG	= 2,
        JNG     = 3,
        KOALA	= 4,
        LBM     = 5,
        IFF     = LBM,
        MNG     = 6,
        PBM     = 7,
        PBMRAW	= 8,
        PCD     = 9,
        PCX     = 10,
        PGM     = 11,
        PGMRAW	= 12,
        PNG     = 13,
        PPM     = 14,
        PPMRAW	= 15,
        RAS     = 16,
        TARGA	= 17,
        TIFF	= 18,
        WBMP	= 19,
        PSD     = 20,
        CUT     = 21,
        XBM     = 22,
        XPM     = 23,
        DDS     = 24,
        GIF     = 25,
        HDR     = 26,
        FAXG3	= 27,
        SGI     = 28,
        EXR     = 29,
        J2K     = 30,
        JP2     = 31,
        PFM     = 32,
        PICT	= 33,
        RAW     = 34
    };
    

    class Error {
    public:
        Error
        (const String& reason,
         const String& filename = "") :
        reason(reason), filename(filename) {}
        
        String reason;
        String filename;
    };

private:

    fipImage*           m_image;
    const ImageFormat*  m_format;

    /** Ensures that FreeImage is not initialized on multiple threads simultaneously. */
    static GMutex       s_freeImageMutex;

    Image();

    // Intentionally not implemented to prevent copy construction
    Image(const Image&);
    Image& operator=(const Image&);

    /** Initialize the FreeImage library on first use */
    static void initFreeImage();

protected:
    // Weighting polynomial http://paulbourke.net/texture_colour/imageprocess/
    float R(float x) const {
        static const float coeff[4] = { 1.0f, -4.0f, 6.0f, -4.0f };
        float result = 0.0f;
        for (int j = 0; j < 4; ++j) {
            result += coeff[j] * pow(max(0.0f, x + 2 - j), 3.0f);
        }
        return result / 6.0f;
    }

public:

    virtual ~Image();

    /** Determines if the format of a file is supported.
        Does not open file unless \a allowCheckSignature is true.

        Does not check if the pixel format is supported.
    */
    static bool fileSupported(const String& filename, bool allowCheckSignature = false);

    static shared_ptr<Image> create(int width, int height, const ImageFormat* imageFormat);

    /** Loads an image from file specified by \a filename.  
        
        \param imageFormat If specified and bit-compatible (ImageFormat::canInterpretAs()) with the
        image's original file format, reinterprets the data in the new
        format (i.e., as if with a C++ reinterpret_cast).  For
        example, RGB8 data could be reinterpreted as SRGB8, RGB8I,
        RGB8UI, BGR8, etc.  

        \sa fromBinaryInput, convert
    */
    static shared_ptr<Image> fromFile(const String& filename, const ImageFormat* imageFormat = ImageFormat::AUTO());

    /** Loads an image from existing BinaryInput \a bi. 

        \sa fromFile, convert
    */
    static shared_ptr<Image> fromBinaryInput(BinaryInput& bi, const ImageFormat* imageFormat = ImageFormat::AUTO());

    /** Loads an image from existing PixelTransferBuffer \a buffer.  Performs a copy of pixel data. */
    static shared_ptr<Image> fromPixelTransferBuffer(const shared_ptr<PixelTransferBuffer>& buffer);

    /**
        Takes in an Array of Images and loades them all into a single CPUPixelTransferBuffer. This is meant 
        to be used for creating 2D_Texture Arrays. 
    */
    static shared_ptr<CPUPixelTransferBuffer> arrayToPixelTransferBuffer(const Array<shared_ptr<Image> >& images);

    /** 
        The contents are undefined after resize.

        Does not resize if the width, height, and format are unchanged.

        \param fmt If ImageFormat::AUTO(), then the current format is retained
     */
    void setSize(int w, int h, const ImageFormat* fmt = ImageFormat::AUTO());

    /** Copies all elements from \a buffer, adjusting the size and
        format of this to match. */
    void set(const shared_ptr<PixelTransferBuffer>& buffer);

    /** Copies only pixels from \a buffer to the specified location
        in the image.  Does not adjust the size nor format of this to match */
    void set(const shared_ptr<PixelTransferBuffer>& buffer, int x, int y);
    
    /** Convert the internal representation to the specified format
        using G3D::ImageConvert. */
    void convert(const ImageFormat* dstFmt);

    /** Saves internal pixel data to file specified by \a filename.
        Does not convert pixel format before saving. 
		Throws string exception if failure.*/
    void save(const String& filename) const;

    /** Saves GIF of a sequence of images to file specified by \a filename/
        The save is not lossy if the images are RGB8 format and have a palette size of at most 256 colros.
		All images must have same width and height as specified by first image in sequence.
		Fails silently if empty sequence or 0 framerate.
		Throws string exception if failure. */
    static void saveGIF(const String& filename, const Array<shared_ptr<Image>>& sequence, const double& fps);

    /** Saves internal pixel data to existing BinaryOutput \a bo.  Does not convert pixel format before saving. */
    void serialize(BinaryOutput& bo, ImageFileFormat fileFormat) const;

    /** Extracts a copy of the pixel data. */
    shared_ptr<CPUPixelTransferBuffer> toPixelTransferBuffer() const;

    /** Extracts a copy of the pixel data within the specified rectangle. */
    shared_ptr<CPUPixelTransferBuffer> toPixelTransferBuffer(Rect2D rect) const;

    /** Copies the underlying pixel data */
    shared_ptr<Image> clone() const;

    const ImageFormat* format() const;

    /** Width in pixels */
    int width() const;

    /** Height in pixels */
    int height() const;

    Rect2D bounds() const {
        return Rect2D::xywh(0, 0, static_cast<float>(width()), static_cast<float>(height()));
    }

    /// Direct replacements for old GImage functions for now
    /** \deprecated Do not call*/
    bool convertToL8();    
	bool convertToR8();

    /** \deprecated Do not call*/
    bool convertToRGB8();

    /** \deprecated Do not call*/
    bool convertToRGBA8();

    void flipVertical();
    void flipHorizontal();
    void rotateCW(double radians);
    void rotateCCW(double radians);

    /** 8-bit and 16-bit values are returned on the range [0, 1] losslessly. 32-bit float values are mapped with some precision loss. float32 values are unmodified. */
    void get(const Point2int32& pos, Color4& color) const;
    void get(const Point2int32& pos, Color3& color) const;
    void get(const Point2int32& pos, Color1& color) const;
    void get(const Point2int32& pos, Color4unorm8& color) const;
    void get(const Point2int32& pos, Color3unorm8& color) const;
    void get(const Point2int32& pos, Color1unorm8& color) const;

    void set(const Point2int32& pos, const Color4& color);
    void set(const Point2int32& pos, const Color3& color);
    void set(const Point2int32& pos, const Color1& color);
    void set(const Point2int32& pos, const Color4unorm8& color);
    void set(const Point2int32& pos, const Color3unorm8& color);
    void set(const Point2int32& pos, const Color1unorm8& color);
    
    void setAll(const Color4& color);
    void setAll(const Color3& color);
    void setAll(const Color1& color);
    void setAll(const Color4unorm8& color);
    void setAll(const Color3unorm8& color);
    void setAll(const Color1unorm8& color);

    /** Slower than the other set methods */
    template <class ValueType>
    void set(Point2int32 pos, const ValueType& v, WrapMode wrap) {
        // The uint32 tests make negative numbers look huge so that they fail the test
        if ((uint32(pos.x) >= uint32(width())) || (uint32(pos.y) >= uint32(height()))) {
            // Exceptional cases that modify pos
            switch (wrap) {
            case WrapMode::CLAMP:
                pos = pos.clamp(Vector2int32(0, 0), Vector2int32(width() - 1, height() - 1));
                break;

            case WrapMode::ZERO:
            case WrapMode::IGNORE:
                // Do nothing
                return;

            case WrapMode::TILE:
                pos.x = iWrap(pos.x, width());
                pos.y = iWrap(pos.y, height());
                break;

            case WrapMode::ERROR:
                alwaysAssertM(((uint32(pos.x) < uint32(width())) && (uint32(pos.y) < uint32(height()))), 
                    G3D::format("Index out of bounds: pos = (%d, %d), image dimensions = %d x %d",
                    pos.x, pos.y, width(), height()));
            }
        }

        set(pos, v);
    }

    template<class ValueType>
    void set(int x, int y, const ValueType& v, WrapMode wrap = WrapMode::ERROR) {
        set(Point2int32(x, y), v, wrap);
    }

    /** Provides functional access with wrap modes.  This is slower than the other get() methods. */
    template <class ValueType>
    ValueType get(Point2int32 pos, WrapMode wrap = WrapMode::ERROR) const {
        ValueType v;

        // The uint32 tests make negative numbers look huge so that they fail the test
        if ((uint32(pos.x) >= uint32(width())) || (uint32(pos.y) >= uint32(height()))) {
            // Exceptional cases that modify pos
            switch (wrap) {
            case WrapMode::CLAMP:
            case WrapMode::IGNORE:
                pos = pos.clamp(Vector2int32(0, 0), Vector2int32(width() - 1, height() - 1));
                break;

            case WrapMode::TILE:
                pos.x = iWrap(pos.x, width());
                pos.y = iWrap(pos.y, height());
                break;

            case WrapMode::ZERO:
                // No fetch.  All color types initialize to zero, so just return
                return v;

            case WrapMode::ERROR:
                alwaysAssertM(((uint32(pos.x) < uint32(width())) && (uint32(pos.y) < uint32(height()))), 
                    G3D::format("Index out of bounds: pos = (%d, %d), image dimensions = %d x %d",
                    pos.x, pos.y, width(), height()));
            }
        }

        get(pos, v);
        return v;
    }

    /** 
      Helper function for convenient functional access to elements and wrap modes.
      This is slower than the other get() methods.

      Example:
      \code
      const Color3& c(im->get<Color3>(x, y));
      const Color3& d((im->get<Color3>(2 * x, y) + im->get<Color3>(2 * x + 1, y)) / 2);
      \endcode
     */
    template <class ValueType>
    ValueType get(int x, int y, WrapMode w = WrapMode::ERROR) const {
        return get<ValueType>(Point2int32(x, y), w);
    }

    Color4 bicubic(float x, float y, WrapMode w = WrapMode::ERROR) const;

    Color4 bicubic(const Point2& pos, WrapMode w = WrapMode::ERROR) const;

    /** 
        \brief Returns the nearest neighbor's pixel value, as in
        OpenGL's GL_NEAREST texture filter mode.
        
        Values are considered to be at pixel centers, so
        <code>image->nearest(x, y) == image->get<Color4>(iFloor(x),
        iFloor(y))</code>.

        Note that this method uses pixel addressing, not texel addressing, so the lower-right
        edge of the image is at (width(), height()), not (1, 1)

        \sa bilinear, get
    */
    Color4 nearest(const Point2& pos, WrapMode w = WrapMode::ERROR) const;

    /** 
        \brief Returns the nearest neighbor's pixel value, as in
        OpenGL's GL_NEAREST texture filter mode.
        
        Values are considered to be at pixel centers, so
        <code>image->nearest(x, y) == image->get<Color4>(iFloor(x),
        iFloor(y))</code>.

        Note that this method uses pixel addressing, not texel
        addressing, so the lower-right edge of the image is at
        (width(), height()), not (1, 1)

        \sa bilinear, get
    */
    Color4 nearest(int x, int y, WrapMode w = WrapMode::ERROR) const;

    /** 
      \brief Bilinear interpolation, in the style of OpenGL's
      GL_LINEAR.

      Needs to access elements from (floor(x), floor(y)) to (floor(x)
      + 1, floor(y) + 1) and will use the wrap mode appropriately
      (possibly generating out of bounds errors).

      Guaranteed to match nearest(x, y) at integer locations.  

      \sa nearest, get
     */ 
    Color4 bilinear(float x, float y, WrapMode wrap = WrapMode::ERROR) const;

    /** 
      \copydoc bilinear
     */ 
    Color4 bilinear(const Point2& pos, WrapMode wrap = WrapMode::ERROR) const;

};

} // namespace G3D

#endif // G3D_Image_h
