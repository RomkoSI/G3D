/**
 \file MotionBlur.cpp
 \maintainer Morgan McGuire


      sharpImage      sharpVelocity       z-buffer
          |                |                 |
          |                |                 |
         src#              |                 |
          |             tileMax*             |
          |                |                 |
          |            neighborMax**         |
          |                |                 |
          `----------------+-----------------'
                           |
                         output

   #   no guard band                          
   *   1/maxBlurRadius scale, after guard band is removed

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "GLG3D/MotionBlur.h"
#include "GLG3D/Texture.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Shader.h"
#include "GLG3D/MotionBlurSettings.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Draw.h"
#include "GLG3D/SlowMesh.h"

namespace G3D {
    
void MotionBlur::apply
 (RenderDevice*                 rd, 
  const shared_ptr<Texture>&    color, 
  const shared_ptr<Texture>&    velocity, 
  const shared_ptr<Texture>&    depth,
  const shared_ptr<Camera>&     camera,
  Vector2int16                  trimBandThickness) {

    if (! camera->motionBlurSettings().enabled()) {
        return;
    }

    BEGIN_PROFILER_EVENT("G3D::MotionBlur::apply");

    if (isNull(m_randomBuffer)) {
        makeRandomBuffer();
    }

    const int   dimension = (camera->fieldOfViewDirection() == FOVDirection::HORIZONTAL) ? color->width() : color->height();

    const int   maxBlurRadiusPixels     = iMax(4, iCeil(float(dimension) * camera->motionBlurSettings().maxBlurDiameterFraction() / 2.0f));
    const int   numSamplesOdd           = nextOdd(camera->motionBlurSettings().numSamples());
    const float exposureTimeFraction    = camera->motionBlurSettings().exposureFraction();

    updateBuffers(velocity, maxBlurRadiusPixels, trimBandThickness);

    shared_ptr<Texture> src;

    // Copy the input to another buffer

    if (((notNull(rd->framebuffer()) &&
        (color == rd->framebuffer()->get(Framebuffer::COLOR0)->texture())) ||
        ! trimBandThickness.isZero())) {
        // The input color buffer is the current framebuffer's draw target.
        // Make a copy so that we can read from it during the final gatherBlur pass.
        // Note that if we knew that we were performing multiple effects at the same time
        // (e.g., Film, DepthOfField, and MotionBlur), we could avoid this copy by 
        // connecting the output of one to the input of the next.

        if (isNull(m_cachedSrc) || (m_cachedSrc->format() != color->format())) {
            // Reallocate the underlying texture
            bool generateMipMaps = false;
            m_cachedSrc = Texture::createEmpty("G3D::MotionBlur::src", color->width() - trimBandThickness.x * 2, color->height() - trimBandThickness.y * 2, color->format(), Texture::DIM_2D, generateMipMaps);
        } else {
            m_cachedSrc->resize(color->width() - trimBandThickness.x * 2, color->height() - trimBandThickness.y * 2);
        }

        src = m_cachedSrc;

        // Copy and strip the trim band
        Texture::copy(color, src, 0, 0, 1.0f, trimBandThickness, CubeFace::POS_X, CubeFace::POS_X, rd, false);

    } else {
        src = color;
    }

    computeTileMinMax(rd, velocity, maxBlurRadiusPixels, trimBandThickness);
    computeNeighborMinMax(rd, m_tileMinMaxFramebuffer->texture(0));
    gatherBlur(rd, src, m_neighborMinMaxFramebuffer->texture(0), velocity, depth, numSamplesOdd, maxBlurRadiusPixels, exposureTimeFraction, trimBandThickness);
            
    if (m_debugShowTiles) {
        rd->push2D(); {
            debugDrawTiles(rd, m_neighborMinMaxFramebuffer->texture(0), maxBlurRadiusPixels);
        } rd->pop2D();
    }

    END_PROFILER_EVENT();
}


void MotionBlur::computeTileMinMax
  (RenderDevice*                rd, 
   const shared_ptr<Texture>&   velocity,        
   int                          maxBlurRadiusPixels,
   const Vector2int16           trimBandThickness) {

    Args args;
    GBuffer::bindReadArgs
        (args,
        GBuffer::Field::SS_EXPRESSIVE_MOTION,
        velocity);

    GBuffer::bindWriteUniform
        (args,
        GBuffer::Field::SS_EXPRESSIVE_MOTION,
        velocity->encoding());

    args.setMacro("maxBlurRadius", maxBlurRadiusPixels);

    // Horizontal pass
    rd->push2D(m_tileMinMaxTempFramebuffer); {
        rd->clear();
        args.setUniform("inputShift", Vector2(trimBandThickness));
        args.setMacro("INPUT_HAS_MIN_SPEED", 0);
        args.setRect(rd->viewport());
		LAUNCH_SHADER("MotionBlur_tileMinMax.*", args);
    } rd->pop2D();

    // Vertical pass
    GBuffer::bindReadArgs
        (args,
        GBuffer::Field::SS_EXPRESSIVE_MOTION,
        m_tileMinMaxTempFramebuffer->texture(0));

    rd->push2D(m_tileMinMaxFramebuffer); {
        rd->clear();
        args.setUniform("inputShift", Vector2::zero());
        args.setMacro("INPUT_HAS_MIN_SPEED", 1);
        args.setRect(rd->viewport());
		LAUNCH_SHADER("MotionBlur_tileMinMax.*", args);
    } rd->pop2D();
}


void MotionBlur::computeNeighborMinMax
    (RenderDevice*              rd,
     const shared_ptr<Texture>& tileMax) {

    rd->push2D(m_neighborMinMaxFramebuffer); {

        rd->setColorClearValue(Color4::zero());
        rd->clear(true, false, false);

        Args args;
        GBuffer::bindReadArgs
           (args,
            GBuffer::Field::SS_EXPRESSIVE_MOTION,
            tileMax);

        GBuffer::bindWriteUniform
           (args,
            GBuffer::Field::SS_EXPRESSIVE_MOTION,
            tileMax->encoding());

        args.setRect(rd->viewport());
        LAUNCH_SHADER("MotionBlur_neighborMinMax.*", args);

    } rd->pop2D();
}


void MotionBlur::gatherBlur
   (RenderDevice*                     rd, 
    const shared_ptr<Texture>&        color,
    const shared_ptr<Texture>&        neighborMax,
    const shared_ptr<Texture>&        velocity,        
    const shared_ptr<Texture>&        depth,
    int                               numSamplesOdd,
    int                               maxBlurRadiusPixels,
    float                             exposureTimeFraction,
    Vector2int16                      trimBandThickness) {
        
    // Switch to 2D mode using the current framebuffer
    rd->push2D(); {
        rd->clear(true, false, false);
        rd->setGuardBandClip2D(trimBandThickness);

        Args args;

        GBuffer::bindReadArgs
           (args,
            GBuffer::Field::SS_EXPRESSIVE_MOTION,
            velocity);

        neighborMax->setShaderArgs(args, "neighborMinMax_", Sampler::buffer());

        args.setUniform("colorBuffer",                color, Sampler::buffer());
        args.setUniform("randomBuffer",               m_randomBuffer, Sampler::buffer());
        args.setUniform("exposureTime",               exposureTimeFraction);

        args.setMacro("numSamplesOdd",                numSamplesOdd);
        args.setMacro("maxBlurRadius",                maxBlurRadiusPixels);

        args.setUniform("depthBuffer",                depth, Sampler::buffer());

        args.setUniform("trimBandThickness",          trimBandThickness);
        
        args.setRect(rd->viewport());
		LAUNCH_SHADER("MotionBlur_gather.*", args);

    } rd->pop2D();
}


void MotionBlur::updateBuffers(const shared_ptr<Texture>& velocityTexture, int maxBlurRadiusPixels, Vector2int16 inputGuardBandThickness) {
    const int w = (velocityTexture->width()  - inputGuardBandThickness.x * 2);
    const int h = (velocityTexture->height() - inputGuardBandThickness.y * 2);

	// Tile boundaries will appear if the tiles are not radius x radius
    const int smallWidth  = iCeil(w / float(maxBlurRadiusPixels));
    const int smallHeight = iCeil(h / float(maxBlurRadiusPixels));

    if (isNull(m_tileMinMaxFramebuffer)) {
        const bool generateMipMaps = false;
        Texture::Encoding encoding = velocityTexture->encoding();

        // Add a "G" channel
        if (encoding.format->numberFormat == ImageFormat::FLOATING_POINT_FORMAT) {
            encoding.format = ImageFormat::RGB16F();
        } else {
            encoding.format = ImageFormat::RGB8();
        }
        // Ensure consistent mapping across the new G channel
        encoding.readMultiplyFirst.g = encoding.readMultiplyFirst.r;
        encoding.readAddSecond.g     = encoding.readAddSecond.r;

        m_tileMinMaxTempFramebuffer = Framebuffer::create(Texture::createEmpty("G3D::MotionBlur::m_tileMinMaxTempFramebuffer", h, smallWidth, encoding, Texture::DIM_2D, generateMipMaps));
        m_tileMinMaxTempFramebuffer->texture(0)->visualization = Texture::Visualization::unitVector();

        m_tileMinMaxFramebuffer = Framebuffer::create(Texture::createEmpty("G3D::MotionBlur::m_tileMinMaxFramebuffer", smallWidth, smallHeight, encoding, Texture::DIM_2D, generateMipMaps));
        m_tileMinMaxFramebuffer->texture(0)->visualization = Texture::Visualization::unitVector();

        m_neighborMinMaxFramebuffer = Framebuffer::create(Texture::createEmpty("G3D::MotionBlur::m_neighborMaxFramebuffer", smallWidth, smallHeight, encoding, Texture::DIM_2D, generateMipMaps));
        m_neighborMinMaxFramebuffer->texture(0)->visualization = m_tileMinMaxFramebuffer->texture(0)->visualization;
    }

    // Resize if needed
    m_tileMinMaxFramebuffer->resize(smallWidth, smallHeight);
    m_tileMinMaxTempFramebuffer->resize(h, smallWidth);
    m_neighborMinMaxFramebuffer->resize(smallWidth, smallHeight);
}


MotionBlur::MotionBlur() : 
    m_debugShowTiles(false) {
}


void MotionBlur::makeRandomBuffer() {
    static const int N = 32;
    Color3unorm8 buf[N * N];
    Random rnd;

    for (int i = N * N - 1; i >= 0; --i) {
        Color3unorm8& p = buf[i];
        p.r = unorm8::fromBits(rnd.integer(0, 255));
    }
    const bool generateMipMaps = false;
    m_randomBuffer = Texture::fromMemory("randomBuffer", buf, ImageFormat::RGB8(),
        N, N, 1, 1, ImageFormat::R8(), Texture::DIM_2D, generateMipMaps);
}


void MotionBlur::debugDrawTiles
   (RenderDevice*               rd,
    const shared_ptr<Texture>&  neighborMax,
    int                         maxBlurRadiusPixels) {

    // read back the neighborhood velocity for each tile
    const shared_ptr<Image> cpuNeighborMax = neighborMax->toImage();

    // Draw tile boundaries
	{
		SlowMesh mesh(PrimitiveType::LINES);
		mesh.setColor(Color3::black());
        
		for (int x = 0; x < rd->width(); x += maxBlurRadiusPixels) {
            mesh.makeVertex(Point2((float)x, 0));
            mesh.makeVertex(Point2((float)x, float(rd->height())));
        }
        
		for (int y = 0; y < rd->height(); y += maxBlurRadiusPixels) {
            mesh.makeVertex(Point2(0, (float)y));
            mesh.makeVertex(Point2(float(rd->width()), (float)y));
        }

		mesh.render(rd);
    }

    // Show velocity vectors
	{
		SlowMesh mesh(PrimitiveType::LINES);
		mesh.setColor(Color3::white());

        for (int x = 0; x < cpuNeighborMax->width(); ++x) {
            for (int y = 0; y < cpuNeighborMax->height(); ++y) {

                const Point2& center = Point2(x + 0.5f, y + 0.5f) * (float)maxBlurRadiusPixels;
                mesh.makeVertex(center); 

                const Vector3& N = Vector3(cpuNeighborMax->get<Color3>(x, y) * neighborMax->encoding().readMultiplyFirst.rgb() + neighborMax->encoding().readAddSecond.rgb());            
                mesh.makeVertex(center + N.xy());
            }
        }
		mesh.render(rd);
    }
}

} // G3D
