#ifndef g3dmath_glsl // -*- c++ -*-
#define g3dmath_glsl
#include <compatibility.glsl>

/**
  \file g3dmath.glsl
  \author Morgan McGuire (http://graphics.cs.williams.edu), Michael Mara (http://www.illuminationcodified.com)
  Defines basic mathematical constants and functions used in several shaders
*/

// Some constants
const float pi = 3.1415927;
const float invPi = 1.0 / pi;
const float inv8pi = 1.0 / (8.0 * pi);

const float meters      = 1.0;
const float centimeters = 0.01;
const float millimeters = 0.001;
const float inf         = 1.0 / 0.0;

#ifndef vec1
#define vec1 float
#endif

#define Vector2 vec2
#define Point2  vec2
#define Vector3 vec3
#define Point3  vec3
#define Vector4 vec4

#define Color3  vec3
#define Radiance3 vec3
#define Biradiance3 vec3
#define Irradiance3 vec3
#define Radiosity3 vec3
#define Power3 vec3

#define Color4  vec4
#define Radiance4 vec4
#define Biradiance4 vec4
#define Irradiance4 vec4
#define Radiosity4 vec4
#define Power4 vec4

#define Vector2int32 int2
#define Vector3int32 int3
#define Matrix4      mat4
#define Matrix3      mat3
#define Matrix2      mat2


#foreach (gentype) in (int), (ivec2), (ivec3), (ivec4), (float), (vec2), (vec3), (vec4)
void swap(inout $(gentype) a, inout $(gentype) b) {
    $(gentype) temp = a;
    a = b;
    b = temp;
}

/** Compute x^6 */
$(gentype) pow6($(gentype) x) {
    // x^3
    x *= x * x;

    // x^6;
    return x * x;
}

/** Compute x^8 */
$(gentype) pow8($(gentype) x) {
    // x^2
    x *= x;

    // x^4
    x *= x;

    // x^8;
    return x * x;
}

/** Compute x^8 */
$(gentype) pow16($(gentype) x) {
    // x^2
    x *= x;

    // x^4
    x *= x;

    // x^8;
    x *= x;

    // x^16
    return x * x;
}

/** Compute v^64 */
$(gentype) pow64($(gentype) v) {
    // v^2
    v *= v;

    // v^4
    v *= v;

    // v^64
    return v * v * v;
}
#endforeach


#foreach (gentype) in (float), (vec2), (vec3), (vec4)
    /** 
    Perlin's smootherstep that is analogous to smoothstep() but 
     has zero 1st and 2nd order derivatives at t=0 and t=1, from
     Texturing and Modeling, Third Edition: A Procedural Approach
     (see http://en.wikipedia.org/wiki/Smoothstep) */
    $(gentype) smootherstep($(gentype) edge0, $(gentype) edge1, $(gentype) x) {
        // Scale, and clamp x to 0..1 range
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        // Evaluate polynomial
        return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
    }

    /** Avoids the clamping and division for smootherstep(0, 1, x) when x is known to be on the range [0, 1] */
    $(gentype) unitSmootherstep($(gentype) x) {
        return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
    }

    /** Avoids the clamping and division for smoothstep(0, 1, x) when x is known to be on the range [0, 1] */
    $(gentype) unitSmoothstep($(gentype) x) {
        return x * x * (3.0 - 2.0 * x);
    }
#endforeach

#foreach (gentype) in (vec2), (vec3), (vec4)
    $(gentype) smootherstep(float edge0, float edge1, $(gentype) x) {
        // Scale, and clamp x to 0..1 range
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        // Evaluate polynomial
        return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
    }
#endforeach


#foreach (gentype) in (int), (ivec2), (ivec3), (ivec4), (float), (vec2), (vec3), (vec4)
    $(gentype) square(in $(gentype) x) {
        return x * x;
    }
#endforeach

