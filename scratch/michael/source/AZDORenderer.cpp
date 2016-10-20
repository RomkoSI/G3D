/**
\file GLG3D/AZDORenderer.cpp

*/
#include "AZDORenderer.h"

static void renderUniversalSurfaceDepthOnly(RenderDevice*                      rd,
    const Array<shared_ptr<Surface> >& surfaceArray,
    const shared_ptr<Texture>&         previousDepthBuffer,
    const float                        minZSeparation) {
    debugAssertGLOk();

    static shared_ptr<Shader> depthNonOpaqueShader =
        Shader::fromFiles
        (System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.vrt"),
        System::findDataFile("UniversalSurface/UniversalSurface_depthOnlyNonOpaque.pix"));

    static shared_ptr<Shader> depthShader =
        Shader::fromFiles(System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.vrt")
#ifdef G3D_OSX // OS X crashes if there isn't a shader bound for depth only rendering
        , System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.pix")
#endif
        );

    static shared_ptr<Shader> depthPeelShader =
        Shader::fromFiles(System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.vrt"),
        System::findDataFile("UniversalSurface/UniversalSurface_depthPeel.pix"));

    static bool initialized = false;
    static shared_ptr<VertexBuffer> combinedIndexBuffer;
    static Array<IndexStream> allIndexStreams;
    static Array<IndexStream> noAlphaIndexStreams;
    static Array<shared_ptr<BindlessTextureHandle>> lambertianTextures;
    if (!initialized) {
        initialized = true;
        int totalIndexCount = 0;
        for (int i = 0; i < surfaceArray.size(); ++i) {
            shared_ptr<UniversalSurface> surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[i]);
            totalIndexCount += surface->cpuGeom().index->size();
        }
        combinedIndexBuffer = VertexBuffer::create(totalIndexCount * sizeof(int) + 8 * surfaceArray.size());
        
        for (int i = 0; i < surfaceArray.size(); ++i) {
            shared_ptr<UniversalSurface> surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[i]);
            int currSize = surface->cpuGeom().index->size();
            allIndexStreams.append(IndexStream(*surface->cpuGeom().index, combinedIndexBuffer));
            const shared_ptr<UniversalMaterial>& material = surface->material();
            const shared_ptr<Texture>& lambertian = material->bsdf()->lambertian().texture();
            const bool thisSurfaceNeedsAlphaTest = (material->alphaHint() != AlphaHint::ONE) && notNull(lambertian) && !lambertian->opaque();
            lambertianTextures.append(shared_ptr<BindlessTextureHandle>(new BindlessTextureHandle(notNull(lambertian) ? lambertian : Texture::white(), material->sampler())));
            if ((!surface->hasTransmission()) && (! thisSurfaceNeedsAlphaTest)) {
                noAlphaIndexStreams.append(allIndexStreams.last());
            }
        }
    }

    rd->setColorWrite(false);
    const CullFace cull = rd->cullFace();
    BEGIN_PROFILER_EVENT("AZDORenderer::nonAlphaDepthOnly");
    static Array<shared_ptr<Surface> > noAlphaSurfaces;
    for (int i = 0; i < surfaceArray.size(); ++i) {
        shared_ptr<UniversalSurface> surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[i]);
        const shared_ptr<UniversalMaterial>& material = surface->material();
        const shared_ptr<Texture>& lambertian = material->bsdf()->lambertian().texture();
        const bool thisSurfaceNeedsAlphaTest = (material->alphaHint() != AlphaHint::ONE) && notNull(lambertian) && !lambertian->opaque();

        if ((!surface->hasTransmission()) && (!thisSurfaceNeedsAlphaTest)) {
            noAlphaSurfaces.append(surfaceArray[i]);
        }        
    }
    

    // Process opaque surfaces first, front-to-back to maximize early-z test performance
    //for (int g = noAlphaSurfaces.size() - 1; g >= 0; --g) {
    {
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(noAlphaSurfaces[0]);        
        debugAssertM(surface, "renderUniversalSurfaceDepthOnly");        
        const shared_ptr<UniversalSurface::GPUGeom>& geom = surface->gpuGeom();

        // TODO: handle cull face

        // Needed for every type of pass 
        CFrame cframe;
        surface->getCoordinateFrame(cframe, false);
        if (geom->hasBones()) {
            rd->setObjectToWorldMatrix(CFrame());
        } else {
            rd->setObjectToWorldMatrix(cframe);
        }

        Args args;
        surface->setShaderArgs(args);

        args.setMacro("HAS_ALPHA", 0);
        args.setMacro("USE_PARALLAX_MAPPING", 0);

        // Don't use lightMaps for depth only...
        args.setMacro("NUM_LIGHTMAP_DIRECTIONS", 0);
        args.setMacro("NUM_LIGHTS", 0);
        args.setMacro("USE_IMAGE_STORE", 0);

        UniversalSurface::bindDepthPeelArgs(args, rd, previousDepthBuffer, minZSeparation);
        for (int i = noAlphaIndexStreams.size() - 1; i >= 0; --i) {
            args.appendIndexStream(noAlphaIndexStreams[i]);
        }
        
        if (notNull(previousDepthBuffer)) {
            LAUNCH_SHADER_PTR(depthPeelShader, args);
        }
        else {
            LAUNCH_SHADER_PTR(depthShader, args);
        }
    } // for each surface    
    END_PROFILER_EVENT();
    noAlphaSurfaces.fastClear();

    BEGIN_PROFILER_EVENT("AZDORenderer::alphaDepthOnly");
    // Now process surfaces with alpha 
    {
        const shared_ptr<UniversalSurface>& surface0 = dynamic_pointer_cast<UniversalSurface>(surfaceArray[0]);
        debugAssertM(surface0, "Surface::renderDepthOnlyHomogeneous passed the wrong subclass");


        const shared_ptr<UniversalSurface::GPUGeom>& geom = surface0->gpuGeom();

        // Needed for every type of pass 
        CFrame cframe;
        surface0->getCoordinateFrame(cframe, false);

        rd->setObjectToWorldMatrix(cframe);
        

        Args args;
        surface0->setShaderArgs(args);
        args.setMacro("HAS_ALPHA", 1);

        UniversalSurface::bindDepthPeelArgs(args, rd, previousDepthBuffer, minZSeparation);
        int index = 0;
        for (int g = 0; g < surfaceArray.size(); ++g) {
            const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[g]);
            const shared_ptr<Texture>& lambertian = surface->material()->bsdf()->lambertian().texture();
            const bool thisSurfaceNeedsAlphaTest = (surface->material()->alphaHint() != AlphaHint::ONE) && notNull(lambertian) && !lambertian->opaque();

            if (!(surface->hasTransmission() || thisSurfaceNeedsAlphaTest)) {
                // This is an opaque surface, or one that we shouldn't render at all
                continue;
            }
            else {
                args.setArrayUniform("alphaHints", index, surface->material()->alphaHint());
                args.setArrayUniform("lambertianTextures", index, lambertianTextures[g]);
                args.appendIndexStream(allIndexStreams[g]);
                ++index;
            }
        }
        args.setMacro("NUM_TEXTURES", index);
        debugPrintf("NUM_TEXTURES: %d\n", index);

        // The depth with alpha shader handles the depth peel case internally
        
        LAUNCH_SHADER("shader/UniversalSurface_depthAlphaCombined.*", args);
    }
    END_PROFILER_EVENT();
    screenPrintf("AZDO");
}

