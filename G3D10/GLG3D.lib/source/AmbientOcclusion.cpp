/**
 \file AmbientOcclusion.cpp
 \author Morgan McGuire and Michael Mara, NVIDIA and Williams College, http://research.nvidia.com, http://graphics.cs.williams.edu

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2013, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
#include "G3D/units.h"
#include "GLG3D/Camera.h"
#include "GLG3D/AmbientOcclusionSettings.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Shader.h"
#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/Profiler.h"
#include "GLG3D/GApp.h"

namespace G3D {

G3D_DECLARE_SYMBOL(EDGE_SHARPNESS);
G3D_DECLARE_SYMBOL(SCALE);
G3D_DECLARE_SYMBOL(R);

G3D_DECLARE_SYMBOL(USE_DERIVATIVE_BLUR);
G3D_DECLARE_SYMBOL(NUM_SAMPLES);
G3D_DECLARE_SYMBOL(NUM_SPIRAL_TURNS);
G3D_DECLARE_SYMBOL(previousMIPNumber);
G3D_DECLARE_SYMBOL(texture);

G3D_DECLARE_SYMBOL(radius);
G3D_DECLARE_SYMBOL(radius2);
G3D_DECLARE_SYMBOL(invRadius2);
G3D_DECLARE_SYMBOL(bias);
G3D_DECLARE_SYMBOL(projScale);
G3D_DECLARE_SYMBOL(CS_Z_buffer);
G3D_DECLARE_SYMBOL(intensityDivR6);
G3D_DECLARE_SYMBOL(intensity);
G3D_DECLARE_SYMBOL(source);
G3D_DECLARE_SYMBOL(axis);
G3D_DECLARE_SYMBOL(DEPTH_AND_STENCIL_buffer);

G3D_DECLARE_SYMBOL(clipInfo);
G3D_DECLARE_SYMBOL(projInfo);

G3D_DECLARE_SYMBOL(peeled_CS_Z_buffer);
G3D_DECLARE_SYMBOL(USE_DEPTH_PEEL);
G3D_DECLARE_SYMBOL(DIFFERENT_DEPTH_RESOLUTIONS);
G3D_DECLARE_SYMBOL(peeledToUnpeeledScale);

G3D_DECLARE_SYMBOL(CS_Z_PACKED_TOGETHER);
G3D_DECLARE_SYMBOL(peeledDepthBuffer);
G3D_DECLARE_SYMBOL(USE_PEELED_DEPTH_BUFFER);

G3D_DECLARE_SYMBOL(USE_NORMALS);
G3D_DECLARE_SYMBOL(normalBuffer);
G3D_DECLARE_SYMBOL(normalReadScaleBias);

G3D_DECLARE_SYMBOL(MDB_WEIGHTS);



/** This must be greater than or equal to the MAX_MIP_LEVEL  defined in AmbientOcclusion_AO.pix. */
#define MAX_MIP_LEVEL (5)

/** Used to allow us to depth test versus the sky without an explicit check, speeds up rendering when some of the skybox is visible */
#define Z_COORD (-1.0f)
    
shared_ptr<AmbientOcclusion::PerViewBuffers> AmbientOcclusion::PerViewBuffers::create() {
    shared_ptr<PerViewBuffers> buffers = shared_ptr<PerViewBuffers>(new PerViewBuffers());
    return buffers;

}

static const ImageFormat* cszBufferImageFormat(const bool twoChannelFormat, const AmbientOcclusionSettings::ZStorage zStorage) {
    if (twoChannelFormat) {
        switch (zStorage) {
        case AmbientOcclusionSettings::ZStorage::FLOAT:
            return ImageFormat::RG32F();
        case AmbientOcclusionSettings::ZStorage::HALF:
        default:
            return ImageFormat::RG16F();
        }
    } else {
        switch (zStorage) {
        case AmbientOcclusionSettings::ZStorage::FLOAT:
            return ImageFormat::R32F();
        case AmbientOcclusionSettings::ZStorage::HALF:
        default:
            return ImageFormat::R16F();
        }
    }
}