/** Computes x<sup>5</sup> */
float pow5(in float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

float max3(float a, float b, float c) {
    return max(a, max(b, c));
}

float max4(float a, float b, float c, float d) {
    return max(max(a, d), max(b, c));
}

float min3(float a, float b, float c) {
    return min(a, min(b, c));
}

float min4(float a, float b, float c, float d) {
    return min(min(a, d), min(b, c));
}

float maxComponent(vec2 a) {
    return max(a.x, a.y);
}

float maxComponent(vec3 a) {
    return max3(a.x, a.y, a.z);
}

float maxComponent(vec4 a) {
    return max4(a.x, a.y, a.z, a.w);
}


float minComponent(vec2 a) {
    return min(a.x, a.y);
}

float minComponent(vec3 a) {
    return min3(a.x, a.y, a.z);
}

float minComponent(vec4 a) {
    return min4(a.x, a.y, a.z, a.w);
}

float meanComponent(vec4 a) {
    return (a.r + a.g + a.b + a.a) * (1.0 / 4.0);
}

float meanComponent(vec3 a) {
    return (a.r + a.g + a.b) * (1.0 / 3.0);
}

float meanComponent(vec2 a) {
    return (a.r + a.g) * (1.0 / 2.0);
}

float meanComponent(float a) {
    return a;
}

/** Compute Schlick's approximation of the Fresnel coefficient.  The
    original goes to 1.0 at glancing angles, which makes objects
    appear to glow, so we scale it down to 0.9.

    We never put a Fresnel term on perfectly diffuse surfaces, so, if
    F0 is exactly black, then we keep the result black.
    */
// Must match GLG3D/UniversalBSDF.h
vec3 schlickFresnel(in vec3 F0, in float cos_i, float smoothness) {
    return (F0.r + F0.g + F0.b > 0.0) ? mix(F0, vec3(1.0), 0.9 * (0.02 + smoothness * 0.98) * pow5(1.0 - max(0.0, cos_i))) : F0;
}

/** Matches UniversalBSDF::unpackGlossyExponent*/
float unpackGlossyExponent(in float e) {
    return square((e * 255.0 - 1.0) * (1.0 / 253.0)) * 8192.0f + 0.5f;
}

float packGlossyExponent(in float x) {
    // Never let the exponent go above the max representable non-mirror value in a uint8
    return (clamp(sqrt((x - 0.5f) * (1.0f / 8192.0f)), 0.0, 1.0) * 253.0 + 1.0) * (1.0 / 255.0);
}


#if G3D_SHADER_STAGE == G3D_FRAGMENT_SHADER
/**
 Computes pow2(mipLevel), avoiding the expensive log2 call needed for the 
 actual MIP level.
 */
float computeSampleRate(vec2 texCoord, vec2 samplerSize) {
    texCoord *= samplerSize;
    return maxComponent(max(abs(dFdx(texCoord)), abs(dFdy(texCoord))));
}
#else
float computeSampleRate(vec2 texCoord, vec2 samplerSize) {
    return 0;
}
#endif


/** http://blog.selfshadow.com/2011/07/22/specular-showdown/ */ 
float computeToksvigGlossyExponent(float rawGlossyExponent, float rawNormalLength) {
    rawNormalLength = min(1.0, rawNormalLength);
    float ft = rawNormalLength / lerp(max(0.1, rawGlossyExponent), 1.0, rawNormalLength);
    float scale = (1.0 + ft * rawGlossyExponent) / (1.0 + rawGlossyExponent);
    return scale * rawGlossyExponent;
}


/** \cite http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl */
vec3 RGBtoHSV(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = (c.g < c.b) ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = (c.r < p.x) ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float eps = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + eps)), d / (q.x + eps), q.x);
}

/** \cite http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl */
vec3 HSVtoRGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), clamp(c.y, 0.0, 1.0));
}


