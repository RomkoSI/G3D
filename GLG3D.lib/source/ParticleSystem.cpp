/**
  \file ParticleSystem.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2015-08-30
  \edited  2016-02-14
 
 G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */

#include "G3D/Noise.h"
#include "GLG3D/ParticleSystem.h"
#include "GLG3D/ParticleSurface.h"
#include "GLG3D/Scene.h"
#include "G3D/Vector4uint16.h"
#include "GLG3D/ParticleSystemModel.h"

namespace G3D {

void ParticleSystem::ParticleBuffer::free(const shared_ptr<Block>& block) {
    if (notNull(block)) {
        const int i = blockArray.rfindIndex(block);
        debugAssertM(i != -1, "Could not find block to free in ParticleSystem::ParticleBuffer::free()");
        blockArray.fastRemove(i);
    }
}


void ParticleSystem::ParticleBuffer::removeUnusedBlocks() {
    for (int b = blockArray.size() - 1; b >= 0; --b) {
        if (isNull(blockArray[b]->surface.lock()) && isNull(blockArray[b]->particleSystem.lock())) {
            blockArray.fastRemove(b);
        }
    }
}    


void ParticleSystem::ParticleBuffer::allocateVertexBuffer(const int newReserve) {
    vertexBuffer.reset();

    const int gpuParticleSize = sizeof(Vector4) + sizeof(Vector3) + sizeof(Vector4uint16);
    const int padding = 8 * 3; // 8 bytes padding per AttributeArray
    vertexBuffer = VertexBuffer::create(gpuParticleSize * newReserve + padding);

    position            = AttributeArray(Vector4(), newReserve, vertexBuffer);
    shape               = AttributeArray(Vector3(), newReserve, vertexBuffer);
    materialProperties  = AttributeArray(Vector4uint16(), newReserve, vertexBuffer);

    count = 0;
}


void ParticleSystem::ParticleBuffer::compact(const int newReserveCount) {
    removeUnusedBlocks();

    // Save the old data to copy from. These variables keep the GPU
    // memory alive.
    const shared_ptr<VertexBuffer>  oldVertexBuffer       = vertexBuffer;
    const AttributeArray            oldPosition           = position;
    const AttributeArray            oldShape              = shape;
    const AttributeArray            oldMaterialProperties = materialProperties;

    allocateVertexBuffer(newReserveCount);

    debugAssertGLOk();
    glBindBuffer(GL_COPY_READ_BUFFER,  oldVertexBuffer->openGLVertexBufferObject());
    glBindBuffer(GL_COPY_WRITE_BUFFER, vertexBuffer->openGLVertexBufferObject());
    debugAssertGLOk();

    // Update all blocks to the indices in the new vertex buffer,
    // copying their data as we progress so that they will be up to date
    for (const shared_ptr<Block>& block : blockArray) {

        const int oldStartIndex = block->startIndex;
        block->startIndex = count;
        count += block->reserve;

        // Copy the vertex ranges
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr)oldPosition.startAddress() + oldStartIndex * sizeof(Vector4),           (GLintptr)position.startAddress() + (block->startIndex * sizeof(Vector4)),           (GLsizeiptr)(block->count * sizeof(Vector4)));
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr)oldShape.startAddress() + oldStartIndex * sizeof(Vector3),              (GLintptr)shape.startAddress() + (block->startIndex * sizeof(Vector3)),              (GLsizeiptr)(block->count * sizeof(Vector3)));
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr)oldMaterialProperties.startAddress() + oldStartIndex * sizeof(Vector4uint16), (GLintptr)materialProperties.startAddress() + (block->startIndex * sizeof(Vector4uint16)), (GLsizeiptr)(block->count * sizeof(Vector4uint16)));
        debugAssertGLOk();
    }

    // Indices for sorted transparency will be handled at render time

    glBindBuffer(GL_COPY_READ_BUFFER, GL_NONE);
    glBindBuffer(GL_COPY_WRITE_BUFFER, GL_NONE);
    debugAssertGLOk();
}


