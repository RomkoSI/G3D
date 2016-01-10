#ifndef GLG3D_MotionBlur_h
#define GLG3D_MotionBlur_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/GBuffer.h"

namespace G3D {

class Texture;
class RenderDevice;
class Shader;
class MotionBlurSettings;
class Camera;

/** 
   Screen-space post-processed motion blur. Based on:
   		
   A Reconstruction Filter for Plausible Motion Blur.
   McGuire, Hennessy, Bukowski, and Osman, I3D 2012
   http://graphics.cs.williams.edu/papers/MotionBlurI3D12/
 */
class MotionBlur : public ReferenceCountedObject {
protected:

    /** The source color is copied into this if needed. Saved between invocations to avoid reallocating the texture.*/
    shared_ptr<Texture>         m_cachedSrc;

    bool                        m_debugShowTiles;
	
    /** Size ceil(w / maxBlurRadius) x ceil(h / maxBlurRadius). RG = max velocity in tile, B = min speed in tile */
    shared_ptr<Framebuffer>     m_tileMinMaxFramebuffer;

    /** Size h x ceil(w / maxBlurRadius). RG = max velocity in tile, B = min speed in tile */
    shared_ptr<Framebuffer>     m_tileMinMaxTempFramebuffer;
	
    /** Size ceil(w / maxBlurRadius) x ceil(h / maxBlurRadius). 
        RG = max velocity in neighborhood, B = min speed in neighborhood */
    shared_ptr<Framebuffer>     m_neighborMinMaxFramebuffer;

    /** 32x32 buffer of RG values on [0, 1) */
    shared_ptr<Texture>         m_randomBuffer;

    /** Compute m_tileMax from sharpVelocity */
    void computeTileMinMax
    (RenderDevice*                     rd, 
     const shared_ptr<Texture>&        velocity,        
     int                               maxBlurRadiusPixels,
     const Vector2int16                trimBandThickness);

    /** Compute m_neighborMax from m_tileMax */
    void computeNeighborMinMax
    (RenderDevice*                     rd,
     const shared_ptr<Texture>&        tileMax);
    
    /** Called from apply() to compute the blurry image to the current
        frame buffer by gathering. */
    virtual void gatherBlur
    (RenderDevice*                     rd, 
     const shared_ptr<Texture>&        color,
     const shared_ptr<Texture>&        neighborMax,
     const shared_ptr<Texture>&        velocity,
     const shared_ptr<Texture>&        depth,
     int                               numSamplesOdd,
     int                               maxBlurRadiusPixels,
     float                             exposureTimeFraction,
     Vector2int16                      trimBandThickness);

    MotionBlur();

    /** Allocates tileMax and neighborMax as needed */
    void updateBuffers
     (const shared_ptr<Texture>&       velocityTexture, 
      int                              maxBlurRadiusPixels,
      Vector2int16                     trimBandThickness);
    
    void makeRandomBuffer();

    /** Debug visualization of the motion blur tiles and directions. Called from apply() */
    void debugDrawTiles
      (RenderDevice*                   rd,
       const shared_ptr<Texture>&      neighborMax,
       int                             maxBlurRadiusPixels);
	
    /** Returns n if it is odd, otherwise returns n + 1 */
    inline static int nextOdd(int n) {
        return n + 1 - (n & 1);
    }

public:

    static shared_ptr<MotionBlur> create() {
        return shared_ptr<MotionBlur>(new MotionBlur());
    }
    
    /**
        \param trimBandThickness Input texture coordinates are clamped to
        a region inset on all sides by this amount.  Set this to non-zero if 
        the input color buffer is larger than the desired output region but does
        not have useful data around the border.  Typically m_settings.depthGuardBandThickness - m_settings.colorGuardBandThickness.

    */        
    virtual void apply
        (RenderDevice*                     rd, 
         const shared_ptr<Texture>&        color, 
         const shared_ptr<Texture>&        velocity, 
         const shared_ptr<Texture>&        depth,
         const shared_ptr<Camera>&         camera,
         Vector2int16                      trimBandThickness);

    /** Toggle visualization showing the tile boundaries, which are 
        set by maxBlurRadiusPixels.*/
    void setDebugShowTiles(bool b) {
        m_debugShowTiles = b;
    }

    bool debugShowTiles() const {
        return m_debugShowTiles;
    }
};

} // namespace

#endif // GLG3D_MotionBlur_h
