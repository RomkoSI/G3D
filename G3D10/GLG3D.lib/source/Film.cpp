/**
 \file GLG3D.lib/source/Film.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2008-07-01
 \edited  2016-04-11

 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
*/

#include "G3D/debugAssert.h"
#include "G3D/fileutils.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Film.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"

namespace G3D {

Film::Film() {}


shared_ptr<Film> Film::create() {
    return shared_ptr<Film>(new Film());
}


void Film::exposeAndRender
   (RenderDevice*               rd,
    const FilmSettings&         settings,
    const shared_ptr<Texture>&  input,
    int                         sourceTrimBandThickness, 
    int                         sourceColorBandThickness,
    shared_ptr<Texture>&        output,
    CubeFace                    outputCubeFace,
    int                         outputMipLevel) {
    
    if (isNull(output)) {
        // Allocate new output texture
        const bool generateMipMaps = false;
        output = Texture::createEmpty("Exposed image", input->width(), input->height(), input->format(), 
                                      input->dimension(), generateMipMaps);
    }
    //    debugAssertM(output->width() % 4 == 0 && output->width() % 4 == 0, "Width and Height must be a multiple of 4");
    const shared_ptr<Framebuffer>& fb = Framebuffer::create("Film temp");
    fb->set(Framebuffer::COLOR0, output, outputCubeFace, outputMipLevel);
    rd->push2D(fb); {
        rd->clear();
        exposeAndRender(rd, settings, input, sourceTrimBandThickness, sourceColorBandThickness);
    } rd->pop2D();

    // Override the document gamma
    output->visualization = Texture::Visualization::sRGB();
    output->visualization.documentGamma = settings.gamma();
}


void Film::Filter::allocate(const String& name, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceDepthGuardBandThickness, const ImageFormat* fmt) const {
    if (isNull(argTarget)) {
        // Allocate the framebuffer or resize as needed
        const int w = source->width()  - 2 * sourceDepthGuardBandThickness;
        const int h = source->height() - 2 * sourceDepthGuardBandThickness;
        if (isNull(m_intermediateResultFramebuffer)) {
            m_intermediateResultFramebuffer = Framebuffer::create(
                Texture::createEmpty("G3D::Film::" + name + "::m_intermediateResultFramebuffer", 
                                     w, h, ImageFormat::RGBA8(), Texture::DIM_2D, false));
        } else {
            m_intermediateResultFramebuffer->texture(0)->resize(w, h);
        }

        target = m_intermediateResultFramebuffer;
    } else {
        target = argTarget;
    }
}


void Film::FXAAFilter::apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceTrimBandThickness, int sourceDepthGuardBandThickness) const  {
    debugAssert((sourceTrimBandThickness == 0) && (sourceDepthGuardBandThickness == 0));
    allocate("WideAAFilter", source, argTarget, sourceDepthGuardBandThickness);

    rd->push2D(target); {
        Args args;
        source->setShaderArgs(args, "sourceTexture_", Sampler::video());
        args.setRect(rd->viewport());

        if (settings.antialiasingHighQuality()) {
            LAUNCH_SHADER("Film_FXAA_310.*", args);
        } else {
            LAUNCH_SHADER("Film_FXAA_311.*", args);
        }
    } rd->pop2D();
}


void Film::WideAAFilter::apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceTrimBandThickness, int sourceDepthGuardBandThickness) const  {
    debugAssert((sourceTrimBandThickness == 0) && (sourceDepthGuardBandThickness == 0));
    
    allocate("WideAAFilter", source, argTarget, sourceDepthGuardBandThickness);

    rd->push2D(target); {
        Args args;
        source->setShaderArgs(args, "sourceTexture_", Sampler::video());
        args.setUniform("radius", settings.antialiasingFilterRadius());
        args.setRect(rd->viewport());
        LAUNCH_SHADER("Film_wideAA.*", args);
    } rd->pop2D();
}


void Film::DebugZoomFilter::apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceTrimBandThickness, int sourceDepthGuardBandThickness) const  {
    debugAssert((sourceTrimBandThickness == 0) && (sourceDepthGuardBandThickness == 0));
    debugAssert(settings.debugZoom() > 1);

    allocate("DebugZoomFilter", source, argTarget, sourceDepthGuardBandThickness);
    const bool invertY = target->invertY();
    
    rd->push2D(target); {
        Args args;
        args.setUniform("source", source, Sampler::video());
        args.setUniform("yOffset", invertY ? rd->height() : 0);
        args.setUniform("ySign", invertY ? -1 : 1);
        args.setUniform("dstOffset", Point2::zero());
        args.setUniform("offset", Vector2int32((target->vector2Bounds() -
            target->vector2Bounds() / float(settings.debugZoom())) / 2));
        args.setUniform("scale", settings.debugZoom());
        args.setRect(rd->viewport());
        LAUNCH_SHADER("Film_zoom.*", args);
    } rd->pop2D();
}