shared_ptr<ParticleSystem::Block> ParticleSystem::ParticleBuffer::alloc
(const shared_ptr<ParticleSystem>& particleSystem,
    const shared_ptr<ParticleSurface>& surface,
    int numElementsToReserve) {

    if (reserve() > 0) {
        const int maximumNewBlockSize = reserve() - count;
        const int maximumWastedElementsAllowed = reserve() / 2;

        // Count how much space is wasted due to garbage collected
        // elements...maybe it is time for compaction. Note that we don't 
        // implement a free list for reuse.
        int allocatedElementsPotentiallyInUse = 0;
        for (const shared_ptr<Block>& block : blockArray) {
            const shared_ptr<ParticleSystem>& ps = block->particleSystem.lock();
            const shared_ptr<ParticleSurface>& psurf = block->surface.lock();
            if (notNull(ps) || notNull(psurf)) {
                allocatedElementsPotentiallyInUse += block->reserve;
            }
        }
        const int wastedElements = count - allocatedElementsPotentiallyInUse;

        if ((numElementsToReserve > maximumNewBlockSize) || (wastedElements > maximumWastedElementsAllowed)) {
            // Compact. There is either no space or too much wasted space (or both)
            const int newReserveCount = allocatedElementsPotentiallyInUse + numElementsToReserve;
            compact(newReserveCount);
        }
    } else {
        allocateVertexBuffer(numElementsToReserve);
    }

    alwaysAssertM(numElementsToReserve <= reserve() - count, "Still out of GPU space for particles *after* compaction and allocation check");

    // There is space at the end of the buffer, so use it
    const shared_ptr<Block> block(new Block(particleSystem, surface, count, numElementsToReserve));
    count += numElementsToReserve;
    blockArray.append(block);
    return block;
}


/////////////// ParticleMaterial Implementation ////////////////

shared_ptr<UniversalMaterial> ParticleMaterial::s_material;

Array<weak_ptr<ParticleMaterial>> ParticleMaterial::s_materialArray;


shared_ptr<ParticleMaterial> ParticleMaterial::create(const UniversalMaterial::Specification& material) {
    return create(UniversalMaterial::create(material));
}

static shared_ptr<Texture> convertToTextureArray(const String& name, shared_ptr<Texture> tex) {
    shared_ptr<Texture> result = Texture::createEmpty(name, tex->width(), tex->height(), 
                                        tex->encoding(), Texture::DIM_2D_ARRAY, false, 1);
    
    Texture::copy(tex, result);
    result->generateMipMaps();
    return result;
}

static shared_ptr<Texture> layeredTextureConcatenate(
    const String& name,
    const shared_ptr<Texture>& original, 
    const shared_ptr<Texture>& newLayer,
    const int numLayers) {
    
    const shared_ptr<Texture>& previousLayers = notNull(original) ? original : Texture::opaqueBlack(Texture::DIM_2D_ARRAY);

    int width   = max(previousLayers->width(), newLayer->width());
    int height  = max(previousLayers->height(), newLayer->height());
    // TODO: Handle different texture formats?
    shared_ptr<Texture> result = Texture::createEmpty(name, width, height, 
        newLayer->encoding(), Texture::DIM_2D_ARRAY, false, numLayers);

    for (int i = 0; i < numLayers - 1; ++i) {
        int prevLayerIndex = min(i, previousLayers->depth() - 1);
        Texture::copy(previousLayers, result, 0, 0, 1.0f, Vector2int16(0, 0), 
            CubeFace::POS_X, CubeFace::POS_X, NULL, false, prevLayerIndex, i);
    }
    Texture::copy(newLayer, result, 0, 0, 1.0f, Vector2int16(0, 0),
        CubeFace::POS_X, CubeFace::POS_X, NULL, false, 0, numLayers - 1);
    result->generateMipMaps();
    return result;
}

