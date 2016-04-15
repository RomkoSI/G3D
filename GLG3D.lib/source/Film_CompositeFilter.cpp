/**
 \file GLG3D.lib/source/Film_CompositeFilter.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2008-07-01
 \edited  2016-04-11

 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
*/

#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/Film.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "GLG3D/GaussianBlur.h"

namespace G3D {

Film::CompositeFilter::CompositeFilter() {
    m_framebuffer           = Framebuffer::create("Film::m_framebuffer");
    m_blurryFramebuffer     = Framebuffer::create("Film::m_blurryFramebuffer");
    m_tempFramebuffer       = Framebuffer::create("Film::m_tempFramebuffer");
    m_postGammaFramebuffer  = Framebuffer::create("Film::m_postGammaFramebuffer");

    const bool generateMipMaps = false;
    m_toneCurve             = Framebuffer::create(Texture::createEmpty("G3D::Film::m_toneCurve", 256, 1, 
                                                                       GLCaps::supportsTexture(ImageFormat::R16F()) ?
                                                                       ImageFormat::R16F() : 
                                                                       (GLCaps::supportsTexture(ImageFormat::R32F()) ?
                                                                        ImageFormat::R32F() : ImageFormat::RGBA16F()), Texture::DIM_2D, generateMipMaps));

    m_lastToneCurve = FilmSettings().toneCurve();

    // Force the tone curve to not match initially
    m_lastToneCurve.control[0] = -1;

    m_intermediateFormat = GLCaps::firstSupportedTexture(Array<const ImageFormat*>(ImageFormat::R11G11B10F(), ImageFormat::RGB16F(), ImageFormat::RGB32F(), ImageFormat::RGBA8()));
}


void Film::CompositeFilter::maybeUpdateToneCurve(const FilmSettings& settings) const {
    for (int i = 0; i < settings.toneCurve().control.size(); ++i) {
        if ((m_lastToneCurve.control[i] != settings.toneCurve().control[i]) ||
            (m_lastToneCurve.time[i] != settings.toneCurve().time[i])) {

            // A control point changed
            m_lastToneCurve = settings.toneCurve();

            // Update the texture
            shared_ptr<PixelTransferBuffer> buffer;
            if (GLCaps::enumVendor() == GLCaps::AMD) {
                // Workaround for Radeon bug that causes glTexSubImage2D to fail when reading from GLPixelTransferBuffer for this particular
                // case. It is not affected by the image resolution or format.
                buffer = CPUPixelTransferBuffer::create(m_toneCurve->width(), 1, ImageFormat::R32F());
            } else {
                buffer = GLPixelTransferBuffer::create(m_toneCurve->width(), 1, ImageFormat::R32F());
            }

            float* tone = reinterpret_cast<float*>(buffer->mapWrite());
            const float k = 3.0f;
            for (int j = 0; j < buffer->width(); ++j) {
                const float x = j / float(buffer->width() - 1);
                // The underlying curve is in a warped (power of k) space to make it easier to edit small numbers.
                // We don't actually use log(x) since the input values go to zero.
                tone[j] = max(powf(m_lastToneCurve.evaluate(powf(x, 1.0f / k)), k), 0.0f);
            }
            buffer->unmap();
            tone = NULL;
            m_toneCurve->texture(0)->update(buffer);
            return;
        } // if 
    }  // for
}


void Film::CompositeFilter::apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceTrimBandThickness, int sourceDepthGuardBandThickness) const {
    debugAssert(sourceTrimBandThickness <= sourceDepthGuardBandThickness);
    allocate("CompositeFilter", source, argTarget, sourceDepthGuardBandThickness);

    const bool invertY = target->invertY();
    const int w = target->width(), h = target->height();

    maybeUpdateToneCurve(settings);

    int blurDiameter = iRound(settings.bloomRadiusFraction() * 2.0f * max(w, h));
    if (isEven(blurDiameter)) {
        ++blurDiameter;
    }

    float bloomStrength = settings.bloomStrength();
    if (blurDiameter <= 1) {
        // Turn off bloom; the filter radius is too small
        bloomStrength = 0;
    }

    // Allocate intermediate buffers, perhaps because the input size is different than was previously used.
    if (isNull(m_temp) || (m_blurry->width() != w / 4) || (m_blurry->height() != h / 4)) {
        bool generateMipMaps = false;
        // Make smaller to save fill rate, since it will be blurry anyway
        m_preBloom = Texture::createEmpty("G3D::Film::CompositeFilter::m_preBloom", w, h,         m_intermediateFormat, Texture::DIM_2D, generateMipMaps);
        m_temp     = Texture::createEmpty("G3D::Film::CompositeFilter::m_temp",     w, h / 4,     m_intermediateFormat, Texture::DIM_2D, generateMipMaps);
        m_blurry   = Texture::createEmpty("G3D::Film::CompositeFilter::m_blurry",   w / 4, h / 4, m_intermediateFormat, Texture::DIM_2D, generateMipMaps);

        // Clear the newly created textures
        m_preBloom->clear(CubeFace::POS_X, 0, rd);
        m_temp->clear(CubeFace::POS_X, 0, rd);
        m_blurry->clear(CubeFace::POS_X, 0, rd);

        m_framebuffer->set(Framebuffer::COLOR0, m_preBloom);
        m_tempFramebuffer->set(Framebuffer::COLOR0, m_temp);
        m_blurryFramebuffer->set(Framebuffer::COLOR0, m_blurry);
    }

    // Bloom
    if (bloomStrength > 0) {
        rd->push2D(m_framebuffer); {
            rd->clear();
            Args args;
            source->setShaderArgs(args, "sourceTexture_", Sampler::video());
            args.setUniform("ySign", invertY ? -1 : 1);
            args.setUniform("yOffset", invertY ? source->height() : 0);
            args.setUniform("guardBandSize", Vector2int32(sourceDepthGuardBandThickness, sourceDepthGuardBandThickness));
            args.setUniform("sensitivity", settings.sensitivity());
            args.setUniform("toneCurve", m_toneCurve->texture(0), Sampler::video());
            args.setRect(rd->viewport());
            LAUNCH_SHADER("Film_bloomExpose.pix", args);
        } rd->pop2D();

        // Blur and subsample vertically
        rd->push2D(m_tempFramebuffer); {
            GaussianBlur::apply(rd, m_framebuffer->texture(0), Vector2(0, 1), blurDiameter, m_temp->vector2Bounds());
        } rd->pop2D();

        // Blur and subsample horizontally
        rd->push2D(m_blurryFramebuffer); {
            GaussianBlur::apply(rd, m_tempFramebuffer->texture(0), Vector2(1, 0), blurDiameter, m_blurry->vector2Bounds());
        } rd->pop2D();
    }    
    
    rd->push2D(target); {
        Args args;
        args.setMacro("BLOOM", (bloomStrength > 0) ? 1 : 0);
        // Combine, fix saturation, gamma correct and draw
        source->setShaderArgs(args, "sourceTexture_", Sampler::video());
        args.setUniform("ySign", invertY ? -1 : 1);
        args.setUniform("yOffset", invertY ? source->height() : 0);
        args.setUniform("guardBandSize", Vector2int32(sourceDepthGuardBandThickness, sourceDepthGuardBandThickness));

        args.setUniform("toneCurve", m_toneCurve->texture(0), Sampler::video());

        if (bloomStrength > 0) {
            args.setUniform("bloomTexture", (bloomStrength > 0) ? m_blurry : Texture::zero(), Sampler::video());
            args.setUniform("bloomStrengthScaled", bloomStrength * 5.0f);
        }

        args.setUniform("sensitivity", settings.sensitivity());
        args.setUniform("invGamma", 1.0f / settings.gamma());
        args.setUniform("vignetteTopStrength", clamp(settings.vignetteTopStrength(), 0.0f, 1.0f));
        args.setUniform("vignetteBottomStrength", clamp(settings.vignetteBottomStrength(), 0.0f, 1.0f));
        args.setUniform("vignetteSize", settings.vignetteSizeFraction());
        args.setRect(rd->viewport());
        LAUNCH_SHADER("Film_composite.*", args);
    }; rd->pop2D();
}

} // G3D
 