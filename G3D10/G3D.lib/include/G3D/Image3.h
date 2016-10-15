/**
  \file G3D/Image3.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-01-31
  \edited  2016-02-15
*/
#pragma once
#include "G3D/platform.h"
#include "G3D/Map2D.h"
#include "G3D/Color3.h"

namespace G3D {

/**
 RGB image with 32-bit floating point storage for each channel. 
 Can have depth (to be CPU versions of layered textures)

 See also G3D::Image3unorm8, G3D::GImage.
 */
class Image3 : public Map2D<Color3, Color3> {
public:
    friend class CubeMap;
    typedef Image3      Type;

protected:

    Image3(int w = 1, int h = 1, WrapMode wrap = WrapMode::ERROR, int d = 1);

    void copyArray(const Color1* src, int w, int h, int d = 1);
    void copyArray(const Color3* src, int w, int h, int d = 1);
    void copyArray(const Color4* src, int w, int h, int d = 1);
    void copyArray(const Color1unorm8* src, int w, int h, int d = 1);
    void copyArray(const Color3unorm8* src, int w, int h, int d = 1);
    void copyArray(const Color4unorm8* src, int w, int h, int d = 1);

public:

    const class ImageFormat* format() const;

    /** Creates an all-zero width x height image. */
    static shared_ptr<Image3> createEmpty(int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);

    /** Creates a 0 x 0 image. */
    static shared_ptr<Image3> createEmpty(WrapMode wrap = WrapMode::ERROR);

    static shared_ptr<Image3> fromFile(const String& filename, WrapMode wrap = WrapMode::ERROR);
    
    static shared_ptr<Image3> fromArray(const class Color1unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);
    static shared_ptr<Image3> fromArray(const class Color3unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);
    static shared_ptr<Image3> fromArray(const class Color4unorm8* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);
    static shared_ptr<Image3> fromArray(const class Color1* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);
    static shared_ptr<Image3> fromArray(const class Color3* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);
    static shared_ptr<Image3> fromArray(const class Color4* ptr, int width, int height, WrapMode wrap = WrapMode::ERROR, int depth = 1);

    static shared_ptr<Image3> fromImage3unorm8(const shared_ptr<class Image3unorm8>& im);

    /** Loads from any of the file formats supported by G3D::GImage.  If there is an alpha channel on the input,
        it is stripped. Converts 8-bit formats to the range (0, 1) */
    void load(const String& filename);

    /** Saves in any of the formats supported by G3D::GImage. 
        Assumes the data is on the range (0, 1) if saving to an 8-bit format.
        
        If depth >1, only saves the depth = 0 layer.
        */
    void save(const String& filename);
};

} // G3D

