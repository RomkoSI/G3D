/**
 @file Map2D.h

 More flexible support than provided by G3D::GImage.

 @maintainer Morgan McGuire, morgan@cs.brown.edu
 @created 2004-10-10
 @edited  2009-03-24
 */
#ifndef G3D_Map2D_h
#define G3D_Map2D_h

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Array.h"
#include "G3D/vectorMath.h"
#include "G3D/Vector2int16.h"
#include "G3D/ReferenceCount.h"
#include "G3D/AtomicInt32.h"
#include "G3D/Thread.h"
#include "G3D/Rect2D.h"
#include "G3D/WrapMode.h"

#include "G3D/G3DString.h"

namespace G3D {
namespace _internal {

/** The default compute type for a type is the type itself. */
template<typename Storage> class _GetComputeType {
public:
    typedef Storage Type;
};

} // _internal
} // G3D

// This weird syntax is needed to support VC6, which doesn't
// properly implement template overloading.
#define DECLARE_COMPUTE_TYPE(StorageType, ComputeType)            \
namespace G3D {                                                   \
    namespace _internal {                                         \
        template<> class _GetComputeType < StorageType > {        \
        public:                                                   \
            typedef ComputeType Type;                             \
        };                                                        \
    }                                                             \
}

DECLARE_COMPUTE_TYPE( float32,  float64)
DECLARE_COMPUTE_TYPE( float64,  float64)

DECLARE_COMPUTE_TYPE( int8,     float32)
DECLARE_COMPUTE_TYPE( int16,    float32)
DECLARE_COMPUTE_TYPE( int32,    float64)
DECLARE_COMPUTE_TYPE( int64,    float64)

DECLARE_COMPUTE_TYPE( uint8,     float32)
DECLARE_COMPUTE_TYPE( uint16,    float32)
DECLARE_COMPUTE_TYPE( uint32,    float64)
DECLARE_COMPUTE_TYPE( uint64,    float64)

DECLARE_COMPUTE_TYPE( Vector2,  Vector2)
DECLARE_COMPUTE_TYPE( Vector2int16,  Vector2)

DECLARE_COMPUTE_TYPE( Vector3,  Vector3)
DECLARE_COMPUTE_TYPE( Vector3int16,  Vector3)

DECLARE_COMPUTE_TYPE( Vector4,  Vector4)

DECLARE_COMPUTE_TYPE( Color3,  Color3)
DECLARE_COMPUTE_TYPE( Color3uint8,  Color3)

DECLARE_COMPUTE_TYPE( Color4,  Color4)
DECLARE_COMPUTE_TYPE( Color4uint8,  Color4)
#undef DECLARE_COMPUTE_TYPE