/** 
 Generate the ith 2D Hammersley point out of N on the unit square [0, 1]^2
 \cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec2 hammersleySquare(uint i, const uint N) {
    vec2 P;
    P.x = float(i) * (1.0 / float(N));

    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    P.y = float(i) * 2.3283064365386963e-10; // / 0x100000000

    return P;
}


/** 
 Generate the ith 2D Hammersley point out of N on the cosine-weighted unit hemisphere
 with Z = up.
 \cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec3 hammersleyHemi(uint i, const uint N) {
    vec2 P = hammersleySquare(i, N);
    float phi = P.y * 2.0 * pi;
    float cosTheta = 1.0 - P.x;
    float sinTheta = sqrt(1.0 - square(cosTheta));
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

/** 
 Generate the ith 2D Hammersley point out of N on the cosine-weighted unit hemisphere
 with Z = up.
 \cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec3 hammersleyCosHemi(uint i, const uint N) {
    vec3 P = hammersleyHemi(i, N);
    P.z = sqrt(P.z);
    return P;
}
mat4x4 identity4x4() {
    return mat4x4(1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  0,0,0,1);
}

/** Constructs a 4x4 translation matrix, assuming T * v multiplication. */
mat4x4 translate4x4(Vector3 t) {
    return mat4x4(1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  t,1);
}

/** Constructs a 4x4 Y->Z rotation matrix, assuming R * v multiplication. a is in radians.*/
mat4x4 pitch4x4(float a) {
    return mat4x4(1, 0, 0, 0,
                  0,cos(a),-sin(a), 0,
                  0,sin(a), cos(a), 0,
                  0, 0, 0, 1);
}

/** Constructs a 4x4 X->Y rotation matrix, assuming R * v multiplication. a is in radians.*/
mat4x4 roll4x4(float a) {
    return mat4x4(cos(a),-sin(a),0,0,
                  sin(a), cos(a),0,0,
                  0,0,1,0,
                  0,0,0,1);
}

/** Constructs a 4x4 Z->X rotation matrix, assuming R * v multiplication. a is in radians.*/
mat4x4 yaw4x4(float a) {
    return mat4x4(cos(a),0,sin(a),0,
                  0,1,0,0,
                  -sin(a),0,cos(a),0,
                  0,0,0,1);
}

#if G3D_SHADER_STAGE == G3D_FRAGMENT_SHADER
/** Computes the MIP-map level for a texture coordinate.
    \cite The OpenGL Graphics System: A Specification 4.2
    chapter 3.9.11, equation 3.21 */
float mipMapLevel(in vec2 texture_coordinate, in vec2 textureSize) {
    vec2  dx        = dFdx(texture_coordinate);
    vec2  dy        = dFdy(texture_coordinate);
    float delta_max_sqr = max(dot(dx, dx), dot(dy, dy));
  
    return 0.5 * log2(delta_max_sqr * square(maxComponent(textureSize))); // == log2(sqrt(delta_max_sqr));
}
#endif


/**
 A bicubic magnification filter, primarily useful for magnifying LOD 0
 without bilinear magnification artifacts. This just adjusts the rate
 at which we move between taps, not the number of taps.
 \cite https://www.shadertoy.com/view/4df3Dn
*/
vec4 textureLodSmooth(sampler2D tex, vec2 uv, int lod) {
    vec2 res = textureSize(tex, lod);
	uv = uv*res + 0.5;
	vec2 iuv = floor( uv );
	vec2 fuv = fract( uv );
	uv = iuv + fuv*fuv*(3.0-2.0*fuv); // fuv*fuv*fuv*(fuv*(fuv*6.0-15.0)+10.0);;
	uv = (uv - 0.5) / res;
	return textureLod(tex, uv, lod);
}


/**
 Implementation of Sigg and Hadwiger's Fast Third-Order Texture Filtering 

 http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter20.html
 \cite http://vec3.ca/bicubic-filtering-in-fewer-taps/
 \cite http://pastebin.com/raw.php?i=YLLSBRFq
 */
