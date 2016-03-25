/**
  \file GLG3D/ParticleSystem.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2015-08-30
  \edited  2016-02-22
 
 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2016, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */
#ifndef GLG3D_ParticleSystem_h
#define GLG3D_ParticleSystem_h

#include "G3D/platform.h"
#include "G3D/G3DString.h"
#include "G3D/g3dmath.h"
#include "G3D/enumclass.h"
#include "G3D/Cylinder.h"
#include "G3D/Spline.h"
#include "G3D/enumclass.h"
#include "G3D/Vector3.h"
#include "G3D/WeakCache.h"
#include "G3D/Random.h"
#include "GLG3D/Scene.h"
#include "GLG3D/UniversalMaterial.h"
#include "GLG3D/Entity.h"
#include "GLG3D/ParticleSystemModel.h"
#include "GLG3D/VisibleEntity.h"

using std::unique_ptr;

namespace G3D {

class ParticleSurface;
class MeshShape;
class ParticleSystem;
class ParticleSystemModel;

/** For efficiency, individual Particles use materials that have been pre-registered with the ParticleSystem
    that are addressed with these handles. Particle Materials are not collected until the Entity that registered
    them is no longer in memory, even if no individual Particle uses that Material. Particle materials
    are packed into a single large texture array, so the memory requirements are that of the largest material
    times the number of materials. 
    
   \sa ParticleSystem
*/
class ParticleMaterial : public ReferenceCountedObject {
private:
    friend class ParticleSystem;
    friend class ParticleSurface;

    /** A single material for all particle systems, using a 2D_ARRAY texture */
    static shared_ptr<UniversalMaterial>     s_material;

    /** Back-pointers used for reallocating slots in s_material when it is garbage collected */
    static Array<weak_ptr<ParticleMaterial>> s_materialArray;

    /** Index into the s_material 2D_ARRAY texture layer. */
    int             m_textureIndex;

    /** Width in texels of the part of the layer that is used by this ParticleMaterial */
    int             m_texelWidth;

    ParticleMaterial(int index, int texelWidth) : m_textureIndex(index), m_texelWidth(texelWidth) {}

    /** Adds in newMaterial to the s_material array, and returns the layer it resides on. */
    static int insertMaterial(shared_ptr<UniversalMaterial> newMaterial);

public:

    /** Allocates a material handle for use with particles. This call is very slow and should not be made per frame. */
    static shared_ptr<ParticleMaterial> create(const UniversalMaterial::Specification& material);

    /** Allocates a material handle for use with particles. This call is very slow and should not be made per frame. */
    static shared_ptr<ParticleMaterial> create(const shared_ptr<UniversalMaterial>& material);

    /** No garbage collection of now-unused materials is performed during the destructor because that would slow
        scene loading. Instead, unused materials are repurposed during creation of new materials. You can force
        reallocation to clear memory by invoking ParticleMaterial::freeAllUnusedMaterials */
    ~ParticleMaterial();

    static void freeAllUnusedMaterials();
};



/**
  \brief An Entity composed of multiple translucent particles.

  Particles always face the camera's z-axis. This causes them to produce inconsistent results
  for algorithms that use multiple views.

  Assumes that particles are transparent and thus do not write to depth (except for shadow maps)
  or motion buffers.

  Performs "soft particle" fade out near surfaces to hide the intersection with solid geometry.

  Renders substantially faster when order-independent transparency is enabled on the Renderer.
  In that case, static (canMove == false) ParticleSystems perform no CPU work per frame.

  In sorted transparency mode, whole ParticleSystem surfaces are sorted (against all other transparent surfaces)
  and then particles are sorted within each ParticleSystem surface. This is necessary so that
  particles interact reasonably with glass and other transparent surfaces. However, this
  means that two ParticleSystems
  that overlap each other will not have their particles sorted together correctly.

  \beta 

  \sa ParticleMaterial

  Memory diagram (: = weak_ptr, | = shared_ptr )
  \verbatim
             .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .
             :                                                     : particleSystem
             v                  m_block                            :                     blockArray
      [ ParticleSystem ] -----------------------------> [ ParticleSystem::Block ] <-------------------- [ ParticleBuffer ]
             ^                                              ^      :
             | m_entity                                     |      : surface
             |                  m_block                     |      : 
      [ParticleSurface ] -----------------------------------'      :
             ^                                                     :
             :. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. :
  \endverbatim
*/
class ParticleSystem : public VisibleEntity {
public:
    friend class ParticleSurface;
    friend class ParticleSystemModel::Emitter;

    /** Particle-specific forces that can be shared between ParticleSystem instances. */
    class PhysicsEnvironment : public ReferenceCountedObject {
    public:

        /** Magnitude of small random disturbances of particles. This is currently applied in the
            local reference frame of the particles, for performance.*/
        float                           maxBrownianVelocity;

        /** m/s */
        Vector3                         windVelocity;

        /** m/s^2 */
        Vector3                         gravitationalAcceleration;

    private:

