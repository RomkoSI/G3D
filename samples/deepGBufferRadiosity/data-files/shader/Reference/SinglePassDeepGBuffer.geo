#version 440 // -*- c++ -*-

#extension NV_viewport_array2 : enable
#extension NV_geometry_shader_passthrough : enable

#if defined(GL_NV_viewport_array2) && defined(GL_NV_geometry_shader_passthrough)
//  Fast path for 
#   define USE_VIEWPORT_MULTICAST
#endif

layout(triangles) in;

#define NUM_LAYERS 2

#if USE_VIEWPORT_MULTICAST
    layout(viewport_relative) out int gl_Layer;
    layout(triangle_strip, max_vertices = 3) out;

    layout(passthrough) in gl_PerVertex {
        vec4 gl_Position;
    };

    in layout(location = 0, passthrough) vec3 csPosition[];
    in layout(location = 1, passthrough) vec3 csPrevPosition[];

    // TODO: Passthrough all other attributes here
#else
    out int gl_Layer;
    out int gl_ViewportIndex;
    layout(triangle_strip, max_vertices = 3 * NUM_LAYERS) out;

    in  layout(location = 0) vec2 csPosition_in[];
    out layout(location = 0) vec2 csPosition;

    in layout(location = 1) vec3 csPrevPosition_in[];
    out layout(location = 1) vec3 csPrevPosition;

    // TODO: Passthrough all other attributes here
#endif


void main() {
#if USE_VIEWPORT_MULTICAST
#   define VIEWPORT_MASK ((1 << NUM_LAYERS) - 1)
    gl_ViewportMask[0] = VIEWPORT_MASK;
#else
    // For each triangle
    for (int i = 0; i < NUM_LAYERS; ++i) {
        gl_Layer = i;
        for (int j = 0; j < 3; ++j) {
            csPosition      = csPosition_in[j];
            csPrevPosition  = csPrevPosition_in[j];
            // Pass through any extra attributes here

            gl_Position = gl_in[j].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
#endif
}
