/**
 \file   DepthOfField.cpp
 \author Morgan McGuire
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "GLG3D/DepthOfField.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Draw.h"

namespace G3D {

DepthOfField::DepthOfField() {
    // Intentionally empty
}


shared_ptr<DepthOfField> DepthOfField::create() {
    return shared_ptr<DepthOfField>(new DepthOfField());
}


void DepthOfField::apply
(RenderDevice*                  rd, 
 shared_ptr<Texture>            color,
 shared_ptr<Texture>            depth, 
 const shared_ptr<Camera>&      camera,
 Vector2int16                   trimBandThickness,
 DebugOption                    debugOption) {
    
    if ((camera->depthOfFieldSettings().model() == DepthOfFieldModel::NONE) || ! camera->depthOfFieldSettings().enabled()) {
        const shared_ptr<Framebuffer>& f = rd->framebuffer();
        const shared_ptr<Framebuffer::Attachment>& a = f->get(Framebuffer::COLOR0);
       
        if (isNull(f) || (a->texture() != color)) {
            Texture::copy(color, a->texture(), 0, 0, 1.0f, trimBandThickness, CubeFace::POS_X, CubeFace::POS_X, rd, false);
        }

        // Exit abruptly because DoF is disabled
        return;
    }

    alwaysAssertM(notNull(color), "Color buffer may not be NULL");
    alwaysAssertM(notNull(depth), "Depth buffer may not be NULL");

    BEGIN_PROFILER_EVENT("DepthOfField::apply");
    resizeBuffers(color, trimBandThickness);

    float farRadiusRescale = 1.0f;
    computeCoC(rd, color, depth, camera, trimBandThickness, farRadiusRescale);
    blurPass(rd, m_packedBuffer, m_packedBuffer, m_horizontalFramebuffer, true, camera);
    blurPass(rd, m_tempBlurBuffer, m_tempNearBuffer, m_verticalFramebuffer, false, camera);
    composite(rd, m_packedBuffer, m_blurBuffer, m_nearBuffer, debugOption, trimBandThickness, farRadiusRescale);
	END_PROFILER_EVENT();
}


/** Limit the maximum radius allowed for physical blur to 1% of the viewport or 12 pixels */
static float maxPhysicalBlurRadius(const Rect2D& viewport) {
    return max(viewport.width() / 100.0f, 12.0f);
}


