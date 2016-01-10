#ifndef GLG3D_MotionBlurSettings_h
#define GLG3D_MotionBlurSettings_h

#include "G3D/platform.h"
#include "GLG3D/GBuffer.h"
namespace G3D {

class Any;

/** \see Camera, MotionBlur, DepthOfFieldSettings */
class MotionBlurSettings {
private:
    bool                        m_enabled;
    float                       m_exposureFraction;
    float                       m_cameraMotionInfluence;
    float                       m_maxBlurDiameterFraction;
    int                         m_numSamples;

public:

    MotionBlurSettings();

    MotionBlurSettings(const Any&);

    Any toAny() const;

    /** Fraction of the frame interval during which the shutter is
        open.  Default is 0.75 (75%).  Larger values create more motion blur, 
        smaller values create less blur.*/
    float exposureFraction() const {
        return m_exposureFraction;
    }

    void setExposureFraction(float f) {
        m_exposureFraction = f;
    }

    /**
       Impact of the camera's own motion on motion blur, usually on
       the range [0, 1].  Default is 0.5.  Large values give the
       result that would be seen through a video camera.  Small values
       suppress camera motion, matching what happens when the viewer's
       eye tracks fixed world-space point in real life despite head
       motion.  Note that objects fixed to the camera (like the view
       model) will blur backwards if this is not 1.0--try spinning
       with extended arms rapidly while focusing on a fixed point in
       the room--your own hand will blur out.
     */
    float cameraMotionInfluence() const {
        return m_cameraMotionInfluence;
    }

    void setCameraMotionInfluence(float c) {
        m_cameraMotionInfluence = c;
    }

    /** Objects are blurred over at most this distance as a fraction
        of the screen dimension along the dominant field of view
        axis. Default is 0.10 (10%).*/
    float maxBlurDiameterFraction() const {
        return m_maxBlurDiameterFraction;
    }

    void setMaxBlurDiameterFraction(float f) {
        m_maxBlurDiameterFraction = f;
    }


    bool enabled() const {
        return m_enabled;
    }

    void setEnabled(bool e) {
        m_enabled = e;
    }

    /** Number of samples taken per pixel when computing motion blur. Larger numbers are slower.  The 
        actual number of samples used by the current implementation must be odd, so this is always
        rounded to the odd number greater than or equal to it.

        Default = 11.
     */
    int numSamples() const {
        return m_numSamples;
    }

    void setNumSamples(int n) {
        m_numSamples = n;
    }

    void extendGBufferSpecification(GBuffer::Specification& spec) const;

};

}

#endif // G3D_MotionBlurSettings_h