        PhysicsEnvironment() : 
            maxBrownianVelocity(0.0f), 
            windVelocity(Vector3::zero()), 
            gravitationalAcceleration(Vector3::zero()) {}

    public:

        static shared_ptr<PhysicsEnvironment> create() {
            return shared_ptr<PhysicsEnvironment>(new PhysicsEnvironment());
        }
    };

    class Particle {
    public:
        // The implementation assumes that the following values are in this order
        // and tightly packed in memory. This allows memory mapping and SSE operations.

        /** Relative to the ParticleSystem's frame.            
            This is either in object or world space depending on the value of the
            ParticleSystem::particlesAreInWorldSpace flag. */
        Point3                  position;

        /** Rotation about the view's z-axis. Purely cosmetic. \sa angularVelocity */
        float                   angle;

        /** In world-space units. */
        float                   radius;

        /** Scales the material's own coverage. Useful for fading out particles. */
        float                   coverage;

        /** Arbitrary data visible in the shader as an additional attribute. */
        float                   userdataFloat;

        /** Used for simulation in some animation modes. Not mapped to the GPU. */
        // Stored here in the structure for SIMD 16-byte alignment of velocity relative to the position.
        float                   mass;

        /** Used for simulation in some animation modes. Not mapped to the GPU.  
            This is either in object or world space depending on the value of the
            ParticleSystem::particlesAreInWorldSpace flag.

            \sa angularVelocity 
            \beta Velocity or previous position will likely be mapped to the GPU in the future
            for rendering motion blur.
           */
        Vector3                 velocity;

        /** Used for simulation in some animation modes. Not mapped to the GPU. */
        float                   angularVelocity;
        // End packed

        /** Relative to  Scene::time() baseline */
        SimTime                 spawnTime;

        /** Relative to  Scene::time() baseline */
        SimTime                 expireTime;

        /** Zero means no friction/air resistance, higher values
            represent increased drag. 
            
            Used for simulation in some animation modes. Not mapped to the GPU. */
        float                   dragCoefficient;

        /** Mapped to the GPU as a texture layer index. */
        shared_ptr<ParticleMaterial> material;

        /** Arbitrary data visible in the shader as additional attributes. */
        uint16                  userdataInt;

        /** Not mapped to the GPU. */
        uint16                  emitterIndex;

        /** Useful for computing buoyancy */
        float density() const {
             return mass / (radius * radius * radius * (4.0f / 3.0f) * pif());
        }

        Particle() : 
            position(Vector3::zero()),
            angle(0),
            radius(0.1f),
            coverage(1.0f),
            userdataFloat(0.0f),
            mass(0.1f),
            velocity(Vector3::zero()),
            angularVelocity(0),
            spawnTime(0),
            expireTime(finf()),
            dragCoefficient(0.5f),
            userdataInt(0),
            emitterIndex(0)
        {}
    };

protected:

    shared_ptr<ParticleSystemModel> particleSystemModel() const;

    ////////////////////////////////////////////////////////////////////////
    // Shared state

    
    /** Memory management element for the ParticleBuffer */
    class Block {
    public:
        weak_ptr<ParticleSystem>    particleSystem;

        /** If NULL, this block may be garbage collected. */
        weak_ptr<ParticleSurface>   surface;

        /** In s_particleBuffer. */
        int                         startIndex;

        /** Number of elements currently in use. */
        int                         count;

        /** Total size (in elements) reserved, including the count that are in use. */
        int                         reserve;

        Block
           (const shared_ptr<ParticleSystem>&   ps, 
            const shared_ptr<ParticleSurface>&  s, 
            int                                 startIndex, 
            int                                 reserve) :

            particleSystem(ps), surface(s), startIndex(startIndex), count(0), reserve(reserve) {

            debugAssert(notNull(ps));
            debugAssert(notNull(s));
        }
    };

    /** Not threadsafe */
    class ParticleBuffer {
    public:
        /** Mask for materialProperties[3] */
        static const uint16         CASTS_SHADOWS    = 0x1;

        /** Mask for materialProperties[3] */
        static const uint16         RECEIVES_SHADOWS = 0x2;

        shared_ptr<VertexBuffer>    vertexBuffer;

        /** Center of each particle, world-space XYZ, and the angle in GL_FLOAT x 4 */
        AttributeArray              position;

        /** world-space radius, coverage, userdata in GL_FLOAT x 3 */
        AttributeArray              shape;

        /** material index, texture-space width/height in pixels (packed to top-left),
            Entity expressive rendering flags (e.g.: casts shadows, receives shadows), and
            userdataInt in GL_UNSIGNED_SHORT x 4 format  */
        AttributeArray              materialProperties;

        /** Only used for sorted transparency. Recomputed for each draw call based on a depth sort. GL_UNSIGNED_INT */
        IndexStream                 indexStream;

        /** Elements actually in use. */
        int                         count;

        /** A list of all Blocks in s_particleBuffer, which include potentially live ParticleSystems and where their particles
            are stored in s_particleBuffer. Used for managing allocation of space within s_particleBuffer.     
        */
        Array<shared_ptr<Block>>    blockArray;