void DepthOfField::computeCoC
(RenderDevice*                  rd, 
 const shared_ptr<Texture>&     color, 
 const shared_ptr<Texture>&     depth, 
 const shared_ptr<Camera>&      camera,
 Vector2int16                   trimBandThickness,
 float&                         farRadiusRescale) {

    rd->push2D(m_packedFramebuffer); {
        rd->clear();
        Args args;

        args.setUniform("clipInfo",             camera->projection().reconstructFromDepthClipInfo());
        args.setUniform("COLOR_buffer",         color, Sampler::video());
        args.setUniform("DEPTH_buffer",         depth, Sampler::buffer());
        //args.setUniform("projInfo",             camera->projection().reconstructFromDepthProjInfo(depth->width(), depth->height()));
        args.setUniform("trimBandThickness",    trimBandThickness);
        args.setRect(rd->viewport());

        const float maxCoCRadiusPixels = ceil(camera->maxCircleOfConfusionRadiusPixels(color->rect2DBounds()));
        const float axisSize = (camera->fieldOfViewDirection() == FOVDirection::HORIZONTAL) ? float(color->width()) : float(color->height());
        
        if (camera->depthOfFieldSettings().model() == DepthOfFieldModel::ARTIST) {

            args.setUniform("nearBlurryPlaneZ", camera->depthOfFieldSettings().nearBlurryPlaneZ());
            args.setUniform("nearSharpPlaneZ",  camera->depthOfFieldSettings().nearSharpPlaneZ());
            args.setUniform("farSharpPlaneZ",   camera->depthOfFieldSettings().farSharpPlaneZ());
            args.setUniform("farBlurryPlaneZ",  camera->depthOfFieldSettings().farBlurryPlaneZ());

            // This is a positive number
            const float nearScale =             
                camera->depthOfFieldSettings().nearBlurRadiusFraction() / 
                (camera->depthOfFieldSettings().nearBlurryPlaneZ() - camera->depthOfFieldSettings().nearSharpPlaneZ());
            alwaysAssertM(nearScale >= 0.0f, "Near normalization must be a non-negative factor");
            args.setUniform("nearScale", nearScale * axisSize / maxCoCRadiusPixels); 

            // This is a positive number
            const float farScale =             
                camera->depthOfFieldSettings().farBlurRadiusFraction() / 
                (camera->depthOfFieldSettings().farSharpPlaneZ() - camera->depthOfFieldSettings().farBlurryPlaneZ());
            alwaysAssertM(farScale >= 0.0f, "Far normalization must be a non-negative factor");
            args.setUniform("farScale", farScale * axisSize / maxCoCRadiusPixels);

            farRadiusRescale = 
                max(camera->depthOfFieldSettings().farBlurRadiusFraction(), camera->depthOfFieldSettings().nearBlurRadiusFraction()) /
                max(camera->depthOfFieldSettings().farBlurRadiusFraction(), 0.0001f);

        } else {
            farRadiusRescale = 1.0f;
            const float scale = 
                camera->imagePlanePixelsPerMeter(rd->viewport()) * camera->depthOfFieldSettings().lensRadius() / 
                (camera->depthOfFieldSettings().focusPlaneZ() * maxCoCRadiusPixels);
            args.setUniform("focusPlaneZ", camera->depthOfFieldSettings().focusPlaneZ());
            args.setUniform("scale", scale);
        }
        
        args.setMacro("MODEL", camera->depthOfFieldSettings().model().toString());
        args.setMacro("PACK_WITH_COLOR", 1);

        // In case the output is an unsigned format
        args.setUniform("writeScaleBias", Vector2(0.5f, 0.5f));
        LAUNCH_SHADER("DepthOfField_circleOfConfusion.pix", args);

    } rd->pop2D();
}


void DepthOfField::blurPass
(RenderDevice*                  rd,
 const shared_ptr<Texture>&     blurInput,
 const shared_ptr<Texture>&     nearInput,
 const shared_ptr<Framebuffer>& output,
 bool                           horizontal,
 const shared_ptr<Camera>&      camera) {

    alwaysAssertM(notNull(blurInput), "Input is NULL");

    // Dimension along which the blur fraction is measured
    const float dimension = 
        float((camera->fieldOfViewDirection() == FOVDirection::HORIZONTAL) ?
            m_packedBuffer->width() : m_packedBuffer->height());

    const int maxCoCRadiusPixels = iCeil(camera->maxCircleOfConfusionRadiusPixels(m_packedBuffer->rect2DBounds()));

    int nearBlurRadiusPixels =
            iCeil((camera->depthOfFieldSettings().model() == DepthOfFieldModel::ARTIST) ? 
                  (camera->depthOfFieldSettings().nearBlurRadiusFraction() * dimension) :
                   maxPhysicalBlurRadius(m_packedBuffer->rect2DBounds()));

    // Avoid ever showing the downsampled buffer without blur
    if (nearBlurRadiusPixels < 2) {
        nearBlurRadiusPixels = 0;
    }

    rd->push2D(output); {
        rd->clear();
        Args args;

        args.setUniform("blurSourceBuffer",   blurInput, Sampler::buffer());
        args.setUniform("nearSourceBuffer",   nearInput, Sampler::buffer(), true);
        args.setUniform("maxCoCRadiusPixels", maxCoCRadiusPixels);
        args.setUniform("nearBlurRadiusPixels", nearBlurRadiusPixels);
        args.setUniform("invNearBlurRadiusPixels", 1.0f / max(float(nearBlurRadiusPixels), 0.0001f));
        args.setMacro("HORIZONTAL", horizontal ? 1 : 0);
        args.setRect(rd->viewport());
        LAUNCH_SHADER("DepthOfField_blur.*", args);
    } rd->pop2D();
}


