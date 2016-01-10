/**
  \file GLG3D/ParticleSurface.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2015-08-30
  \edited  2015-09-15
 
 G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */
#ifndef GLG3D_ParticleSurface_h
#define GLG3D_ParticleSurface_h

#include "G3D/platform.h"
#include "GLG3D/Surface.h"
#include "GLG3D/ParticleSystem.h"

namespace G3D {

/**
    Each ParticleSurface is the set of particles for a single ParticleSystem (not a single particle--this allows them
    to be culled reasonably without creating a huge amount of CPU work managing the particles).
      
    All particles for all ParticleSystems are submitted as a single draw call. 
      
    In sorted transparency mode, the ParticleSurface sorts for each draw call. In OIT mode, there is no CPU
    work per draw call (however, there may be necessary copying during pose for CPU-animated particles).
  
*/
class ParticleSurface : public Surface {
protected:
    friend ParticleSystem;

    /** This is a POINTER to a block so that in the event of reallocation, the Surface
        will still know where to find its data. */
    shared_ptr<ParticleSystem::Block>    m_block;
    AABox                       m_objectSpaceBoxBounds;
    Sphere                      m_objectSpaceSphereBounds;

    ParticleSurface() {}
    ParticleSurface(const shared_ptr<Entity>& entity) {
        m_entity = entity;
    }

    // To be called by ParticleSystem only
    static shared_ptr<ParticleSurface> create(const shared_ptr<Entity>& entity) {
        return shared_ptr<ParticleSurface>(new ParticleSurface(entity));
    }

    static void sortAndUploadIndices(shared_ptr<ParticleSurface>surface, const Vector3& csz);

    /** If \param sort is true, construct an index array to render back-to-front (using \param csz), otherwise submit everything in a giant multi-draw call */
    static void setShaderArgs(Args & args, const Array<shared_ptr<Surface>>& surfaceArray, const bool sort, const Vector3& csz);

public:
    
    /** ParticleSystem is defined to act entirely transparently */
    virtual bool anyOpaque() const override {
        return false;
    }

    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification &specification) const override {
        return false;
    }

    virtual bool canRenderIntoSVO() const override {
        return false;
    }

    virtual bool hasTransmission() const override {
        return true;
    }

    virtual bool requiresBlending() const override {
        return true;
    }
    
    /** May be infinite */
    virtual void getObjectSpaceBoundingBox(AABox& box, bool previous = false) const override;

    /** May be infinite */
    virtual void getObjectSpaceBoundingSphere(Sphere& sphere, bool previous = false) const override;

    virtual void render
       (RenderDevice* 	rd,
        const LightingEnvironment& 	environment,
        RenderPassType 	passType,
        const String & 	singlePassBlendedWritePixelDeclaration) const override;

    virtual void renderDepthOnlyHomogeneous
       (RenderDevice* 	                        rd,
        const Array< shared_ptr< Surface > > & 	surfaceArray,
        const shared_ptr< Texture > & 	        depthPeelTexture,
        const float 	                        depthPeelEpsilon,
        bool 	                                requireBinaryAlpha,
        const Color3&                           transmissionWeight) const override;

    virtual void renderHomogeneous
       (RenderDevice *rd, 
        const Array< shared_ptr< Surface > > &surfaceArray, 
        const LightingEnvironment &lightingEnvironment, 
        RenderPassType passType, 
        const String &singlePassBlendedWritePixelDeclaration) const override;

    virtual void renderWireframeHomogeneous	
       (RenderDevice * 	rd,
        const Array< shared_ptr< Surface > > & 	surfaceArray,
        const Color4 & 	color,
        bool 	previous) const override;

};

} // namespace

#endif