void AmbientOcclusion::PerViewBuffers::resizeBuffers(const String& name, shared_ptr<Texture> depthTexture, const shared_ptr<Texture>& peeledDepthTexture, const AmbientOcclusionSettings::ZStorage zStorage) {
    bool rebind = false;
    const int width = depthTexture->width();
    const int height = depthTexture->height();
    const ImageFormat* csZFormat = cszBufferImageFormat(notNull(peeledDepthTexture), zStorage);

    static int cszBufferIndex = 0;
    if (notNull(cszBuffer) && ((cszBuffer->width() != width) || (cszBuffer->height() != height))) {
        cszBuffer = shared_ptr<Texture>();
        cszBufferIndex = 0;
    }

    if (isNull(cszBuffer) || (csZFormat != cszBuffer->format())) {
        debugAssert(width > 0 && height > 0);
        cszBuffer = Texture::createEmpty(name + format("::cszBuffer%d", cszBufferIndex), width, height, csZFormat, Texture::DIM_2D, true);
        cszBuffer->visualization.min = -50.0f;
        cszBuffer->visualization.max = -0.1f;

        // The buffers have to be explicitly cleared or they won't allocate MIP maps on OS X
        cszBuffer->clear();
        for (int i = 0; i <= MAX_MIP_LEVEL; ++i) {
            cszFramebuffers.append(Framebuffer::create(name + "::cszFramebuffers[" + G3D::format("%d", i) + "]"));
        }
        rebind = true;
        ++cszBufferIndex;

    } else if ((cszBuffer->width() != width) || (cszBuffer->height() != height)) {
        // Resize
        cszBuffer->resize(width, height);
        rebind = true;
    }

    if (csZFormat != cszBuffer->format()) {
        cszBuffer = Texture::createEmpty(name + format("::cszBuffer%d", cszBufferIndex), width, height, csZFormat, Texture::DIM_2D, true);
        ++cszBufferIndex;
        rebind = true;
    }

    if (rebind) {
        for (int i = 0; i <= MAX_MIP_LEVEL; ++i) {
            cszFramebuffers[i]->set(Framebuffer::COLOR0, cszBuffer, CubeFace::POS_X, i);
        }
    }
}


void AmbientOcclusion::update
   (RenderDevice*                       rd,         
    const AmbientOcclusionSettings&     settings, 
    const shared_ptr<Camera>&           camera,               
    const shared_ptr<Texture>&          depthTexture,
    const shared_ptr<Texture>&          peeledDepthBuffer,   
    const shared_ptr<Texture>&          normalBuffer,
    const shared_ptr<Texture>&          ssVelocityBuffer,
    const Vector2int16                  guardBandSizev) {

    alwaysAssertM(guardBandSizev.x == guardBandSizev.y, "Guard band must be the same size in each dimension");
    const int guardBandSize = guardBandSizev.x;

    if (supported() && settings.enabled) {
        
        m_guardBandSize = guardBandSize;
        compute(rd, settings, depthTexture, camera, peeledDepthBuffer, normalBuffer, ssVelocityBuffer);
        if (settings.blurRadius != 0) {
            m_texture = m_resultBuffer;
        } else { // No blur passes, so pull out the raw buffer!
            m_texture = m_temporallyFilteredBuffer;
        }

    } else {

        m_texture = Texture::white();

    }
}


shared_ptr<AmbientOcclusion> AmbientOcclusion::create(const String& name) {
    return shared_ptr<AmbientOcclusion>(new AmbientOcclusion(name));
}


void AmbientOcclusion::initializePerViewBuffers(int size) {
    int oldSize = m_perViewBuffers.size();
    m_perViewBuffers.resize(size);
    for (int i = oldSize; i < m_perViewBuffers.size(); ++i) {
        m_perViewBuffers[i] = PerViewBuffers::create();
    }
}

