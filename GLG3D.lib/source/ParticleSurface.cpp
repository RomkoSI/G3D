/**
  \file ParticleSurface.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2015-08-30
  \edited  2015-09-22
 
 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */

#include "G3D/Projection.h"
#include "GLG3D/ParticleSurface.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Shader.h"

namespace G3D {

void ParticleSurface::getObjectSpaceBoundingBox(AABox & box, bool previous) const {
    alwaysAssertM(! previous, "Previous bounding box not implemented for ParticleSurface");
    box = m_objectSpaceBoxBounds;
}


void ParticleSurface::getObjectSpaceBoundingSphere(Sphere & sphere, bool previous) const {
    alwaysAssertM(! previous, "Previous bounding sphere not implemented for ParticleSurface");
    sphere = m_objectSpaceSphereBounds;
}

void ParticleSurface::sortAndUploadIndices(shared_ptr<ParticleSurface> particleSurface, const Vector3& csz) {
    ParticleSystem::s_sortArray.fastClear();
    
    shared_ptr<ParticleSystem::Block> block = particleSurface->m_block;
    shared_ptr<ParticleSystem> particleSystem = block->particleSystem.lock();
    for (int i = 0; i < particleSystem->m_particle.size(); ++i) {
        const ParticleSystem::Particle& particle = particleSystem->m_particle[i];
        CFrame cframe;
        particleSurface->getCoordinateFrame(cframe);
        const Point3& wsPosition = particleSystem->particlesAreInWorldSpace() ?
            particle.position :
            cframe.pointToWorldSpace(particle.position);
        const float particleCSZ = dot(wsPosition, csz);

        ParticleSystem::SortProxy proxy;
        proxy.index = block->startIndex + i;
        proxy.z = particleCSZ;
        ParticleSystem::s_sortArray.append(proxy);
    }
    ParticleSystem::s_sortArray.sort(SORT_DECREASING);
    ParticleSystem::ParticleBuffer& pBuffer = ParticleSystem::s_particleBuffer;
    bool needsReallocation = true;
    if (pBuffer.indexStream.valid() &&
        (pBuffer.indexStream.maxSize() >= size_t(ParticleSystem::s_sortArray.size()))) {
        needsReallocation = false;
    }

    if (needsReallocation) {
        int numToAllocate = ParticleSystem::s_sortArray.size() * 2;
        shared_ptr<VertexBuffer> vb = VertexBuffer::create(sizeof(int) * numToAllocate + 8);
        int ignored;
        pBuffer.indexStream = IndexStream(ignored, numToAllocate, vb);
    }
    static Array<int> sortedIndices;
    sortedIndices.fastClear();
    for (const ParticleSystem::SortProxy& p : ParticleSystem::s_sortArray) {
        sortedIndices.append(p.index);
    }
    pBuffer.indexStream.update(sortedIndices);
}

void ParticleSurface::setShaderArgs(Args & args, const Array<shared_ptr<Surface>>& surfaceArray, const bool sorted, const Vector3& csz) {
    
    const ParticleSystem::ParticleBuffer& particleBuffer = ParticleSystem::s_particleBuffer;
    args.setAttributeArray("position", particleBuffer.position);
    args.setAttributeArray("shape", particleBuffer.shape);
    args.setAttributeArray("materialProperties", particleBuffer.materialProperties);
    args.setPrimitiveType(PrimitiveType::POINTS);


    //         if sorted transparency [there will only be one Surface per draw call in this case]:
    //             Sort indices of visible surfaces into a single index buffer
    //             Submit all work as GL_POINTS with glDrawElements
    //             GS expands points to triangle strips (quad)
    if (sorted) {
        alwaysAssertM(surfaceArray.size() == 1, "Sorted transparency requires surfaces to be submitted one at a time.");
        shared_ptr<ParticleSurface> particleSurface = dynamic_pointer_cast<ParticleSurface>(surfaceArray[0]);
        sortAndUploadIndices(particleSurface, csz);
        args.setIndexStream(ParticleSystem::s_particleBuffer.indexStream);
    } else {
        //         if OIT:
        //             Submit the consecutive vertex ranges for the visible surfaces using glMultiDrawArrays as GL_POINTS
        //             GS expands points to triangle strips (quad)

        // TODO: Should we preallocate these arrays somewhere?
        Array<int> offsets;
        Array<int> counts;
        for (const shared_ptr<Surface>& s : surfaceArray) {
            const shared_ptr<ParticleSurface>& surface = dynamic_pointer_cast<ParticleSurface>(s);
            debugAssert(notNull(surface));
            const shared_ptr<ParticleSystem::Block>& block = surface->m_block;
            offsets.append(block->startIndex);
            counts.append(block->count);
        }
        args.setMultiDrawArrays(offsets, counts);
    }

}

void ParticleSurface::render
   (RenderDevice*                       rd,
    const LightingEnvironment& 	        environment,
    RenderPassType 	                    passType,
    const String& 	                    singlePassBlendedWritePixelDeclaration) const {
    static Array<shared_ptr<Surface>> surfaceArray;
    surfaceArray.fastClear();
    surfaceArray.append(dynamic_pointer_cast<Surface>(const_cast<ParticleSurface*>(this)->shared_from_this()));
    renderHomogeneous(rd, surfaceArray, environment, passType, singlePassBlendedWritePixelDeclaration);
}


void ParticleSurface::renderDepthOnlyHomogeneous
   (RenderDevice*                       rd,
    const Array<shared_ptr<Surface>>&   surfaceArray,
    const shared_ptr<Texture>& 	        depthPeelTexture,
    const float 	                    depthPeelEpsilon,
    bool 	                            requireBinaryAlpha,
    const Color3&                       transmissionWeight) const {

    if (requireBinaryAlpha || (surfaceArray.size() == 0)) { return; }

    rd->setDepthWrite(true);
    rd->setDepthTest(RenderDevice::DEPTH_LESS);

    Args args;
    ParticleMaterial::s_material->setShaderArgs(args, "material.");
    const Vector3& csz = rd->cameraToWorldMatrix().lookVector();
    setShaderArgs(args, surfaceArray, false, csz);
    const CullFace old = rd->cullFace();
    rd->setCullFace(CullFace::NONE);
    LAUNCH_SHADER_WITH_HINT("ParticleSurface_stochasticDepthOnly.*", args, name());
    rd->setCullFace(old);
}


void ParticleSurface::renderHomogeneous
   (RenderDevice*                       rd, 
    const Array<shared_ptr<Surface>>&   surfaceArray,
    const LightingEnvironment&          lightingEnvironment, 
    RenderPassType                      passType,
    const String&                       singlePassBlendedWritePixelDeclaration) const {

    if (surfaceArray.size() == 0) {
        return;
    }
    if (passType != RenderPassType::SINGLE_PASS_UNORDERED_BLENDED_SAMPLES &&
        passType != RenderPassType::MULTIPASS_BLENDED_SAMPLES) {
        return;
    }

    const ParticleSystem::ParticleBuffer& particleBuffer = ParticleSystem::s_particleBuffer;

    rd->pushState(); {
        rd->setDepthWrite(false);
        rd->setDepthTest(RenderDevice::DEPTH_LESS);


        Args args;
        args.setMacro("DECLARE_writePixel", singlePassBlendedWritePixelDeclaration);
        lightingEnvironment.setShaderArgs(args);

        args.setUniform("depthBuffer", rd->framebuffer()->texture(Framebuffer::DEPTH), Sampler::video());
        args.setUniform("directAlphaBoost", 1.0f);
        args.setUniform("directValueBoost", 1.0f);
        args.setUniform("directSaturationBoost", 1.0f);
        args.setUniform("environmentSaturationBoost", 1.0f);

        args.setMacro("HAS_ALPHA", 1);
        args.setMacro("HAS_TRANSMISSIVE", 1);
        args.setMacro("HAS_EMISSIVE", 0);
        args.setMacro("ALPHA_HINT", AlphaHint::BLEND);

        Projection proj(rd->projectionMatrix(), rd->viewport());
        args.setUniform("nearPlaneZ", proj.nearPlaneZ());
        args.setUniform("clipInfo", proj.reconstructFromDepthClipInfo());
        ParticleMaterial::s_material->setShaderArgs(args, "material.");
        const Vector3& csz = rd->cameraToWorldMatrix().lookVector();
        bool sortedMode = (passType == RenderPassType::MULTIPASS_BLENDED_SAMPLES);
        if (sortedMode) {
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        }
        // If sorting is necessary, it is handled in this function
        setShaderArgs(args, surfaceArray, sortedMode, csz);

        {
            // Hack copied from UniversalSurface
            const float backZ = -100;
            args.setUniform("backgroundZ", backZ);
            // Find out how big the back plane is in meters
            float backPlaneZ = min(-0.5f, backZ);
            Projection P(rd->projectionMatrix());
            P.setFarPlaneZ(backPlaneZ);
            Vector3 ur, ul, ll, lr;
            P.getFarViewportCorners(rd->viewport(), ur, ul, ll, lr);

            // Since we use the lengths only, do not bother taking to world space
            Vector2 backSizeMeters((ur - ul).length(), (ur - lr).length());
            args.setUniform("backSizeMeters", backSizeMeters);
        }
        LAUNCH_SHADER_WITH_HINT("ParticleSurface_render.*", args, name());
    } rd->popState();

}

void ParticleSurface::renderWireframeHomogeneous	
   (RenderDevice* 	                    rd,
    const Array<shared_ptr<Surface>>& 	surfaceArray,
    const Color4&                       color,
    bool                                previous) const {

    rd->pushState(); {
        rd->setDepthWrite(false);
        rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
        rd->setPolygonOffset(-0.5f);
        Args args;
        args.setUniform("wireframeColor", color);
        const Vector3& csz = rd->cameraToWorldMatrix().lookVector();
        setShaderArgs(args, surfaceArray, false, csz);
        Projection proj(rd->projectionMatrix());
        args.setUniform("nearPlaneZ", proj.nearPlaneZ());
        LAUNCH_SHADER_WITH_HINT("ParticleSurface_wireframe.*", args, name());

    } rd->popState();
}


} // namespace G3D