int ParticleMaterial::insertMaterial(shared_ptr<UniversalMaterial> newMaterial) {
    alwaysAssertM(isNull(newMaterial->bump()), "We do not yet support bump-mapped particles");
    alwaysAssertM(newMaterial->bsdf()->etaReflect() == newMaterial->bsdf()->etaTransmit(), 
        "We do not yet support refracting particles");
    alwaysAssertM(newMaterial->customShaderPrefix() == "", "We do not support custom shader prefixes for particles.");
    alwaysAssertM(newMaterial->numLightMapDirections() == 0, "We do not support light maps on particles.");
    shared_ptr<Texture> lambertian      = newMaterial->bsdf()->lambertian().texture();
    shared_ptr<Texture> glossy          = newMaterial->bsdf()->glossy().texture();
    shared_ptr<Texture> transmissive    = newMaterial->bsdf()->transmissive().texture();
    shared_ptr<Texture> emissive        = newMaterial->emissive().texture();
    int layerIndex = 0;

    const String emissiveName     = "G3D::ParticleMaterial Emissive";
    const String glossyName       = "G3D::ParticleMaterial Glossy";
    const String lambertianName   = "G3D::ParticleMaterial Lambertian";
    const String transmissiveName = "G3D::ParticleMaterial Transmissive";

    if (isNull(s_material)) { 
        
        UniversalMaterial::Specification settings;
        settings.setAlphaHint(AlphaHint::BLEND);
        //settings.setBump                  TODO: support
        //settings.setConstant              TODO: remove
        //settings.setCustomShaderPrefix    TODO: remove
        settings.setEmissive(convertToTextureArray(emissiveName, emissive));
        //settings.setEta
        settings.setGlossy(convertToTextureArray(glossyName, glossy));
        settings.setLambertian(convertToTextureArray(lambertianName, lambertian));
        //settings.setLightMaps;
        //settings.setMirrorHint(MirrorQuality::);
        settings.setRefractionHint(RefractionHint::NONE);
        //settings.setSampler();
        settings.setTransmissive(convertToTextureArray(transmissiveName, transmissive));
        
       
        s_material = UniversalMaterial::create("Particle Uber-Material", settings);

    } else {

        shared_ptr<Texture> currentLambertian   = s_material->bsdf()->lambertian().texture();
        shared_ptr<Texture> currentGlossy       = s_material->bsdf()->glossy().texture();
        shared_ptr<Texture> currentTransmissive = s_material->bsdf()->transmissive().texture();
        shared_ptr<Texture> currentEmissive     = s_material->emissive().texture();
        
        layerIndex = currentLambertian->depth();
        int numLayers = layerIndex + 1;
        

        UniversalMaterial::Specification settings;
        settings.setAlphaHint(AlphaHint::BLEND);
        //settings.setBump                  TODO: support
        //settings.setConstant              TODO: remove
        //settings.setCustomShaderPrefix    TODO: remove
        settings.setEmissive(layeredTextureConcatenate(emissiveName, currentEmissive, emissive, numLayers));
        //settings.setEta
        settings.setGlossy(layeredTextureConcatenate(glossyName, currentGlossy, glossy, numLayers));
        settings.setLambertian(layeredTextureConcatenate(lambertianName, currentLambertian, lambertian, numLayers));
        //settings.setLightMaps;
        //settings.setMirrorHint(MirrorQuality::);
        settings.setRefractionHint(RefractionHint::NONE);
        //settings.setSampler();
        settings.setTransmissive(layeredTextureConcatenate(transmissiveName, currentTransmissive, transmissive, numLayers));
        s_material = UniversalMaterial::create("Particle Uber-Material", settings);
    }
    return layerIndex;
}

shared_ptr<ParticleMaterial> ParticleMaterial::create(const shared_ptr<UniversalMaterial>& material) {
    debugAssertM(material->bsdf()->lambertian().texture()->width() ==
        material->bsdf()->lambertian().texture()->height(), "Particle materials must be square");
    int index = insertMaterial(material);
    return shared_ptr<ParticleMaterial>(new ParticleMaterial(index, material->bsdf()->lambertian().texture()->width()));
}


ParticleMaterial::~ParticleMaterial() {
}


void ParticleMaterial::freeAllUnusedMaterials() {
    alwaysAssertM(false, "ParticleMaterial::freeAllUnusedMaterials not yet implemented.");
}

/////////////////// ParticleSystem Implementation /////////////////

ParticleSystem::ParticleBuffer ParticleSystem::s_particleBuffer;
Array<ParticleSystem::SortProxy> ParticleSystem::s_sortArray;
bool ParticleSystem::s_preferLowResolutionTransparency = false;

