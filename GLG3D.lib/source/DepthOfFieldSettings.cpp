#include "G3D/Any.h"
#include "GLG3D/DepthOfFieldSettings.h"

namespace G3D {

DepthOfFieldSettings::DepthOfFieldSettings() :
    m_enabled(true),
    m_lensRadius(0.01f), 
    m_focusPlaneZ(-10.0f), 
    m_model(DepthOfFieldModel::NONE),
    m_nearRadiusFraction(0.015f),
    m_nearBlurryZ(-0.25f),
    m_nearSharpZ(-1.0f),
    m_farSharpZ(-40.0f),
    m_farBlurryZ(-100.0f),
    m_farRadiusFraction(0.005) {
}
	
	
DepthOfFieldSettings::DepthOfFieldSettings(const Any& any) {
    *this = DepthOfFieldSettings();

    AnyTableReader reader("DepthOfFieldSettings", any);

    reader.getIfPresent("enabled", m_enabled);
    reader.getIfPresent("model", m_model);

    reader.getIfPresent("lensRadius", m_lensRadius);
    reader.getIfPresent("focusPlaneZ", m_focusPlaneZ);
    
    reader.getIfPresent("nearBlurRadiusFraction", m_nearRadiusFraction);
    reader.getIfPresent("nearBlurryPlaneZ",       m_nearBlurryZ);
    reader.getIfPresent("nearSharpPlaneZ",        m_nearSharpZ);
    reader.getIfPresent("farSharpPlaneZ",         m_farSharpZ);
    reader.getIfPresent("farBlurryPlaneZ",        m_farBlurryZ);
    reader.getIfPresent("farBlurRadiusFraction",  m_farRadiusFraction);

    reader.verifyDone();
}


Any DepthOfFieldSettings::toAny() const {
    Any any(Any::TABLE, "DepthOfFieldSettings");

    any["enabled"]               = m_enabled;
    any["model"]                 = m_model;
    any["focusPlaneZ"]           = m_focusPlaneZ;
    any["lensRadius"]            = m_lensRadius;
    any["nearBlurRadiusFraction"] = m_nearRadiusFraction;
    any["nearBlurryPlaneZ"]      = m_nearBlurryZ;
    any["nearSharpPlaneZ"]       = m_nearSharpZ;
    any["farSharpPlaneZ"]        = m_farSharpZ;
    any["farBlurryPlaneZ"]       = m_farBlurryZ;
    any["farBlurRadiusFraction"] = m_farRadiusFraction;

    return any;
}


float DepthOfFieldSettings::circleOfConfusionRadiusPixels(const float z, const float imagePlanePixelsPerMeter, const float screenPixelSize) const {
    switch ((DepthOfFieldModel::Value)m_model) {
    case DepthOfFieldModel::NONE:
        return 0.0f;

    case DepthOfFieldModel::PHYSICAL:
        {
            debugAssert(z < 0);
            
            //              Actual position z
            //                     |
            //      ()-----______  |
            //     (  )          --|---___| 
            //     (  )    ______--|---   |          
            //      ()-----        |      |
            //                           
            //      Lens                 Rays converge at m_focusPlaneZ
            //
            //                     |<---->|
            //                 | focusPlaneZ - z |
            //                            
            //       |<------------------>| 
            //          | focusPlaneZ |
            //
            // By similar triangles,
            //
            //   | focusPlaneZ - z |
            //  --------------------- lensRadius = radius at z
            //         -focusPlaneZ 
            
            // Circle of confusion at z, in meters
            const float rzmeters = (z - m_focusPlaneZ) * m_lensRadius / m_focusPlaneZ;
            
            // Project
            const float rimeters = rzmeters / -z;
            
            // Convert to pixels
            const float ripixels = rimeters * imagePlanePixelsPerMeter;
            
            return ripixels;
        } break;
        
    case DepthOfFieldModel::ARTIST:
        {
            // Radius relative to screen dimension
            float r = 0;

            if (z >= m_nearSharpZ) {
                // Near field
                
                // Blurriness fraction
                const float a = (min(z, m_nearBlurryZ) - m_nearSharpZ) / (m_nearSharpZ - m_nearBlurryZ);
                
                r = lerp(0.0f, m_nearRadiusFraction, a);
                
            } else if (z <= m_farSharpZ) {
                return 0.0f;
            } else {
                // Far field
                const float a = (m_farSharpZ - max(z, m_farBlurryZ)) / (m_farBlurryZ - m_farSharpZ);
                r = lerp(0.0f, m_farRadiusFraction, a);
            }

            debugAssertM(r >= 0 && r <= 1.0f, "Illegal circle of confusion radius");
            return r * screenPixelSize;
        }
        break;

    default:
        return 0.0f;
    }

}

void DepthOfFieldSettings::extendGBufferSpecification(GBuffer::Specification& spec) const {
    // DoF currently only requires depth + color for any setting
}

} // namespace G3D
