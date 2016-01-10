/**
  \file GLG3D/AmbientOcclusionSettings.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2012-08-02
  \edited  2014-05-12
*/

#ifndef GLG3D_AmbientOcclusionSettings_h
#define GLG3D_AmbientOcclusionSettings_h

#include "G3D/platform.h"
#include "GLG3D/TemporalFilter.h"
#include "GLG3D/GBuffer.h"
namespace G3D {

class Any;

/** For use with AmbientOcclusion.

 This is not an inner class of AmbientOcclusion to avoid creating a dependency
 between Lighting and AmbientOcclusion.
*/
class AmbientOcclusionSettings {
public:
    /** Radius in world-space units */
    float                       radius;

    /** 
        Increase if you have low-poly curves that are getting too
        much self-shadowing in shallow corners.  Decrease if you see white 
        lines in sharp corners.

        Bias addresses two problems.  The first is that a
        tessellated concave surface should geometrically exhibit
        stronger occlusion near edges and vertices, but this is
        often undesirable if the surface is supposed to appear as a
        smooth curve.  Increasing bias increases the maximum
        concavity that can occur before AO begins.

        The second is that due to limited precision in the depth
        buffer, a surface could appear to occlude itself.
    */
    float                       bias;

    /** Darkness multiplier */
    float                       intensity;

    /** Total number of direct samples to take at each pixel.  Must be greater
        than 2.  The default is 19.  Higher values increase image quality. */
    int                         numSamples;


    /** Increase to make depth edges crisper. Decrease to reduce flicker. Default is 1.0. */
    float                       edgeSharpness;

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
    
    /** 
        Increases sharpness at edges. Has no effect if not using a precomputed normal buffer, or the blur radius is zero.
        Use normals in the blur passes in addition to depth to use as weights. Default is true. 
    */
    bool                        useNormalsInBlur;

    /** If true, ensure that the "bilateral" weights are monotonically decreasing moving away from the current pixel. Default is true. */
    bool                        monotonicallyDecreasingBilateralWeights;

    /** 
        Increases quality of AO in scenes around overlapping objects.
        Increases runtime cost by about 1.5x.

        If true, requires the depth peel buffer to be non-null.
    */ 
    bool                        useDepthPeelBuffer;
    
    /**
        Avoids white "halos" around objects, enables using normals in the blur
        Has negligble cost on most GPUs. 

         If true, requires the normal buffer to be non-null.
    */
    bool                        useNormalBuffer;

    /**
        A hint for how far (in meters) to buffer the depth peel for the ao.
        
        Since AmbientOcclusion does not perform the depth peel, this is commonly 
        read by the application, which in turn performs the depth peel and passes
        the resulting buffer into AmbientOcclusion.
    */
    float                       depthPeelSeparationHint;

    /** 
        What encoding scheme to pack CSZ values into when computing 
        the heirarchy for raw AO sampling
    */
    G3D_DECLARE_ENUM_CLASS(ZStorage, HALF, FLOAT);
    ZStorage                    zStorage;

    bool                        highQualityBlur;

    /**
        Perform an extra packing step to minimize bandwidth on the blur passes.
        If normals are used in the blur, this will pack CS_Z+normals into RGBA8,
        with RG encoding a 16bit normalized CS_Z value, and BA encoding normals in oct16.

        If normals are not used in the blur, simply packs CS_Z values.
    */
    bool                        packBlurKeys;

    /** Temporal filtering occurs before spatial filtering. */
    TemporalFilter::Settings    temporalFilterSettings;

    /** Vary sample locations with respect to time. This increases
        temporal jitter, but combined with temporal filtering, temporal artifacts
        can be reduced and image quality increased */
    bool                        temporallyVarySamples;

    bool                        enabled;

    AmbientOcclusionSettings();

    AmbientOcclusionSettings(const Any& a);

    Any toAny() const;

    /** The number of spiral turns to use when generating the per-pixel taps. 
        If numSamples < 100, this is the calculated optimal value for minimizing discrepancy. 
        Otherwise its just a large prime that will at least not cause the samples to degenerate into perfect lines */
    int numSpiralTurns() const;

    /** 
        Ensures the GBuffer specification has all the fields needed to render this effect 
        \sa GApp::extendGBufferSpecification
    */
    void extendGBufferSpecification(GBuffer::Specification& spec) const;

};     

} // namespace G3D

#endif