ParticleSystem::ParticleSystem() : m_particlesChangedSinceBounds(true), 
    m_particlesChangedSincePose(true), 
    m_rng(4028146898U, false),
    m_particlesAreInWorldSpace(true), 
    m_initTime(0) {
}

void ParticleSystem::updateBounds() {   
    if (! m_particlesChangedSinceBounds) { return; }

    m_lastObjectSpaceAABoxBounds = AABox::empty();
    float objectSpaceBoundingRadiusSquared = 0.0f;

    float largestRadius = 0.0f;
    for (int i = 0; i < m_particle.size(); ++i) {
        const Particle& P = m_particle[i];
        largestRadius = max(largestRadius, P.radius);
        // We save a lot of computation by forming the bounds off the centers
        // and then conservatively expanding them by the worst-case radius
        m_lastObjectSpaceAABoxBounds.merge(P.position);
        objectSpaceBoundingRadiusSquared = max(P.position.squaredMagnitude(), objectSpaceBoundingRadiusSquared);
    }

    // Expand by the worst radius observed
    const float objectSpaceBoundingRadius = sqrt(objectSpaceBoundingRadiusSquared) + largestRadius;

    // Extra bound from rotating the square so that its diagonals are axis-aligned (worst-case)
    const float dilatedLargestRadius = sqrt(2.0f) * largestRadius;

    m_lastObjectSpaceAABoxBounds = m_lastObjectSpaceAABoxBounds.minkowskiSum
        (AABox(Point3(-dilatedLargestRadius, -dilatedLargestRadius, -dilatedLargestRadius),
               Point3(dilatedLargestRadius, dilatedLargestRadius, dilatedLargestRadius)));
 
    if (m_lastObjectSpaceAABoxBounds.isEmpty()) {
        m_lastBoxBounds = Box(m_frame.translation);
        m_lastAABoxBounds = AABox::empty();
    } else if (m_particlesAreInWorldSpace) {
        // Take to object space
        m_lastAABoxBounds = m_lastObjectSpaceAABoxBounds;
        m_lastBoxBounds = m_lastAABoxBounds;
        m_frame.toObjectSpace(m_lastObjectSpaceAABoxBounds).getBounds(m_lastObjectSpaceAABoxBounds);
        m_lastAABoxBounds.getBounds(m_lastSphereBounds);
    } else {
        // Take to world space
        m_lastBoxBounds = m_frame.toWorldSpace(m_lastObjectSpaceAABoxBounds);
        m_lastBoxBounds.getBounds(m_lastAABoxBounds);
        m_lastSphereBounds = Sphere(m_frame.translation, objectSpaceBoundingRadius);
    }

    m_lastBoxBoundArray.fastClear();
    m_lastBoxBoundArray.append(m_lastBoxBounds);
}


void ParticleSystem::applyPhysics(float t, float dt) {
    debugAssert(notNull(m_physicsEnvironment));
    // Convert to the local reference frame
    const Vector3&  gravitationalAcceleration   = m_particlesAreInWorldSpace ? m_physicsEnvironment->gravitationalAcceleration : m_frame.vectorToObjectSpace(m_physicsEnvironment->gravitationalAcceleration);
    const Vector3&  windVelocity                = m_particlesAreInWorldSpace ? m_physicsEnvironment->windVelocity : m_frame.vectorToObjectSpace(m_physicsEnvironment->windVelocity);

    // Compensate for the [-2, 2] range below
    const float     maxBrownianVelocity         = m_physicsEnvironment->maxBrownianVelocity * 0.35f;
    const int       brownianTemporalOffset      = int(t * (m_physicsEnvironment->windVelocity.length() + 1.f) - 1000.0f); 

    for (int i = 0; i < m_particle.size(); ++i) {
        Particle& P = m_particle[i];
        
        // https://en.wikipedia.org/wiki/Drag_equation
        const float area = pif() * square(P.radius);
        
        // Sample three, different artbitray noise functions
        const Point3int32& fixedPos = Point3int32(P.position * 200.0f);
        const Vector3 brownianDirection(Noise::common().sampleFloat(brownianTemporalOffset, fixedPos.y + 10208, fixedPos.z + 55010, 2),
                                        Noise::common().sampleFloat(brownianTemporalOffset, fixedPos.z + 10208, fixedPos.x + 55010, 2),
                                        Noise::common().sampleFloat(brownianTemporalOffset, fixedPos.x + 10208, fixedPos.y + 55010, 2));
        const Vector3& localWindVelocity = windVelocity + maxBrownianVelocity * brownianDirection;
        const Vector3& relativeVelocity = localWindVelocity - P.velocity;
        const Vector3& dragForce = relativeVelocity.directionOrZero() * (0.5f * 1.185f * relativeVelocity.squaredMagnitude() * P.dragCoefficient * area);
        
        const Vector3& acceleration = gravitationalAcceleration + dragForce / P.mass;
        
        P.velocity += acceleration * dt;
        P.position += P.velocity * dt;
        P.angle += P.angularVelocity * dt;
    }
    
    markChanged();
}