void Film::EffectsDisabledBlitFilter::apply(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& source, const shared_ptr<Framebuffer>& argTarget, int sourceTrimBandThickness, int sourceDepthGuardBandThickness) const  {
    debugAssert(sourceTrimBandThickness <= sourceDepthGuardBandThickness);
    allocate("EffectsDisabledBlitFilter", source, argTarget, sourceDepthGuardBandThickness);
    rd->push2D(target); {
        Args args;
        source->setShaderArgs(args, "sourceTexture_", Sampler::video());
        args.setUniform("guardBandSize", Vector2int32(sourceDepthGuardBandThickness, sourceDepthGuardBandThickness));
        args.setRect(rd->viewport());
        LAUNCH_SHADER("Film_effectsDisabledBlit.pix", args);
    } rd->pop2D();
}


void Film::exposeAndRender(RenderDevice* rd, const FilmSettings& settings, const shared_ptr<Texture>& input, int sourceTrimBandThickness, int sourceDepthGuardBandThickness) {
    BEGIN_PROFILER_EVENT("Film::exposeAndRender");

    debugAssertM(rd->drawFramebuffer(), "In G3D 10, the drawFramebuffer should never be null");
   
    // Keep the size of this small array equal to the maximum number of filters that can be bound
    typedef SmallArray<Filter*, 6> FilterChain;
    FilterChain filterChain;

    // Build the filter chain:
    if (settings.effectsEnabled()) {
        filterChain.push(&m_compositeFilter);

        if (settings.antialiasingEnabled()) {
            filterChain.push(&m_fxaaFilter);

            if (settings.antialiasingFilterRadius() > 0) {
                filterChain.push(&m_wideAAFilter);
            }
        }

        if (settings.debugZoom() > 1) {
            filterChain.push(&m_debugZoomFilter);
        }
    } else {
        filterChain.push(&m_effectsDisabledBlitFilter);
    }

    // Run the filters:    
    for (int i = 0; i < filterChain.size(); ++i) {
        const bool first = (i == 0);
        const bool last  = (i == filterChain.size() - 1);

        // The first filter reads from the input
        // The others read from the previous filter's framebuffer
        const shared_ptr<Texture>& source = first ? input : filterChain[i - 1]->target->texture(Framebuffer::COLOR0);

        filterChain[i]->apply(rd, settings, source, last ? rd->drawFramebuffer() : nullptr, first ? sourceTrimBandThickness : 0, first ? sourceDepthGuardBandThickness : 0);
    }

#if 0
    ///////////////////////////////////////////////////////////////////////////////

    const shared_ptr<Framebuffer>& targetFramebuffer = rd->drawFramebuffer();
    const bool invertY = notNull(rd->drawFramebuffer()) && rd->drawFramebuffer()->invertY();

    // Size based on output width, taking cropping from the offset shift into account
    const int w = rd->width();
    const int h = rd->height();

    const Vector2int16 guardBandSize((input->width() - w) / 2, (input->height() - h) / 2);
        
    if (settings.effectsEnabled()) {

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
            m_preBloom = Texture::createEmpty("G3D::Film::m_preBloom", w, h, m_intermediateFormat, Texture::DIM_2D, generateMipMaps);
            m_temp = Texture::createEmpty("G3D::Film::m_temp", w, h / 4, m_intermediateFormat, Texture::DIM_2D, generateMipMaps);
            m_blurry = Texture::createEmpty("G3D::Film::m_blurry", w / 4, h / 4, m_intermediateFormat, Texture::DIM_2D, generateMipMaps);

            // Clear the newly created textures
            m_preBloom->clear(CubeFace::POS_X, 0, rd);
            m_temp->clear(CubeFace::POS_X, 0, rd);
            m_blurry->clear(CubeFace::POS_X, 0, rd);

            m_framebuffer->set(Framebuffer::COLOR0, m_preBloom);
            m_tempFramebuffer->set(Framebuffer::COLOR0, m_temp);
            m_blurryFramebuffer->set(Framebuffer::COLOR0, m_blurry);

            m_postGammaFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("G3D::Film::m_postGamma", w, h, ImageFormat::getFormatWithAlpha(m_targetFormat), Texture::DIM_2D, generateMipMaps));
            m_fxaaFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("G3D::Film::m_fxaaFramebuffer->texture(0)", w, h, m_targetFormat, Texture::DIM_2D, generateMipMaps));
            m_zoomFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("G3D::Film::m_zoom", w, h, m_targetFormat, Texture::DIM_2D, generateMipMaps));
        }

        if (settings.debugZoom() > 1) {
            rd->push2D(m_zoomFramebuffer);
        } else {
            rd->push2D();
        }
        {
            // Bloom
            if (bloomStrength > 0) {
                shared_ptr<Framebuffer> oldFB = rd->drawFramebuffer();
                rd->setFramebuffer(m_framebuffer);
                rd->clear();
                Args args;
                input->setShaderArgs(args, "sourceTexture_", Sampler::video());
                args.setUniform("ySign", invertY ? -1 : 1);
                args.setUniform("yOffset", invertY ? input->height() : 0);
                args.setUniform("guardBandSize", guardBandSize);
                args.setUniform("sensitivity", settings.sensitivity());
                args.setUniform("toneCurve", m_toneCurve->texture(0), Sampler::video());
                args.setRect(rd->viewport());
                LAUNCH_SHADER("Film_bloomExpose.pix", args);

                // Blur and subsample vertically
                rd->setFramebuffer(m_tempFramebuffer);
                GaussianBlur::apply(rd, m_framebuffer->texture(0), Vector2(0, 1), blurDiameter, m_temp->vector2Bounds());

                // Blur and subsample horizontally
                rd->setFramebuffer(m_blurryFramebuffer);
                GaussianBlur::apply(rd, m_tempFramebuffer->texture(0), Vector2(1, 0), blurDiameter, m_blurry->vector2Bounds());

                rd->setFramebuffer(oldFB);
            }

            if (settings.antialiasingEnabled()) {
                rd->push2D(m_postGammaFramebuffer);
                // Clear the gamma framebuffer
                rd->clear();
            }

            {
                Args args;
                args.setMacro("BLOOM", (bloomStrength > 0) ? 1 : 0);
                // Combine, fix saturation, gamma correct and draw
                input->setShaderArgs(args, "sourceTexture_", Sampler::video());
                args.setUniform("ySign", invertY ? -1 : 1);
                args.setUniform("yOffset", invertY ? input->height() : 0);
                args.setUniform("guardBandSize", guardBandSize);

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
                if (rd->drawFramebuffer() == targetFramebuffer) {
                    args.setRect(Rect2D::xywh(Point2::zero(), Vector2(float(w), float(h))));
                } else {
                    args.setRect(rd->viewport());
                }
                LAUNCH_SHADER("Film_composite.*", args);
            }

            if (settings.antialiasingEnabled()) {
                // Unbind the m_postGammaFramebuffer
                rd->pop2D();

                if (settings.antialiasingFilterRadius() > 0) {
                    rd->push2D(m_fxaaFramebuffer);
                }

                {
                    // FXAA pass
                    Args args;
                    m_postGammaFramebuffer->texture(0)->setShaderArgs(args, "sourceTexture_", Sampler::video());
                    if (rd->drawFramebuffer() == targetFramebuffer) {
                        args.setRect(Rect2D::xywh(Point2::zero(), Vector2(float(w), float(h))));
                    } else {
                        args.setRect(rd->viewport());
                    }
                    if (settings.antialiasingHighQuality()) {
                        LAUNCH_SHADER("Film_FXAA_310.*", args);
                    } else {
                        LAUNCH_SHADER("Film_FXAA_311.*", args);
                    }
                }

                if (settings.antialiasingFilterRadius() > 0) {
                    rd->pop2D();
                    // Wide filter pass
                    Args args;
                    m_fxaaFramebuffer->texture(0)->setShaderArgs(args, "sourceTexture_", Sampler::video());
                    args.setUniform("radius", settings.antialiasingFilterRadius());
                    if (rd->drawFramebuffer() == targetFramebuffer) {
                        args.setRect(Rect2D::xywh(Point2::zero(), Vector2(float(w), float(h))));
                    } else {
                        args.setRect(rd->viewport());
                    }
                    LAUNCH_SHADER("Film_wideAA.*", args);
                }

            } // Antialiasing

        } rd->pop2D();

        if (settings.debugZoom() > 1) {
            rd->push2D(); {
                Args args;
                args.setUniform("source", m_zoomFramebuffer->texture(0), Sampler::video());

                args.setUniform("yOffset", invertY ? rd->height() : 0);
                args.setUniform("ySign", invertY ? -1 : 1);

                args.setUniform("dstOffset", Point2::zero());
                args.setUniform("offset", Vector2int32((m_zoomFramebuffer->vector2Bounds() -
                    m_zoomFramebuffer->vector2Bounds() / float(settings.debugZoom())) / 2));
                args.setUniform("scale", settings.debugZoom());
                if (rd->drawFramebuffer() == targetFramebuffer) {
                    args.setRect(Rect2D::xywh(Point2::zero(), Vector2(float(w), float(h))));
                } else {
                    args.setRect(rd->viewport());
                }
                LAUNCH_SHADER("Film_zoom.*", args);
            } rd->pop2D();
        }
    } else {
        rd->push2D(targetFramebuffer); {
            Args args;
            input->setShaderArgs(args, "sourceTexture_", Sampler::video());
            args.setUniform("ySign", invertY ? -1 : 1);
            args.setUniform("yOffset", invertY ? input->height() : 0);
            args.setUniform("guardBandSize", guardBandSize);
            args.setRect(rd->viewport());
            LAUNCH_SHADER("Film_effectsDisabledBlit.pix", args);
        } rd->pop2D();
    }
#endif

    END_PROFILER_EVENT();
}


}