namespace G3D {

/**
  Map of values across a discrete 2D plane.  Can be thought of as a generic class for 2D images, 
  allowing flexibility as to pixel format and convenient methods.
  In fact, the "pixels" can be any values
  on a grid that can be sensibly interpolated--RGB colors, scalars, 4D vectors, and so on.

  Other "image" classes in G3D:

  G3D::Image - Supports file formats, fast, Color3uint8 and Color4uint8 formats.  No interpolation.

  G3D::shared_ptr<Texture> - Represents image on the graphics card (not directly readable on the CPU).  Supports 2D, 3D, and a variety of interpolation methods, loads file formats.

  G3D::Image3 - A subclass of Map2D<Color3> that supports image loading and saving and conversion to Texture.

  G3D::Image4 - A subclass of Map2D<Color4> that supports image loading and saving and conversion to Texture.

  G3D::Image3uint8 - A subclass of Map2D<Color3uint8> that supports image loading and saving and conversion to Texture.

  G3D::Image4uint8 -  A subclass of Map2D<Color4uint8> that supports image loading and saving and conversion to Texture.

  There are two type parameters-- the first (@ Storage) is the type 
  used to store the "pixel" values efficiently and 
  the second (@a Compute) is
  the type operated on by computation.  The Compute::Compute(Storage&) constructor
  is used to convert between storage and computation types.
  @a Storage is often an integer version of @a Compute, for example 
  <code>Map2D<double, uint8></code>.  By default, the computation type is:

  <pre>
     Storage       Computation

     uint8          float32
     uint16         float32
     uint32         float64
     uint64         float64

     int8           float32
     int16          float32
     int32          float64
     int64          float64

     float32        float64
     float64        float64

     Vector2        Vector2
     Vector2int16   Vector2

     Vector3        Vector3
     Vector3int16   Vector3

     Vector4        Vector4

     Color3         Color3
     Color3uint8    Color3

     Color4         Color4
     Color4uint8    Color4
    </pre>
  Any other storage type defaults to itself as the computation type.

  The computation type can be any that 
  supports lerp, +, -, *, /, and an empty constructor.

  Assign value:

    <code>im->set(x, y, 7);</code> or 
    <code>im->get(x, y) = 7;</code>

  Read value:

    <code>int c = im(x, y);</code>

  Can also sample with nearest neighbor, bilinear, and bicubic
  interpolation.  
  
  Sampling follows OpenGL conventions, where 
  pixel values represent grid points and (0.5, 0.5) is half-way
  between two vertical and two horizontal grid points.  
  To draw an image of dimensions w x h with nearest neighbor
  sampling, render pixels from [0, 0] to [w - 1, h - 1].

  Under the WrapMode::CLAMP wrap mode, the value of bilinear interpolation
  becomes constant outside [1, w - 2] horizontally.  Nearest neighbor
  interpolation is constant outside [0, w - 1] and bicubic outside
  [3, w - 4].  The class does not offer quadratic interpolation because
  the interpolation filter could not center over a pixel.

  In order to allow subclasses to mimic layered textures, Map2Ds can have an optional depth.
  Only the constructor and resize() handles depth in this class, to access anything but layer 0
  subclasses must implement the functionality themselves.

  
  \author Morgan McGuire, http://graphics.cs.williams.edu
 */
template< typename Storage, 
          typename Compute = typename G3D::_internal::_GetComputeType<Storage>::Type>
class Map2D : public ReferenceCountedObject {

//
// It doesn't make sense to automatically convert from Compute back to Storage
// because the rounding rule (and scaling) is application dependent.
// Thus the interpolation methods all return type Compute.
//

public:

    typedef Storage StorageType;
    typedef Compute ComputeType;
    typedef Map2D<Storage, Compute> Type;

protected:
    
    Storage             ZERO;

    /** Width, in pixels. */
    uint32              w;

    /** Height, in pixels. */
    uint32              h;

    /** Depth, in pixels, defaults to 1 */
    uint32              d;

    WrapMode            m_wrapMode;

    /** 0 if no mutating method has been invoked 
        since the last call to setChanged(); */
    AtomicInt32         m_changed;

    Array<Storage>      data;

    /** Handles the exceptional cases from get */
    const Storage& slowGet(int x, int y, WrapMode wrap) {
        switch (wrap) {
        case WrapMode::CLAMP:
            return fastGet(iClamp(x, 0, w - 1), iClamp(y, 0, h - 1));

        case WrapMode::TILE:
            return fastGet(iWrap(x, w), iWrap(y, h));

        case WrapMode::ZERO:
            return ZERO;

        case WrapMode::ERROR:
            alwaysAssertM(((uint32)x < w) && ((uint32)y < h), 
                format("Index out of bounds: (%d, %d), w = %d, h = %d",
                x, y, w, h));

            // intentionally fall through
        case WrapMode::IGNORE:
            // intentionally fall through
        default:
            {
                static Storage temp;
                return temp;
            }
        }
    }

public:

    /** Unsafe access to the underlying data structure with no wrapping support; requires that (x, y) is in bounds. */
    inline const Storage& fastGet(int x, int y) const {
        debugAssert(((uint32)x < w) && ((uint32)y < h));
        return data[x + y * w];
    }

    /** Unsafe access to the underlying data structure with no wrapping support; requires that (x, y) is in bounds. */
    inline void fastSet(int x, int y, const Storage& v) {
        debugAssert(((uint32)x < w) && ((uint32)y < h));
        data[x + y * w] = v;
    }

protected:

