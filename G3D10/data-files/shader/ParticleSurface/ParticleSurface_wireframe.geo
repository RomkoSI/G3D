#version 330 // -*- c++ -*-
#include <compatibility.glsl>
#include <g3dmath.glsl>

layout(points) in;

// If you are debugging where your actual triangles are, turn on.
#define SHOW_CENTER_VERTEX 0
#if SHOW_CENTER_VERTEX
layout(line_strip, max_vertices = 10) out;
#else
layout(line_strip, max_vertices = 5) out;
#endif


uniform float   nearPlaneZ;

// These arrays have a single element because they are GL_POINTS
in layout(location = 0) vec3    wsCenterVertexOutput[];
in layout(location = 1) vec3    shapeVertexOutput[];
in layout(location = 2) int4    materialIndexVertexOutput[];
in layout(location = 3) float   angleVertexOutput[];

float3 wsRight, wsUp;
float2 csRight, csUp;


/** Produce a vertex.  Note that x and y are compile-time constants, so 
    most of this arithmetic compiles out. */
void emit(float x, float y) {
    gl_Position = g3d_ProjectionMatrix * float4(gl_in[0].gl_Position.xy + csRight * x + csUp * y,  gl_in[0].gl_Position.z, 1.0);
    EmitVertex();
}


void main() {
    float radius      = shapeVertexOutput[0].x;
    float csZ   = gl_in[0].gl_Position.z;
    if (csZ >= nearPlaneZ) return; // culled by near plane

    float angle = angleVertexOutput[0];

    // Rotate the particle
    csRight = float2(cos(angle), sin(angle)) * radius;
    csUp    = float2(-csRight.y, csRight.x);

    wsRight = g3d_CameraToWorldMatrix[0].xyz * csRight.x + g3d_CameraToWorldMatrix[1].xyz * csRight.y;
    wsUp    = g3d_CameraToWorldMatrix[0].xyz * csUp.x    + g3d_CameraToWorldMatrix[1].xyz * csUp.y;
    

    // 
    //   C-------D    C-------D
    //   |       |    | \   / |
    //   |       |    |   E   |
    //   |       |    | /   \ |
    //   A-------B    A-------B
    //
    //     ABDCA       ABDCAEC DEB

    emit(-1, -1); // A
    emit(-1, +1); // B
    emit(+1, +1); // D
    emit(+1, -1); // C
    emit(-1, -1); // A
#if SHOW_CENTER_VERTEX
    emit( 0,  0); // E
    emit(+1, -1); // C
    EndPrimitive();
    emit(+1, +1); // D
    emit( 0,  0); // E
    emit(-1, +1); // B
#endif
    EndPrimitive();
}