void ParticleSystem::markChanged() {
    m_particlesChangedSinceBounds = true;
    m_particlesChangedSincePose = true;
    m_lastChangeTime = System::time();
}


shared_ptr<Entity> ParticleSystem::create
   (const String&               name, 
    Scene*                      scene, 
    AnyTableReader&             propertyTable, 
    const ModelTable&           modelTable) {

    const shared_ptr<ParticleSystem> ps(new ParticleSystem());

    ps->Entity::init(name, scene, propertyTable);
    ps->VisibleEntity::init(propertyTable, modelTable);
    ps->ParticleSystem::init(propertyTable);
    propertyTable.verifyDone();

    return ps;
}


void ParticleSystem::init(AnyTableReader& propertyTable) {
    propertyTable.getIfPresent("particlesAreInWorldSpace", m_particlesAreInWorldSpace);
    init();
}


void ParticleSystem::init() {
    m_physicsEnvironment = PhysicsEnvironment::create();    
    m_initTime = notNull(m_scene) ? m_scene->time() : 0.0f;

    // Spawn the initial particles. For VERTICES and FACES, this will
    // place randomly, but we then override these to ensure coverage
    // of a unique set below.
    const Array<shared_ptr<ParticleSystemModel::Emitter>>& emitterArray = particleSystemModel()->emitterArray();
    for (int e = 0; e < emitterArray.size(); ++e) {
        const shared_ptr<ParticleSystemModel::Emitter>& emitter = emitterArray[e];

        const float initialDensity = emitter->specification().initialDensity;

        int numParticlesToEmit = 0;
        switch (emitter->specification().location) {
        case ParticleSystemModel::Emitter::SpawnLocation::VERTICES:
        {
            const int n = dynamic_pointer_cast<MeshShape>(emitter->m_spawnShape)->vertexArray().size();
            numParticlesToEmit = iClamp(iRound(n * initialDensity), 0, n);            
            break;
        }

        case ParticleSystemModel::Emitter::SpawnLocation::FACES:
        {
            const int n = dynamic_pointer_cast<MeshShape>(emitter->m_spawnShape)->indexArray().size() / 3;
            numParticlesToEmit = iClamp(iRound(n * initialDensity), 0, n);            
            break;
        }

        case ParticleSystemModel::Emitter::SpawnLocation::SURFACE:
            numParticlesToEmit = iRound(emitter->m_spawnShape->area() * initialDensity);
            break;

        case ParticleSystemModel::Emitter::SpawnLocation::VOLUME:
            numParticlesToEmit = iRound(emitter->m_spawnShape->volume() * initialDensity);
            break;                    
        }

        if (numParticlesToEmit > 0) {
            emitter->spawnParticles(this, numParticlesToEmit, m_scene->time(), m_scene->time() - m_initTime, 0.0f, e);
    

            // For faces and vertices, fix up locations
            switch (emitter->specification().location) {
            case ParticleSystemModel::Emitter::SpawnLocation::VERTICES:
            {
                // Copy the original array
                Array<Point3> vertexArray = dynamic_pointer_cast<MeshShape>(emitter->m_spawnShape)->vertexArray();
                vertexArray.randomize(m_rng);
                for (int i = 0; i < numParticlesToEmit; ++i) {
                    m_particle[m_particle.size() - i - 1].position = vertexArray[i];
                }
                break;
            }

            case ParticleSystemModel::Emitter::SpawnLocation::FACES:
            {
                // Copy the original array
                const Array<Point3>& vertexArray = dynamic_pointer_cast<MeshShape>(emitter->m_spawnShape)->vertexArray();
                const Array<int>& indexArray = dynamic_pointer_cast<MeshShape>(emitter->m_spawnShape)->indexArray();
                
                // Find all face centers
                Array<Point3> centroid;
                centroid.resize(indexArray.size() / 3);
                for (int f = 0; f < centroid.length(); ++f) {
                    centroid[f] = (vertexArray[indexArray[f * 3]] +
                        vertexArray[indexArray[f * 3 + 1]] +
                        vertexArray[indexArray[f * 3 + 2]]) / 3.0f;
                }
                centroid.randomize(m_rng);
                for (int i = 0; i < numParticlesToEmit; ++i) {
                    m_particle[m_particle.size() - i - 1].position = centroid[i];
                }
                break;
            }

            default:;
            } // switch
        } // if numParticlesToEmit > 0
    } // for
}


