/**
 \file DeepGBufferRadiosity.cpp
 \author Michael Mara, NVIDIA, http://research.nvidia.com

 */
#include "DeepGBufferRadiosity.h"

/** Floating point bits per pixel for CSZ: 16 or 32. */
#define ZBITS (32)

/** This must be greater than or equal to the MAX_MIP_LEVEL and  defined in AmbientOcclusion_AO.pix. */
#define MAX_MIP_LEVEL (5)

/** Used to allow us to depth test versus the sky without an explicit check, speeds up rendering when some of the skybox is visible */
#define Z_COORD (-1.0f)
    

static const ImageFormat* cszBufferImageFormat(bool twoChannelFormat) {
    return twoChannelFormat ? 
                ((ZBITS == 16) ? ImageFormat::RG16F() : ImageFormat::RG32F()) :
                // R16F is too low-precision, but we provide it as a fallback
                (ZBITS == 16) ? 

                    (GLCaps::supportsTextureDrawBuffer(ImageFormat::R16F()) ? ImageFormat::R16F() : ImageFormat::L16F()) :

                    (GLCaps::supportsTextureDrawBuffer(ImageFormat::R32F()) ? 
                        ImageFormat::R32F() : 
                        (GLCaps::supportsTextureDrawBuffer(ImageFormat::L32F()) ? 
                            ImageFormat::L32F() :
                            ImageFormat::RG32F()));
}


static const ImageFormat* colorImageFormat(bool useHalfPrecisionColor) { 
    return useHalfPrecisionColor ? ImageFormat::RGBA16F() : ImageFormat::RGBA32F();
}


static const ImageFormat* colorInputImageFormat(bool useHalfPrecisionColor) { 
    return useHalfPrecisionColor ? ImageFormat::R11G11B10F() : ImageFormat::RGBA32F();
}

static const ImageFormat* normalImageFormat(bool useOct16) {
    return useOct16 ? ImageFormat::RGBA8() : ImageFormat::RGB10A2();
}


static Sampler cszSettings() {
    Sampler cszSettings = Sampler::buffer();
    cszSettings.interpolateMode   = InterpolateMode::NEAREST_MIPMAP;
    cszSettings.maxMipMap         = MAX_MIP_LEVEL + 1;
    return cszSettings;
}

void DeepGBufferRadiosity::MipMappedBuffers::initializeTextures(int width, int height, const ImageFormat* csZFormat, const ImageFormat* colorFormat, const ImageFormat* normalFormat) {
    cszBuffer           = Texture::createEmpty("DeepGBufferRadiosity::cszBuffer", width, height, csZFormat, Texture::DIM_2D, true);
    frontColorBuffer    = Texture::createEmpty("DeepGBufferRadiosity::frontColorBuffer", width, height, colorFormat, Texture::DIM_2D, true);
    peeledColorBuffer   = Texture::createEmpty("DeepGBufferRadiosity::peeledColorBuffer", width, height, colorFormat, Texture::DIM_2D, true);
    normalBuffer        = Texture::createEmpty("DeepGBufferRadiosity::normalBuffer", width, height, normalFormat, Texture::DIM_2D, true);
    peeledNormalBuffer  = Texture::createEmpty("DeepGBufferRadiosity::peeledNormalBuffer(unused if Oct16 enabled)", width, height, normalFormat, Texture::DIM_2D, true);
    // The buffers have to be explicitly cleared or they won't allocate MIP maps on OS X
    cszBuffer->clear();
    frontColorBuffer->clear();
    peeledColorBuffer->clear();
    normalBuffer->clear();
    peeledNormalBuffer->clear();
}


