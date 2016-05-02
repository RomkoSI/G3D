#ifndef TemporalFilter_h
#define TemporalFilter_h

#include "GLG3D/Args.h"

namespace G3D {

class Framebuffer;
class Texture;
class Camera;
class CoordinateFrame;
class GuiPane;

/** A simple helper class for temporally filtering a screen-space buffer filled with world-space
 * data. This is designed to be simple to use, not to maximize speed.
 * Directly use functions from temporalFiler.glsl in a full-screen pass instead of using this class if performance 
 * or memory is a concern.
 */
class TemporalFilter {
public:
    class Settings {
    public:
        /** Between 0 and 1. If 0, no filtering occurs, if 1, 
            use the previous value for texels that pass reverse-reprojection.

            Default is 0.9.
            */
        float hysteresis;

        /** World-space distance between reverse-reprojection expected and actual positions
            beyond which we start discounting the weight of the previous value.

            Default is 0.05 meters.
            */
        float falloffStartDistance;

        /** World-space distance between reverse-reprojection expected and actual positions
            beyond which we don't use the previous value at all. We smoothstep a discount value between
            falloffStartDistance and falloffEndDistance.
            
            Default is 0.07 meters.
            */
        float falloffEndDistance;

        void setArgs(Args& args) const;

        Settings(): hysteresis(0.9f), falloffStartDistance(0.05f), falloffEndDistance(0.07f) {}
        Settings(const Any& a);
        Any toAny() const;
        void makeGui(GuiPane* parent);
    };
protected:

    /** The last result of using this filter. If null, or the wrong size/format, 
        we will construct a new texture and copy the current value into it on apply() */
    shared_ptr<Texture> m_previousTexture;

    /** A copy of the previous depth buffer from the last run of this filter */
    shared_ptr<Texture> m_previousDepthBuffer;

    shared_ptr<Framebuffer> m_resultFramebuffer;

public:
    /** Do not modify the return value; it is used to write into for the next iteration.
    
        numFilterComponents must be 1, 2, 3, or 4. The first numFilterComponents components will be blended according to the temporal filter,
        the rest will simply be copied from the current value buffer. This is useful if the signal you are filtering is only in the first N components.

        If alpha = 0.0f, this directly returns unfilteredValue.
    */
    shared_ptr<Texture> apply(RenderDevice* rd, const shared_ptr<Camera>& camera, const shared_ptr<Texture>& unfilteredValue, 
        const shared_ptr<Texture>& depth, const shared_ptr<Texture>& ssVelocity, const Vector2& guardBandSize, int numFilterComponents = 4, const Settings& settings = Settings());

    shared_ptr<Texture> apply(RenderDevice* rd, 
        const Vector3&                  clipConstant,
        const Vector4&                  projConstant,
        const CoordinateFrame&          currentCameraFrame,
        const CoordinateFrame&          prevCameraFrame, 
        const shared_ptr<Texture>&      unfilteredValue, 
        const shared_ptr<Texture>&      depth, 
        const shared_ptr<Texture>&      ssVelocity, 
        const Vector2&                  guardBandSize,
        int                             numFilterComponents = 4, 
        const Settings&                 settings = Settings());
    
};

}
#endif
