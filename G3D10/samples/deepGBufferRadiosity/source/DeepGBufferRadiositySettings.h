/**
  \file GLG3D/AA.h

  \maintainer Michael Mara, http://illuminationcodified.com

  \created 2014-05-17
  \edited  2014-05-23
*/

#ifndef DeepGBufferRadiositySettings_h
#define DeepGBufferRadiositySettings_h

#include <G3D/G3DAll.h>

/** For use with DeepGBufferRadiosity */
class DeepGBufferRadiositySettings {
public:

    bool                        enabled;

    /** Total number of direct samples to take at each pixel.  Must be greater
        than 2.  The default is 19.  Higher values increase image quality. */
    int                         numSamples;

    /** Radius in world-space units */
    float                       radius;

    /** Number of iterations to do each frame */
    int                         numBounces;

    /** Bias addresses two quality parameters.  The first is that a
        tessellated concave surface should geometrically exhibit
        stronger occlusion near edges and vertices, but this is
        often undesirable if the surface is supposed to appear as a
        smooth curve.  Increasing bias increases the maximum
        concavity that can occur before DeepGBufferRadiosity begins.

        The second is that due to limited precision in the depth
        buffer, a surface could appear to occlude itself.
    */
    float                       bias;

    /** Set to true to drastically increase performance by increasing cache efficiency, at the expense of accuracy */
    bool                        useMipMaps;



    /** Default is to step in 2-pixel intervals. This constant can be increased while R decreases to improve
        performance at the expense of some dithering artifacts. 
    
        Morgan found that a scale of 3 left a 1-pixel checkerboard grid that was
        unobjectionable after shading was applied but eliminated most temporal incoherence
        from using small numbers of sample taps.

        Must be at least 1.
    */
    int                         blurStepSize;

    /** Filter radius in pixels. This will be multiplied by blurStepSize. Default is 4. */
    int                         blurRadius;
    
    /** Increase to make depth edges crisper. Decrease to reduce flicker. Default is 1.0. */
    float                       edgeSharpness;


    /** If true, ensure that the "bilateral" weights are monotonically decreasing moving away from the current pixel. Default is true. */
    bool                        monotonicallyDecreasingBilateralWeights;

    /** 
        Increases quality and stability of DeepGBufferRadiosity, with a performance hit.

        If true, requires the depth peel buffer to be non-null.
    */ 
    bool                        useDepthPeelBuffer;   

    /**
        A hint for how far (in meters) to buffer the depth peel for the ao.
        
        Since AmbientOcclusion does not perform the depth peel, this is commonly 
        read by the application, which in turn performs the depth peel and passes
        the resulting buffer into AmbientOcclusion.
    */
    float                       depthPeelSeparationHint;

    /** Compute DeepGBufferRadiosity for the second layer */
    bool                        computePeeledLayer;

        /** Vary sample locations with respect to time. This increases
        temporal jitter, but combined with temporal filtering, temporal artifacts
        can be reduced and image quality increased */
    bool                        temporallyVarySamples;

    /** Temporal filtering occurs before spatial filtering. */
    TemporalFilter::Settings    temporalFilterSettings;

    /** 
        How much to discount previous frame's bounces as input into the radiosity iteration.
        the range is [0.0, 1.0], with 1.0 having no information propograted between frames as input,
        and 0.0 being no damping whatsoever. Default is 1.0.
     */
    float                       propagationDamping;

    /** 
        If true, uses the normal at each sample in calculating the contribution. 
        Set to true to reduce light leaking and increase accuracy. Set to false to
        greatly reduce bandwidth and thus increase performance. Default is true.
    */
    bool                        useTapNormal;

    /** NPR term for increasing indirect illumination when it is an unsaturated value. 1.0 is physically-based. */
    float                       unsaturatedBoost;

    /** NPR term for increasing indirect illumination when it is an saturated value. 1.0 is physically-based. */
    float                       saturatedBoost;

    /** Use Oct16 to encode normals. This decreases bandwidth at the cost of extra computation. Default is false. */
    bool                        useOct16;

    /** The index of the largest mipLevel to use during gather. Increase to reduce bandwidth, decrease to improve quality. Default is 0. */
    int                         minMipLevel;

    /** If true, store input and output color buffers at half precision. This will just about halve bandwidth at the cost a accuracy. Default is false */
    bool                        useHalfPrecisionColors;

    /** The proportion of the guard band to caclulate radiosity for.
    
        Because temporal filtering and multiple scattering events both read the output of
        the indirect pass as the input to the next indirect pass, said pass must output
        closer to the full resolution of the input, rather than the final output size.

        1.0 gives full quality, 0.0 gives maximum performance. Default is 1.0. */
    float                       computeGuardBandFraction;

    DeepGBufferRadiositySettings();

    DeepGBufferRadiositySettings(const Any& a);

    Any toAny() const;

    /** The number of spiral turns to use when generating the per-pixel taps. 
        If numSamples < 100, this is the calculated optimal value for minimizing discrepancy (among integers). 
        Otherwise its just a large prime that will at least not cause the samples to degenerate into perfect lines */
    int numSpiralTurns() const;

};     

#endif