    Map2D(int w, int h, WrapMode wrap, int d = 1) : w(0), h(0), d(1), m_wrapMode(wrap), m_changed(1) {
        ZERO = Storage(Compute(Storage()) * 0);
        resize(w, h, d);
    }

public:

    /**
     Although Map2D is not threadsafe (except for the setChanged() method),
     you can use this mutex to create your own threadsafe access to a Map2D.
     Not used by the default implementation.
    */
    GMutex mutex;

    static shared_ptr< Map2D<Storage, Compute> > create(int w = 0, int h = 0, WrapMode wrap = WrapMode::ERROR, int d = 1) {
        return createShared< Map2D<Storage, Compute> >(w, h, wrap, d);
    }

    /** Resizes without clearing, leaving garbage.
      */
    void resize(uint32 newW, uint32 newH, uint32 newD = 1) {
        if ((newW != w) || (newH != h) || (newD != d)) {
            w = newW;
            h = newH;
            d = newD;
            data.resize(w * h * d);
            setChanged(true);
        }
    }

    /** 
     Returns true if this map has been written to since the last call to setChanged(false).
     This is useful if you are caching a texture map other value that must be recomputed
     whenever this changes.
     */
    bool changed() {
        return m_changed.value() != 0;
    }

    /** Set/unset the changed flag. */
    void setChanged(bool c) {
        m_changed = c ? 1 : 0;
    }

    /** Returns a pointer to the underlying row-major data. There is no padding at the end of the row.
        Be careful--this will be reallocated during a resize.  You should call setChanged(true) if you mutate the array.*/
    Storage* getCArray() {
        return data.getCArray();
    }


    const Storage* getCArray() const {
        return data.getCArray();
    }


    /** Row-major array.  You should call setChanged(true) if you mutate the array. */
    Array<Storage>& getArray() {
        return data;
    }


    const Array<Storage>& getArray() const {
        return data;
    }

    /** is (x, y) strictly within the image bounds, or will it trigger some kind of wrap mode */
    inline bool inBounds(int x, int y) const {
        return (((uint32)x < w) && ((uint32)y < h));
    }

    /** is (x, y) strictly within the image bounds, or will it trigger some kind of wrap mode */
    inline bool inBounds(const Vector2int16& v) const {
        return inBounds(v.x, v.y);
    }

    /** Get the value at (x, y).
    
        Note that the type of image->get(x, y) is 
        the storage type, not the computation
        type.  If the constructor promoting Storage to Compute rescales values
        (as, for example Color3(Color3uint8&) does), this will not match the value
        returned by Map2D::nearest.
      */
    inline const Storage& get(int x, int y, WrapMode wrap) const {
        if (((uint32)x < w) && ((uint32)y < h)) {
            return data[x + y * w];
        } else {
            // Remove the const to allow a slowGet on this object
            // (we're returning a const reference so this is ok)
            return const_cast<Type*>(this)->slowGet(x, y, wrap);
        }
#       ifndef G3D_WINDOWS
            // gcc gives a useless warning that the above code might reach the end of the function;
            // we use this line to supress the warning.
            return ZERO;
#       endif
    }

    inline const Storage& get(int x, int y) const {
        return get(x, y, m_wrapMode);
    }

    inline const Storage& get(const Vector2int16& p) const {
        return get(p.x, p.y, m_wrapMode);
    }

    inline const Storage& get(const Vector2int16& p, WrapMode wrap) const {
        return get(p.x, p.y, wrap);
    }

    inline Storage& get(int x, int y, WrapMode wrap) {
        return const_cast<Storage&>(const_cast<const Type*>(this)->get(x, y, wrap));
#       ifndef G3D_WINDOWS
            // gcc gives a useless warning that the above code might reach the end of the function;
            // we use this line to supress the warning.
            return ZERO;
#       endif
    }

    inline Storage& get(int x, int y) {
        return const_cast<Storage&>(const_cast<const Type*>(this)->get(x, y));
#       ifndef G3D_WINDOWS
            // gcc gives a useless warning that the above code might reach the end of the function;
            // we use this line to supress the warning.
            return ZERO;
#       endif
    }

