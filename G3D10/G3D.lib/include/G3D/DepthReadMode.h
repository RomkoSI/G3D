/** 
  \file G3D/DepthReadMode.h
 
  \maintainer Michael Mara, http://www.illuminationcodified.com
 
  \created 2013-10-10
  \edited  2013-10-10

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#ifndef G3D_DepthReadMode_h
#define G3D_DepthReadMode_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"

namespace G3D {

    /** A m_depth texture can automatically perform the m_depth
        comparison used for shadow mapping on a texture lookup.  The
        result of a texture lookup is thus the shadowed amount (which
        will be percentage closer filtered on newer hardware) and
        <I>not</I> the actual m_depth from the light's point of view.
       
        This combines GL_TEXTURE_COMPARE_MODE_ARB and GL_TEXTURE_COMPARE_FUNC_ARB from
        http://www.nvidia.com/dev_content/nvopenglspecs/GL_ARB_shadow.txt

        For best results on percentage closer hardware (GeForceFX and Radeon9xxx or better), 
        create shadow maps as m_depth textures with BILINEAR_NO_MIPMAP sampling.

        See also G3D::RenderDevice::configureShadowMap and the Collision_Demo.
     */
class DepthReadMode {
public:
    /** Don't use this enum; use InterpolateMode instances instead. */
    enum Value {DEPTH_NORMAL = 0, DEPTH_LEQUAL = 1, DEPTH_GEQUAL = 2} value;
    
    static const char* toString(int i, Value& v) {
        static const char* str[] = {"DEPTH_NORMAL", "DEPTH_LEQUAL", "DEPTH_GEQUAL", NULL}; 
        static const Value val[] = {DEPTH_NORMAL, DEPTH_LEQUAL, DEPTH_GEQUAL};
        const char* s = str[i];
        if (s) {
            v = val[i];
        }
        return s;
    }


    G3D_DECLARE_ENUM_CLASS_METHODS(DepthReadMode);
};


} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::DepthReadMode);

#endif
