/**
 \file    GLG3D/Component.h
 \author  Morgan McGuire, http://graphics.cs.williams.edu
 \created 2009-02-19
 \edited  2012-09-04
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_Component_h
#define G3D_Component_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/ImageFormat.h"
#include "G3D/Image1.h"
#include "G3D/Image3.h"
#include "G3D/Image4.h"
#include "G3D/SpeedLoad.h"
#include "GLG3D/Texture.h"

namespace G3D {

/** Used by Component */
enum ImageStorage {
    /** Ensure that all image data is stored exclusively on the CPU. */
    MOVE_TO_CPU, 

    /** Ensure that all image data is stored exclusively on the GPU. */
    MOVE_TO_GPU, 

    /** Ensure that all image data is stored at least on the CPU. */
    COPY_TO_CPU, 

    /** Ensure that all image data is stored at least on the GPU. */
    COPY_TO_GPU,

    /** Do not change image storage */
    IMAGE_STORAGE_CURRENT
};


class ImageUtils {
public:
    /** Returns the equivalent 8-bit version of a float format */
    static const ImageFormat* to8(const ImageFormat* f);
};

#ifdef __clang__
#   define DO_NOT_WARN_IF_UNUSED __attribute__ ((unused))
#else
#	define DO_NOT_WARN_IF_UNUSED
#endif

static DO_NOT_WARN_IF_UNUSED Color4 handleTextureEncoding(const Color4& c, const shared_ptr<Texture>& t) {
    if (notNull(t)) {
        const Texture::Encoding& encoding = t->encoding();
        return c * encoding.readMultiplyFirst + encoding.readAddSecond;
    } else {
        return c;
    }
}


static DO_NOT_WARN_IF_UNUSED Color3 handleTextureEncoding(const Color3& c, const shared_ptr<Texture>& t) {
    if (notNull(t)) {
        const Texture::Encoding& encoding = t->encoding();
        return c * encoding.readMultiplyFirst.rgb() + encoding.readAddSecond.rgb();
    } else {
        return c;
    }
}


/** Manages CPU and GPU versions of image data and performs
    conversions as needed.

    \param Image the CPU floating-point image format to use.  On the GPU, the corresponding uint8 format is used.

    Primarily used by Component. */
template<class Image>
class MapComponent : public ReferenceCountedObject {
public:

    typedef shared_ptr<MapComponent<Image> > Ref;

private:
            
#   define MyType class MapComponent<Image>

    shared_ptr<Image>              m_cpuImage;
    shared_ptr<Texture>            m_gpuImage;
    typename Image::StorageType    m_min;
    typename Image::StorageType    m_max;
    typename Image::ComputeType    m_mean;

    static void getTexture(const shared_ptr<Image>& im, shared_ptr<Texture>& tex) {
            
        Texture::Dimension dim;
        if (isPow2(im->width()) && isPow2(im->height())) {
            dim = Texture::DIM_2D;
        } else {
            dim = Texture::DIM_2D;
        }

        Texture::Encoding e;
        e.format = ImageUtils::to8(im->format());
        tex = Texture::fromMemory
            ("Converted", im->getCArray(), im->format(),
             im->width(), im->height(), 1, 1, e, dim);
    }

    static void convert(const Color4& c, Color3& v) {
        v = c.rgb();
    }

    static void convert(const Color4& c, Color4& v) {
        v = c;
    }

    static void convert(const Color4& c, Color1& v) {
        v.value = c.r;
    }

