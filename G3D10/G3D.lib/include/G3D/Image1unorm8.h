/**
  \file G3D/Image1unorm8.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-01-31
  \edited  2011-08-11
*/

#ifndef G3D_Image1unorm8_h
#define G3D_Image1unorm8_h

#include "G3D/platform.h"
#include "G3D/Map2D.h"
#include "G3D/Color1unorm8.h"
#include "G3D/Color1.h"

namespace G3D {

/**
 Compact storage for luminance/alpha-only/red-only normalized 8-bit images.

 See also G3D::Image3, G3D::GImage
 */
class Image1unorm8 : public Map2D<Color1unorm8, Color1> {
public:

    typedef Image1unorm8      Type;

protected:

    Image1unorm8(int w, int h, WrapMode wrap);

    void copyArray(const Color1* src, int w, int h);
    void copyArray(const Color3* src, int w, int h);
    void copyArray(const Color4* src, int w, int h);
    void copyArray(const Color1unorm8* src, int w, int h);
    void copyArray(const Color3unorm8* src, int w, int h);
    void copyArray(const Color4unorm8* src, int w, int h);

public:

    const class ImageFormat* format() const;

    /** Creates an all-zero width x height image. */
    static shared_ptr<Image1unorm8> createEmpty(int width, int height, WrapMode wrap = WrapMode::ERROR);

    /** Creates a 0 x 0 image. */
    static shared_ptr<Image1unorm8> createEmpty(WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image1unorm8> fromFile(const String& filename, WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image1unorm8> fromArray(const class Color1unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image1unorm8> fromArray(const class Color3unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image1unorm8> fromArray(const class Color4unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image1unorm8> fromArray(const class Color1* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image1unorm8> fromArray(const class Color3* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);
    static shared_ptr<Image1unorm8> fromArray(const class Color4* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image1unorm8> fromImage1(const shared_ptr<class Image1>& im);
    static shared_ptr<Image1unorm8> fromImage3unorm8(const shared_ptr<class Image3unorm8>& im);

    /** Loads from any of the file formats supported by G3D::GImage.  If there is an alpha channel on the input,
        it is stripped. */
    void load(const String& filename);

    /** Saves in any of the formats supported by G3D::GImage. */
    void save(const String& filename);
};

} // G3D

#endif