void AmbientOcclusion::packBlurKeys
        (RenderDevice* rd, 
        const AmbientOcclusionSettings& settings, 
        const shared_ptr<Texture>& cszBuffer, 
        const Vector3& clipInfo,
        const shared_ptr<Texture>& normalBuffer) {
    rd->push2D(m_packedKeyBuffer); {
        Args args;
        
        // TODO: compute far plane as distance where AO radius drops to < pixel
        cszBuffer->setShaderArgs(args, "csZ_", Sampler::buffer());
        normalBuffer->setShaderArgs(args, "normal_", Sampler::buffer());
        args.setRect(rd->viewport());
        LAUNCH_SHADER_WITH_HINT(m_shaderFilenamePrefix + "packBilateralKey.pix", args, name());

    } rd->pop2D();
    
}

void AmbientOcclusion::compute
   (RenderDevice*                   rd,
    const AmbientOcclusionSettings& settings,
    const shared_ptr<Texture>&      depthBuffer, 
    const Vector3&                  clipConstant,
    const Vector4&                  projConstant,
    float                           projScale,
    const CFrame&                   currentCameraFrame,
    const CFrame&                   prevCameraFrame,
    const shared_ptr<Texture>&      peeledDepthBuffer,
    const shared_ptr<Texture>&      normalBuffer,
    const shared_ptr<Texture>&      ssVelocityBuffer) {

    BEGIN_PROFILER_EVENT("AmbientOcclusion");

    alwaysAssertM(depthBuffer, "Depth buffer is required.");
    
    int depthBufferCount = 1;

    if (notNull(peeledDepthBuffer)) {
        ++depthBufferCount;
    }

    initializePerViewBuffers(depthBufferCount);
    resizeBuffers(depthBuffer, settings.packBlurKeys);
#   if COMBINE_CSZ_INTO_ONE_TEXTURE == 1
        m_perViewBuffers[0]->resizeBuffers(m_name, depthBuffer, settings.useDepthPeelBuffer ? peeledDepthBuffer : shared_ptr<Texture>(), settings.zStorage);
        computeCSZ(rd, m_perViewBuffers[0]->cszFramebuffers, m_perViewBuffers[0]->cszBuffer, settings, depthBuffer, clipConstant, peeledDepthBuffer);
#   else
        shared_ptr<Texture> depthTexture = depthBuffer;
        for (int i = 0; i < m_perViewBuffers.size(); ++i) {
            m_perViewBuffers[i]->resizeBuffers(depthTexture, shared_ptr<Texture>());
            computeCSZ(rd, m_perViewBuffers[i]->cszFramebuffers, m_perViewBuffers[i]->cszBuffer, settings, depthTexture, clipConstant, shared_ptr<Texture>());
            depthTexture = peeledDepthBuffer;
        }
#   endif

#   if COMBINE_CSZ_INTO_ONE_TEXTURE == 1
       shared_ptr<Texture> depthPeelCSZ = m_perViewBuffers[0]->cszBuffer;
#   else
       shared_ptr<Texture> depthPeelCSZ = notNull(peeledDepthBuffer) ? m_perViewBuffers[1]->cszBuffer : shared_ptr<Texture>();
#   endif

    computeRawAO(rd, settings, depthBuffer, clipConstant, projConstant, projScale, m_perViewBuffers[0]->cszBuffer, depthPeelCSZ, normalBuffer);
    if (notNull(ssVelocityBuffer) && (settings.temporalFilterSettings.hysteresis > 0.0f)) {
        m_temporallyFilteredBuffer = m_temporalFilter.apply(rd, clipConstant, projConstant, currentCameraFrame, prevCameraFrame, m_rawAOBuffer, 
        depthBuffer, ssVelocityBuffer, Vector2(float(m_guardBandSize), float(m_guardBandSize)), 1, settings.temporalFilterSettings);
    } else {
        m_temporallyFilteredBuffer = m_rawAOBuffer;
    }
    

    if (settings.blurRadius != 0) {
        if (settings.packBlurKeys) {
            packBlurKeys(rd, settings, m_perViewBuffers[0]->cszBuffer, clipConstant, normalBuffer);
        }

        BEGIN_PROFILER_EVENT("Blur");
        alwaysAssertM(settings.blurRadius >= 0 && settings.blurRadius <= 6, "The AO blur radius must be a nonnegative number, 6 or less");
        alwaysAssertM(settings.blurStepSize > 0, "Must use a positive blur step size");
        blurHorizontal(rd, settings, depthBuffer, projConstant, normalBuffer);
        blurVertical(rd, settings, depthBuffer, projConstant, normalBuffer);
        END_PROFILER_EVENT();
    } // else the result is still in the m_temporallyFilteredBuffer 

    END_PROFILER_EVENT();
}