void DeepGBufferRadiosity::MipMappedBuffers::prepare(const shared_ptr<Texture>& depthTexture, const shared_ptr<Texture>& frontColorLayer, const shared_ptr<Texture>& frontNormalLayer,
                                               const shared_ptr<Texture>& peeledDepthTexture, const shared_ptr<Texture>& peeledColorLayer, 
                                               const shared_ptr<Texture>& peeledNormalLayer, bool useOct16Normals, bool useHalfPrecisionColors) {
    debugAssert(depthTexture->rect2DBounds() == frontColorLayer->rect2DBounds());
    bool rebind = false;
    const int width = depthTexture->width();
    const int height = depthTexture->height();
    m_hasPeeledLayer = notNull(peeledDepthTexture) && notNull(peeledColorLayer) && notNull(peeledNormalLayer);
    const ImageFormat* csZFormat = cszBufferImageFormat(m_hasPeeledLayer);
    const ImageFormat* normalFormat = normalImageFormat(useOct16Normals);
    const ImageFormat* colorFormat = colorInputImageFormat(useHalfPrecisionColors);
    if (notNull(cszBuffer) &&((cszBuffer->width() != width) || (cszBuffer->height() != height))) {
        cszBuffer = shared_ptr<Texture>();
    }

    
    if (isNull(cszBuffer)) {
        alwaysAssertM(ZBITS == 16 || ZBITS == 32, "Only ZBITS = 16 and 32 are supported.");
        debugAssert(width > 0 && height > 0);
        initializeTextures(width, height, csZFormat, colorFormat, normalFormat);

        rebind = true;

    } else if ((cszBuffer->width() != width) || (cszBuffer->height() != height)) {
        // Resize
        resize(width, height);
        rebind = true;
    }

    if ( csZFormat != cszBuffer->format() ) {
        cszBuffer           = Texture::createEmpty("DeepGBufferRadiosity::cszBuffer", width, height, csZFormat, Texture::DIM_2D, true);
        cszBuffer->clear();
        rebind = true;
    }

    if ( normalFormat != normalBuffer->format() ) {
        normalBuffer            = Texture::createEmpty("DeepGBufferRadiosity::normalBuffer", width, height, normalFormat, Texture::DIM_2D, true);
        normalBuffer->clear();
        peeledNormalBuffer      = Texture::createEmpty("DeepGBufferRadiosity::peeledNormalBuffer", width, height, normalFormat, Texture::DIM_2D, true);
        peeledNormalBuffer->clear();
        rebind = true;
    }

    if ( colorFormat != frontColorBuffer->format() ) {
        frontColorBuffer    = Texture::createEmpty("DeepGBufferRadiosity::frontColorBuffer", width, height, colorFormat, Texture::DIM_2D, true);
        frontColorBuffer->clear();
        peeledColorBuffer  = Texture::createEmpty("DeepGBufferRadiosity::peeledColorBuffer", width, height, colorFormat, Texture::DIM_2D, true);
        peeledColorBuffer->clear();
        rebind = true;
    }

    if (rebind) {
        m_framebuffers.fastClear(); 
        for (int i = 0; i <= MAX_MIP_LEVEL; ++i) {
            m_framebuffers.append(Framebuffer::create("DeepGBufferRadiosity::m_framebuffers[" + G3D::format("%d", i) + "]"));
            m_framebuffers[i]->set(Framebuffer::COLOR0, frontColorBuffer,   CubeFace::POS_X, i);
            m_framebuffers[i]->set(Framebuffer::COLOR1, normalBuffer,       CubeFace::POS_X, i);
            m_framebuffers[i]->set(Framebuffer::COLOR2, cszBuffer,          CubeFace::POS_X, i);
            m_framebuffers[i]->set(Framebuffer::COLOR3, peeledColorBuffer,  CubeFace::POS_X, i);
            m_framebuffers[i]->set(Framebuffer::COLOR4, peeledNormalBuffer, CubeFace::POS_X, i);
        }
    } 
}

void DeepGBufferRadiosity::MipMappedBuffers::computeFullRes(RenderDevice* rd, const shared_ptr<Texture>& depthTexture, const shared_ptr<Texture>& frontColorLayer, const shared_ptr<Texture>& frontNormalLayer,
                const shared_ptr<Texture>& peeledDepthTexture, const shared_ptr<Texture>& peeledColorLayer,   const shared_ptr<Texture>& peeledNormalLayer, const Vector3& clipInfo, bool useOct16Normals) {
    rd->push2D(m_framebuffers[0]); {
        Args args;
        args.setUniform("clipInfo",                 clipInfo);
        args.setUniform("DEPTH_AND_STENCIL_buffer", depthTexture, Sampler::buffer());
        args.setMacro("USE_OCT16", useOct16Normals ? 1 : 0);
        args.setMacro("USE_PEELED_BUFFERS", m_hasPeeledLayer ? 1 : 0);
        if (m_hasPeeledLayer) {
            args.setUniform("peeledDepthBuffer", peeledDepthTexture, Sampler::buffer());
            args.setUniform("peeledColorBuffer", peeledColorLayer, Sampler::buffer());
            args.setUniform("peeledNormalBuffer", peeledNormalLayer, Sampler::buffer());
        }
        frontColorLayer->setShaderArgs(args, "colorBuffer_", Sampler::buffer());
        frontNormalLayer->setShaderArgs(args, "normal_", Sampler::buffer());
        args.setRect(rd->viewport());
        
        LAUNCH_SHADER("DeepGBufferRadiosity_reconstructCSZ.*", args);

    } rd->pop2D();
}