void AZDORenderer::render
   (RenderDevice*                       rd,
    const shared_ptr<Framebuffer>&      framebuffer,
    const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
    LightingEnvironment&                lightingEnvironment,
    const shared_ptr<GBuffer>&          gbuffer,
    const Array<shared_ptr<Surface>>&   allSurfaces) {

    alwaysAssertM(!lightingEnvironment.ambientOcclusionSettings.enabled || notNull(lightingEnvironment.ambientOcclusion),
        "Ambient occlusion is enabled but no ambient occlusion object is bound to the lighting environment");

    const shared_ptr<Camera>& camera = gbuffer->camera();

    // Share the depth buffer with the forward-rendering pipeline
    framebuffer->set(Framebuffer::DEPTH, gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
    depthPeelFramebuffer->resize(framebuffer->width(), framebuffer->height());

    // Cull and sort
    Array<shared_ptr<Surface> > sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces;
    cullAndSort(rd, gbuffer, allSurfaces, sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces);

    const bool requireBinaryAlpha = false; // TODO: Mike, what should this value be?
    // Bind the main framebuffer
    rd->pushState(framebuffer); {
        rd->clear();
        rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());

        const bool needDepthPeel = lightingEnvironment.ambientOcclusionSettings.useDepthPeelBuffer;

        BEGIN_PROFILER_EVENT("AZDORenderer::computeGBuffer");
        const shared_ptr<Camera>& camera = gbuffer->camera();

        Surface::renderIntoGBuffer(rd, sortedVisibleSurfaces, gbuffer, camera->previousFrame(), camera->expressivePreviousFrame());

        if (notNull(depthPeelFramebuffer)) {
            rd->pushState(depthPeelFramebuffer); {
                rd->clear();
                rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());
                BEGIN_PROFILER_EVENT("Surface::renderDepthOnly");

                rd->pushState(); {

                    rd->setCullFace(CullFace::BACK);
                    rd->setDepthWrite(true);
                    rd->setColorWrite(false);

                    // Categorize by subclass (derived type)
                    Array< Array<shared_ptr<Surface> > > derivedTable;
                    categorizeByDerivedType(sortedVisibleSurfaces, derivedTable);

                    for (int t = 0; t < derivedTable.size(); ++t) {
                        Array<shared_ptr<Surface> >& derivedArray = derivedTable[t];
                        debugAssertM(derivedArray.size() > 0, "categorizeByDerivedType produced an empty subarray");
                        // debugPrintf("Invoking on type %s\n", typeid(*derivedArray[0]).raw_name());
                        if (isNull(dynamic_pointer_cast<UniversalSurface>(derivedArray[0]))) {
                            derivedArray[0]->renderDepthOnlyHomogeneous(rd, derivedArray,
                                gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL),
                                lightingEnvironment.ambientOcclusionSettings.depthPeelSeparationHint, requireBinaryAlpha);
                        } else {
                            //Test here
                            renderUniversalSurfaceDepthOnly(rd, derivedArray,
                                gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL),
                                lightingEnvironment.ambientOcclusionSettings.depthPeelSeparationHint, requireBinaryAlpha);
                        }
                    
                    }

                } rd->popState();

                END_PROFILER_EVENT();
            } rd->popState();
        }
        END_PROFILER_EVENT();

        // Shadowing + AO
        computeShadowing(rd, allSurfaces, gbuffer, depthPeelFramebuffer, lightingEnvironment);

        // Maybe launch deferred pass
        if (deferredShading()) {
            renderDeferredShading(rd, gbuffer, lightingEnvironment);
        }

        // Main forward pass
        renderOpaqueSamples(rd, deferredShading() ? forwardOpaqueSurfaces : sortedVisibleSurfaces, gbuffer, lightingEnvironment);

        // Prepare screen-space lighting for the *next* frame
        lightingEnvironment.copyScreenSpaceBuffers(framebuffer, gbuffer->colorGuardBandThickness());

        renderOpaqueScreenSpaceRefractingSamples(rd, deferredShading() ? forwardOpaqueSurfaces : sortedVisibleSurfaces, gbuffer, lightingEnvironment);

        // Samples that require blending
        if (m_orderIndependentTransparency) {
            renderOrderIndependentBlendedSamples(rd, forwardBlendedSurfaces, gbuffer, lightingEnvironment);
        }
        else {
            renderSortedBlendedSamples(rd, forwardBlendedSurfaces, gbuffer, lightingEnvironment);
        }

    } rd->popState();
}