    inline Storage& get(const Vector2int16& p) {
        return get(p.x, p.y);
    }

    /** Sets the changed flag to true */
    inline void set(const Vector2int16& p, const Storage& v) {
        set(p.x, p.y, v);
    }

    /** Sets the changed flag to true */
    void set(int x, int y, const Storage& v, WrapMode wrap) {
        setChanged(true);
        if (((uint32)x < w) && ((uint32)y < h)) {
            // In bounds, wrapping isn't an issue.
            data[x + y * w] = v;
        } else {
            const_cast<Storage&>(slowGet(x, y, wrap)) = v;
        }
    }

    void set(int x, int y, const Storage& v) {
        set(x, y, v, m_wrapMode);
    }


    void setAll(const Storage& v) {
        for(int i = 0; i < data.size(); ++i) {
            data[i] = v;
        }
        setChanged(true);
    }

    /** Copy values from \a src, which must have the same size */
    template<class T>
    void set(const shared_ptr<Map2D<Storage, T> >& src) {
        debugAssert(src->width() == width());
        debugAssert(src->height() == height());
        const Array<Storage>& s = src->data;
        int N = w * h;
        for (int i = 0; i < N; ++i) {
            data[i] = s[i];
        }
        setChanged(true);
    }

    /** flips if @a flip is true*/
    void maybeFlipVertical(bool flip) {
        if (flip) {
            flipVertical();
        }
    }

    virtual void flipVertical() {
        int halfHeight = h/2;
        Storage* d = data.getCArray();
        for (int y = 0; y < halfHeight; ++y) {
            int o1 = y * w;
            int o2 = (h - y - 1) * w;
            for (int x = 0; x < (int)w; ++x) {
                int i1 = o1 + x;
                int i2 = o2 + x;
                Storage temp = d[i1];
                d[i1] = d[i2];
                d[i2] = temp;
            }
        }
        setChanged(true);
    }
    
    virtual void flipHorizontal() {
        int halfWidth = w / 2;
        Storage* d = data.getCArray();
        for (int x = 0; x < halfWidth; ++x) {
            for (int y = 0; y < (int)h; ++y) {
                int i1 = y * w + x;
                int i2 = y * w + (w - x - 1);
                Storage temp = d[i1];
                d[i1] = d[i2];
                d[i2] = temp;
            }
        }
        setChanged(true);
    }
    
    /**
     Crops this map so that it only contains pixels between (x, y) and (x + w - 1, y + h - 1) inclusive.
     */
    virtual void crop(int newX, int newY, int newW, int newH) {
        alwaysAssertM(newX + newW <= (int)w, "Cannot grow when cropping");
        alwaysAssertM(newY + newH <= (int)h, "Cannot grow when cropping");
        alwaysAssertM(newX >= 0 && newY >= 0, "Origin out of bounds.");

        // Always safe to copy towards the upper left, provided 
        // that we're iterating towards the lower right.  This lets us avoid
        // reallocating the underlying array.
        for (int y = 0; y < newH; ++y) {
            for (int x = 0; x < newW; ++x) {
                data[x + y * newW] = data[(x + newX) + (y + newY) * w];
            }
        }

        resize(newW, newH);
    }

    /** iRounds to the nearest x0 and y0. */
    virtual void crop(const Rect2D& rect) {
        crop(iRound(rect.x0()), iRound(rect.y0()), iRound(rect.x1()) - iRound(rect.x0()), iRound(rect.y1()) - iRound(rect.y0()));
    }

    /** Returns the nearest neighbor.  Pixel values are considered
        to be at the upper left corner, so <code>image->nearest(x, y) == image(x, y)</code>
      */
    inline Compute nearest(float x, float y, WrapMode wrap) const {
        int ix = iRound(x);
        int iy = iRound(y);
        return Compute(get(ix, iy, wrap));
    }

    inline Compute nearest(float x, float y) const {
        return nearest(x, y, m_wrapMode);
    }

    inline Compute nearest(const Vector2& p) const {
        return nearest(p.x, p.y);
    }