void DeepGBufferRadiosity::MipMappedBuffers::computeMipMaps(RenderDevice* rd, bool hasPeeledLayer, bool useOct16Normals) {
    // Generate the other levels
    Args args;
    args.setUniform("CS_Z_buffer", cszBuffer,          Sampler::buffer());
    frontColorBuffer->setShaderArgs(args, "colorBuffer_", Sampler::buffer());
    normalBuffer->setShaderArgs(args, "normal_", Sampler::buffer());
    args.setMacro("USE_OCT16", useOct16Normals ? 1 : 0);
    if (hasPeeledLayer) {
        args.setUniform("peeledColorBuffer", peeledColorBuffer, Sampler::buffer());
        args.setUniform("peeledNormalBuffer", peeledNormalBuffer, Sampler::buffer());
    }
    args.setMacro("HAS_PEELED_BUFFER", hasPeeledLayer);
    for (int i = 1; i <= MAX_MIP_LEVEL; ++i) {
        rd->push2D(m_framebuffers[i]); {
            rd->clear();
            args.setUniform("previousMIPNumber", i - 1);
            args.setRect(rd->viewport());
            LAUNCH_SHADER("DeepGBufferRadiosity_minify.*", args);
        } rd->pop2D();
    }
}

void DeepGBufferRadiosity::MipMappedBuffers::compute(RenderDevice* rd, const shared_ptr<Texture>& depthTexture, const shared_ptr<Texture>& frontColorLayer, const shared_ptr<Texture>& frontNormalLayer,
                                               const shared_ptr<Texture>& peeledDepthTexture, const shared_ptr<Texture>& peeledColorLayer,  const shared_ptr<Texture>& peeledNormalLayer,
                                               const Vector3& clipInfo, bool useOct16Normals, bool useHalfPrecisionColors) {
    prepare(depthTexture, frontColorLayer, frontNormalLayer, peeledDepthTexture, peeledColorLayer, peeledNormalLayer, useOct16Normals, useHalfPrecisionColors);
    computeFullRes(rd, depthTexture, frontColorLayer, frontNormalLayer, peeledDepthTexture, peeledColorLayer, peeledNormalLayer, clipInfo, useOct16Normals);
    computeMipMaps(rd, m_hasPeeledLayer, useOct16Normals);
}


void DeepGBufferRadiosity::MipMappedBuffers::setArgs(Args& args) const {
    args.setUniform("CS_Z_buffer",  cszBuffer, Sampler::buffer());
    args.setMacro("USE_DEPTH_PEEL", m_hasPeeledLayer ? 1 : 0);
    frontColorBuffer->setShaderArgs(args, "colorBuffer_", Sampler::buffer()); 
    normalBuffer->setShaderArgs(args, "normal_", Sampler::buffer());

    if (m_hasPeeledLayer) {
        args.setUniform("peeledNormalBuffer", peeledNormalBuffer, Sampler::buffer());
        args.setUniform("peeledColorBuffer", peeledColorBuffer, Sampler::buffer());
    }
}


static bool willComputePeeledLayer(const DeepGBufferRadiositySettings& settings) {
    return settings.useDepthPeelBuffer && settings.computePeeledLayer;
}


static void computeNextBounceBuffer
    (RenderDevice*                  rd, 
    const shared_ptr<Framebuffer>&  fb, 
    const shared_ptr<Texture>&      lambertianBuffer, 
    const shared_ptr<Texture>&      directBuffer,
    const shared_ptr<Texture>&      ssiiResult, 
    const DeepGBufferRadiositySettings&      settings) {

    rd->push2D(fb); {
        Args args;
        args.setUniform("lambertianBuffer", lambertianBuffer, Sampler::buffer());
        args.setUniform("directBuffer", directBuffer, Sampler::buffer());
        args.setUniform("indirectBuffer", ssiiResult, Sampler::buffer()); 
        args.setUniform("saturatedLightBoost",  settings.saturatedBoost);
        args.setUniform("unsaturatedLightBoost",settings.unsaturatedBoost);
        args.setRect(rd->viewport());
        LAUNCH_SHADER("DeepGBufferRadiosity_nextBounce.*", args);
    } rd->pop2D();
}