void AZDORenderer::renderDeferredShading(RenderDevice* rd, const shared_ptr<GBuffer>& gbuffer, const LightingEnvironment& environment) {
    // Make a pass over the screen, performing shading
    rd->push2D(); {
        rd->setGuardBandClip2D(gbuffer->colorGuardBandThickness());

        // Don't shade the skybox on this pass because it will be forward rendered
        rd->setDepthTest(RenderDevice::DEPTH_GREATER);
        Args args;

        environment.setShaderArgs(args);
        gbuffer->setShaderArgsRead(args, "gbuffer_");

        args.setRect(rd->viewport());

        LAUNCH_SHADER("DefaultRenderer_deferredShade.pix", args);
    } rd->pop2D();
}


void AZDORenderer::renderOpaqueSamples
    (RenderDevice*                       rd,
    Array<shared_ptr<Surface> >&        surfaceArray,
    const shared_ptr<GBuffer>&          gbuffer,
    const LightingEnvironment&          environment) {

    //screenPrintf("renderOpaqueSamples: %d", surfaceArray.length());
    BEGIN_PROFILER_EVENT("AZDORenderer::renderOpaqueSamples");
    forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::OPAQUE_SAMPLES, Surface::defaultWritePixelDeclaration(), ARBITRARY);
    END_PROFILER_EVENT();
}