    /** Returns the average value of all elements of the map */
    Compute average() const {
        if ((w == 0) || (h == 0)) {
            return ZERO;
        }

        // To avoid overflows, compute the average of row averages

        Compute rowSum = ZERO;
        for (unsigned int y = 0; y < h; ++y) {
            Compute sum = ZERO;
            int offset = y * w;
            for (unsigned int x = 0; x < w; ++x) {
                sum += Compute(data[offset + x]);
            }
            rowSum += sum * (1.0f / w);
        }

        return rowSum * (1.0f / h);
    }

    /** 
      Needs to access elements from (floor(x), floor(y))
      to (floor(x) + 1, floor(y) + 1) and will use
      the wrap mode appropriately (possibly generating 
      out of bounds errors).

      Guaranteed to match nearest(x, y) at integers. */ 
    Compute bilinear(float x, float y, WrapMode wrap) const {
        const int i = iFloor(x);
        const int j = iFloor(y);
    
        const float fX = x - i;
        const float fY = y - j;

        // Horizontal interpolation, first row
        const Compute& t0 = (Compute)get(i, j, wrap);
        const Compute& t1 = (Compute)get(i + 1, j, wrap);

        // Horizontal interpolation, second row
        const Compute& t2 = (Compute)get(i, j + 1, wrap);
        const Compute& t3 = (Compute)get(i + 1, j + 1, wrap);

        const Compute& A = lerp(t0, t1, fX);
        const Compute& B = lerp(t2, t3, fX);

        // Vertical interpolation
        return lerp(A, B, fY);
    }

    Compute bilinear(float x, float y) const {
        return bilinear(x, y, m_wrapMode);
    }

    inline Compute bilinear(const Vector2& p) const {
        return bilinear(p.x, p.y, m_wrapMode);
    }

    inline Compute bilinear(const Vector2& p, WrapMode wrap) const {
        return bilinear(p.x, p.y, wrap);
    }


    // Weighting polynomial http://paulbourke.net/texture_colour/imageprocess/
    static float R(float x) {
        static const float coeff[4] = { 1.0f, -4.0f, 6.0f, -4.0f };
        float result = 0.0f;
        for (int j = 0; j < 4; ++j) {
            result += coeff[j] * pow(max(0.0f, x + 2 - j), 3.0f);
        }
        return result / 6.0f;
    }

    /**
     Uses Catmull-Rom splines to interpolate between grid
     values. 
     */
    Compute bicubic(float x, float y, WrapMode wrap) const {
        const int ix = int(x);
        const int iy = int(y);

        // Fractional part (Bourke's dx, dy)
        const float fx = x - floor(x);
        const float fy = y - floor(y);

        Compute result= Compute(ZERO);
        for (int m = -1; m <= 2; ++m) {
            for (int n = -1; n <= 2; ++n) {
                result += get(ix + m, iy + n, WrapMode::CLAMP) * R(float(m) - fx) * R(fy - float(n));
            }
        }
        return result;
    }

    Compute bicubic(float x, float y) const {
        return bicubic(x, y, m_wrapMode);
    }

    inline Compute bicubic(const Vector2& p, WrapMode wrap) const {
        return bicubic(p.x, p.y, wrap);
    }

    inline Compute bicubic(const Vector2& p) const {
        return bicubic(p.x, p.y, m_wrapMode);
    }

    /** Pixel width */
    inline int32 width() const {
        return (int32)w;
    }


    /** Pixel height */
    inline int32 height() const {
        return (int32)h;
    }


    /** Dimensions in pixels */
    Vector2int16 size() const {
        return Vector2int16(w, h);
    }

    /** Rectangle from (0, 0) to (w, h) */
    Rect2D rect2DBounds() const {
        return Rect2D::xywh(0, 0, float(w), float(h));
    }

    /** Number of bytes occupied by the image data and this structure */
    size_t sizeInMemory() const {
        return data.size() * sizeof(Storage) + sizeof(*this);
    }


    WrapMode wrapMode() const {
        return m_wrapMode;
    }


    void setWrapMode(WrapMode m) {
        m_wrapMode = m;
    }
};



}

#endif // G3D_IMAGE_H