shared_ptr<Texture> DeepGBufferRadiosity::getActualResultTexture(const DeepGBufferRadiositySettings& settings, bool peeled) {
    if (peeled) {
        if (settings.blurRadius != 0) {
            return m_resultPeeledBuffer;
        } else { // No blur passes, so pull out the raw buffer!
            return m_rawIIPeeledBuffer;
        }
    } else {
        if (settings.blurRadius != 0) {
            return m_resultBuffer;
        } else { // No blur passes, so pull out the raw buffer!
            return m_temporallyFilteredResult;
        }
    }
}


void DeepGBufferRadiosity::update
   (RenderDevice*                       rd,         
    const DeepGBufferRadiositySettings&          settings, 
    const shared_ptr<Camera>&           camera,               
    const shared_ptr<Texture>&          depthTexture,
    const shared_ptr<Texture>&          previousBounceBuffer, 
    const shared_ptr<Texture>&          peeledDepthBuffer,   
    const shared_ptr<Texture>&          peeledColorBuffer,  
    const shared_ptr<Texture>&          normalBuffer,
    const shared_ptr<Texture>&          peeledNormalBuffer,
    const shared_ptr<Texture>&          lambertianBuffer,
    const shared_ptr<Texture>&          peeledLambertianBuffer,
    const Vector2int16                  inputGuardBandSizev,
    const Vector2int16                  outputGuardBandSizev,
    const shared_ptr<GBuffer>&          gbuffer,
    const shared_ptr<Scene>&            scene) {
    alwaysAssertM(settings.numBounces >= 0, "Can't have negative bounces of light!");
    alwaysAssertM(inputGuardBandSizev.x == inputGuardBandSizev.y, "Guard band must be the same size in each dimension");
    alwaysAssertM(outputGuardBandSizev.x == outputGuardBandSizev.y, "Guard band must be the same size in each dimension");

    if (settings.enabled) {
        int currentBounce = 1;
        m_inputGuardBandSize = inputGuardBandSizev.x;
        m_outputGuardBandSize = outputGuardBandSizev.y;

        bool computePeeledLayer = willComputePeeledLayer(settings);
        DeepGBufferRadiositySettings currentSettings(settings);
        float originalTemporalAlpha = currentSettings.temporalFilterSettings.hysteresis;

        if (currentSettings.numBounces > 1) {
            // Don't temporally filter until the end.
            currentSettings.temporalFilterSettings.hysteresis = 0.0f;
        }

        compute(rd, currentSettings, depthTexture, previousBounceBuffer, camera,  peeledDepthBuffer, peeledColorBuffer, normalBuffer, peeledNormalBuffer, computePeeledLayer, gbuffer, scene);
        m_texture = getActualResultTexture(currentSettings, false);
        m_peeledTexture = computePeeledLayer ? getActualResultTexture(currentSettings, true) : shared_ptr<Texture>();

        for (int i = 1; i < settings.numBounces; ++i) {
            currentBounce = i + 1;
            if (currentBounce == settings.numBounces) { // Filter on the final bounce
                currentSettings.temporalFilterSettings.hysteresis = originalTemporalAlpha;
            }

            alwaysAssertM(!computePeeledLayer || notNull(peeledLambertianBuffer), "If doing multiple DeepGBufferRadiosity bounces requiring peeled layer, must pass in a peeled lambertian buffer");
            alwaysAssertM(notNull(lambertianBuffer), "If doing multiple DeepGBufferRadiosity bounces, must pass in a lambertian buffer");

            static shared_ptr<Texture> nextBounceBuffer = Texture::createEmpty("DeepGBufferRadiosity::nextBounceBuffer", previousBounceBuffer->width(), previousBounceBuffer->height(), previousBounceBuffer->format()); 
            static shared_ptr<Framebuffer> nextBounceFB = Framebuffer::create(nextBounceBuffer);

            nextBounceBuffer->resize(previousBounceBuffer->width(), previousBounceBuffer->height());

            static shared_ptr<Texture> nextBouncePeeledBuffer = Texture::createEmpty("DeepGBufferRadiosity::nextBouncePeeledBuffer", previousBounceBuffer->width(), previousBounceBuffer->height(), previousBounceBuffer->format()); 
            static shared_ptr<Framebuffer> nextBouncePeeledFB = Framebuffer::create(nextBouncePeeledBuffer);
            nextBouncePeeledBuffer->resize(previousBounceBuffer->width(), previousBounceBuffer->height());
            
            computeNextBounceBuffer(rd, nextBounceFB, lambertianBuffer, previousBounceBuffer, m_texture, currentSettings);

            if (computePeeledLayer) {
                computeNextBounceBuffer(rd, nextBouncePeeledFB, peeledLambertianBuffer, peeledColorBuffer, m_peeledTexture, currentSettings);
            } 
            
            compute(rd, currentSettings, depthTexture, nextBounceBuffer, camera,  peeledDepthBuffer, computePeeledLayer ? nextBouncePeeledBuffer : peeledColorBuffer, normalBuffer, peeledNormalBuffer,  computePeeledLayer, gbuffer, scene);
            m_texture = getActualResultTexture(currentSettings, false);
            m_peeledTexture = computePeeledLayer ? getActualResultTexture(currentSettings, true) : shared_ptr<Texture>();
        }

    } else {

        m_texture = Texture::white();

    }
}