void AZDORenderer::renderOpaqueScreenSpaceRefractingSamples
    (RenderDevice*                       rd,
    Array<shared_ptr<Surface> >&        surfaceArray,
    const shared_ptr<GBuffer>&          gbuffer,
    const LightingEnvironment&          environment) {

    BEGIN_PROFILER_EVENT("AZDORenderer::renderOpaqueScreenSpaceRefractingSamples");
    //screenPrintf("renderOpaqueScreenSpaceRefractingSamples: %d", surfaceArray.length());
    forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::OPAQUE_SAMPLES_WITH_SCREEN_SPACE_REFRACTION, Surface::defaultWritePixelDeclaration(), ARBITRARY);
    END_PROFILER_EVENT();
}


void AZDORenderer::renderSortedBlendedSamples
    (RenderDevice*                       rd,
    Array<shared_ptr<Surface> >&        surfaceArray,
    const shared_ptr<GBuffer>&          gbuffer,
    const LightingEnvironment&          environment) {

    BEGIN_PROFILER_EVENT("AZDORenderer::renderSortedBlendedSamples");
    //screenPrintf("renderBlendedSamples: %d", surfaceArray.length());
    forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::MULTIPASS_BLENDED_SAMPLES, Surface::defaultWritePixelDeclaration(), BACK_TO_FRONT);
    END_PROFILER_EVENT();
}


