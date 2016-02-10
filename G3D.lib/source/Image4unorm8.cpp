/**
  \file Image4unorm8.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-01-31
  \edited  2012-12-25
*/

#include "G3D/Image4unorm8.h"
#include "G3D/Image4.h"
#include "G3D/Image3unorm8.h"
#include "G3D/Image3.h"
#include "G3D/Image.h"
#include "G3D/Color1.h"
#include "G3D/Color1unorm8.h"
#include "G3D/Color4.h"
#include "G3D/Color4unorm8.h"
#include "G3D/ImageFormat.h"
#include "G3D/PixelTransferBuffer.h"
#include "G3D/CPUPixelTransferBuffer.h"

namespace G3D {

void Image4unorm8::speedSerialize(class BinaryOutput& b) const {
    b.writeInt32(w);
    b.writeInt32(h);
    m_wrapMode.serialize(b);
    b.writeInt32(ImageFormat::CODE_RGBA8);
    
    // Write the data
    //GImage temp(GImage::SHARE_DATA, (uint8*)data.getCArray(), w, h, format());
    //temp.encode(GImage::PNG, b);
}

    
Image4unorm8::Ref Image4unorm8::speedCreate(class BinaryInput& b) {
    const int w = b.readInt32();
    const int h = b.readInt32();
    WrapMode wrap;
    wrap.deserialize(b);

    ImageFormat::Code fmt = ImageFormat::Code(b.readInt32());

    alwaysAssertM(fmt == ImageFormat::CODE_RGBA8, 
                  G3D::format("Cannot SpeedCreate an Image4unorm8 from %s",
                         ImageFormat::fromCode(fmt)->name().c_str()));

    Ref im = createEmpty(w, h, wrap);

    // Read the data
    //GImage temp(GImage::SHARE_DATA, (uint8*)im->data.getCArray(), im->w, im->h, im->format());
    //temp.decode(b, GImage::PNG);
    //alwaysAssertM(im->data.getCArray() == temp.pixel4(), "GImage::decode failed to use the memory provided");

    return im;
}


Image4unorm8::Image4unorm8(int w, int h, WrapMode wrap) : Map2D<Color4unorm8, Color4>(w, h, wrap) {
    setAll(Color4unorm8(unorm8::zero(), unorm8::zero(), unorm8::zero(), unorm8::zero()));
}


Image4unorm8::Ref Image4unorm8::fromImage4(const shared_ptr<Image4>& im) {
    Ref out = createEmpty(static_cast<WrapMode>(im->wrapMode()));
    out->copyArray(im->getCArray(), im->width(), im->height());

    return out;
}


Image4unorm8::Ref Image4unorm8::createEmpty(int width, int height, WrapMode wrap) {
    return shared_ptr<Image4unorm8>(new Type(width, height, wrap));
}


Image4unorm8::Ref Image4unorm8::createEmpty(WrapMode wrap) {
    return createEmpty(0, 0, wrap);
}


Image4unorm8::Ref Image4unorm8::fromFile(const String& filename, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->load(filename);
    return out;
}


Image4unorm8::Ref Image4unorm8::fromArray(const class Color3unorm8* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image4unorm8::Ref Image4unorm8::fromArray(const class Color1* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image4unorm8::Ref Image4unorm8::fromArray(const class Color1unorm8* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image4unorm8::Ref Image4unorm8::fromArray(const class Color3* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image4unorm8::Ref Image4unorm8::fromArray(const class Color4unorm8* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


Image4unorm8::Ref Image4unorm8::fromArray(const class Color4* ptr, int w, int h, WrapMode wrap) {
    Ref out = createEmpty(wrap);
    out->copyArray(ptr, w, h);
    return out;
}


void Image4unorm8::load(const String& filename) {
    shared_ptr<Image> image = Image::fromFile(filename);
    if (image->format() != ImageFormat::RGBA8()) {
        image->convertToRGBA8();
    }

    switch (image->format()->code)
    {
        case ImageFormat::CODE_L8:
            copyArray(static_cast<const Color1unorm8*>(image->toPixelTransferBuffer()->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_L32F:
            copyArray(static_cast<const Color1*>(image->toPixelTransferBuffer()->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGB8:
            copyArray(static_cast<const Color3unorm8*>(image->toPixelTransferBuffer()->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGB32F:
            copyArray(static_cast<const Color3*>(image->toPixelTransferBuffer()->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGBA8:
            copyArray(static_cast<const Color4unorm8*>(image->toPixelTransferBuffer()->buffer()), image->width(), image->height());
            break;
        case ImageFormat::CODE_RGBA32F:
            copyArray(static_cast<const Color4*>(image->toPixelTransferBuffer()->buffer()), image->width(), image->height());
            break;
        default:
            debugAssertM(false, "Trying to load unsupported image format");
            break;
    }

    setChanged(true);
}


void Image4unorm8::copyArray(const Color1unorm8* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color4unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i].r = dst[i].g = dst[i].b = src[i].value;
        dst[i].a = unorm8::one();
    }
}

void Image4unorm8::copyArray(const Color1* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color4unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i].r = dst[i].g = dst[i].b = Color1unorm8(src[i]).value;
        dst[i].a = unorm8::one();
    }
}


void Image4unorm8::copyArray(const Color4unorm8* ptr, int w, int h) {
    resize(w, h);
    System::memcpy(getCArray(), ptr, w * h * 4);
}


void Image4unorm8::copyArray(const Color4* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color4unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color4unorm8(src[i]);
    }
}


void Image4unorm8::copyArray(const Color3unorm8* ptr, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color4unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color4unorm8(ptr[i], unorm8::one());
    }
}


void Image4unorm8::copyArray(const Color3* src, int w, int h) {
    resize(w, h);
    int N = w * h;

    Color4unorm8* dst = getCArray();
    for (int i = 0; i < N; ++i) {
        dst[i] = Color4unorm8(Color4(src[i], 1.0f));
    }
}


/** Saves in any of the formats supported by G3D::GImage. */
void Image4unorm8::save(const String& filename) {
    shared_ptr<CPUPixelTransferBuffer> buffer = CPUPixelTransferBuffer::create(width(), height(), format(), MemoryManager::create());
    System::memcpy(buffer->buffer(), getCArray(),  (size_t)width() * height() * format()->cpuBitsPerPixel / 8);
    shared_ptr<Image> image = Image::fromPixelTransferBuffer(buffer);
    image->save(filename);
}


shared_ptr<class Image1unorm8> Image4unorm8::getChannel(int c) const {
    debugAssert(c >= 0 && c <= 3);

    shared_ptr<Image1unorm8> dst = Image1unorm8::createEmpty(width(), height(), wrapMode());
    const Color4unorm8* srcArray = getCArray();
    Color1unorm8* dstArray = dst->getCArray();

    const int N = width() * height();
    for (int i = 0; i < N; ++i) {
        dstArray[i] = Color1unorm8(srcArray[i][c]);
    }

    return dst;
}


const ImageFormat* Image4unorm8::format() const {
    return ImageFormat::RGBA8();
}

} // G3D