vec4 textureLodBicubic(sampler2D tex, vec2 uv, int lod) {
    //--------------------------------------------------------------------------------------
    // Calculate the center of the texel to avoid any filtering

    float2 textureDimensions    = textureSize(tex, lod);
    float2 invTextureDimensions = 1.0 / textureDimensions;

    uv *= textureDimensions;

    float2 texelCenter   = floor(uv - 0.5) + 0.5;
    float2 fracOffset    = uv - texelCenter;
    float2 fracOffset_x2 = fracOffset * fracOffset;
    float2 fracOffset_x3 = fracOffset * fracOffset_x2;

    //--------------------------------------------------------------------------------------
    // Calculate the filter weights (B-Spline Weighting Function)

    float2 weight0 = fracOffset_x2 - 0.5 * (fracOffset_x3 + fracOffset);
    float2 weight1 = 1.5 * fracOffset_x3 - 2.5 * fracOffset_x2 + 1.0;
    float2 weight3 = 0.5 * ( fracOffset_x3 - fracOffset_x2 );
    float2 weight2 = 1.0 - weight0 - weight1 - weight3;

    //--------------------------------------------------------------------------------------
    // Calculate the texture coordinates

    float2 scalingFactor0 = weight0 + weight1;
    float2 scalingFactor1 = weight2 + weight3;

    float2 f0 = weight1 / (weight0 + weight1);
    float2 f1 = weight3 / (weight2 + weight3);

    float2 texCoord0 = texelCenter - 1.0 + f0;
    float2 texCoord1 = texelCenter + 1.0 + f1;

    texCoord0 *= invTextureDimensions;
    texCoord1 *= invTextureDimensions;

    //--------------------------------------------------------------------------------------
    // Sample the texture

    return textureLod(tex, float2(texCoord0.x, texCoord0.y), lod) * scalingFactor0.x * scalingFactor0.y +
           textureLod(tex, float2(texCoord1.x, texCoord0.y), lod) * scalingFactor1.x * scalingFactor0.y +
           textureLod(tex, float2(texCoord0.x, texCoord1.y), lod) * scalingFactor0.x * scalingFactor1.y +
           textureLod(tex, float2(texCoord1.x, texCoord1.y), lod) * scalingFactor1.x * scalingFactor1.y;
}

/* L1 norm */
float norm1(vec3 v) {
    return abs(v.x) + abs(v.y) + abs(v.z);
}


float norm2(vec3 v) {
    return length(v);
}


/** L^4 norm */
float norm4(vec2 v) {
    v *= v;
    v *= v;
    return pow(v.x + v.y, 0.25);
}

/** L^8 norm */
float norm8(vec2 v) {
    v *= v;
    v *= v;
    v *= v;
    return pow(v.x + v.y, 0.125);
}

float normInf(vec3 v) {
    return maxComponent(abs(v));
}

float infIfNegative(float x) { return (x >= 0.0) ? x : inf; }


struct Ray {
    Point3      origin;
    /** Unit direction of propagation */
    Vector3     direction;
};


/** */
uint extractEvenBits(uint x) {
	x = x & 0x55555555u;
	x = (x | (x >> 1)) & 0x33333333u;
	x = (x | (x >> 2)) & 0x0F0F0F0Fu;
	x = (x | (x >> 4)) & 0x00FF00FFu;
	x = (x | (x >> 8)) & 0x0000FFFFu;

	return x;
}


/** Expands a 10-bit integer into 30 bits
by inserting 2 zeros after each bit. */
uint expandBits(uint v) {
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;

	return v;
}


/** Calculates a 30-bit Morton code for the
given 3D point located within the unit cube [0,1]. */
uint interleaveBits(uvec3 inputval) {
	uint xx = expandBits(inputval.x);
	uint yy = expandBits(inputval.y);
	uint zz = expandBits(inputval.z);

	return xx + yy * uint(2) + zz * uint(4);
}

#endif