void DeepGBufferRadiosity::update
   (RenderDevice*                   rd,
    const DeepGBufferRadiositySettings&      settings, 
    const shared_ptr<GBuffer>&      gBuffer,
    const shared_ptr<Texture>&      previousBounceBuffer,
    const shared_ptr<GBuffer>&      peeledGBuffer,
    const shared_ptr<Texture>&      peeledPreviousBounceBuffer,
    const Vector2int16              inputGuardBandSize,
    const Vector2int16              outputGuardBandSize,
    const shared_ptr<Scene>         scene) {

    const shared_ptr<Texture>&  depthTexture            = gBuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL);
    const shared_ptr<Texture>&  peeledDepthBuffer       = notNull(peeledGBuffer) ? peeledGBuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL) : shared_ptr<Texture>();
    const shared_ptr<Texture>&  normalBuffer            = gBuffer->texture(GBuffer::Field::CS_NORMAL);
    const shared_ptr<Texture>&  peeledNormalBuffer      = notNull(peeledGBuffer) ? peeledGBuffer->texture(GBuffer::Field::CS_NORMAL) : shared_ptr<Texture>();
    const shared_ptr<Texture>&  lambertianBuffer        = gBuffer->texture(GBuffer::Field::LAMBERTIAN);
    const shared_ptr<Texture>&  peeledLambertianBuffer  = notNull(peeledGBuffer) ? peeledGBuffer->texture(GBuffer::Field::LAMBERTIAN) : shared_ptr<Texture>();
    update(rd, settings, gBuffer->camera(), depthTexture, previousBounceBuffer, peeledDepthBuffer, 
        peeledPreviousBounceBuffer, normalBuffer, peeledNormalBuffer, lambertianBuffer, peeledLambertianBuffer, inputGuardBandSize, outputGuardBandSize, gBuffer, scene);
}


shared_ptr<DeepGBufferRadiosity> DeepGBufferRadiosity::create() {
    return shared_ptr<DeepGBufferRadiosity>(new DeepGBufferRadiosity());
}


void DeepGBufferRadiosity::compute
   (RenderDevice*                   rd,
    const DeepGBufferRadiositySettings&      settings,
    const shared_ptr<Texture>&      depthBuffer, 
    const shared_ptr<Texture>&      colorBuffer,
    const Vector3&                  clipConstant,
    const Vector4&                  projConstant,
    float                           projScale,
    const Matrix4&                  projectionMatrix,
    const shared_ptr<Texture>&      peeledDepthBuffer,
    const shared_ptr<Texture>&      peeledColorBuffer,
    const shared_ptr<Texture>&      normalBuffer,
    const shared_ptr<Texture>&      peeledNormalBuffer,
    bool                            computePeeledLayer,
    const shared_ptr<GBuffer>&      gbuffer,
    const shared_ptr<Scene>&        scene) {

    alwaysAssertM(depthBuffer, "Depth buffer is required.");

    BEGIN_PROFILER_EVENT("DeepGBufferRadiosity"); {

        BEGIN_PROFILER_EVENT("Buffer Preparation"); {
            resizeBuffers(depthBuffer, settings.useHalfPrecisionColors);
            m_mipMappedBuffers.compute(rd, depthBuffer, colorBuffer, normalBuffer, peeledDepthBuffer, peeledColorBuffer, peeledNormalBuffer, clipConstant, settings.useOct16, settings.useHalfPrecisionColors);
        } END_PROFILER_EVENT();
    
        computeRawII(rd, settings, depthBuffer, clipConstant, projConstant, projScale, projectionMatrix, m_mipMappedBuffers, computePeeledLayer);

        BEGIN_PROFILER_EVENT("Reconstruction Filter"); {
            // +1 avoids issues with bilinear filtering into the actual guard band
            const float r = max(float(m_inputGuardBandSize), float(m_outputGuardBandSize) * (1.0f - settings.computeGuardBandFraction))+1;
            m_temporallyFilteredResult = m_temporalFilter.apply(rd, clipConstant, projConstant, gbuffer->camera()->frame(), 
                    gbuffer->camera()->previousFrame(), m_rawIIBuffer, depthBuffer, gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE), 
                    Vector2(r, r), 4, settings.temporalFilterSettings);
    

            if (settings.blurRadius != 0) {
                alwaysAssertM(settings.blurRadius >= 0, "The AO blur radius must be a nonnegative number/");
                alwaysAssertM(settings.blurStepSize > 0, "Must use a positive blur step size");
                blurHorizontal(rd, settings,  projConstant, m_mipMappedBuffers.csz(), m_mipMappedBuffers.normals(), m_mipMappedBuffers.peeledNormals(), computePeeledLayer);
                blurVertical(rd, settings, projConstant, m_mipMappedBuffers.csz(), m_mipMappedBuffers.normals(), m_mipMappedBuffers.peeledNormals(), computePeeledLayer);

            } // else the result is still in the rawAOBuffer 
        } END_PROFILER_EVENT();

    } END_PROFILER_EVENT();
}


