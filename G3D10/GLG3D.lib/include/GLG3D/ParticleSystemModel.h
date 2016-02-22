/**
  \file GLG3D/ParticleSystemModel.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2015-08-30
  \edited  2016-02-10
 
 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2016, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */
#ifndef GLG3D_ParticleSystemModel_h
#define GLG3D_ParticleSystemModel_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/enumclass.h"
#include "G3D/Spline.h"
#include "G3D/g3dmath.h"
#include "G3D/Box.h"
#include "G3D/Cylinder.h"
#include "G3D/Sphere.h"
#include "G3D/Vector3.h"
#include "GLG3D/Model.h"
#include "GLG3D/Shape.h"
#include "GLG3D/ArticulatedModel.h"
#include <utility> // for std::pair

namespace G3D {

class ParticleMaterial;
class ParticleSystem;
class MeshShape;

/** 
  Shared state for quickly instantiating particular types of ParticleSystems. 
 */
class ParticleSystemModel : public Model {
    friend class ParticleSystem;
public:

    /** All properties are in object space */
    class Emitter : public ReferenceCountedObject {
        friend class ParticleSystemModel;
        friend class ParticleSystem;

    public:

        G3D_DECLARE_ENUM_CLASS (SpawnLocation,

           /** Only available for a mesh shape */
           VERTICES,
         
           /** All faces are treated as equal probability for spawn locations. Only available for a mesh shape. */
           FACES,
         
           /** All faces are treated as a single surface with uniform probability with respect to area  */
           SURFACE,

           VOLUME
        );


        class Specification {
        public:

            SpawnLocation                       location;

            /** 0 = no noise, 1 = Perlin noise, 2 = squared perlin noise, etc. */
            float                               noisePower;

            /** Density to instantly spawn when the particle system is instantiated. 
                Units vary by location type:
                
                Volumetric: particles/m^3
                Surface: particles/m^2 of surface
                Vertices: fraction of unique vertices to cover with particles
                Faces: fraction of unique faces to cover with particles
                Origin: absolute number of particles to spawn
                */
            float                               initialDensity;

            /** Units are those of initialDensity per second. The curve is rate vs. time in seconds. 
                This is frequently initialized from a single float. */
            Spline<float>                       rateCurve;

            /** Default is 0 */
            SimTime                             coverageFadeInTime;

            /** Default is 0 */
            SimTime                             coverageFadeOutTime;

            /** Can be infinity. Gaussian distribution */
            SimTime                             particleLifetimeMean;

            /** Defaults to zero */
            SimTime                             particleLifetimeVariance;

            UniversalMaterial::Specification    material;

            Box                                 box;
            
            Cylinder                            cylinder;

            Sphere                              sphere;

            ArticulatedModel::Specification     mesh;

            /** Which shape was specified? mesh, box, cylinder, or sphere? */
            Shape::Type                         shapeType;

            /** Automatically normalized on load */
            Vector3                             velocityDirectionMean;

            /** Maximum angle off velocityDirectionMean. Defaults to 180. */
            float                               velocityConeAngleDegrees;

            /** Gaussian distribution */
            float                               velocityMagnitudeMean;
            float                               velocityMagnitudeVariance;

            float                               radiusMean;
            float                               radiusVariance;

            float                               angularVelocityMean;
            float                               angularVelocityVariance;

            /** kg/m^3 */
            float                               particleMassDensity;

            float                               dragCoefficient;

            Specification() : 
                location(SpawnLocation::SURFACE),
                noisePower(0.0f),
                initialDensity(0.0f),
                rateCurve(0.0f),
                coverageFadeInTime(0.0f),
                coverageFadeOutTime(0.0f),
                particleLifetimeMean(finf()),
                particleLifetimeVariance(0.0f),
                shapeType(Shape::Type::SPHERE),
                velocityDirectionMean(0.0f, 0.0f, 0.0f),
                velocityConeAngleDegrees(0.0f),
                velocityMagnitudeMean(0.0f), 
                velocityMagnitudeVariance(0.0f), 
                radiusMean(1.0f), 
                radiusVariance(0.0f),
                angularVelocityMean(0.0f),
                angularVelocityVariance(0.0f),
                particleMassDensity(0.1f),
                dragCoefficient(0.1f) {}

            Specification(const Any& a);

            size_t hashCode() const;

            bool operator==(const Specification& other) const;

            Any toAny() const;
        };
    protected:

        Specification                   m_specification;
        shared_ptr<Shape>               m_spawnShape;
        shared_ptr<ParticleMaterial>    m_material;
        
        explicit Emitter(const Specification& s);

    public:

        static shared_ptr<Emitter> create(const Specification& s) {
            return shared_ptr<Emitter>(new Emitter(s));
        }
        
        const Specification& specification() const {
            return m_specification;
        }

        /** \param initialSpawn True during instantiation of the ParticleSystem.  */
        virtual void spawnParticles
           (ParticleSystem*             system,
            int                         numParticlesToEmit, 
            SimTime                     time,
            SimTime                     timeSinceParticleSystemInit,
            SimTime                     deltaTime, 
            int                         emitterIndex) const;
    };

public:

    /**
      A single ParticleSystemModel::Emitter::Specification will cast directly to a 
      ParticleSystemModel::Specification at Any parsing time.
     */
    class Specification {
    public:

        Array<Emitter::Specification>    emitterArray;

        Specification() {}

        explicit Specification(const Emitter::Specification& spec) {
            emitterArray.append(spec);
        }

        Specification(const Any& a);

        size_t hashCode() const;

        bool operator==(const Specification& other) const;

        Any toAny() const;
    };
protected:

    String                              m_name;
    Array<shared_ptr<Emitter>>          m_emitterArray;

    /** These are stored explicitly to avoid chasing shared pointers during
        simulation */
    Array<std::pair<float, float>>      m_coverageFadeTime;
    bool                                m_hasCoverageFadeTime;
    bool                                m_hasExpireTime;

    ParticleSystemModel(const Specification& spec, const String& name);
    void init();

public:
    
    static shared_ptr<ParticleSystemModel> create(const Specification& specification, const String& name = "");
    static lazy_ptr<Model> lazyCreate(const Specification& s, const String& name = "") {
        return lazy_ptr<Model>( [s, name] { return ParticleSystemModel::create(s, name); } );
    }

    /** Fade in and fade out time for emitter e. Used during ParticleSystem::onSimulation */
    const std::pair<float, float>& coverageFadeTime(int e) const {
        return m_coverageFadeTime[e];
    }

    /** True if any fadeTime is non-zero */
    bool hasCoverageFadeTime() const {
        return m_hasCoverageFadeTime;
    }

    /** True if any emitter's mean expiration time is finite */
    bool hasExpireTime() const {
        return m_hasExpireTime;
    }

    /** \beta We should support multiple materials per model */
    ParticleSystemModel() {}

    virtual const String& name() const override {
        return m_name;
    }

    virtual const String& className() const override {
        static String cName = "ParticleSystemModel";
        return cName;
    }

    const Array<shared_ptr<Emitter>>& emitterArray() const {
        return m_emitterArray;
    }
};

} // namespace G3D

#endif
