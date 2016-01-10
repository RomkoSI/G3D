#include "GLG3D/MotionBlurSettings.h"
#include "G3D/Any.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

MotionBlurSettings::MotionBlurSettings() : 
    m_enabled(false), 
    m_exposureFraction(0.75f), 
    m_cameraMotionInfluence(0.5f),
    m_maxBlurDiameterFraction(0.10f),
    m_numSamples(27) {}


MotionBlurSettings::MotionBlurSettings(const Any& any) {
    *this = MotionBlurSettings();

    AnyTableReader reader("MotionBlurSettings", any);

    reader.getIfPresent("enabled",                  m_enabled);
    reader.getIfPresent("exposureFraction",         m_exposureFraction);
    reader.getIfPresent("cameraMotionInfluence",    m_cameraMotionInfluence);    
    reader.getIfPresent("maxBlurDiameterFraction",  m_maxBlurDiameterFraction);
    reader.getIfPresent("numSamples",               m_numSamples);
    reader.verifyDone();

    m_cameraMotionInfluence = clamp(m_cameraMotionInfluence, 0.0f, 2.0f);
    m_exposureFraction      = clamp(m_exposureFraction, 0.0f, 2.0f);
}


Any MotionBlurSettings::toAny() const {

    Any any(Any::TABLE, "MotionBlurSettings");

    any["enabled"]                  = m_enabled;
    any["exposureFraction"]         = m_exposureFraction;
    any["cameraMotionInfluence"]    = m_cameraMotionInfluence;
    any["maxBlurDiameterFraction"]  = m_maxBlurDiameterFraction;
    any["numSamples"]               = m_numSamples;

    return any;
}

void MotionBlurSettings::extendGBufferSpecification(GBuffer::Specification& spec) const {
    if (m_enabled) {
        if (spec.encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION].format == NULL) {
            // We do not scale and bias to the entire range (256 * x - 128) because we need
            // to be able to represent fractional-pixel offsets. Note that scaled and biased
            // UNORM cannot exactly represent zero, so we scale by almost but not quite 128.
            spec.encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION] =
                Texture::Encoding(GLCaps::supportsTexture(ImageFormat::RG8()) ?
                    ImageFormat::RG8() :
                    ImageFormat::RGBA8(),
                    FrameName::SCREEN, 16320.0f / 127.0f, -64.0f);
        }
    }
}

}
