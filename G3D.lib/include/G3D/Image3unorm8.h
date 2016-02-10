/**
  \file G3D/Image3unorm8.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-01-31
  \edited  2016-02-10
*/

#ifndef G3D_Image3unorm8_h
#define G3D_Image3unorm8_h

#include "G3D/platform.h"
#include "G3D/Map2D.h"
#include "G3D/Color3unorm8.h"
#include "G3D/Color3.h"

namespace G3D {

typedef shared_ptr<class Image3unorm8> Image3unorm8Ref;

/**
 Compact storage for RGB 8-bit per channel images.

 See also G3D::Image3, G3D::GImage
 */
class Image3unorm8 : public Map2D<Color3unorm8, Color3> {
public:

    typedef Image3unorm8      Type;

protected:

    Image3unorm8(int w, int h, WrapMode wrap);

    void copyArray(const Color1* src, int w, int h);
    void copyArray(const Color3* src, int w, int h);
    void copyArray(const Color4* src, int w, int h);
    void copyArray(const Color1unorm8* src, int w, int h);
    void copyArray(const Color3unorm8* src, int w, int h);
    void copyArray(const Color4unorm8* src, int w, int h);

public:

    /** \sa SpeedLoad */
    void speedSerialize(class BinaryOutput& b) const;

    /** \sa SpeedLoad */
    static shared_ptr<class Image3unorm8> speedCreate(class BinaryInput& b);

    const class ImageFormat* format() const;

    /** Creates an all-zero width x height image. */
    static shared_ptr<Image3unorm8> createEmpty(int width, int height, WrapMode wrap = WrapMode::ERROR);


    /** Creates a 0 x 0 image. */
    static shared_ptr<Image3unorm8> createEmpty(WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image3unorm8> fromFile(const String& filename, WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image3unorm8> fromGImage(const class GImage& im, WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image3unorm8> fromArray(const class Color1unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image3unorm8> fromArray(const class Color3unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image3unorm8> fromArray(const class Color4unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image3unorm8> fromArray(const class Color1* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image3unorm8> fromArray(const class Color3* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image3unorm8> fromArray(const class Color4* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image3unorm8> fromImage3(const shared_ptr<class Image3>& im);
    static shared_ptr<Image3unorm8> fromImage4(const shared_ptr<class Image4>& im);
    static shared_ptr<Image3unorm8> fromImage1unorm8(const shared_ptr<class Image1unorm8>& im);

    /** Loads from any of the file formats supported by G3D::GImage.  If there is an alpha channel on the input,
        it is stripped. */
    void load(const String& filename);

    /** Saves in any of the formats supported by G3D::GImage. */
    void save(const String& filename);

    /** Extracts color channel 0 <= c <= 2 and returns it as a new monochrome image. */
    shared_ptr<class Image1unorm8> getChannel(int c) const;
};

} // G3D

#endif
