#ifndef AmbientOcclusion_packBilateralKey_glsl
#define AmbientOcclusion_packBilateralKey_glsl

float signNotZero(in float k) {
    return k >= 0.0 ? 1.0 : -1.0;
}

vec2 signNotZero(in vec2 v) {
    return vec2(signNotZero(v.x), signNotZero(v.y));
}

vec2 octEncode(in vec3 v) {
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0) {
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    }
    return result;
}

vec3 octDecode(vec2 o) {
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0) {
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    }
    return normalize(v);
}

vec4 packBilateralKey(in float csZ, in vec3 normal, in float nearZ, in float farZ) {
    vec4 result;

    float normalizedZ = clamp(-(csZ - nearZ) / (nearZ - farZ), 0.0, 1.0);
    float temp = floor(normalizedZ * 255.0);
    result.x = temp / 255.0;
    result.y = (normalizedZ * 255.0) - temp;

    vec2 octNormal = octEncode(normal);
    // Pack to 0-1
    result.zw = octNormal * 0.5 + 0.5;


    return result;
}

void unpackBilateralKey(vec4 key, in float nearZ, in float farZ, out float zKey, out vec3 normal) {
    zKey = key.x + key.y * (1.0 / 256.0);
    normal = octDecode(key.zw * 2.0 - 1.0);
}

#endif