void AZDORenderer::renderOrderIndependentBlendedSamples
    (RenderDevice*                       rd,
    Array<shared_ptr<Surface> >&        surfaceArray,
    const shared_ptr<GBuffer>&          gbuffer,
    const LightingEnvironment&          environment) {
    BEGIN_PROFILER_EVENT("AZDORenderer::renderOrderIndependentBlendedSamples");
    if (surfaceArray.size() > 0) {

        //screenPrintf("renderOrderIndependentBlendedSamples: %d", surfaceArray.length());

        // Do we need to allocate the OIT buffers?
        if (isNull(m_oitFramebuffer)) {
            m_oitFramebuffer = Framebuffer::create("G3D::AZDORenderer::m_oitFramebuffer");
            m_oitFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("G3D::AZDORenderer accum", rd->width(), rd->height(), ImageFormat::RGBA16F()));

            const shared_ptr<Texture>& texture = Texture::createEmpty("G3D::AZDORenderer revealage", rd->width(), rd->height(), ImageFormat::R8());
            texture->visualization.channels = Texture::Visualization::RasL;
            m_oitFramebuffer->set(Framebuffer::COLOR1, texture);

            m_oitFramebuffer->setClearValue(Framebuffer::COLOR0, Color4::zero());
            m_oitFramebuffer->setClearValue(Framebuffer::COLOR1, Color4::one());
        }

        // Do we need to resize the OIT buffers?
        if ((m_oitFramebuffer->width() != rd->width()) ||
            (m_oitFramebuffer->height() != rd->height())) {
            m_oitFramebuffer->texture(Framebuffer::COLOR0)->resize(rd->width(), rd->height());
            m_oitFramebuffer->texture(Framebuffer::COLOR1)->resize(rd->width(), rd->height());
        }

        m_oitFramebuffer->set(Framebuffer::DEPTH, rd->drawFramebuffer()->texture(Framebuffer::DEPTH));

        ////////////////////////////////////////////////////////////////////////////////////
        //
        // 3D accumulation pass over transparent surfaces
        //

        // The following must not contain newlines because it is a macro
        static const String oitWriteDeclaration = STR(
            layout(location = 0) out float4 _accum;
        layout(location = 1) out float  _revealage;
        layout(location = 2) out float3 _modulate;

        void writePixel(vec4 premultipliedReflect, vec3 transmit, float csZ) {
            /* Perform this operation before modifying the coverage to account for transmission */
            _modulate = premultipliedReflect.a * (vec3(1.0) - transmit);

            /* Modulate the net coverage for composition by the transmission. This does not affect the color channels of the
            transparent surface because the caller's BSDF model should have already taken into account if transmission modulates
            reflection. See

            McGuire and Enderton, Colored Stochastic Shadow Maps, ACM I3D, February 2011
            http://graphics.cs.williams.edu/papers/CSSM/

            for a full explanation and derivation.*/
            premultipliedReflect.a *= (1.0 - (transmit.r + transmit.g + transmit.b) * (1.0 / 3.0), 0, 1);

            // Intermediate terms to be cubed
            float a = min(1.0, premultipliedReflect.a) * 8.0 + 0.01;
            float b = -gl_FragCoord.z * 0.95 + 1.0;

            /* If a lot of the scene is close to the far plane, then gl_FragCoord.z does not
            provide enough discrimination. Add this term to compensate:

            b /= sqrt(abs(csZ)); */

            float w = clamp(a * a * a * 1e3 * b * b * b, 1e-2, 3e2);
            _accum = premultipliedReflect * w;
            _revealage = premultipliedReflect.a;
        });

        const shared_ptr<Framebuffer>& oldBuffer = rd->drawFramebuffer();

        rd->setFramebuffer(m_oitFramebuffer);
        rd->clearFramebuffer(true, false);

        // After the clear, bind the color buffer from the main screen
        m_oitFramebuffer->set(Framebuffer::COLOR2, oldBuffer->texture(Framebuffer::COLOR0));
        rd->pushState(m_oitFramebuffer); {

            // Set blending modes
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE, RenderDevice::BLENDEQ_ADD, RenderDevice::BLENDEQ_SAME_AS_RGB, Framebuffer::COLOR0);
            rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_ONE_MINUS_SRC_COLOR, RenderDevice::BLENDEQ_ADD, RenderDevice::BLENDEQ_SAME_AS_RGB, Framebuffer::COLOR1);
            rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_ONE_MINUS_SRC_COLOR, RenderDevice::BLENDEQ_ADD, RenderDevice::BLENDEQ_SAME_AS_RGB, Framebuffer::COLOR2);

            forwardShade(rd, surfaceArray, gbuffer, environment, RenderPassType::SINGLE_PASS_UNORDERED_BLENDED_SAMPLES, oitWriteDeclaration, ARBITRARY);
        } rd->popState();

        // Remove the color buffer binding
        m_oitFramebuffer->set(Framebuffer::COLOR2, shared_ptr<Texture>());
        rd->setFramebuffer(oldBuffer);

        ////////////////////////////////////////////////////////////////////////////////////
        //
        // 2D compositing pass
        //

        rd->push2D(); {
            rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
            rd->setBlendFunc(RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA, RenderDevice::BLEND_ONE);
            Args args;
            args.setUniform("accumTexture", m_oitFramebuffer->texture(0), Sampler::buffer());
            args.setUniform("revealageTexture", m_oitFramebuffer->texture(1), Sampler::buffer());
            args.setRect(rd->viewport());
            LAUNCH_SHADER("DefaultRenderer_compositeWeightedBlendedOIT.pix", args);
        } rd->pop2D();
    }

    END_PROFILER_EVENT();
}