void DeepGBufferRadiosity::resizeBuffers(const shared_ptr<Texture>& depthTexture, bool halfPrecisionColors) {
    bool rebind = false;
    const int width = depthTexture->width();
    const int height = depthTexture->height();
    const ImageFormat* colorFormat = colorImageFormat(halfPrecisionColors);
    if (isNull(m_rawIIFramebuffer) || colorFormat != m_rawIIBuffer->format()) {
        // Allocate for the first call  
        m_rawIIFramebuffer          = Framebuffer::create("DeepGBufferRadiosity::m_rawIIFramebuffer");
        m_hBlurredFramebuffer       = Framebuffer::create("DeepGBufferRadiosity::m_hBlurredFramebuffer");
        m_resultFramebuffer         = Framebuffer::create("DeepGBufferRadiosity::m_resultFramebuffer");

        m_resultPeeledFramebuffer   = Framebuffer::create("DeepGBufferRadiosity::m_resultPeeledFramebuffer");
        m_hBlurredPeeledFramebuffer = Framebuffer::create("DeepGBufferRadiosity::m_hBlurredPeeledFramebuffer");

        m_rawIIBuffer               = Texture::createEmpty("DeepGBufferRadiosity::m_rawIIBuffer",    width, height, colorFormat, Texture::DIM_2D, false);
        m_hBlurredBuffer            = Texture::createEmpty("DeepGBufferRadiosity::m_hBlurredBuffer", width, height, colorFormat, Texture::DIM_2D, false);
        m_resultBuffer              = Texture::createEmpty("DeepGBufferRadiosity::m_resultBuffer",   width, height, colorFormat, Texture::DIM_2D, false);

        m_rawIIPeeledBuffer         = Texture::createEmpty("DeepGBufferRadiosity::m_rawIIPeeledBuffer",    width, height, colorFormat, Texture::DIM_2D, false);
        m_hBlurredPeeledBuffer      = Texture::createEmpty("DeepGBufferRadiosity::m_hBlurredPeeledBuffer", width, height, colorFormat, Texture::DIM_2D, false);
        m_resultPeeledBuffer        = Texture::createEmpty("DeepGBufferRadiosity::m_resultPeeledBuffer",   width, height, colorFormat, Texture::DIM_2D, false);

        rebind = true;

    } else if ((m_rawIIBuffer->width() != width) || (m_rawIIBuffer->height() != height)) {
        // Resize
        m_rawIIBuffer->resize(width, height);
        m_hBlurredBuffer->resize(width, height);
        m_resultBuffer->resize(width, height);
        m_rawIIPeeledBuffer->resize(width, height);
        m_hBlurredPeeledBuffer->resize(width, height);
        m_resultPeeledBuffer->resize(width, height);
        rebind = true;
    }

    if (rebind) {
        m_rawIIBuffer->clear();
        m_hBlurredBuffer->clear();
        m_resultBuffer->clear();
        m_rawIIPeeledBuffer->clear();
        m_hBlurredPeeledBuffer->clear();
        m_resultPeeledBuffer->clear();


        m_rawIIFramebuffer->clear();
        m_hBlurredFramebuffer->clear();
        // Sizes have changed or just been allocated
        m_rawIIFramebuffer->set(Framebuffer::COLOR0, m_rawIIBuffer);
        m_rawIIFramebuffer->set(Framebuffer::COLOR1, m_rawIIPeeledBuffer);

        m_hBlurredFramebuffer->set(Framebuffer::COLOR0, m_hBlurredBuffer);

        m_resultFramebuffer->clear();
        m_resultFramebuffer->set(Framebuffer::COLOR0, m_resultBuffer);
        m_resultFramebuffer->set(Framebuffer::DEPTH, depthTexture);


        m_hBlurredPeeledFramebuffer->clear();
        m_hBlurredPeeledFramebuffer->set(Framebuffer::COLOR0, m_hBlurredPeeledBuffer);

        m_resultPeeledFramebuffer->clear();
        m_resultPeeledFramebuffer->set(Framebuffer::COLOR0, m_resultPeeledBuffer);        
    }
}


