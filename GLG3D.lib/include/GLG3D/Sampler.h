/**
  \file GLG3D/Sampler.h

  \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2013-10-03
  \edited  2014-03-26
*/

#ifndef GLG3D_Sampler_h
#define GLG3D_Sampler_h

#include "G3D/WrapMode.h"
#include "G3D/InterpolateMode.h"
#include "G3D/DepthReadMode.h"
#include "G3D/WeakCache.h"
#include "GLG3D/glheaders.h"

namespace G3D {

class Shader;
class Texture;

/**
 \brief A class holding all of the parameters one would want to use to when accessing a Texture

 \sa GLSamplerObject, Texture
 */
class Sampler {
public:

    /** Default is TRILINEAR_MIPMAP */
    InterpolateMode    interpolateMode;

    /** Default is TILE */
    WrapMode            xWrapMode;

    /** Wrap mode for the Y coordinate. Default is TILE */
    WrapMode            yWrapMode;

    /** Default is DEPTH_NORMAL */
    DepthReadMode       depthReadMode;

    /** Default is 4.0 */
    float               maxAnisotropy;

    /**
        Highest MIP-map level that will be used during rendering.
        The highest level that actually exists will be L =
        log(max(m_width, m_height), 2)), although it is fine to set
        maxMipMap higher than this.  Must be larger than minMipMap.
        Default is 1000.

        Setting the max mipmap level is useful for preventing
        adjacent areas of a texture from being blurred together when
        viewed at a distance.  It may decrease performance, however,
        by forcing a larger texture into cache than would otherwise
        be required.
        */
    int             maxMipMap;

    /**
        Lowest MIP-map level that will be used during rendering.
        Level 0 is the full-size image.  Default is -1000, matching
        the OpenGL spec.  @cite
        http://oss.sgi.com/projects/ogl-sample/registry/SGIS/texture_lod.txt
        */
    int             minMipMap;

    /** Add to the MIP level after it is computed. >0 = make blurrier, <0 = make sharper*/
    float           mipBias;

    Sampler(WrapMode m = WrapMode::TILE, InterpolateMode i = InterpolateMode::TRILINEAR_MIPMAP);

    Sampler(WrapMode x, WrapMode y, InterpolateMode i = InterpolateMode::TRILINEAR_MIPMAP);

    /** \param any Must be in the form of a table of the fields or appear as
        a call to a static factory method, e.g.,:

        - Sampler::Sampler{  interpolateMode = "TRILINEAR_MIPMAP", wrapMode = "TILE", ... }
        - Sampler::Sampler::video()
    */
    Sampler(const Any& any);

    Any toAny() const;

    static const Sampler& defaults();

    static const Sampler& defaultClamp();

    /** 
        Useful defaults for video/image processing.
        BILINEAR_NO_MIPMAP / CLAMP / DEPTH_NORMAL
    */
    static const Sampler& video();

    /** 
        Useful defaults for general purpose computing.
        NEAREST_NO_MIPMAP / CLAMP / DEPTH_NORMAL
    */
    static const Sampler& buffer();

    /** 
        Useful defaults for shadow maps.
        BILINEAR_NO_MIPMAP / CLAMP / DEPTH_LEQUAL
    */
    static const Sampler& shadow();

    /** 
        Useful defaults for light maps.
        TRILINEAR_MIPMAP / CLAMP / DEPTH_NORMAL / maxAnisotropy = 4.0
    */
    static const Sampler& lightMap();

    /**
        Useful defaults for cube maps
        TRILINEAR_MIPMAP / CLAMP / DEPTH_NORMAL
        */
    static const Sampler& cubeMap();

    /**
        Useful defaults for directly visualizing data
        NEAREST_MIPMAP / CLAMP / DEPTH_NORMAL
    */
    static const Sampler& visualization();

    bool operator==(const Sampler& other) const;

    bool equals(const Sampler& other) const;

    size_t hashCode() const;
};

} //G3D

template <> struct HashTrait<G3D::Sampler> {
    static size_t hashCode(const G3D::Sampler& key) { return key.hashCode(); }
};

#endif