void DepthOfField::composite
(RenderDevice*          rd,
 shared_ptr<Texture>    packedBuffer, 
 shared_ptr<Texture>    blurBuffer,
 shared_ptr<Texture>    nearBuffer,
 DebugOption            debugOption,
 Vector2int16           outputGuardBandThickness,
 float                  farRadiusRescale) {

    debugAssert(farRadiusRescale >= 0.0);
    rd->push2D(); {
        rd->clear(true, false, false);
        rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
        rd->setDepthWrite(false);
        Args args;
        
        args.setUniform("blurBuffer",   blurBuffer, Sampler::video());
        args.setUniform("nearBuffer",   nearBuffer, Sampler::video());
        args.setUniform("packedBuffer", packedBuffer, Sampler::buffer());
        args.setUniform("packedBufferInvSize", Vector2(1.0f, 1.0f) / packedBuffer->vector2Bounds());
        args.setUniform("farRadiusRescale", farRadiusRescale);

        args.setUniform("debugOption",  debugOption);
		args.setRect(Rect2D::xywh(Vector2(outputGuardBandThickness), rd->viewport().wh() - 2.0f * Vector2(outputGuardBandThickness)));

        LAUNCH_SHADER("DepthOfField_composite.*", args);
    } rd->pop2D();
}


/** Allocates or resizes a texture and framebuffer to match a target
    format and dimensions. */
static void matchTarget
(const String&             textureName,
 const shared_ptr<Texture>&     target, 
 int                            divWidth, 
 int                            divHeight,
 int                            guardBandRemoveX,
 int                            guardBandRemoveY,
 const ImageFormat*             format,
 shared_ptr<Texture>&           texture, 
 shared_ptr<Framebuffer>&       framebuffer,
 Framebuffer::AttachmentPoint   attachmentPoint = Framebuffer::COLOR0,
 bool                           generateMipMaps = false) {

    alwaysAssertM(format, "Format may not be NULL");

    const int w = (target->width()  - guardBandRemoveX * 2) / divWidth;
    const int h = (target->height() - guardBandRemoveY * 2) / divHeight;
    if (isNull(texture) || (texture->format() != format)) {
        // Allocate
        texture = Texture::createEmpty
            (textureName, 
             w, 
             h,
             format,
             Texture::DIM_2D,
             generateMipMaps);

        if (isNull(framebuffer)) {
            framebuffer = Framebuffer::create("");
        }
        framebuffer->set(attachmentPoint, texture);

    } else if ((texture->width() != w) ||
               (texture->height() != h)) {
        texture->resize(w, h);
    }
}


void DepthOfField::resizeBuffers(shared_ptr<Texture> target, const Vector2int16 trimBandThickness) {
    const ImageFormat* plusAlphaFormat = ImageFormat::getFormatWithAlpha(target->format());

    // Need an alpha channel for storing radius in the packed and far temp buffers
    matchTarget("G3D::DepthOfField::m_packedBuffer", target, 1, 1, trimBandThickness.x, trimBandThickness.y, plusAlphaFormat,     m_packedBuffer,    m_packedFramebuffer,     Framebuffer::COLOR0);

    matchTarget("G3D::DepthOfField::m_tempNearBuffer", target, 4, 1, trimBandThickness.x, trimBandThickness.y, plusAlphaFormat,     m_tempNearBuffer,  m_horizontalFramebuffer, Framebuffer::COLOR0);
    matchTarget("G3D::DepthOfField::m_tempBlurBuffer", target, 4, 1, trimBandThickness.x, trimBandThickness.y, plusAlphaFormat,     m_tempBlurBuffer,  m_horizontalFramebuffer, Framebuffer::COLOR1);

    // Need an alpha channel (for coverage) in the near buffer
    matchTarget("G3D::DepthOfField::m_nearBuffer", target, 4, 4, trimBandThickness.x, trimBandThickness.y, plusAlphaFormat,     m_nearBuffer,      m_verticalFramebuffer,   Framebuffer::COLOR0);
    matchTarget("G3D::DepthOfField::m_blurBuffer", target, 4, 4, trimBandThickness.x, trimBandThickness.y, target->format(),    m_blurBuffer,      m_verticalFramebuffer,   Framebuffer::COLOR1);
}


} // Namespace G3D