void AmbientOcclusion::resizeBuffers(const shared_ptr<Texture>& depthTexture, bool packKeys) {
    bool rebind = false;
    const int width = depthTexture->width();
    const int height = depthTexture->height();

    const ImageFormat* intermediateFormat = packKeys ? ImageFormat::R8() : ImageFormat::RG16F();

    if (isNull(m_rawAOFramebuffer)) {
        // Allocate for the first call  
        m_rawAOFramebuffer    = Framebuffer::create(m_name + "::m_rawAOFramebuffer");
        m_hBlurredFramebuffer = Framebuffer::create(m_name + "::m_hBlurredFramebuffer");
        m_resultFramebuffer   = Framebuffer::create(m_name + "::m_resultFramebuffer");

        // Using RG16F here avoids the need to pack and unpack depth values used as the bilateral key
        m_rawAOBuffer         = Texture::createEmpty(m_name + "::m_rawAOBuffer", width, height, intermediateFormat, Texture::DIM_2D);
        m_hBlurredBuffer      = Texture::createEmpty(m_name + "::m_hBlurredBuffer", width, height, intermediateFormat, Texture::DIM_2D);
        m_resultBuffer        = Texture::createEmpty(m_name + "::m_resultBuffer", width, height, GLCaps::supportsTextureDrawBuffer(ImageFormat::R8()) ? ImageFormat::R8() : ImageFormat::RGB8(), Texture::DIM_2D);
        
        m_packedKeyBuffer = Framebuffer::create(m_name + "::m_packedKeyFramebuffer");
        m_packedKeyBuffer->set(Framebuffer::COLOR0, Texture::createEmpty(m_name + "::m_packedKeyBuffer", width, height, ImageFormat::RGBA16()));
        

        rebind = true;

    } else if ((m_rawAOBuffer->width() != width) || (m_rawAOBuffer->height() != height)) {
        // Resize
        m_rawAOBuffer->resize(width, height);
        m_hBlurredBuffer->resize(width, height);
        m_packedKeyBuffer->resize(width, height);

        rebind = true;
    }

    if (intermediateFormat != m_rawAOBuffer->format()) {
        m_rawAOBuffer = Texture::createEmpty(m_name + "::m_rawAOBuffer", width, height, intermediateFormat, Texture::DIM_2D);
        m_hBlurredBuffer = Texture::createEmpty(m_name + "::m_hBlurredBuffer", width, height, intermediateFormat, Texture::DIM_2D);
        rebind = true;
    }
    

    if (rebind) {
        // Sizes have changed or just been allocated
        m_rawAOFramebuffer->set(Framebuffer::COLOR0, m_rawAOBuffer);
        m_hBlurredFramebuffer->set(Framebuffer::COLOR0, m_hBlurredBuffer);

        m_resultFramebuffer->clear();
        m_resultBuffer->resize(width, height);
        m_resultFramebuffer->set(Framebuffer::COLOR0, m_resultBuffer);
        m_resultFramebuffer->set(Framebuffer::DEPTH, depthTexture);
    }
}


static const Sampler& cszSamplerSettings() {
    static Sampler cszSettings = Sampler::buffer();
    static bool init = false;
    if (! init) {
        init = true;
        cszSettings.interpolateMode   = InterpolateMode::NEAREST_MIPMAP;
        cszSettings.maxMipMap         = MAX_MIP_LEVEL;
    }
    return cszSettings;
}