shared_ptr<ParticleSystem> ParticleSystem::create
   (const String&               name, 
    Scene*                      scene, 
    const CFrame&               position, 
    const shared_ptr<Model>&    model) {

    const shared_ptr<ParticleSystem> ps(new ParticleSystem());

    bool canChange     = true;
    bool shouldBeSaved = true;
    bool visible       = true;

    ps->Entity::init(name, scene, position, shared_ptr<SplineTrack>(), canChange, shouldBeSaved);
    ps->VisibleEntity::init(model, visible, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline(), MD3Model::PoseSequence());
    ps->ParticleSystem::init();

    return ps;
}


Any ParticleSystem::toAny(const bool forceAll) const {
    Any a = VisibleEntity::toAny(forceAll);
    a.setName("ParticleSystem");

    // Model, pose, and visible flag must already have been set, so no need to change anything
    return a;
}


void ParticleSystem::onPose(Array<shared_ptr<Surface>>& surfaceArray) {
    if (m_particlesChangedSincePose) {
        updateBounds();
    }

    if (m_particle.size() == 0 || visible() == false) {
        return;
    }

    
    shared_ptr<ParticleSurface> surface = ParticleSurface::create(dynamic_pointer_cast<Entity>(shared_from_this()));
    surface->m_preferLowResolutionTransparency = s_preferLowResolutionTransparency;
    surface->m_block = m_block;
#   ifdef G3D_DEBUG
        if (notNull(surface->m_block)) {
            const shared_ptr<ParticleSystem>& ps = surface->m_block->particleSystem.lock();
            debugAssertM(isNull(ps) || (ps.get() == this), "Block back-pointer does not match ParticleSystem this");
        }
#   endif

    if (isNull(surface->m_block) || (surface->m_block->reserve < m_particle.size())) {
        // Free our existing block, since we're prepared to copy our data to the new one
        s_particleBuffer.free(surface->m_block);
        surface->m_block.reset();
        m_block.reset();

        // Out of reserved space in the current block. Allocate a new block at the end of s_particleBuffer.
        // Always allocate more space than needed to allow growing.
        const int desiredBlockSize = max(100, m_particle.size() * 2);

        surface->m_block = s_particleBuffer.alloc(dynamic_pointer_cast<ParticleSystem>(shared_from_this()), 
                                                    surface,
                                                    desiredBlockSize);
        m_block = surface->m_block;
        
    }

    surface->m_objectSpaceBoxBounds    = m_lastObjectSpaceAABoxBounds;
    surface->m_objectSpaceSphereBounds = frame().toObjectSpace(m_lastSphereBounds);

    // Copy particles into s_particleBuffer, preserving unmodified values from other systems
    debugAssert(notNull(surface->m_block));
    // Since all 3 attribute arrays point into the same buffer, we can only map one at a time, 
    // and thus need to do pointer arithmetic to access all 3 at the same time.
    uint8* mappedVertexBuffer = (uint8*)s_particleBuffer.position.mapBuffer(GL_WRITE_ONLY) - (intptr_t)s_particleBuffer.position.startAddress();
    Vector4*        positionPtr             = (Vector4*)        (mappedVertexBuffer + (intptr_t)s_particleBuffer.position.startAddress()) 
                                                + surface->m_block->startIndex;
    Vector3*        shapePtr                = (Vector3*)        (mappedVertexBuffer + (intptr_t)s_particleBuffer.shape.startAddress()) 
                                                + surface->m_block->startIndex;
    Vector4uint16*  materialPropertiesPtr   = (Vector4uint16*)  (mappedVertexBuffer + (intptr_t)s_particleBuffer.materialProperties.startAddress()) 
                                                + surface->m_block->startIndex;

    uint16 flags = 0;
    if (m_expressiveLightScatteringProperties.castsShadows) {
        flags |= ParticleBuffer::CASTS_SHADOWS;
    }

    if (m_expressiveLightScatteringProperties.receivesShadows) {
        flags |= ParticleBuffer::RECEIVES_SHADOWS;
    }

    if (m_particlesAreInWorldSpace) {
        for (int p = 0; p < m_particle.size(); ++p) {
            const Particle& particle = m_particle[p];
            positionPtr[p] = (const Vector4&)particle.position;
        } 
    } else {
        for (int p = 0; p < m_particle.size(); ++p) {
            const Particle& particle = m_particle[p];
            positionPtr[p] = Vector4(m_frame.pointToWorldSpace(particle.position), particle.angle);
        } 
    }

    for (int p = 0; p < m_particle.size(); ++p) {
        const Particle& particle = m_particle[p];
        shapePtr[p]              = (const Vector3&)particle.radius;

        const shared_ptr<ParticleMaterial>& material = particle.material;
        materialPropertiesPtr[p] = Vector4uint16(material->m_textureIndex, material->m_texelWidth, flags, particle.userdataInt);
    }
    // We only mapped the buffer via .position
    s_particleBuffer.position.unmapBuffer();

    surface->m_block->count = m_particle.size();

    // Indices for sorted transparency will be handled at render time
    surfaceArray.append(surface);
}


