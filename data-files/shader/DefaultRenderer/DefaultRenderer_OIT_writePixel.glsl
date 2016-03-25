/** \file DefaultRenderer_OIT_writePixel.glsl 

  This shader corresponds to listing 1 of: 

   McGuire and Mara, A Phenomenological Scattering Model for Order-Independent Transparency, 
   Proceedings of the ACM Symposium on Interactive 3D Graphics and Games (I3D), Feburary 28, 2016

  http://graphics.cs.williams.edu/papers/TransparencyI3D16/

  It is designed to be used with the G3D Innovation Engine SVN main branch from date 2016-02-28
  available at http://g3d.cs.williams.edu.
 */


/** (Ar, Ag, Ab, Aa) */
layout(location = 0) out float4 _accum;

/** (Br, Bg, Bb, D^2) */
layout(location = 1) out float4 _modulate;

/** (deltax, deltay) */
layout(location = 2) out float2 _refraction;

uniform Texture2D _depthTexture;
uniform vec3      _clipInfo;

/** Result is in texture coordinates */
Vector2 computeRefractionOffset
   (float           backgroundZ,
    Vector3         csN,
    Point3          csPosition,
    float           etaRatio) {

    /* Incoming ray direction from eye, pointing away from csPosition */
    Vector3 csw_i = normalize(-csPosition);

    /* Refracted ray direction, pointing away from wsPos */
    Vector3 csw_o = refract(-csw_i, csN, etaRatio);

    bool totalInternalRefraction = (dot(csw_o, csw_o) < 0.01);
    if (totalInternalRefraction) {
        /* No transmitted light */
        return Vector2(0.0);
    } else {
        /* Take to the reference frame of the background (i.e., offset) */
        Vector3 d = csw_o;

        /* Take to the reference frame of the background, where it is the plane z = 0 */
        Point3 P = csPosition;
        P.z -= backgroundZ;

        /* Find the xy intersection with the plane z = 0 */
        Point2 hit = (P.xy - d.xy * P.z / d.z);

        /* Hit is now scaled in meters from the center of the screen; adjust scale and offset based
          on the actual size of the background */
        Point2 backCoord = (hit / backSizeMeters) + Vector2(0.5);

        if (! g3d_InvertY) {
            backCoord.y = 1.0 - backCoord.y;
        }

        Point2 startCoord = (csPosition.xy / backSizeMeters) + Vector2(0.5);

        return backCoord - startCoord;
    }
}

/* Pasted in from reconstructFromDepth.glsl because we're defining a macro and can't have includes */
float _reconstructCSZ(float d, vec3 clipInfo) {
    return clipInfo[0] / (clipInfo[1] * d + clipInfo[2]);
}

/** Not used in the final version */
float randomVal(vec3 p) {
    return frac(sin(p.x * 1e2 + p.y) * 1e5 + sin(p.y * 1e3 + p.z * 1e2) * 1e3);
}


/** Instead of writing directly to the framebuffer in a forward or deferred shader, the 
    G3D engine invokes this writePixel() function. This allows mixing different shading 
    models with different OIT models. */
void writePixel
   (Radiance3   premultipliedReflectionAndEmission, 
    float       coverage, 
    Color3      transmissionCoefficient, 
    float       collimation, 
    float       etaRatio, 
    Point3      csPosition, 
    Vector3     csNormal) {

    /* Perform this operation before modifying the coverage to account for transmission */
    _modulate.rgb = coverage * (vec3(1.0) - transmissionCoefficient);
    
    /* Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
    transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
    reflection. See:

    McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
    http://graphics.cs.williams.edu/papers/CSSM/

    for a full explanation and derivation.*/
    coverage *= 1.0 - (transmissionCoefficient.r + transmissionCoefficient.g + transmissionCoefficient.b) * (1.0 / 3.0);

    /* Intermediate terms to be cubed */
    float tmp = 1.0 - gl_FragCoord.z * 0.99;

    /* tmp *= tmp * tmp; */ /*Enable if you want more discrimination between lots of transparent surfaces, e.g., for CAD. Risks underflow on individual surfaces in the general case. */
    
    /* If a lot of the scene is close to the far plane (e.g., you know that you have a really close far plane for CAD 
       or a scene with distant mountains and glass castles in fog), then gl_FragCoord.z does not
       provide enough discrimination. You can add this term to compensate:
   
       tmp /= sqrt(abs(csZ));

       Only used for the car engine scene in the paper, where the far plane is at 2m. 
    */

    /*
      If you're running out of compositing range and overflowing to inf, multiply the upper bound (3e2) by a small
      constant. Note that R11G11B10F has no "inf" and maps all floating point specials to NaN, so you won't actuall
      see inf in the frame buffer.
      */

    /* Weight function tuned for the general case. Used for all images in the paper */
    float w = clamp(coverage * tmp * tmp * tmp * 1e3, 1e-2, 3e2 * 0.1); 

    /* Alternative weighting that gives better discrimination for lots of partial-coverage edges (e.g., foliage) */
    /* float w = clamp(tmp * 1e3, 1e-2, 3e2 * 0.1); */

    _accum = vec4(premultipliedReflectionAndEmission, coverage) * w;
    float backgroundZ = csPosition.z - 4;

    Vector2 refractionOffset = (etaRatio == 1.0) ? Vector2(0) : computeRefractionOffset(backgroundZ, csNormal, csPosition, etaRatio);

    float trueBackgroundCSZ = _reconstructCSZ(texelFetch(_depthTexture.sampler, ivec2(gl_FragCoord.xy), 0).r, _clipInfo);

    /* Diffusion scaling constant. Adjust based on the precision of the _modulate.a texture channel. */
    const float k_0 = 8.0;

    /** Focus rate. Increase to make objects come into focus behind a frosted glass surface more quickly,
        decrease to defocus them quickly. */
    const float k_1 = 0.1;

    /* Compute standard deviation */
    _modulate.a = k_0 * coverage * (1.0 - collimation) * (1.0 - k_1 / (k_1 + csPosition.z - trueBackgroundCSZ)) / abs(csPosition.z);

    /* Convert to variance */
    _modulate.a *= _modulate.a;

    /* Prevent underflow in 8-bit color channels: */
    if (_modulate.a > 0) {
        _modulate.a = max(_modulate.a, 1 / 256.0);
    }

    /* We tried this stochastic rounding scheme to avoid banding for very low coverage surfaces, but
       it doesn't look good:

    if ((_modulate.a > 0) && (_modulate.a < 254.5 / 255.0)) {
        _modulate.a = clamp(_modulate.a + randomVal(vec3(gl_FragCoord.xy * 100.0, 0)) * (1.0 / 255.0), 0.0, 1.0);
    }
    */

    /* Encode into snorm. Maximum offset is 1 / 8 of the screen */
    _refraction = refractionOffset * coverage * 8.0;
}