    inline MapComponent(const class shared_ptr<Image>& im, const shared_ptr<Texture>& tex) : 
        m_cpuImage(im), m_gpuImage(tex), m_min(Image::StorageType::one()), m_max(Image::StorageType::zero()),
        m_mean(Image::ComputeType::zero()) {

        // Compute min, max, mean
        if (m_gpuImage && m_gpuImage->min().isFinite()) {
            // Use previously computed data stored in the texture
            convert(m_gpuImage->min(), m_min);
            convert(m_gpuImage->max(), m_max);
            convert(m_gpuImage->mean(), m_mean);
        } else {
            bool cpuWasNull = isNull(m_cpuImage);

            if (isNull(m_cpuImage) && m_gpuImage) {
                m_gpuImage->getImage(m_cpuImage);
            }

            if (m_cpuImage) {
                const typename Image::StorageType* ptr = m_cpuImage->getCArray();
                typename Image::ComputeType sum = Image::ComputeType::zero();
                const int N = m_cpuImage->width() * m_cpuImage->height();
                for (int i = 0; i < N; ++i) {
                    m_min  = m_min.min(ptr[i]);
                    m_max  = m_max.min(ptr[i]);
                    sum   += typename Image::ComputeType(ptr[i]);
                }
                m_mean = sum / (float)N;
            }

            if (cpuWasNull) {
                // Throw away the CPU image to conserve memory
   	            m_cpuImage.reset();
            }
        }
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. */
    static void speedSerialize(const Image3::Ref& im, const Color3& minValue, BinaryOutput& b) {
        (void)minValue;
        // uint8x3
        b.writeUInt8('u');
        b.writeUInt8('c');
        b.writeUInt8(3);
        Image3unorm8::fromImage3(im)->speedSerialize(b);
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. */
    static void speedSerialize(const shared_ptr<Image4>& im, const Color4& minValue, BinaryOutput& b) {
        b.writeUInt8('u');
        b.writeUInt8('c');
        if (minValue.a < 1.0f) {
            b.writeUInt8(4);
            Image4unorm8::fromImage4(im)->speedSerialize(b);
        } else {
            // The alpha channel is unused, so compress this to RGB8.
            b.writeUInt8(3);
            Image3unorm8::Ref im3 = Image3unorm8::fromImage4(im);
            im3->speedSerialize(b);
        }
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. 

        Note that the im is never actually used, since we don't want
        to waste time converting to float!
    */
    static void speedDeserialize(Image3::Ref& ignore, shared_ptr<Texture>& tex, const Color3& minValue, BinaryInput& b) {
        (void)minValue;

        uint8 s = b.readUInt8();
        alwaysAssertM(s == 'u', "Wrong sign value when reading Image3unorm8");
        uint8 type = b.readUInt8();
        alwaysAssertM(type == 'c', "Wrong type when reading Image3unorm8");
        uint8 channels = b.readUInt8();
        alwaysAssertM(channels == 3, "Wrong number of channels when reading Image3unorm8");

        Image3unorm8::Ref im = Image3unorm8::speedCreate(b);

        Texture::Dimension dim;
        if (isPow2(im->width()) && isPow2(im->height())) {
            dim = Texture::DIM_2D;
        } else {
            dim = Texture::DIM_2D;
        }

        tex = Texture::fromMemory
            ("SpeedLoaded", im->getCArray(), im->format(),
             im->width(), im->height(), 1, 1, im->format(),
             dim);
    }

    /** Overloads to allow conversion of Image3 and Image4 to uint8,
        since Component knows that there is only 8-bit data in the
        floats. 
    */
    static void speedDeserialize(Image4::Ref ignore, shared_ptr<Texture>& tex, const Color4& minValue, BinaryInput& b) {
        uint8 s = b.readUInt8();
        alwaysAssertM(s == 'u', "Wrong sign value in SpeedLoad");
        uint8 type = b.readUInt8();
        alwaysAssertM(type == 'c', "Wrong type in SpeedLoad");
        uint8 channels = b.readUInt8();
        uint8 expectedChannels = (minValue.a < 1.0f) ? 4 : 3;
        alwaysAssertM(channels == expectedChannels, "Wrong number of channels when reading Image3unorm8");

        if (channels == 4) {
            Image4unorm8::Ref im = Image4unorm8::speedCreate(b);

            Texture::Dimension dim;
            if (isPow2(im->width()) && isPow2(im->height())) {
                dim = Texture::DIM_2D;
            } else {
                dim = Texture::DIM_2D;
            }

            tex = Texture::fromMemory
                ("SpeedLoaded", im->getCArray(), im->format(),
                 im->width(), im->height(), 1, 1, im->format(),
                 dim);
        } else {
            alwaysAssertM(channels == 3, "Wrong number of channels");

            Image3unorm8::Ref im = Image3unorm8::speedCreate(b);

            Texture::Dimension dim;
            if (isPow2(im->width()) && isPow2(im->height())) {
                dim = Texture::DIM_2D;
            } else {
                dim = Texture::DIM_2D;
            }
            

            tex = Texture::fromMemory
                ("SpeedLoaded", im->getCArray(), im->format(),
                 im->width(), im->height(), 1, 1, im->format(),
                 dim);
        }
    }

    
    /** For speedCreate */
    MapComponent() {}

public:

    /** \sa SpeedLoad */
    static Ref speedCreate(BinaryInput& b) {
        Ref m(new MapComponent<Image>());
        
        m->m_min.deserialize(b);
        m->m_max.deserialize(b);
        m->m_mean.deserialize(b);

        speedDeserialize(m->m_cpuImage, m->m_gpuImage, m->m_min, b);

        return m;
    }


    /** \sa SpeedLoad */
    void speedSerialize(BinaryOutput& b) const {
        m_min.serialize(b);
        m_max.serialize(b);
        m_mean.serialize(b);

        // Save 8-bit data.  If the CPU image was NULL, we end up
        // reading it from the GPU to the CPU, converting to float,
        // and then converting back to uint8.  But we don't serialize
        // very often--this is a preprocess--so the perf hit is
        // irrelevant.

        // Don't bother saving the alpha channel if m_min == 1
        speedSerialize(image(), m_min, b);
    }

    /** Returns NULL if both are NULL */
    static shared_ptr<MapComponent<Image> > create
    (const class shared_ptr<Image>& im, 
     const shared_ptr<Texture>&                         tex) {

        if (isNull(im) && isNull(tex)) {
	        return shared_ptr<MapComponent<Image> >();
	    } else {
	        return shared_ptr<MapComponent<Image> >(new MapComponent<Image>(im, tex));
	    }
    }


    /** Largest value in each channel of the image */
    const typename Image::StorageType& max() const {
        return m_max;
    }

    /** Smallest value in each channel of the image */
    const typename Image::StorageType& min() const {
        return m_min;
    }

    /** Average value in each channel of the image */
    const typename Image::ComputeType& mean() const {
        return m_mean;
    }
           
    /** Returns the CPU image portion of this component, synthesizing
        it if necessary.  Returns NULL if there is no GPU data to 
        synthesize from.*/
    const class shared_ptr<Image>& image() const {
        MyType* me = const_cast<MyType*>(this);
        if (isNull(me->m_cpuImage)) {
            debugAssert(me->m_gpuImage);
            // Download from GPU.  This works because C++
            // dispatches the override on the static type,
            // so it doesn't matter that the pointer is NULL
            // before the call.
            m_gpuImage->getImage(me->m_cpuImage);
        }
                
        return m_cpuImage;
    }
    

    /** Returns the GPU image portion of this component, synthesizing
        it if necessary.  */
    const shared_ptr<Texture>& texture() const {
        MyType* me = const_cast<MyType*>(this);
        if (isNull(me->m_gpuImage)) {
            debugAssert(me->m_cpuImage);
                    
            // Upload from CPU
            getTexture(m_cpuImage, me->m_gpuImage);
        }
                
        return m_gpuImage;
    }

    void setStorage(ImageStorage s) const {
        MyType* me = const_cast<MyType*>(this);
        switch (s) {
        case MOVE_TO_CPU:
            image();
            me->m_gpuImage.reset();
            break;

        case MOVE_TO_GPU:
            texture();
            me->m_cpuImage.reset();
            break;

        case COPY_TO_GPU:
            texture();
            break;

        case COPY_TO_CPU:
            image();
            break;

        case IMAGE_STORAGE_CURRENT:
            // Nothing to do
            break;
        }
    }

#   undef MyType
};


/** \brief Common code for G3D::Component1, G3D::Component3, and G3D::Component4.

    Product of a constant and an image. 

    The image may be stored on either the GPU (G3D::Texture) or
    CPU (G3D::Map2D subclass), and both factors are optional. The
    details of this class are rarely needed to use UniversalMaterial, since
    it provides constructors from all combinations of data
    types.
    
    Supports only floating point image formats because bilinear 
    sampling of them is about 9x faster than sampling int formats.
    */
template<class Color, class Image>
class Component {
public:

private:

    Color                     m_max;
    Color                     m_min;
    Color                     m_mean;

    /** NULL if there is no map. This is a reference so that
        multiple Components may share a texture and jointly
        move it to and from the GPU.*/
    shared_ptr<MapComponent<Image> >  m_map;

    void init() {
		if (notNull(m_map)) {
			m_max  = Color(m_map->max());
			m_min  = Color(m_map->min());
			m_mean = Color(m_map->mean()); 
		} else {
			m_max  = Color::nan();
			m_min  = Color::nan();
			m_mean = Color::nan();
		}
    }

    static float alpha(const Color1& c) {
        return 1.0;
    }
    static float alpha(const Color3& c) {
        return 1.0;
    }
    static float alpha(const Color4& c) {
        return c.a;
    }


public:
        
    /** Assumes a map of NULL if not specified */
    Component
    (const shared_ptr<MapComponent<Image> >& map = shared_ptr<MapComponent<Image> >()) :
        m_map(map) {
        init();
    }

    Component
    (const shared_ptr<Image>& map) :
        m_map(MapComponent<Image>::create(map, shared_ptr<Texture>())) {
        init();
    }

    Component
    (const shared_ptr<Texture>&             map) :
        m_map(MapComponent<Image>::create(shared_ptr<Image>(), map)) {
        init();
    }
    
    /** \sa SpeedLoad */
    void speedSerialize(BinaryOutput& b) const {
        SpeedLoad::writeHeader(b, "Component");

        // Save the size of the color field to help ensure that
        // this was properly deserialized
        b.writeInt32(sizeof(Color));

        m_min.serialize(b);
        m_max.serialize(b);
        m_mean.serialize(b);
        b.writeBool8(notNull(m_map));
        
        if (m_map) {
            m_map->speedSerialize(b);
        }
    }


    /** \sa SpeedLoad */
    void speedDeserialize(BinaryInput& b) {
        SpeedLoad::readHeader(b, "Component");
        
        const size_t colorSize = (size_t)b.readInt32();
        alwaysAssertM(colorSize == sizeof(Color), 
                      "Tried to SpeedLoad a component in the wrong format.");

        m_min.deserialize(b);
        m_max.deserialize(b);
        m_mean.deserialize(b);
        bool hasMap = b.readBool8();
        if (hasMap) {
            m_map = MapComponent<Image>::speedCreate(b);
        }
    }

    bool operator==(const Component<Color, Image>& other) const {
        return (m_map == other.m_map);
    }
        
    /** Return map sampled at \param pos.  Optimized to only perform as many
        operations as needed.

        If the component contains a texture map that has not been
        converted to a CPU image, that conversion is
        performed. Because that process is not threadsafe, when using
        sample() in a multithreaded environment, first invoke setStorage(COPY_TO_CPU)
        on every Component from a single thread to prime the CPU data
        structures.

        Coordinates are normalized; will be scaled by the image width and height
        automatically.
    */
    Color sample(const Vector2& pos) const {
		debugAssertM(notNull(m_map), "Tried to sample a component without a map");
        const shared_ptr<Image>& im = m_map->image();
        return handleTextureEncoding(im->bilinear(pos * Vector2(float(im->width()), float(im->height()))), m_map->texture());
    }

    /** Largest value per color channel */
    inline const Color& max() const {
        return m_max;
    }

    /** Smallest value per color channel */
    inline const Color& min() const {
        return m_min;
    }

    /** Average value per color channel */
    inline const Color& mean() const {
        return m_mean;
    }

    /** Causes the image to be created by downloading from GPU if necessary. */
    inline const shared_ptr<Image>& image() const {
		static const shared_ptr<Image> nullTexture;
        return notNull(m_map) ? m_map->image() : nullTexture;
    }

    /** Causes the texture to be created by uploading from CPU if necessary. */
    inline const shared_ptr<Texture>& texture() const {
		static const shared_ptr<Texture> nullTexture;
		return notNull(m_map) ? m_map->texture() : nullTexture;
    }

    /** Does not change storage if the map is NULL */
    inline void setStorage(ImageStorage s) const {
		m_map->setStorage(s);
    }

    /** Says nothing about the alpha channel */
    inline bool notBlack() const {
        return ! isBlack();
    }

    /** Returns true if there is non-unit alpha. */
    inline bool nonUnitAlpha() const {
        return (alpha(m_min) != 1.0f);
    }

    /** Says nothing about the alpha channel */
    inline bool isBlack() const {
        return m_max.rgb().max() == 0.0f;
    }
};

typedef Component<Color1, Image1> Component1;

typedef Component<Color3, Image3> Component3;

typedef Component<Color4, Image4> Component4;

}

#endif