void AmbientOcclusion::computeCSZ
   (RenderDevice*                   rd, 
    const Array<shared_ptr<Framebuffer> >& cszFramebuffers,
    const shared_ptr<Texture>&      csZBuffer,
    const AmbientOcclusionSettings& settings,
    const shared_ptr<Texture>&      depthBuffer, 
    const Vector3&                  clipInfo,
    const shared_ptr<Texture>&      peeledDepthBuffer) {

    BEGIN_PROFILER_EVENT("computeCSZ");
        
    // Generate level 0
    cszFramebuffers[0]->set(Framebuffer::DEPTH, depthBuffer);
    rd->push2D(cszFramebuffers[0]); {
        rd->clear(true, false, false);
        rd->setDepthWrite(false);
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);
        Args args;
        args.append(m_uniformTable);
        args.setUniform(SYMBOL_clipInfo,                 clipInfo);
        args.setUniform(SYMBOL_DEPTH_AND_STENCIL_buffer, depthBuffer, Sampler::buffer());
        alwaysAssertM(!settings.useDepthPeelBuffer || notNull(peeledDepthBuffer),
                    "Tried to run AO with peeled depth buffer, but buffer was null");
        args.setMacro(SYMBOL_USE_PEELED_DEPTH_BUFFER, settings.useDepthPeelBuffer);
        if (settings.useDepthPeelBuffer) {
            args.setUniform(SYMBOL_peeledDepthBuffer, peeledDepthBuffer, Sampler::buffer());
        }
       
        args.setRect(rd->viewport());

        LAUNCH_SHADER_WITH_HINT(m_shaderFilenamePrefix + "reconstructCSZ.*", args, name());
    } rd->pop2D();


    // Generate the other levels (we don't have a depth texture to cull against for these)
    for (int i = 1; i <= MAX_MIP_LEVEL; ++i) {
        Args args;
        args.append(m_uniformTable);
        args.setUniform("CSZ_buffer", csZBuffer, cszSamplerSettings());
        args.setMacro(SYMBOL_USE_PEELED_DEPTH_BUFFER, settings.useDepthPeelBuffer);

        rd->push2D(cszFramebuffers[i]); {
            rd->clear();
            args.setUniform(SYMBOL_previousMIPNumber, i - 1);
            args.setRect(rd->viewport());
            LAUNCH_SHADER_WITH_HINT(m_shaderFilenamePrefix + "minify.*", args, name());
        } rd->pop2D();
    }

    END_PROFILER_EVENT();
}



