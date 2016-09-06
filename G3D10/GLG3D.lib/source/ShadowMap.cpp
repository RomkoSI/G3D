/**
  \file GLG3D.lib/source/ShadowMap.cpp

  \author Morgan McGuire, http://graphics.cs.williams.edu
  \created 2014-12-13
  \edited  2016-09-04

  Copyright 2000-2016, Morgan McGuire
  All rights reserved
 */
#include "GLG3D/ShadowMap.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Sphere.h"
#include "G3D/Projection.h"
#include "GLG3D/Draw.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Light.h"
#include "GLG3D/GaussianBlur.h"

namespace G3D {

ShadowMap::ShadowMap(const String& name) :
    m_name(name), 
    m_baseLayer(name + " m_baseLayer"),
    m_dynamicLayer(name + " m_dynamicLayer"),
    m_vsmSourceBaseLayer(name + "m_vsmSourceBaseLayer"),
    m_vsmSourceDynamicLayer(name + "m_vsmSourceDynamicLayer"),
    m_bias(1.5f * units::centimeters()),
    m_polygonOffset(0.0f),
    m_backfacePolygonOffset(0.0f),
    m_vsmSettings() {
}


shared_ptr<ShadowMap> ShadowMap::create(const String& name, Vector2int16 size, const VSMSettings& vsmSettings) {
    const shared_ptr<ShadowMap>& s(createShared<ShadowMap>(name));
    s->m_vsmSettings = vsmSettings;
    s->setSize(size);
    return s;
}


void ShadowMap::setShaderArgsRead(UniformTable& args, const String& prefix) const {
    if (useVarianceShadowMap()) {
        vsm()->setShaderArgs(args, prefix + "variance_", Sampler::video());
        args.setUniform(prefix + "variance_lightBleedReduction", m_vsmSettings.lightBleedReduction);
    }
    args.setUniform(prefix + "MVP",  unitLightMVP());
    args.setUniform(prefix + "bias", bias());
    depthTexture()->setShaderArgs(args, prefix, Sampler::shadow());
}


void ShadowMap::setSize(Vector2int16 desiredSize) {
    m_dynamicLayer.setSize(desiredSize);
    m_baseLayer.setSize(desiredSize);
    if (useVarianceShadowMap()) {
        const Vector2int16 vsmSize = m_vsmSettings.baseSize;
        m_vsmSourceBaseLayer.setSize(vsmSize);
        m_vsmSourceDynamicLayer.setSize(vsmSize);
        if (vsmSize.x == 0) {
            m_vsmRawFB.reset();
            return;
        }

        const bool generateMipMaps = false;
        const shared_ptr<Texture>& vsmRaw = Texture::createEmpty
            (name() + "_VSMRaw",
                vsmSize.x, vsmSize.y,
                ImageFormat::RG32F(),
                Texture::DIM_2D,
                generateMipMaps);
        
        const shared_ptr<Texture>& vsmHBlur = Texture::createEmpty
            (name() + "_VSMHBlur",
                vsmSize.x / m_vsmSettings.downsampleFactor, vsmSize.y,
                ImageFormat::RG32F(),
                Texture::DIM_2D,
                generateMipMaps);
        
        const shared_ptr<Texture>& vsmFinal = Texture::createEmpty
            (name() + "_VSMFinal",
                vsmSize.x / m_vsmSettings.downsampleFactor, vsmSize.y / m_vsmSettings.downsampleFactor,
                ImageFormat::RG32F(),
                Texture::DIM_2D,
                generateMipMaps);
        
        m_vsmRawFB = Framebuffer::create(name() + "_vsmRawFB");
        m_vsmRawFB->set(Framebuffer::COLOR0, vsmRaw);
        m_vsmHBlurFB = Framebuffer::create(name() + "_vsmHBlurFB");
        m_vsmHBlurFB->set(Framebuffer::COLOR0, vsmHBlur);
        m_vsmFinalFB = Framebuffer::create(name() + "_vsmFinalFB");
        m_vsmFinalFB->set(Framebuffer::COLOR0, vsmFinal);
    }
}


void ShadowMap::Layer::setSize(Vector2int16 desiredSize) {
    if (desiredSize.x == 0) {
        depthTexture.reset();
        framebuffer.reset();
        return;
    }

    alwaysAssertM(GLCaps::supports_GL_ARB_shadow() && (GLCaps::supports_GL_ARB_framebuffer_object() || GLCaps::supports_GL_EXT_framebuffer_object()), "Shadow Maps not supported on this platform");

    const bool generateMipMaps = false;
    depthTexture = Texture::createEmpty
        (name,
         desiredSize.x, desiredSize.y,
         GLCaps::supportsTexture(ImageFormat::DEPTH32F()) ?
             ImageFormat::DEPTH32F() :
             ImageFormat::DEPTH32(), 
         Texture::DIM_2D, 
         generateMipMaps);

    depthTexture->visualization = Texture::Visualization::depthBuffer();

    framebuffer = Framebuffer::create(name + " Frame Buffer");
    framebuffer->uniformTable.setMacro("SHADOW_MAP_FRAMEBUFFER", 1);
    framebuffer->set(Framebuffer::DEPTH, depthTexture);

    lastUpdateTime = 0;
}


bool ShadowMap::enabled() const {
    return notNull(m_dynamicLayer.depthTexture);
}


void ShadowMap::updateDepth
(RenderDevice*                      renderDevice,
 const CoordinateFrame&             lightCFrame, 
 const Matrix4&                     lightProjectionMatrix,
 const Array<shared_ptr<Surface> >& shadowCaster,
 CullFace                           cullFace,
 const Color3&                      transmissionWeight,
 const RenderPassType               passType) {
    debugAssertM((passType == RenderPassType::SHADOW_MAP) || 
        (passType == RenderPassType::OPAQUE_SHADOW_MAP) || 
        (passType == RenderPassType::TRANSPARENT_SHADOW_MAP),
        "ShadowMap::updateDepth must be called with appropriate RenderPassType");

    // Segment the shadow casters into base (static) and dynamic arrays, and 
    // take advantage of this iteration to discover the latest update time
    // so that the whole process can terminate early.
    static Array<shared_ptr<Surface>> baseArray, dynamicArray;
    baseArray.fastClear();
    dynamicArray.fastClear();

    // Find the most recently changed shadow caster. Start with a time later than 0
    // so that the first call to this method always forces rendering, even if there
    // are no casters. 
    RealTime lastBaseShadowCasterChangeTime    = 1.0f;
    RealTime lastDynamicShadowCasterChangeTime = 1.0f;
    size_t   baseShadowCasterEntityHash        = 0;
    size_t   dynamicShadowCasterEntityHash     = 0;

    for (int i = 0; i < shadowCaster.length(); ++i) {
        const shared_ptr<Surface>& c = shadowCaster[i];

        const bool needsToRenderThisPass = 
            (passType == RenderPassType::SHADOW_MAP) ||

            // This test also implicitly includes TRANSPARENT_SHADOW_MAP && c->requiresBlending()
            ((passType == RenderPassType::OPAQUE_SHADOW_MAP) != c->requiresBlending());

        if (needsToRenderThisPass) {
            if (c->canChange()) {
                dynamicArray.append(c);
                // Prevent the hash from being zero by adding 1. Don't XOR the entitys
                // because that would make two surfaces from the same entity cancel.
                dynamicShadowCasterEntityHash += 1 + intptr_t(c->entity().get());
                lastDynamicShadowCasterChangeTime = max(lastDynamicShadowCasterChangeTime, c->lastChangeTime());
            } else {
                baseArray.append(c);
                baseShadowCasterEntityHash += 1 + intptr_t(c->entity().get());
                lastBaseShadowCasterChangeTime = max(lastBaseShadowCasterChangeTime, c->lastChangeTime());
            }
        }
    }

    // Choose whether to target the VSM or the regular shadow map
    const bool vsmPass = (passType == RenderPassType::TRANSPARENT_SHADOW_MAP);
    ShadowMap::Layer& baseLayer    = vsmPass ? m_vsmSourceBaseLayer    : m_baseLayer;
    ShadowMap::Layer& dynamicLayer = vsmPass ? m_vsmSourceDynamicLayer : m_dynamicLayer;

    debugAssertM(! vsmPass || m_vsmSettings.enabled, 
        "Light called ShadowMap::updateDepth with RenderPassType::TRANSPARENT_SHADOW_MAP when VSM was not enabled");

    if ((m_lightProjection != lightProjectionMatrix) ||
        (m_lightFrame != lightCFrame)) {
        // The light itself moved--recompute everything
        dynamicLayer.lastUpdateTime = baseLayer.lastUpdateTime = 0.0;
    }

    if ((max(lastBaseShadowCasterChangeTime, lastDynamicShadowCasterChangeTime) < 
         min(baseLayer.lastUpdateTime, dynamicLayer.lastUpdateTime)) &&
        (baseShadowCasterEntityHash == baseLayer.entityHash) &&
        (dynamicShadowCasterEntityHash == dynamicLayer.entityHash)) {
        // Everything is up to date, so there's no reason to re-render the shadow map
        return;
    }

    m_lightProjection  = lightProjectionMatrix;
    m_lightFrame       = lightCFrame;

    // The light faces along its -z axis, so pull surfaces back along that axis
    // during surface rendering based on the bias depth.
    const Matrix4& zTranslate = Matrix4::translation(0, 0, m_bias);

    m_lightMVP = m_lightProjection * zTranslate * m_lightFrame.inverse();
        
    // Map [-1, 1] to [0, 1] (divide by 2 and add 0.5),
    // applying a bias term to offset the z value
    static const Matrix4 unitize(0.5f, 0.0f, 0.0f, 0.5f,
                                 0.0f, 0.5f, 0.0f, 0.5f,
                                 0.0f, 0.0f, 0.5f, 0.5f,
                                 0.0f, 0.0f, 0.0f, 1.0f);
        
    m_unitLightProjection = unitize * m_lightProjection;
    m_unitLightMVP = unitize * m_lightMVP;

    TransparencyTestMode transparencyTestMode;
    switch (passType) {
    case RenderPassType::SHADOW_MAP:
        // There is no VSM. This is your only chance to write to a shadow map, so go stochastic if you want.
        transparencyTestMode = TransparencyTestMode::STOCHASTIC;
        break;

    case RenderPassType::OPAQUE_SHADOW_MAP:
        // There is a VSM pass coming, but we're still rendering to the Williams map in this pass,
        // so do a hard cutoff at alpha = 1.
        transparencyTestMode = TransparencyTestMode::REJECT_TRANSPARENCY;
        break;

    default: debugAssert(passType == RenderPassType::TRANSPARENT_SHADOW_MAP);
        // This is the VSM pass. Render stochastic, but reject the alpha = 1 pixels
        // that were just rendered in the Williams shadow map.        
        transparencyTestMode = TransparencyTestMode::STOCHASTIC_REJECT_NONTRANSPARENT;
        break;
    }

    if ((lastBaseShadowCasterChangeTime > baseLayer.lastUpdateTime) ||
        (baseShadowCasterEntityHash != baseLayer.entityHash)) {
        baseLayer.updateDepth(renderDevice, this, baseArray, cullFace, transparencyTestMode, nullptr, transmissionWeight);      
    }

    // Render the dynamic layer if the dynamic layer OR the base layer changed
    if ((lastBaseShadowCasterChangeTime > baseLayer.lastUpdateTime) || 
        (lastDynamicShadowCasterChangeTime > dynamicLayer.lastUpdateTime) ||
        (baseShadowCasterEntityHash != baseLayer.entityHash) ||
        (dynamicShadowCasterEntityHash != dynamicLayer.entityHash)) {

        dynamicLayer.updateDepth(renderDevice, this, dynamicArray, cullFace, transparencyTestMode,
            // Only pass the base layer if it is not empty
            (baseShadowCasterEntityHash == 0) ? nullptr : baseLayer.framebuffer, transmissionWeight);
        
        if (vsmPass) {
            renderDevice->push2D(m_vsmRawFB); {
                Projection projection(m_lightProjection);
                Args args;
                args.setUniform("clipInfo", projection.reconstructFromDepthClipInfo());
                m_dynamicLayer.depthTexture->setShaderArgs(args, "opaqueDepth_", Sampler::video());
                m_vsmSourceDynamicLayer.depthTexture->setShaderArgs(args, "stochasticDepth_", Sampler::buffer());
                args.setRect(renderDevice->viewport());
                LAUNCH_SHADER("Light/Light_convertToVSM.pix", args);
            } renderDevice->pop2D();
            if (m_vsmSettings.filterRadius > 0) {
                const float farPlaneZ = Projection(m_lightProjection).farPlaneZ();
                auto blurOneDirection = [renderDevice,farPlaneZ](const VSMSettings& settings, const shared_ptr<Framebuffer>& src, const shared_ptr<Framebuffer>& dst, Vector2int32 direction) {
                    renderDevice->push2D(dst); {
                        Args args;
                        const int guassianBlurTaps = 2 * settings.filterRadius + 1;
                        const String& preamble = GaussianBlur::getPreamble(guassianBlurTaps, true, settings.blurMultiplier);
                        args.setPreamble(preamble);
                        const Vector2 sizeRatio = Vector2(float(src->width()), float(src->height())) / Vector2(float(dst->width()), float(dst->height()));
                        const Vector2int32 s(iRound(log2( sizeRatio.x)), iRound(log2( sizeRatio.y)));
                        args.setMacro("LOG_DOWNSAMPLE_X", s.x);
                        args.setMacro("LOG_DOWNSAMPLE_Y", s.y);
                        args.setUniform("source", src->texture(0), Sampler::video());
                        args.setUniform("direction", Vector2int32(direction));
                        args.setUniform("farPlaneZ", farPlaneZ+0.001f);
                        args.setRect(renderDevice->viewport());
                        LAUNCH_SHADER("Light/Light_vsmFilter.*", args);
                    } renderDevice->pop2D();
                };
                blurOneDirection(m_vsmSettings, m_vsmRawFB, m_vsmHBlurFB, Vector2int32(1,0));
                blurOneDirection(m_vsmSettings, m_vsmHBlurFB, m_vsmFinalFB, Vector2int32(0,1));
            } else {
                Texture::copy(m_vsmRawFB->texture(0), m_vsmFinalFB->texture(0));
            }
        }
    }

    baseLayer.entityHash = baseShadowCasterEntityHash;
    dynamicLayer.entityHash = dynamicShadowCasterEntityHash;
}


void ShadowMap::Layer::updateDepth
(RenderDevice*                      renderDevice,
 ShadowMap*                         shadowMap,
 const Array<shared_ptr<Surface> >& shadowCaster,
 CullFace                           cullFace,
 TransparencyTestMode                      transparencyTestMode,
 const shared_ptr<Framebuffer>&     initialValues,
 const Color3&                      transmissionWeight) {

    renderDevice->pushState(framebuffer); {
        const bool debugShadows = false;
        renderDevice->setColorWrite(debugShadows);
        renderDevice->setAlphaWrite(false);
        renderDevice->setDepthWrite(true);

        if (notNull(initialValues)) {
            initialValues->blitTo(renderDevice, framebuffer, false, false, true ,false, false);
        } else {
            renderDevice->clear(true, true, false);
        }

        // Draw from the light's point of view
        renderDevice->setCameraToWorldMatrix(shadowMap->m_lightFrame);
        renderDevice->setProjectionMatrix(shadowMap->m_lightProjection);

        renderDepthOnly(renderDevice, shadowMap, shadowCaster, cullFace, transparencyTestMode, transmissionWeight);
    } renderDevice->popState();

    lastUpdateTime = System::time();
}


void ShadowMap::Layer::renderDepthOnly(RenderDevice* renderDevice, const ShadowMap* shadowMap, const Array<shared_ptr<Surface> >& shadowCaster, CullFace cullFace, float polygonOffset, TransparencyTestMode transparencyTestMode, const Color3& transmissionWeight) const {
    renderDevice->setPolygonOffset(polygonOffset);
    Surface::renderDepthOnly(renderDevice, shadowCaster, cullFace, nullptr, 0.0f, transparencyTestMode, transmissionWeight);
}


void ShadowMap::Layer::renderDepthOnly(RenderDevice* renderDevice, const ShadowMap* shadowMap, const Array<shared_ptr<Surface> >& shadowCaster, CullFace cullFace, TransparencyTestMode transparencyTestMode, const Color3& transmissionWeight) const {
    if ((cullFace == CullFace::NONE) && 
        (shadowMap->m_backfacePolygonOffset != shadowMap->m_polygonOffset)) {
        // Different culling values
        renderDepthOnly(renderDevice, shadowMap, shadowCaster, CullFace::BACK, shadowMap->m_polygonOffset, transparencyTestMode, transmissionWeight);
        renderDepthOnly(renderDevice, shadowMap, shadowCaster, CullFace::FRONT, shadowMap->m_backfacePolygonOffset, transparencyTestMode, transmissionWeight);
    } else {
        float polygonOffset = (cullFace == CullFace::FRONT) ? shadowMap->m_backfacePolygonOffset : shadowMap->m_polygonOffset;
        renderDepthOnly(renderDevice, shadowMap, shadowCaster, cullFace, polygonOffset, transparencyTestMode, transmissionWeight);
    }
}


void ShadowMap::computeMatrices
(const shared_ptr<Light>& light, 
 AABox          sceneBounds,
 CFrame&        lightFrame,
 Projection&    lightProjection,
 Matrix4&       lightProjectionMatrix,
 float          lightProjX,
 float          lightProjY,
 float          lightProjNearMin,
 float          lightProjFarMax,
 float          intensityCutoff) {

    if (! sceneBounds.isFinite() || sceneBounds.isEmpty()) {
        // Produce some reasonable bounds
        sceneBounds = AABox(Point3(-20, -20, -20), Point3(20, 20, 20));
    }

    lightFrame = light->frame();

    if (light->type() == Light::Type::DIRECTIONAL) {
        Point3 center = sceneBounds.center();
        if (! center.isFinite()) {
            center = Point3::zero();
        }
        // Move directional light away from the scene.  It must be far enough to see all objects
        lightFrame.translation = -lightFrame.lookVector() * 
            min(1e6f, max(sceneBounds.extent().length() / 2.0f, lightProjNearMin, 30.0f)) + center;
    }

    const CFrame& f = lightFrame;

    float lightProjNear = finf();
    float lightProjFar  = 0.0f;

    // TODO: for a spot light, only consider objects this light can see

    // Find nearest and farthest corners of the scene bounding box
    for (int c = 0; c < 8; ++c) {
        const Vector3&  v        = sceneBounds.corner(c);
        const float     distance = -f.pointToObjectSpace(v).z;

        lightProjNear  = min(lightProjNear, distance);
        lightProjFar   = max(lightProjFar, distance);
    }
    
    // Don't let the near get too close to the source, and obey
    // the specified hint.
    lightProjNear = max(lightProjNearMin, lightProjNear);

    // Don't bother tracking shadows past the effective radius
    lightProjFar = min(light->effectSphere(intensityCutoff).radius, lightProjFar);
    lightProjFar = max(lightProjNear + 0.1f, min(lightProjFarMax, lightProjFar));

    debugAssert(lightProjNear < lightProjFar);

    if (light->type() != Light::Type::DIRECTIONAL) {
        // TODO: Square spot

        // Spot light; we can set the lightProj bounds intelligently

        alwaysAssertM(light->spotHalfAngle() <= halfPi(), "Spot light with shadow map and greater than 180-degree bounds");

        // The cutoff is half the angle of extent (See the Red Book, page 193)
        const float angle = light->spotHalfAngle();

        lightProjX = tan(angle) * lightProjNear;
      
        // Symmetric in x and y
        lightProjY = lightProjX;

        lightProjectionMatrix = 
            Matrix4::perspectiveProjection
            (-lightProjX, lightProjX, -lightProjY, 
             lightProjY, lightProjNear, lightProjFar);

    } else if (light->type() == Light::Type::DIRECTIONAL) {
        // Directional light

        // Construct a projection and view matrix for the camera so we can 
        // render the scene from the light's point of view
        //
        // Since we're working with a directional light, 
        // we want to make the center of projection for the shadow map
        // be in the direction of the light but at a finite distance 
        // to preserve z precision.
        
        lightProjectionMatrix = 
            Matrix4::orthogonalProjection
            (-lightProjX, lightProjX, -lightProjY, 
             lightProjY, lightProjNear, lightProjFar);

    } else {
        // Omni light.  Nothing good can happen here, but at least we
        // generate something

        lightProjectionMatrix =
            Matrix4::perspectiveProjection
            (-lightProjX, lightProjX, -lightProjY, 
             lightProjY, lightProjNear, lightProjFar);
    }

    const float fov = atan2(lightProjX, lightProjNear) * 2.0f;
    lightProjection.setFieldOfView(fov, FOVDirection::HORIZONTAL);
    lightProjection.setNearPlaneZ(-lightProjNear);
    lightProjection.setFarPlaneZ(-lightProjFar);
}

///////////////////////////////////////////////////////////////////////////////////////////

ShadowMap::VSMSettings::VSMSettings(const Any& a) {
    *this = VSMSettings();
    a.verifyName("VSMSettings");

    AnyTableReader r(a);
    
    r.getIfPresent("enabled", enabled);
    r.getIfPresent("blurMultiplier", blurMultiplier);
    r.getIfPresent("filterRadius", filterRadius);
    r.getIfPresent("downsampleFactor", downsampleFactor);
    r.getIfPresent("lightBleedReduction", lightBleedReduction);
    r.getIfPresent("baseSize", baseSize);
    downsampleFactor = max(downsampleFactor, 1);
    r.verifyDone();
}


Any ShadowMap::VSMSettings::toAny() const {
    Any a(Any::TABLE, "VSMSettings");
    a["enabled"] = enabled;
    a["blurMultiplier"] = blurMultiplier;
    a["filterRadius"] = filterRadius;
    a["downsampleFactor"] = downsampleFactor;
    a["lightBleedReduction"] = lightBleedReduction;
    a["baseSize"] = baseSize;
    return a;
}


bool ShadowMap::VSMSettings::operator==(const VSMSettings & o) const {
    return (enabled == o.enabled) &&
        (blurMultiplier == o.blurMultiplier) && 
        (filterRadius == o.filterRadius) && 
        (downsampleFactor == o.downsampleFactor) &&
        (lightBleedReduction == o.lightBleedReduction) &&
        (baseSize == o.baseSize);
}

} // G3D
