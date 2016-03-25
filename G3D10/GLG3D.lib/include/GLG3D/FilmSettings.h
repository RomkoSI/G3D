/**
 \file GLG3D/FilmSettings.h
 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2008-07-01
 \edited  2013-07-29
 */
#ifndef GLG3D_FilmSettings_h
#define GLG3D_FilmSettings_h

#include "G3D/platform.h"
#include "G3D/Spline.h"

namespace G3D {

class Any;
class GuiPane;

/** 
 \see G3D::Film, G3D::Camera
*/
class FilmSettings {
private:

    /** \brief Monitor gamma used in tone-mapping. Default is 2.0. */
    float                   m_gamma;

    /** \brief Scale factor applied to the pixel values during exposeAndRender(). */
    float                   m_sensitivity;

    /** \brief 0 = no bloom, 1 = blurred out image. */
    float                   m_bloomStrength;

    /** \brief Bloom filter kernel radius as a fraction 
     of the larger of image width/height.*/
    float                   m_bloomRadiusFraction;

    bool                    m_antialiasingEnabled;
    float                   m_antialiasingFilterRadius;
    bool                    m_antialiasingHighQuality;

    float                   m_vignetteTopStrength;
    float                   m_vignetteBottomStrength;
    float                   m_vignetteSizeFraction;

    int                     m_debugZoom;

    Spline<float>           m_toneCurve;


    /** If false, skips all processing and just blits to the output.

        Defaults to true.*/
    bool                    m_effectsEnabled;


public:

    FilmSettings();

    /**
       \code
       FilmSettings {
         gamma = <number>;
         sensitivity = <number>;
         bloomStrength = <number>;
         bloomRadiusFraction = <number>;
         antialiasingEnabled = <boolean>;
         antialiasingFilterRadius = <number>;
         antialiasingHighQuality = <boolean>;
         vignetteTopStrength = <number>;
         vignetteBottomStrength = <number>;
         vignetteSizeFraction = <number>;
         toneCurve = IDENTITY | CONTRAST | CELLULOID | SATURATE | BURNOUT | NEGATIVE | spline;
         debugZoom = <number>;
         effectsEnabled = <boolean>;
       }
       \endcode
     */
    FilmSettings(const Any& any);

    Any toAny() const;

    /** If > 1, enlarge pixels by this amount relative to the center of the screen for aid in debugging. 
        Enabling debugZoom may compromise performance. */
    int debugZoom() const {
        return m_debugZoom;
    }

    void setDebugZoom(int z) {
        alwaysAssertM(z > 0, "Zoom cannot be negative");
        m_debugZoom = z;
    }

    const Spline<float>& toneCurve() const {
        return m_toneCurve;
    }

    /** Amount of darkness due to vignetting for the top of the screen, on the range [0, 1] */
    float vignetteTopStrength() const {
        return m_vignetteTopStrength;
    }

    void setVignetteTopStrength(float s) {
        m_vignetteTopStrength = s;
    }
    
    void setVignetteBottomStrength(float s) {
        m_vignetteBottomStrength = s;
    }

    void setVignetteSizeFraction(float s) {
        m_vignetteSizeFraction = s;
    }

    /** Amount of darkness due to vignetting for the bottom of the screen, on the range [0, 1] */
    float vignetteBottomStrength() const {
        return m_vignetteBottomStrength;
    }

    /** Fraction of the diagonal radius of the screen covered by vignette, on the range [0, 1] */
    float vignetteSizeFraction() const {
        return m_vignetteSizeFraction;
    }

    /** \brief Monitor gamma used in tone-mapping. Default is 2.0. */
    float gamma() const {
        return m_gamma;
    }

    /** \brief Scale factor applied to the pixel values during exposeAndRender() */
     float sensitivity() const {
        return m_sensitivity;
    }

    /** \brief 0 = no bloom, 1 = blurred out image. */
    float bloomStrength() const {
        return m_bloomStrength;
    }

    /** \brief Bloom filter kernel radius as a fraction 
     of the larger of image width/height.*/
    float bloomRadiusFraction() const {
        return m_bloomRadiusFraction;
    }

    /** Enabled antialiasing post-processing. This reduces the
     artifacts from undersampling edges but may blur textures.
    By default, this is disabled.

    The current antialiasing algorithm is "FXAA 1" by Timothy Lottes.
    This may change in future implementations.
    */
    void setAntialiasingEnabled(bool e) {
        m_antialiasingEnabled = e;
    }

    bool antialiasingEnabled() const {
        return m_antialiasingEnabled;
    }

    float antialiasingFilterRadius() const {
        return m_antialiasingFilterRadius;
    }

    void setAntialiasingHighQuality(bool b) {
        m_antialiasingHighQuality = b;
    }

    bool antialiasingHighQuality() const {
        return m_antialiasingHighQuality;
    }

    /** 0 = FXAA within a pixel. Any larger value blurs taps that are separated from the center by f pixels*/
    void setAntialiasingFilterRadius(float f) {
        m_antialiasingFilterRadius = f;
    }

    void setGamma(float g) {
        m_gamma = g;
    }

    void setSensitivity(float s) {
        m_sensitivity = s;
    }

    void setBloomStrength(float s) {
        m_bloomStrength = s;
    }

    void setBloomRadiusFraction(float f) {
        m_bloomRadiusFraction = f;
    }

    /** If false, skips all processing and just blits to the output. */
    bool effectsEnabled() const {
        return m_effectsEnabled;
    }

    void setEffectsEnabled(bool b) {
        m_effectsEnabled = b;
    }

    /** Adds controls for these settings to the specified GuiPane. */
    void makeGui
    (class GuiPane*, 
     float maxSensitivity = 10.0f, 
     float sliderWidth    = 180, 
     float controlIndent  = 0.0f);

    void setIdentityToneCurve();
    void setCelluloidToneCurve();
    void setSaturateToneCurve();
    void setContrastToneCurve();
    void setBurnoutToneCurve();
    void setNegativeToneCurve();

    /**
    Ensures the GBuffer specification has all the fields needed to render this effect
    \sa GApp::extendGBufferSpecification
    */
    void extendGBufferSpecification(GBuffer::Specification& spec) const;
};

}

#endif