void DeepGBufferRadiosity::computeRawII
   (RenderDevice*                   rd,
    const DeepGBufferRadiositySettings&      settings,
    const shared_ptr<Texture>&      depthBuffer,
    const Vector3&                  clipConstant,
    const Vector4&                  projConstant,
    const float                     projScale,
    const Matrix4&                  projectionMatrix,
    const MipMappedBuffers&         mipMappedBuffers,
    bool                            computePeeledLayer) {

    debugAssert(projScale > 0);
    m_rawIIFramebuffer->set(Framebuffer::DEPTH, depthBuffer);
    rd->push2D(m_rawIIFramebuffer); {
        // For quick early-out testing vs. skybox 
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);
        // Values that are never touched due to the depth test will be white
        rd->setColorClearValue(Color3::white());
        rd->clear(true, false, false);
        Args args;
        args.setMacro("NUM_SAMPLES",   settings.numSamples);
        args.setMacro("NUM_SPIRAL_TURNS", settings.numSpiralTurns());
        args.setMacro("MIN_MIP_LEVEL", settings.minMipLevel);
        args.setUniform("radius",      settings.radius);
        args.setUniform("bias",        settings.bias);
        args.setUniform("clipInfo",     clipConstant);
        args.setUniform("projectionMatrix",  projectionMatrix);
        args.setUniform("projInfo",     projConstant);
        args.setUniform("projScale",    projScale);
        mipMappedBuffers.setArgs(args);

        args.setMacro("USE_OCT16",      settings.useOct16);
        args.setMacro("USE_TAP_NORMAL", settings.useTapNormal);
        args.setMacro("TEMPORALLY_VARY_TAPS",   settings.temporallyVarySamples);
        args.setMacro("USE_MIPMAPS",            settings.useMipMaps);
        args.setMacro("COMPUTE_PEELED_LAYER",   computePeeledLayer);

        // Because temporal filtering and multiple scattering events both read the output of
        // the indirect pass as the input to the next indirect pass, this pass must output
        // closer to the full resolution of the input, rather than the final output size.
        // Setting computeGuardBandFraction = 1.0 gives full quality, 
        // Setting computeGuardBandFraction = 0.0 gives maximum performance
        const float r = max(float(m_inputGuardBandSize), float(m_outputGuardBandSize) * (1.0f - settings.computeGuardBandFraction));
        rd->setClip2D(Rect2D::xyxy(r, r, rd->viewport().width() - r, rd->viewport().height() - r));
        args.setRect(rd->viewport());
        LAUNCH_SHADER("DeepGBufferRadiosity_DeepGBufferRadiosity.*", args);
    } rd->pop2D();
}