void AmbientOcclusion::computeRawAO
   (RenderDevice*                   rd,
    const AmbientOcclusionSettings& settings,
    const shared_ptr<Texture>&      depthBuffer,
    const Vector3&                  clipConstant,
    const Vector4&                  projConstant,
    const float                     projScale,
    const shared_ptr<Texture>&      csZBuffer,
    const shared_ptr<Texture>&      peeledCSZBuffer,
    const shared_ptr<Texture>&      normalBuffer) {

    debugAssert(projScale > 0);
    m_rawAOFramebuffer->set(Framebuffer::DEPTH, depthBuffer);
    rd->push2D(m_rawAOFramebuffer); {

        // For quick early-out testing vs. skybox 
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);

        // Values that are never touched due to the depth test will be white
        rd->setColorClearValue(Color3::white());
        rd->clear(true, false, false);
        Args args;
        args.append(m_uniformTable);
        args.setMacro(SYMBOL_NUM_SAMPLES,    settings.numSamples);
        args.setMacro(SYMBOL_NUM_SPIRAL_TURNS, settings.numSpiralTurns());
        args.setUniform(SYMBOL_radius,       settings.radius);
        args.setUniform(SYMBOL_bias,         settings.bias);
        args.setUniform(SYMBOL_clipInfo,     clipConstant);
        args.setUniform(SYMBOL_projInfo,     projConstant);
        args.setUniform(SYMBOL_projScale,    projScale);
        args.setUniform(SYMBOL_CS_Z_buffer,  csZBuffer, cszSamplerSettings());
        args.setUniform(SYMBOL_intensityDivR6, (float)(settings.intensity / powf(settings.radius, 6.0f)));
        args.setUniform(SYMBOL_intensity, settings.intensity);
        args.setUniform(SYMBOL_radius2, square(settings.radius));
        args.setUniform(SYMBOL_invRadius2, 1.0f / square(settings.radius));
        args.setMacro("TEMPORALLY_VARY_SAMPLES", settings.temporallyVarySamples);
        
        const bool useDepthPeel = settings.useDepthPeelBuffer;
        args.setMacro(SYMBOL_USE_DEPTH_PEEL, useDepthPeel ? 1 : 0);
        if ( useDepthPeel ) {
            bool cszPackedTogether = csZBuffer == peeledCSZBuffer;
            args.setMacro(SYMBOL_CS_Z_PACKED_TOGETHER, cszPackedTogether ? 1 : 0);
            if ( !cszPackedTogether ) {
                args.setUniform(SYMBOL_peeled_CS_Z_buffer, peeledCSZBuffer, cszSamplerSettings());
                const Vector2& peeledExtent     = peeledCSZBuffer->rect2DBounds().extent();
                const Vector2& unpeeledExtent   = csZBuffer->rect2DBounds().extent();
                bool differingDepthExtents = peeledExtent != unpeeledExtent;
                args.setMacro(SYMBOL_DIFFERENT_DEPTH_RESOLUTIONS, differingDepthExtents ? 1 : 0);
                if (differingDepthExtents) {
                    //TODO: only calculate one dimension in the first place
                    args.setUniform(SYMBOL_peeledToUnpeeledScale, (peeledExtent / unpeeledExtent).x);
                }
            } else {
                args.setMacro(SYMBOL_DIFFERENT_DEPTH_RESOLUTIONS, 0);
            }
        } else {
            args.setMacro(SYMBOL_CS_Z_PACKED_TOGETHER, 0);
            args.setMacro(SYMBOL_DIFFERENT_DEPTH_RESOLUTIONS, 0);
        }
        
        if (settings.useNormalBuffer && notNull(normalBuffer)) {
            debugAssertM(normalBuffer->encoding().frame == FrameName::CAMERA, "AmbientOcclusion expects camera-space normals");
            normalBuffer->setShaderArgs(args, "normal_", Sampler::buffer());
        }

        rd->setClip2D(Rect2D::xyxy((float)m_guardBandSize, (float)m_guardBandSize, rd->viewport().width() - m_guardBandSize, rd->viewport().height() - m_guardBandSize));
        args.setRect(rd->viewport());
        LAUNCH_SHADER_WITH_HINT(m_shaderFilenamePrefix + "AO.*", args, name());
    } rd->pop2D();
}


