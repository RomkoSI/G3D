/**
 \file GLG3D/BumpMap.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2009-02-19
 \edited  2015-09-24
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_BumpMap_h
#define G3D_BumpMap_h

#include "G3D/platform.h"
#include "G3D/PixelTransferBuffer.h"
#include "GLG3D/Component.h"

namespace G3D {

class Any;


/** 
  \brief Normal + bump map for use with G3D::UniversalMaterial.

  Supports Blinn normal mapping, Kaneko-Welsh parallax mapping,
  and Tatarchuk style parallax occlusion mapping.
 */
class BumpMap : public ReferenceCountedObject {
public:

    class Settings {
    public:
        /** World-space scale to apply to bump height for
            parallax/displacement mapping. Default is 0.001f.*/
        float           scale;


        /** World-space offset from polygon surface to apply for
         parallax/displacement mapping.  Default is 0.
         
         Called "bias" instead of "offset" to avoid confusion with the
         computed parallax offset.
        */
        float           bias; 

        /**
          - 0  = Blinn normal map (default)
          - 1  = Kaneko-Welsh parallax map
          - >1 = Tatarchuk parallax occlusion map ("steep parallax map")
         */
        int            iterations;

        inline Settings() : scale(0.03f), bias(0.0f), iterations(1) {}

        Settings(const Any& any);

        void serialize(BinaryOutput& b) const {
            b.writeFloat32(scale);
            b.writeFloat32(bias);
            b.writeInt32(iterations);
        }

        void deserialize(BinaryInput& b) {
            scale = b.readFloat32();
            bias = b.readFloat32();
            iterations = b.readInt32();
        }

        Any toAny() const;

        bool operator==(const Settings& s) const;
    
        bool operator!=(const Settings& other) const {
            return ! (*this == other);
        }    
    };

    class Specification {
    public:
        /** If loading a height field, be sure to set  
            texture.preprocess = Texture::Preprocess::normalMap() */
        Texture::Specification          texture;
        Settings                        settings;

        Specification() {}

        /** The \a any should be either a string that is a filename of a height field or
            a table of texture and settings */
        Specification(const Any& any);

        bool operator==(const Specification& other) const;

        bool operator!=(const Specification& other) const {
            return ! (*this == other);
        }
    };

protected:

    /**
       - rgb = tangent-space normal
       - a   = bump height

       (Note that this is compressed to Image4unorm8 on the GPU)
     */
    shared_ptr<MapComponent<Image4> >   m_normalBump;

    Settings                    m_settings;

    BumpMap(const shared_ptr<MapComponent<Image4> >& normalBump, const Settings& settings);

    /** For speedCreate*/
    BumpMap() {}
    
public:

    /** \param normalBump Has tangent-space normals in rgb and bump elevation in a
        \param settings Settings 
        */
    static shared_ptr<BumpMap> create(const shared_ptr<MapComponent<Image4> >& normalBump, const Settings& settings);

    static shared_ptr<BumpMap> create(const Specification& spec);

    /** \sa SpeedLoad */
    static shared_ptr<BumpMap> speedCreate(BinaryInput& b);

    /** \sa SpeedLoad */
    void speedSerialize(BinaryOutput& b) const;

    //static shared_ptr<BumpMap> fromNormalFile(const String& filename, );

    inline const Settings& settings() const {
        return m_settings;
    }

    /**
       Packed normal map and bump map.

       - rgb = tangent-space normal
       - a   = bump height
     */
    inline const MapComponent<Image4>::Ref& normalBumpMap() const {
        return m_normalBump;
    }

    inline void setStorage(ImageStorage s) const {
        m_normalBump->setStorage(s);
    }



     /**
     Given a monochrome, tangent-space bump map, computes a new image where the
     RGB channels are a tangent space normal map and the alpha channel
     is the original bump map.  Assumes the input image is tileable.

     In the resulting image, x = red = tangent, y = green = binormal, and z = blue = normal. 
      */
    static shared_ptr<PixelTransferBuffer> computeNormalMap
        (int                 width,
	     int                 height,
	     int                 channels,
	     const unorm8*       src,
	     const BumpMapPreprocess& preprocess = BumpMapPreprocess());

    /** 
      \param signConvention Set the sign convention based on the coordinate system of your
       source normal map and texture coordinates. It will be fairly
       obvious if you choose the wrong one because the height map will be
       "inside out" along some dimension. -1 = G3D computeNormalMap default, +1 = 3DS Max*/
    static shared_ptr<PixelTransferBuffer> computeBumpMap
        (const shared_ptr<PixelTransferBuffer>& normalMap,
         float signConvention = -1.0f);
};

} // namespace G3D

#endif // G3D_BumpMap_h
