/**
  \file DeepGBufferRadiositySettings.cpp

  \maintainer Michael Mara, http://illuminationcodified.com

  \created 2014-05-13
  \edited  2014-05-13
*/

#include "DeepGBufferRadiositySettings.h"
DeepGBufferRadiositySettings::DeepGBufferRadiositySettings() : 
    enabled(true),
    numSamples(19),
    radius(1.0f),
    numBounces(1),
    bias(0.002f),
    useMipMaps(true),
    blurStepSize(1),
    blurRadius(6),
    edgeSharpness(1.0f),
    monotonicallyDecreasingBilateralWeights(false),
    useDepthPeelBuffer(true),
    depthPeelSeparationHint(0.2f),
    computePeeledLayer(false),
    temporallyVarySamples(true),
    temporalFilterSettings(TemporalFilter::Settings()),
    propagationDamping(1.0f),
    useTapNormal(true),
    unsaturatedBoost(1.0f),
    saturatedBoost(1.0f),
    useOct16(false),
    minMipLevel(0),
    useHalfPrecisionColors(false),
    computeGuardBandFraction(1.0f) { 
}

    
DeepGBufferRadiositySettings::DeepGBufferRadiositySettings(const Any& a) {
    *this = DeepGBufferRadiositySettings();

    a.verifyName("DeepGBufferRadiositySettings");

    AnyTableReader r(a);
    r.getIfPresent("enabled", enabled);
    r.getIfPresent("radius", radius);
    r.getIfPresent("bias", bias);
    r.getIfPresent("numSamples", numSamples);
    r.getIfPresent("edgeSharpness", edgeSharpness);
    r.getIfPresent("blurStepSize", blurStepSize);
    r.getIfPresent("blurRadius", blurRadius);
    r.getIfPresent("monotonicallyDecreasingBilateralWeights", monotonicallyDecreasingBilateralWeights);
    r.getIfPresent("useDepthPeelBuffer", useDepthPeelBuffer);
    r.getIfPresent("depthPeelSeparationHint", depthPeelSeparationHint);
    r.getIfPresent("temporalFilterSettings", temporalFilterSettings);
    r.getIfPresent("temporallyVarySamples", temporallyVarySamples);
    r.getIfPresent("computePeeledLayer", computePeeledLayer);
    r.getIfPresent("unsaturatedBoost", unsaturatedBoost);
    r.getIfPresent("saturatedBoost", saturatedBoost);
    r.getIfPresent("useMipMaps", useMipMaps);
    r.getIfPresent("numBounces", numBounces);
    r.getIfPresent("propagationDamping", propagationDamping);
    r.getIfPresent("useTapNormal", useTapNormal);
    r.getIfPresent("useOct16", useOct16);
    r.getIfPresent("minMipLevel", minMipLevel);
    r.getIfPresent("useHalfPrecisionColors", useHalfPrecisionColors);
    r.getIfPresent("computeGuardBandFraction", computeGuardBandFraction);
    r.verifyDone();
}


Any DeepGBufferRadiositySettings::toAny() const {
    Any a(Any::TABLE, "DeepGBufferRadiositySettings");
    a["enabled"] = enabled;
    a["radius"] = radius;
    a["bias"] = bias;
    a["numSamples"] = numSamples;
    a["edgeSharpness"] = edgeSharpness;
    a["blurStepSize"] = blurStepSize;
    a["blurRadius"] = blurRadius;
    a["monotonicallyDecreasingBilateralWeights"] = monotonicallyDecreasingBilateralWeights;
    a["useDepthPeelBuffer"] = useDepthPeelBuffer;
    a["depthPeelSeparationHint"] = depthPeelSeparationHint;
    a["temporalFilterSettings"] = temporalFilterSettings;
    a["temporallyVarySamples"] = temporallyVarySamples;
    a["computePeeledLayer"] = computePeeledLayer;
    a["unsaturatedBoost"] = unsaturatedBoost;
    a["saturatedBoost"] = saturatedBoost;
    a["useMipMaps"] = useMipMaps;
    a["numBounces"] = numBounces;
    a["useTapNormal"] = useTapNormal;
    a["propagationDamping"] = propagationDamping;
    a["useOct16"] =  useOct16;
    a["minMipLevel"] = minMipLevel;
    a["useHalfPrecisionColors"] = useHalfPrecisionColors;
    a["computeGuardBandFraction"] = computeGuardBandFraction;
    return a;
}

int DeepGBufferRadiositySettings::numSpiralTurns() const {
#define NUM_PRECOMPUTED 100

    static int minDiscrepancyArray[NUM_PRECOMPUTED] = {
    //  0   1   2   3   4   5   6   7   8   9
        1,  1,  1,  2,  3,  2,  5,  2,  3,  2,  // 0
        3,  3,  5,  5,  3,  4,  7,  5,  5,  7,  // 1
        9,  8,  5,  5,  7,  7,  7,  8,  5,  8,  // 2
       11, 12,  7, 10, 13,  8, 11,  8,  7, 14,  // 3
       11, 11, 13, 12, 13, 19, 17, 13, 11, 18,  // 4
       19, 11, 11, 14, 17, 21, 15, 16, 17, 18,  // 5
       13, 17, 11, 17, 19, 18, 25, 18, 19, 19,  // 6
       29, 21, 19, 27, 31, 29, 21, 18, 17, 29,  // 7
       31, 31, 23, 18, 25, 26, 25, 23, 19, 34,  // 8
       19, 27, 21, 25, 39, 29, 17, 21, 27, 29}; // 9

    if (numSamples < NUM_PRECOMPUTED) {
        return minDiscrepancyArray[numSamples];
    } else {
        return 5779; // Some large prime. Hope it does alright. It'll at least never degenerate into a perfect line until we have 5779 samples...
    }

#undef NUM_PRECOMPUTED


}
