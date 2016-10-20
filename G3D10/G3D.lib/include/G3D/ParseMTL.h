/**
 \file G3D/ParseMTL.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2011-07-19
 \edited  2016-03-15

 Copyright 2002-2016, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_ParseMTL_h
#define G3D_ParseMTL_h

#include "G3D/platform.h"
#include "G3D/Table.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Color3.h"
#include "G3D/Vector2.h"
#include "G3D/G3DString.h"

namespace G3D {

class TextInput;

/** \brief Parses Wavefront material (.mtl) files.

    Parsing creates references to texture files on disk, but does not actually
    load those textures.

    Supports the extension of interpolation modes for texture maps.
    "interpolateMode <mode>" sets the mode, which applies to all maps until changed.
    The default is TRILINEAR_MIPMAP. The other options are G3D::InterpolateMode values.

    Supports "lightMap" field extension.

    \sa G3D::ParseOBJ, G3D::ArticulatedModel
*/
class ParseMTL {
public:

    /** Loaded from the MTL file */
    class Material : public ReferenceCountedObject {
    public:

        class Field {
        public:
            /** e.g., Ks, Kd, etc. */
            Color3      constant;

            /** e.g., map_Kd, etc. */
            String      map;

            /** [-mm x y] = bias, gain.  -bm Bump multiplier is put into the y coordinate as well. */
            Vector2     mm;

            explicit Field(float c) : constant(c), mm(0.0f, 1.0f) {}
        };

        String          name;
            
        /** Path relative to which filenames should be resolved */
        String          basePath;

        /** Ambient color of the material, on the range 0-1 */
        Field           Ka;

        /** Diffuse color of the material, on the range 0-1 */
        Field           Kd;

        /** Specular color of the material, on the range 0-1. */
        Field           Ks;

        /** Emissive */
        Field           Ke;

        /** Bump map */
        Field           bump;

        /** Shininess of the material, on the range 0-1000. */
        float           Ns;

        /** Opacity (alpha) level, on the range 0-1, where 1 = opaque (default).
            Some non-standard MTL files (e.g., produced by MeshLab) write Tr = 1 - d instead.
            If ParseMTL encounters Tr, it sets d = 1 - Tr.
         */
        float           d;
        String          map_d;

        /** 1 - Transmission, as processed by 3DS Max 
            (http://casual-effects.blogspot.com/2012/01/translucency-in-obj-mtl-files.html).  Other specification documents
            say that it is transmission (e.g, http://paulbourke.net/dataformats/mtl/) but I haven't found software 
            that implements it that way. */
        Color3          Tf;
        
        /** Illumination model enumeration on the range 0-10:
        
         model |          Property Editor
         ------|---------------------------
         0	   |Color on and Ambient off
         1	   |Color on and Ambient on
         2	   |Highlight on
         3	   |Reflection on and Ray trace on
         4	   |Transparency: Glass on; Reflection: Ray trace on
         5	   |Reflection: Fresnel on and Ray trace on
         6	   |Transparency: Refraction on; Reflection: Fresnel off and Ray trace on
         7	   |Transparency: Refraction on; Reflection: Fresnel on and Ray trace on
         8	   |Reflection on and Ray trace off
         9	   |Transparency: Glass on; Reflection: Ray trace off
         10	   |Casts shadows onto invisible surfaces

         3-7 force mirror glossiness. 2 is probably what you want.

         \cite http://paulbourke.net/dataformats/mtl/
        */
        int             illum;

        /** Index of refraction */
        float           Ni;

        /** (non-standard extension), for lightMaps */
        String          lightMap;

        String          interpolateMode;

    private:

        // We default Ks to -1 because we want to default it to 1 if there 
        // is a map_Ks and 0.75f otherwise (which then gets raised to the ninth power by G3D)
        // We thus have to check and properly set the default whenever we finish parsing a material or
        // assign map_Ks
        Material() : Ka(1.0f), Kd(1.0f), Ks(-1.0f), Ke(0.0f), bump(0.0f), Ns(10.0f), d(1.0f), Tf(1.0f), illum(2), Ni(1.0f), interpolateMode("TRILINEAR_MIPMAP") {}

    public:
        /** We default Ks to 0.8f if there is no map_Ks. 
        This is non-standard but matches G3D's lighting model better. 
        The specification default(and what we default to when there is a map_Ks) is 1.0f.
        Note that we raise Ks to the 9th power when loading into an Articulated Model.
        */
        static shared_ptr<Material> create() {
            return shared_ptr<Material>(new Material());
        }
    };
    
    Table<String, shared_ptr<Material> > materialTable;

private:

    /** Process one line of an OBJ file */
    void processCommand(TextInput& ti, const String& cmd);

    shared_ptr<Material>        m_currentMaterial;

    /** Paths are interpreted relative to this */
    String                 m_basePath;

public:

    ParseMTL();

    /** \param basePath Directory relative to which texture filenames are resolved. If "<AUTO>", the 
     path to the TextInput%'s file is used. */
    void parse(TextInput& ti, const String& basePath = "<AUTO>");

};


} // namespace G3D


template <> struct HashTrait<shared_ptr<G3D::ParseMTL::Material> > {
    static size_t hashCode(const shared_ptr<G3D::ParseMTL::Material>& k) { return reinterpret_cast<size_t>(k.get()); }
};


#endif // G3D_ParseMTL_h

