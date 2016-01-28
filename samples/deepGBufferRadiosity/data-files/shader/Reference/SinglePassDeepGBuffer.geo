#version 440 // -*- c++ -*-



             /*
             Simple 3x geometry amplification for layered rendering
             \created 2013-09-10
             \edited  2013-10-15
             */

#include <compatibility.glsl>

    #expect PREDICT_NEXT_FRAME "0 or 1"

    layout(triangles) in;

#if USE_VIEWPORT_MULTICAST
layout(viewport_relative) out int gl_Layer;
layout(triangle_strip, max_vertices = 3) out;
#else
out int gl_Layer;
#if PREDICT_NEXT_FRAME
#define NUM_LAYERS 3
layout(triangle_strip, max_vertices = 9) out;
#   else
#define NUM_LAYERS (MAX_LAYER_INDEX+1)
layout(triangle_strip, max_vertices = NUM_LAYERS * 3) out;
#endif
#endif


#if USE_VIEWPORT_MULTICAST
layout(passthrough) in gl_PerVertex {
    vec4 gl_Position;
};

// Texture coordinate 
in  layout(location = 0, passthrough) vec2 texCoord[];

in layout(location = 1, passthrough) vec3 wsPosition[];

#if defined(CS_POSITION_CHANGE) || defined(SS_POSITION_CHANGE) || REPROJECT
in layout(location = 7, passthrough) vec3 csPrevPosition[];
#endif

#ifdef NORMALBUMPMAP
#   if (PARALLAXSTEPS > 0)
// Un-normalized (interpolated) tangent space eye vector 
in layout(location = 6, passthrough) vec3 _tsE[];
#   endif    
in layout(location = 4, passthrough) vec3 tan_X[];

in layout(location = 5, passthrough) vec3 tan_Y[];
#endif

in layout(location = 2, passthrough) vec3 tan_Z[];

#else
/** Texture coordinate */
in  layout(location = 0) vec2 texCoord_in[];
out layout(location = 0) vec2 texCoord;

in layout(location = 1) vec3 wsPosition_in[];
out layout(location = 1) vec3 wsPosition;

#if defined(CS_POSITION_CHANGE) || defined(SS_POSITION_CHANGE) || REPROJECT
in layout(location = 7) vec3 csPrevPosition_in[];
out layout(location = 7) vec3 csPrevPosition;
#elif  PREDICT_NEXT_FRAME
in layout(location = 7) vec3 csPrevPosition_in[];
#endif

#ifdef NORMALBUMPMAP
#   if (PARALLAXSTEPS > 0)
/** Un-normalized (interpolated) tangent space eye vector */
in layout(location = 6) vec3 _tsE_in[];
out layout(location = 6) vec3 _tsE;
#   endif    
in layout(location = 4) vec3 tan_X_in[];
out layout(location = 4) vec3 tan_X;

in layout(location = 5) vec3 tan_Y_in[];
out layout(location = 5) vec3 tan_Y;
#endif

in layout(location = 2) vec3 tan_Z_in[];
out layout(location = 2) vec3 tan_Z;
#endif

#ifdef SVO_POSITION
#error "You can't have SVO layered rendering. WTF would that mean anyway?"
#endif



void main() {
#if USE_VIEWPORT_MULTICAST
#define VIEWPORT_MASK ((1 << (MAX_LAYER_INDEX+1)) - 1)

    gl_ViewportMask[0] = VIEWPORT_MASK;
#else
    // For each triangle
    for (int i = 0; i < NUM_LAYERS; ++i) {
        gl_Layer = i;
        for (int j = 0; j < 3; ++j) {
            texCoord = texCoord_in[j];
            wsPosition = wsPosition_in[j];
#           if defined(CS_POSITION_CHANGE) || defined(SS_POSITION_CHANGE) || REPROJECT
            csPrevPosition = csPrevPosition_in[j];
#           endif

#           ifdef NORMALBUMPMAP
#               if (PARALLAXSTEPS > 0)
            _tsE = _tsE_in[j];
#               endif    
            tan_X = tan_X_in[j];
            tan_Y = tan_Y_in[j];
#endif
            tan_Z = tan_Z_in[j];

#           if PREDICT_NEXT_FRAME
            if (i < 2) {
                gl_Position = gl_in[j].gl_Position;
            } else {
                vec3 csPosition = g3d_WorldToCameraMatrix * vec4(wsPosition, 1.0);
                vec3 csVelocity = csPosition - csPrevPosition_in[j];
                gl_Position = g3d_ProjectionMatrix * vec4(csPosition + csVelocity, 1.0);
            }
            EmitVertex();
#           else
            gl_Position = gl_in[j].gl_Position;
            EmitVertex();
#           endif

        }

        EndPrimitive();
    }
#endif

}