        /** Total elements allocated, including those in use */
        int reserve() const {
            return position.size();
        }

        shared_ptr<Block> alloc
            (const shared_ptr<ParticleSystem>& particleSystem, 
             const shared_ptr<ParticleSurface>& surface,
             int numElements);
        
        /** Remove all blocks that are not in use because
            the underlying ParticleSurface referencing it is gone */
        void removeUnusedBlocks();

        void free(const shared_ptr<Block>& block);

        void compact(const int newReserveCount);

        /** Allocate the vertexBuffer and all associated attribute arrays, throwing away all old data. */
        void allocateVertexBuffer(const int newReserve);
    };


    class SortProxy {
    public:
        /** In camera space */
        float           z;

        /** Of the particle in s_particleBuffer */
        int             index;

        bool operator<(const SortProxy& other) const {
            return z < other.z;
        }

        bool operator>(const SortProxy& other) const {
            return z > other.z;
        }
    };

    
    /** Particle data across all ParticleSystem instances. canMove = true, written every frame.
    
       <code>[ Block1: (particle system 1) (space for PS1 to grow) | Block2: (PS2) (PS2 reserve space) | .... ]</code>
    */
    static ParticleBuffer               s_particleBuffer;

    /** Used when sorting values to compute s_particleBuffer.indexStream for sorted transparency */
    static Array<SortProxy>             s_sortArray;

    /** Used to set the preferLowResolutionTransparency 
        hint on the surfaces created from every particle system. */
    static bool                         s_preferLowResolutionTransparency;

   
    Array<Particle>                     m_particle;
    bool                                m_particlesChangedSinceBounds;
    bool                                m_particlesChangedSincePose;
    shared_ptr<PhysicsEnvironment>      m_physicsEnvironment;
    
    /** Used for all randomness in the particle system. Not threadsafe. */
    Random                              m_rng;

    /** Should not be changed once the entity is initialized */
    bool                                m_particlesAreInWorldSpace;

    /** For re-use if nothing has changed */
    shared_ptr<ParticleSystem::Block>   m_block;

    /** Time at which this particle system was created, used for 
        sampling the rate curve in ParticleSystemModel */
    SimTime                             m_initTime;

    ParticleSystem();

    /** Computes net forces from the brownian, wind, and gravity values and then 
        applies euler integration to the particles. */
    virtual void applyPhysics(float t, float dt);

    /** Called by onPose */
    virtual void updateBounds();

    void markChanged();

    void init(AnyTableReader& propertyTable);
    void init();

    /** Called from onSimulation */
    void spawnParticles(SimTime absoluteTime, SimTime deltaTime);

public:
    
    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      options);

    /** For programmatic construction at runtime */
    static shared_ptr<ParticleSystem> create 
    (const String&                  name,
     Scene*                         scene,
     const CFrame&                  position,
     const shared_ptr<Model>&       model);

    /** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "Entity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    void setPhysicsEnvironment(const shared_ptr<PhysicsEnvironment>& p) {
        m_physicsEnvironment = p;
    }

    /** If NULL, no physics forces are introduced by the default implementtion of onSimulation */
    const shared_ptr<PhysicsEnvironment>& physicsEnvironment() const {
        return m_physicsEnvironment;
    }

    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray) override;

    /** If canMove(), then computes forces from physicsEnvironment() and applies basic Euler integration of velocity. 
        If the physicsEnvironment is NULL, then there are no forces. */
    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

    void addParticle(const Particle& p) {
        m_particle.append(p);
        markChanged();
    }

    /** Number of particles */
    int size() const {
        return m_particle.size();
    }

    /** \sa fastRemoveParticle */
    void removeParticle(int index) {
        m_particle.remove(index);
        markChanged();
    }

    /** Uses Array::fastRemove
        \sa remove*/
    void fastRemoveParticle(int index) {
        m_particle.fastRemove(index);
        markChanged();
    }

    const Particle& particle(int index) const {
        return m_particle[index];
    }

    /** Subclassing ParticleSystem to override onSimulation is usually easier and more efficient than
        explicitly replacing particles from outside of the class. */
    void setParticle(int index, const Particle& p) {
        m_particle[index] = p;
        markChanged();
    }

    /**
      Particles stored in world space are more efficient to simulate, but cannot
      be easily moved as a group in the scene editor or due to animation. Use world-space particles
      for smoke and other transient particle effects. The emitter is still in object space.

      Object- (Entity-) space particles are relative to the ParticleSystem Entity
      and can be moved as a group by animation and in the scene editor. Use these for
      long-lived particles such as clouds and particles bolted to other Entitys.
    */
    bool particlesAreInWorldSpace() const {
        return m_particlesAreInWorldSpace;
    }

    static void setPreferLowResolutionTransparency(bool b) {
        s_preferLowResolutionTransparency = b;
    }

    /** Defaults to true, only affects OIT */
    static bool preferLowResolutionTransparency() {
        return s_preferLowResolutionTransparency;
    }

}; 

} // namespace

#endif