void AmbientOcclusion::blurOneDirection
    (RenderDevice*                      rd,
    const AmbientOcclusionSettings&     settings,
    const shared_ptr<Texture>&          depthBuffer,
    const Vector4&                      projConstant,
    const shared_ptr<Texture>&          normalBuffer,
    const Vector2int16&                 axis,
    const shared_ptr<Framebuffer>&      framebuffer,
    const shared_ptr<Texture>&          source) {

    framebuffer->set(Framebuffer::DEPTH, depthBuffer);
    rd->push2D(framebuffer); {
        // For quick early-out testing vs. skybox 
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);
        rd->setColorClearValue(Color3::white());
        rd->clear(true, false, false);
        Args args;
        args.append(m_uniformTable);
        args.setUniform(SYMBOL_source,          source, Sampler::buffer());
        args.setUniform(SYMBOL_axis,            axis);

        args.setUniform(SYMBOL_projInfo, projConstant);
        args.setMacro("HIGH_QUALITY", settings.highQualityBlur);
        args.setMacro(SYMBOL_EDGE_SHARPNESS,    settings.edgeSharpness);
        args.setMacro(SYMBOL_SCALE,             settings.blurStepSize);
        args.setMacro(SYMBOL_R,                 settings.blurRadius);
        args.setMacro(SYMBOL_MDB_WEIGHTS,       settings.monotonicallyDecreasingBilateralWeights ? 1 : 0);
        
        args.setMacro("PACKED_BILATERAL_KEY",   settings.packBlurKeys ? 1 : 0);
        if (settings.packBlurKeys) {
            alwaysAssertM(settings.useNormalsInBlur && settings.useNormalBuffer, "Packed blur keys requires normals in blur");
            m_packedKeyBuffer->texture(0)->setShaderArgs(args, "packedBilateralKey_", Sampler::buffer());
        }

        if (settings.useNormalsInBlur && settings.useNormalBuffer) {
            alwaysAssertM(notNull(normalBuffer), "The normalBuffer was not allocated for use in AO");
            normalBuffer->setShaderArgs(args, "normal_", Sampler::buffer());
        } 

        rd->setClip2D(Rect2D::xyxy((float)m_guardBandSize, (float)m_guardBandSize, rd->viewport().width() - m_guardBandSize, rd->viewport().height() - m_guardBandSize));
        args.setRect(rd->viewport());

        LAUNCH_SHADER_WITH_HINT(m_shaderFilenamePrefix + "blur.*", args, name());
    } rd->pop2D();
}


void AmbientOcclusion::blurHorizontal
   (RenderDevice*                       rd,
    const AmbientOcclusionSettings&     settings,
    const shared_ptr<Texture>&          depthBuffer,
    const Vector4&                      projConstant,
    const shared_ptr<Texture>&          normalBuffer) {
    Vector2int16 axis(1, 0);
    blurOneDirection(rd, settings, depthBuffer, projConstant, normalBuffer, 
                    axis, m_hBlurredFramebuffer, m_temporallyFilteredBuffer);
}


void AmbientOcclusion::blurVertical   
   (RenderDevice*                       rd,
    const AmbientOcclusionSettings&     settings,
    const shared_ptr<Texture>&          depthBuffer,
    const Vector4&                      projConstant,
    const shared_ptr<Texture>&          normalBuffer) {
    Vector2int16 axis(0, 1);
    blurOneDirection(rd, settings, depthBuffer, projConstant, normalBuffer, 
                    axis, m_resultFramebuffer, m_hBlurredBuffer);
}


void AmbientOcclusion::compute
   (RenderDevice*                       rd,
    const AmbientOcclusionSettings&     settings,
    const shared_ptr<Texture>&          depthBuffer, 
    const shared_ptr<Camera>&           camera,
    const shared_ptr<Texture>&          peeledDepthBuffer,
    const shared_ptr<Texture>&          normalBuffer,
    const shared_ptr<Texture>&          ssVelocityBuffer) {

    const Vector3& clipConstant = camera->projection().reconstructFromDepthClipInfo();
    const Vector4& projConstant = camera->projection().reconstructFromDepthProjInfo(depthBuffer->width(), depthBuffer->height());
    
    const CFrame& currentCameraFrame    = camera->frame();
    const CFrame& prevCameraFrame       = camera->previousFrame();
    compute(rd, settings, depthBuffer, clipConstant, projConstant, (float)abs(camera->imagePlanePixelsPerMeter(rd->viewport())), 
            currentCameraFrame, prevCameraFrame, peeledDepthBuffer, normalBuffer, ssVelocityBuffer);
}



bool AmbientOcclusion::supported() {
    static bool supported = 
        // This specific card runs the AO shader really slowly for some reason
        (GLCaps::renderer().find("NVIDIA GeForce GT 330M") == String::npos);

    return supported;
}


void AmbientOcclusion::setShaderArgs(UniformTable& args, const String& prefix, const Sampler& sampler) {
    // The notNull macro is set by the texture()
    texture()->setShaderArgs(args, prefix, sampler);
    args.setUniform(prefix + "offset", Vector2int32(0, 0));
}

} // namespace GLG3D