shared_ptr<ParticleSystemModel> ParticleSystem::particleSystemModel() const {
    return dynamic_pointer_cast<ParticleSystemModel>(m_model);
}


void ParticleSystem::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    if (deltaTime > 0) {
        spawnParticles(absoluteTime, deltaTime);

        const ParticleSystemModel* model = particleSystemModel().get();

        if (model->hasCoverageFadeTime()) {
            for (Particle& P : m_particle) {
                const std::pair<float, float>& fadeTime = model->coverageFadeTime(P.emitterIndex);

                const float expire  = clamp((P.expireTime - absoluteTime) / (fadeTime.first + 1e-6f), 0.0f, 1.0f);
                const float inspire = clamp((absoluteTime - P.spawnTime)  / (fadeTime.second + 1e-6f), 0.0f, 1.0f);
                
                P.coverage = min(expire, inspire);
            }
        }

        if (model->hasExpireTime()) {
            for (int i = 0; i < m_particle.size(); ++i) {
                Particle& P = m_particle[i];
                if (absoluteTime > P.expireTime) {
                    markChanged();
                    m_particle.fastRemove(i);
                    --i;
                }
            }
        }

        // TODO: Morgan have the model indicate if physics is enabled for this system
        applyPhysics(float(absoluteTime), float(deltaTime));
    }
}


void ParticleSystem::spawnParticles(SimTime absoluteTime, SimTime deltaTime) {
    const Array<shared_ptr<ParticleSystemModel::Emitter>>& emitterArray = particleSystemModel()->emitterArray();
    for (int e = 0; e < emitterArray.size(); ++e) {
        const shared_ptr<ParticleSystemModel::Emitter>& emitter = emitterArray[e];
        const int newParticlesToEmit = roundStochastically(emitter->specification().rateCurve.evaluate(absoluteTime - m_initTime) * deltaTime);
        emitter->spawnParticles(this, newParticlesToEmit, absoluteTime, absoluteTime - m_initTime, deltaTime, e);
    }
}


} // namespace