void DeepGBufferRadiosity::blurOneDirection
    (RenderDevice*                      rd,
    const DeepGBufferRadiositySettings&          settings,
    const Vector4&                      projConstant,
    const shared_ptr<Texture>&          cszBuffer,
    const shared_ptr<Texture>&          normalBuffer,
    const Vector2int16&                 axis,
    const shared_ptr<Framebuffer>&      framebuffer,
    const shared_ptr<Texture>&          source,
    bool                                peeledLayer) {

    // Changes inside the loop
    shared_ptr<Texture> input = source;

    int radiusToGo = settings.blurRadius;
    const int MAX_RADIUS = 6;

    while (radiusToGo > 0) {
        const int currentRadius = min(radiusToGo, MAX_RADIUS);
        radiusToGo -= currentRadius;
        rd->push2D(framebuffer); {
            rd->setColorClearValue(Color3::white());
            rd->clear(true, false, false);
            Args args;
            args.setUniform("source",           input, Sampler::buffer());
            args.setUniform("axis",             axis);

            args.setUniform("projInfo",         projConstant);
            args.setUniform("cszBuffer",        cszBuffer, Sampler::buffer());
            args.setMacro("EDGE_SHARPNESS",     settings.edgeSharpness);
            args.setMacro("SCALE",              settings.blurStepSize);
            args.setMacro("R",                  currentRadius);
            args.setMacro("MDB_WEIGHTS",        settings.monotonicallyDecreasingBilateralWeights ? 1 : 0);
            args.setMacro("PEELED_LAYER",       peeledLayer);
            args.setMacro("USE_OCT16",          settings.useOct16);
            normalBuffer->setShaderArgs(args, "normal_", Sampler::buffer());
            
            // Ensure that we blur into a radius that will affect future blurs
            const float r = float(m_outputGuardBandSize - min(settings.blurRadius, MAX_RADIUS));
            rd->setClip2D(Rect2D::xyxy(r, r, rd->viewport().width() - r, rd->viewport().height() - r));
            args.setRect(rd->viewport());
            LAUNCH_SHADER("DeepGBufferRadiosity_blur.*", args)
        } rd->pop2D();

        if (radiusToGo > 0) {
            static shared_ptr<Texture> tempBlurTexture = Texture::createEmpty("DeepGBufferRadiosity::TempBlurTexture", 1, 1);
            framebuffer->texture(0)->copyInto(tempBlurTexture);
            input = tempBlurTexture;
        }
    }
}


void DeepGBufferRadiosity::blurHorizontal
   (RenderDevice*                       rd,
    const DeepGBufferRadiositySettings&          settings,
    const Vector4&                      projConstant,
    const shared_ptr<Texture>&          cszBuffer,
    const shared_ptr<Texture>&          normalBuffer,
    const shared_ptr<Texture>&          normalPeeledBuffer,
    bool                                computePeeledLayer) {

    Vector2int16 axis(1, 0);
    blurOneDirection(rd, settings, projConstant, cszBuffer, normalBuffer, 
                    axis, m_hBlurredFramebuffer, m_temporallyFilteredResult, false);
    if (computePeeledLayer) {
        blurOneDirection(rd, settings, projConstant, cszBuffer, normalPeeledBuffer, 
                    axis, m_hBlurredPeeledFramebuffer, m_rawIIPeeledBuffer, true);
    }
}


void DeepGBufferRadiosity::blurVertical
   (RenderDevice*                       rd,
    const DeepGBufferRadiositySettings&          settings,
    const Vector4&                      projConstant,
    const shared_ptr<Texture>&          cszBuffer,
    const shared_ptr<Texture>&          normalBuffer,
    const shared_ptr<Texture>&          normalPeeledBuffer,
    bool                                computePeeledLayer) {

    Vector2int16 axis(0, 1);
    blurOneDirection(rd, settings, projConstant, cszBuffer, normalBuffer, 
                    axis, m_resultFramebuffer, m_hBlurredBuffer, false);
    if (computePeeledLayer) {
        blurOneDirection(rd, settings, projConstant, cszBuffer, normalPeeledBuffer,  
                    axis, m_resultPeeledFramebuffer, m_hBlurredPeeledBuffer, true);
    }
}


void DeepGBufferRadiosity::compute
   (RenderDevice*                   rd,
    const DeepGBufferRadiositySettings&      settings,
    const shared_ptr<Texture>&      depthBuffer, 
    const shared_ptr<Texture>&      colorBuffer, 
    const shared_ptr<Camera>&       camera,
    const shared_ptr<Texture>&      peeledDepthBuffer,
    const shared_ptr<Texture>&      peeledColorBuffer, 
    const shared_ptr<Texture>&      normalBuffer,
    const shared_ptr<Texture>&      peeledNormalBuffer,
    bool                            computePeeledLayer,
    const shared_ptr<GBuffer>&      gbuffer,
    const shared_ptr<Scene>&        scene) {

    alwaysAssertM(notNull(normalBuffer), "Must use non-null normal buffer in DeepGBufferRadiosity");
    Matrix4 P;
    camera->projection().getProjectUnitMatrix(rd->clip2D(), P);
    const Vector3& clipConstant = camera->projection().reconstructFromDepthClipInfo();
    const Vector4& projConstant = camera->projection().reconstructFromDepthProjInfo(depthBuffer->width(), depthBuffer->height());

    compute(rd, settings, depthBuffer, colorBuffer, clipConstant, projConstant, (float)abs(camera->imagePlanePixelsPerMeter(rd->viewport())), P, 
            peeledDepthBuffer, peeledColorBuffer, normalBuffer, peeledNormalBuffer, computePeeledLayer, gbuffer, scene);
}



