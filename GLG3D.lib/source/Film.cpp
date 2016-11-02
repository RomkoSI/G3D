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
    return createShared<Film>();
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

    // Build the filter chain (in forward order):
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
    
    END_PROFILER_EVENT();
}


}
