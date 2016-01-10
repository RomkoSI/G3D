// -*- c++ -*-
/**
 \file UniversalSurface_vertex.glsl
 \author Morgan McGuire, Michael Mara

  Provides helper transformation functions for universal surface.

 This is packaged separately from UniversalSurface_render.vrt and UniversalSurface_vertex.glsl to make it easy to compute 
 the object-space positions procedurally in related shaders but still use the material and
 lighting model from UniversalSurface.

 \beta

 \created 2013-08-13
 \edited  2014-06-19
 */
#ifndef UniversalSurface_vertexHelpers_h
#define UniversalSurface_vertexHelpers_h

#include <compatibility.glsl>

#expect NUM_LIGHTMAP_DIRECTIONS "0, 1, or 3"

/** Provided to make it easy to perform custom vertex manipulation and procedural vertex generation. Operates in object space. */

void UniversalSurface_customOSVertexTransformation(inout vec4 vertex, inout vec3 normal, inout vec4 packedTangent, inout vec2 tex0, inout vec2 tex1) {
    // Intentionally empty
}


mat4 getBoneMatrix(in sampler2D   boneMatrixTexture, in int index) {
    int baseIndex = index * 2;
    return mat4( 
        texelFetch(boneMatrixTexture, ivec2(baseIndex    , 0), 0),
        texelFetch(boneMatrixTexture, ivec2(baseIndex    , 1), 0),
        texelFetch(boneMatrixTexture, ivec2(baseIndex + 1, 0), 0),
        texelFetch(boneMatrixTexture, ivec2(baseIndex + 1, 1), 0));
}


mat4 UniversalSurface_getFullBoneTransform(in vec4 boneWeights, in ivec4 boneIndices, in sampler2D   boneMatrixTexture) {
    mat4 boneTransform = getBoneMatrix(boneMatrixTexture, boneIndices[0]) * boneWeights[0];
    for (int i = 1; i < 4; ++i) {
        boneTransform += getBoneMatrix(boneMatrixTexture, boneIndices[i]) * boneWeights[i];
    }
    return boneTransform;
}


// Transforms the vertex normal and packed tangent according to the skeletal animation matrices
void UniversalSurface_boneTransform(in vec4 boneWeights, in ivec4 boneIndices, in sampler2D   boneTexture, inout vec4 osVertex, inout vec3 osNormal, inout vec4 osPackedTangent) {
    vec3 wsEyePos = g3d_CameraToWorldMatrix[3].xyz;

    //mat4 boneTransform = mat4(1.0);
    mat4 boneTransform = UniversalSurface_getFullBoneTransform(boneWeights, boneIndices, boneTexture);

    osVertex            = boneTransform * osVertex;
    osNormal            = mat3(boneTransform) * osNormal;
    osPackedTangent.xyz = mat3(boneTransform) * osPackedTangent.xyz;

}

#endif
