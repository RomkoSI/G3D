/**
  \file ParticleSystemModel.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2015-08-30
  \edited  2015-09-17
 
 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */

#include "GLG3D/ParticleSystemModel.h"
#include "GLG3D/ParticleSystem.h"
#include "G3D/Noise.h"
#include "G3D/g3dmath.h"
#include "G3D/Cone.h"

namespace G3D {

ParticleSystemModel::Specification::Specification(const Any& a) {
    *this = Specification();

    if (a.nameBeginsWith("ParticleSystemModel::Emitter")) {        
        emitterArray.append(a);
    } else { 
        a.verifyNameBeginsWith("ParticleSystemModel::Specification");
        Any array = a.get("emitterArray", Any(Any::ARRAY));
        array.getArray(emitterArray);
        hasPhysics = a.get("hasPhysics", true);
    }

}


void ParticleSystemModel::init() {
    // Extract the fade in and out times. 
    m_hasCoverageFadeTime = false;
    m_hasExpireTime = false;
    for (const shared_ptr<Emitter>& emitter : m_emitterArray) {
        const float fi = emitter->specification().coverageFadeInTime;
        const float fo = emitter->specification().coverageFadeOutTime;

        m_hasCoverageFadeTime = m_hasCoverageFadeTime || (fi > 0) || (fo > 0);
        m_coverageFadeTime.append(std::make_pair(fi, fo));
        m_hasExpireTime = m_hasExpireTime || (emitter->specification().particleLifetimeMean < finf());
    }
}


size_t ParticleSystemModel::Specification::hashCode() const {
    size_t h = 0;
    for (const Emitter::Specification& s : emitterArray) {
        h ^= s.hashCode();
    }
    return h;
}


Any ParticleSystemModel::Specification::toAny() const {
    Any a(emitterArray);
    a.setName("ParticleSystemModel::Specification");
    return a;
}


bool ParticleSystemModel::Specification::operator==(const Specification& other) const {
    if (emitterArray.size() != other.emitterArray.size()) {
        return false;
    }

    for (int i = 0; i < emitterArray.size(); ++i) {
        if (! (emitterArray[i] == other.emitterArray[i])) {
            return false;
        }
    }

    return true;
}


shared_ptr<ParticleSystemModel> ParticleSystemModel::create(const Specification& spec, const String& name) {
    return shared_ptr<ParticleSystemModel>(new ParticleSystemModel(spec, name));
}


ParticleSystemModel::ParticleSystemModel(const Specification& spec, const String& name) {
    m_name = name;
    m_emitterArray.resize(spec.emitterArray.size());
    for (int e = 0; e < m_emitterArray.size(); ++e) {
        m_emitterArray[e] = Emitter::create(spec.emitterArray[e]);
    }
    init();
}


ParticleSystemModel::Emitter::Specification::Specification(const Any& a) {
    a.verifyNameBeginsWith("ParticleSystemModel::Emitter");
    *this = Specification();
    AnyTableReader r(a);
    r.getIfPresent("location", location);
    r.getIfPresent("noisePower", noisePower);
    r.getIfPresent("initialDensity", initialDensity);
    r.getIfPresent("rateCurve", rateCurve);
    r.getIfPresent("coverageFadeInTime", coverageFadeInTime);
    r.getIfPresent("coverageFadeOutTime", coverageFadeOutTime);
    r.getIfPresent("particleLifetimeMean", particleLifetimeMean);
    r.getIfPresent("particleLifetimeVariance", particleLifetimeVariance);
    r.getIfPresent("angularVelocityMean", angularVelocityMean);
    r.getIfPresent("angularVelocityVariance", angularVelocityVariance);
    r.getIfPresent("material", material);
    r.getIfPresent("radiusMean", radiusMean);
    r.getIfPresent("radiusVariance", radiusVariance);
    r.getIfPresent("particleMassDensity", particleMassDensity);
    r.getIfPresent("dragCoefficient", dragCoefficient);

    shapeType = Shape::Type::NONE;
    Any shape;
    if (r.getIfPresent("shape", shape)) {
        if (shape.nameBeginsWith("ArticulatedModel") || (shape.type() == Any::STRING)) {
            shapeType = Shape::Type::MESH;
        } else {
            shapeType = Shape::Type(toUpper(shape.name()));
        }

        switch (shapeType) {
        case Shape::Type::BOX:
            box = Box(shape);
            break;

        case Shape::Type::CYLINDER:
            cylinder = Cylinder(shape);
            break;

        case Shape::Type::SPHERE:
            sphere = Sphere(shape);
            break;

        case Shape::Type::MESH:
            mesh = shape;
            break;

        default:
            shape.verify(false, "Shape must be a Box, Cylinder, Sphere, or ArticulatedModel specification");
        }
    }

    r.getIfPresent("velocityDirectionMean", velocityDirectionMean);
    r.getIfPresent("velocityConeAngleDegrees", velocityConeAngleDegrees);
    r.getIfPresent("velocityMagnitudeMean", velocityMagnitudeMean);
    r.getIfPresent("velocityMagnitudeVariance", velocityMagnitudeVariance);
    r.verifyDone();
}


Any ParticleSystemModel::Emitter::Specification::toAny() const {
    Any a;
    a["location"] = location;
    a["noisePower"] = noisePower;
    a["initialDensity"] = initialDensity;
    a["particleMassDensity"] = particleMassDensity;
    if (rateCurve.size() == 1) {
        a["rateCurve"] = rateCurve.evaluate(0);
    } else {
        a["rateCurve"] = rateCurve.toAny("Spline<float>");
    } 
    a["dragCoefficient"] = dragCoefficient;
    a["coverageFadeInTime"] = coverageFadeInTime;
    a["coverageFadeOutTime"] = coverageFadeOutTime;
    a["particleLifetimeMean"] = particleLifetimeMean;
    a["particleLifetimeVariance"] = particleLifetimeVariance;
    a["angularVelocityMean"] = angularVelocityMean;
    a["angularVelocityVariance"] = angularVelocityVariance;
    a["material"] = material;
    switch (shapeType) {
    case Shape::Type::BOX:
        a["shape"] = box;
        break;

    case Shape::Type::CYLINDER:
        a["shape"] = cylinder;
        break;

    case Shape::Type::SPHERE:
        a["shape"] = sphere;
        break;

    case Shape::Type::MESH:
        a["shape"] = mesh;
        break;
    }

    a["velocityDirectionMean"] = velocityDirectionMean;
    a["velocityConeAngleDegrees"] = velocityConeAngleDegrees;
    a["velocityMagnitudeMean"] = velocityMagnitudeMean;
    a[" velocityMagnitudeVariance"] = velocityMagnitudeVariance;
    return a;
}


size_t ParticleSystemModel::Emitter::Specification::hashCode() const {
    return location.hashCode() ^
        (int)(noisePower * 10) ^
        (int)(initialDensity) ^
        rateCurve.hashCode() ^
        (int)(coverageFadeInTime * 100.0f) ^
        (int)(coverageFadeOutTime * 100.0f) ^
        (int)(particleLifetimeMean * 100.0f) ^
        (int)(particleLifetimeVariance * 100.0f) ^
        material.hashCode() ^
        shapeType.hashCode() ^
        box.hashCode() ^
        cylinder.hashCode() ^
        sphere.hashCode() ^
        velocityDirectionMean.hashCode() ^
        (int)(velocityConeAngleDegrees * 100.0f) ^
        (int)(velocityMagnitudeMean * 100.0f) ^
        (int)(velocityMagnitudeVariance * 100.0f);
}


bool ParticleSystemModel::Emitter::Specification::operator==(const Specification& other) const {
    return (other.location == location) &&
        (other.shapeType == shapeType) &&
        (other.noisePower == noisePower) &&
        (other.initialDensity == initialDensity) &&
        (other.rateCurve == rateCurve) &&
        (other.coverageFadeInTime == coverageFadeInTime) &&
        (other.coverageFadeOutTime == coverageFadeOutTime) &&
        (other.particleLifetimeMean == particleLifetimeMean) &&
        (other.particleLifetimeVariance == particleLifetimeVariance) &&
        (other.material == material) &&
        (other.box == box) &&
        (other.cylinder == cylinder) &&
        (other.sphere == sphere) &&
        (other.velocityDirectionMean == velocityDirectionMean) &&
        (other.velocityConeAngleDegrees == velocityConeAngleDegrees) &&
        (other.velocityMagnitudeMean == velocityMagnitudeMean) &&
        (other.velocityMagnitudeVariance == velocityMagnitudeVariance) &&
        (other.radiusMean == radiusMean) &&
        (other.radiusVariance == radiusVariance) &&
        (other.angularVelocityMean == angularVelocityMean) &&
        (other.angularVelocityVariance == angularVelocityVariance) &&
        (other.particleMassDensity == particleMassDensity) &&
        (other.dragCoefficient == dragCoefficient);
}


ParticleSystemModel::Emitter::Emitter(const Specification& s) : m_specification(s) {
    switch (s.shapeType) {
    case Shape::Type::MESH:
        // Load the mesh
        debugAssertM(false, "TODO");
        break;

    case Shape::Type::BOX:
        m_spawnShape = shared_ptr<Shape>(new BoxShape(s.box));
        break;

    case Shape::Type::CYLINDER:
        m_spawnShape = shared_ptr<Shape>(new CylinderShape(s.cylinder));
        break;

    case Shape::Type::SPHERE:
        m_spawnShape = shared_ptr<Shape>(new SphereShape(s.sphere));
        break;

    default:
        alwaysAssertM(false, "Illegal spawn shape");
    }

    m_material = ParticleMaterial::create(s.material);
}


void ParticleSystemModel::Emitter::spawnParticles(ParticleSystem* system, int newParticlesToEmit, SimTime absoluteTime, SimTime relativeTime, SimTime deltaTime, int emitterIndex) const {
    // Give up and just place a particle anywhere in the noise field if we can't find
    // a good location by rejection sampling after this many tries.
    const int MAX_NOISE_SAMPLING_TRIES = 20;

    Random& rng = Random::common();
    Noise& noise = Noise::common();
    
    debugAssert(notNull(m_spawnShape));

    for (int i = 0; i < newParticlesToEmit; ++i) {
        ParticleSystem::Particle particle;
        particle.emitterIndex = emitterIndex;

        for (int j = 0; j < MAX_NOISE_SAMPLING_TRIES; ++j) {
            switch (m_specification.location) {
            case SpawnLocation::FACES:
                alwaysAssertM(m_spawnShape->type() == Shape::Type::MESH, "SpawnLocation::FACES requires a mesh");
                {
                    const shared_ptr<MeshShape>& mesh = dynamic_pointer_cast<MeshShape>(m_spawnShape);
                    const Array<int>& indexArray = mesh->indexArray();
                    const Array<Point3>& vertexArray = mesh->vertexArray();
                    const int i = rng.integer(0, indexArray.size() / 3 - 1) * 3;
                    particle.position = (vertexArray[indexArray[i]] + vertexArray[indexArray[i + 1]] + vertexArray[indexArray[i + 2]]) / 3.0f;
                }
                break;

            case SpawnLocation::VERTICES:
                alwaysAssertM(m_spawnShape->type() == Shape::Type::MESH, "SpawnLocation::VERTICES requires a mesh");
                {
                    const shared_ptr<MeshShape>& mesh = dynamic_pointer_cast<MeshShape>(m_spawnShape);
                    particle.position = mesh->vertexArray().randomElement();
                }
                break;

            case SpawnLocation::SURFACE:
                m_spawnShape->getRandomSurfacePoint(particle.position);
                break;

            case SpawnLocation::VOLUME:
                particle.position = m_spawnShape->randomInteriorPoint();
                if (particle.position.isNaN()) {
                    // For poorly formed meshes...or even good ones with really bad luck...
                    // sometimes randomInteriorPoint will fail. In this case, generate a surface
                    // point, which is technically "on the interior" still, just poorly distributed.
                    m_spawnShape->getRandomSurfacePoint(particle.position);
                }
                break;
            } // switch

            // Unique values every 5m for the lowest frequencies
            if (m_specification.noisePower == 0.0f) {
                // Accept any generated position
                break;
            } else { 
                const Vector3int32 intPos(particle.position * ((1 << 16) / (5 * units::meters())));
                const float acceptProbability = pow(max(0.0f, noise.sampleFloat(intPos.x, intPos.y, intPos.z, 5)), m_specification.noisePower);

                if (rng.uniform() <= acceptProbability) {
                    // An acceptable position!
                    break;
                }        
            }
        }

        particle.angle = rng.uniform(0.0f, 2.0f * pif());
        particle.radius = fabs(rng.gaussian(m_specification.radiusMean, sqrt(m_specification.radiusVariance)));

        if (m_specification.coverageFadeInTime == 0.0f) {
            particle.coverage = 1.0f;
        } else {
            particle.coverage = 0.0f;
        }

        particle.userdataFloat = 0.0f;
        particle.mass = m_specification.particleMassDensity * (4.0f / 3.0f) * pif() * particle.radius * particle.radius * particle.radius;

        if (m_specification.velocityConeAngleDegrees >= 180) {
            particle.velocity = Vector3::random(rng);
        } else if (m_specification.velocityConeAngleDegrees > 0) {
            const Cone emissionCone(Point3(), 
                m_specification.velocityDirectionMean, 
                m_specification.velocityConeAngleDegrees * 0.5f);
            particle.velocity = emissionCone.randomDirectionInCone(rng);
        } else {
            particle.velocity = m_specification.velocityDirectionMean;
        }
        particle.velocity *= fabs(rng.gaussian(m_specification.velocityMagnitudeMean, m_specification.velocityMagnitudeVariance));
        particle.angularVelocity = rng.gaussian(m_specification.angularVelocityMean, m_specification.angularVelocityVariance);

        particle.spawnTime = absoluteTime;
        particle.expireTime = absoluteTime + fabs(rng.gaussian(m_specification.particleLifetimeMean, m_specification.particleLifetimeVariance));

        particle.dragCoefficient = m_specification.dragCoefficient;
        particle.material = m_material;

        particle.userdataInt = 0;

        if (system->particlesAreInWorldSpace()) {
            // Transform to world space
            particle.position = system->frame().pointToWorldSpace(particle.position);
            particle.velocity = system->frame().vectorToWorldSpace(particle.velocity);
        }

        // Directly add to the particle system
        system->m_particle.append(particle);

    } // for new particles

    if (newParticlesToEmit > 0) {
        system->markChanged();
    }
}

#if 0

void ParticleSystem::spawnFace
   (const FaceScatter&  faceScatter, 
    const ParseOBJ&     parseObj, 
    const Color4&       color, 
    const Matrix4&      transform, 
    int                 materialIndex) {

    const Color4unorm8 c(color.pow(1.0f / SmokeVolumeSet::PARTICLE_GAMMA));

    Random rnd(10000, false);
    m_particle.resize(0);
    m_physicsData.resize(0);
    AABox bounds;

    for (ParseOBJ::GroupTable::Iterator git = parseObj.groupTable.begin(); git.isValid(); ++git) {
        const shared_ptr<ParseOBJ::Group>& group = git->value;
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin(); mit.isValid(); ++mit) {
            const shared_ptr<ParseOBJ::Mesh>& mesh = mit->value;
            for (int f = 0; f < mesh->faceArray.size(); ++f) {
                if (rnd.uniform() <= faceScatter.particlesPerFace) {
                    const ParseOBJ::Face& face = mesh->faceArray[f];
                    Particle& particle = m_particle.next();
                    PhysicsData& physicsData = m_physicsData.next();

                    // Use the average vertex as the position
                    Point3 vrt[10];
                    particle.position = Point3::zero();
                    for (int v = 0; v < face.size(); ++v) {
                        vrt[v] = (transform * Vector4(parseObj.vertexArray[face[v].vertex], 1.0f)).xyz();
                        particle.position += vrt[v];
                    }
                    particle.position /= float(face.size());

                    const Vector3& normal = (vrt[1] - vrt[0]).cross(vrt[2] - vrt[0]).direction();
                    particle.position += faceScatter.explodeDistance * normal;

                    particle.normal = normal;
                    particle.materialIndex = materialIndex;
                    particle.color    = c;
                    particle.angle    = rnd.uniform(0.0f, 2.0f) * pif(); 
                    particle.radius   = min(faceScatter.maxRadius, faceScatter.radiusScaleFactor * pow((particle.position - vrt[0]).length(), faceScatter.radiusExponent));
                    particle.emissive = 0;
                    physicsData.angularVelocity = rnd.uniform(-1.0f, 1.0f) * (360.0f * units::degrees()) / (20.0f * units::seconds());
                    bounds.merge(particle.position);
                }
            }  // face
        } // mesh
    } // group

    // Center
    if (faceScatter.moveCenterToOrigin) {
        const Vector3& offset = -bounds.center();
        for (int i = 0; i < m_particle.size(); ++i) {
            m_particle[i].position += offset;
        }

        m_lastObjectSpaceAABoxBounds = bounds + offset;
    } else {
        m_lastObjectSpaceAABoxBounds = bounds;
    }
}

void ParticleSystem::spawnSurface() {
    for (int i = 0; i < numParticles; ++i) {
        Particle& particle = m_particle.next();
        PhysicsData& physicsData = m_physicsData.next();

        Vector3 n;
        mesh.getRandomSurfacePoint(particle.position, n);
        particle.position = (transform * Vector4(particle.position, 1.0f)).xyz() + n * explode;

        particle.materialIndex = materialIndex;
        particle.color    = c;
        particle.angle    = rnd.uniform(0.0f, 2.0f) * pif(); 
        particle.radius   = radius;
        particle.emissive = 0;
        physicsData.angularVelocity = rnd.uniform(-1.0f, 1.0f) * (360.0f * units::degrees()) / (20.0f * units::seconds());
        bounds.merge(particle.position);
    }

    // Center
    const Vector3 offset = -bounds.center();
    for (int i = 0; i < m_particle.size(); ++i) {
        m_particle[i].position += offset;
    }

    m_lastObjectSpaceAABoxBounds = bounds + offset;
}


void ParticleSystem::setFogModel(const Color4& color, int materialIndex) {
    const Color4unorm8 c(Color4(color.r, color.g, color.b, pow(color.a, 1.0f / SmokeVolumeSet::PARTICLE_GAMMA)));
    Random rnd;
    Noise noise;

    m_particle.resize(5000);
    m_physicsData.resize(m_particle.size());
    for (int i = 0; i < m_particle.size(); ++i) {
        SmokeVolumeSet::Particle& particle = m_particle[i];
        PhysicsData& physicsData = m_physicsData[i];

        // Fill sponza 
        particle.position = Point3(rnd.uniform(-16, 14), pow(rnd.uniform(0, 1), 4.0f) * 4 + 0.5f, rnd.uniform(-6.5, 6.5));

        /*
        if (rnd.uniform() < 0.3f) {
            // Arena bridge
            particle.position = Point3(rnd.uniform(-14, 14) - 2.0f, pow(rnd.uniform(0, 1), 5.0f) * 16.0f + 7.2f, rnd.uniform(-8, 8) - 28.0f);
        } else {
            // Arena everywhere
            particle.position = Point3(rnd.uniform(-20, 20), pow(rnd.uniform(0, 1), 5.0f) * 12.0f + 7.2f, rnd.uniform(-20, 20));
        }
        */

        particle.angle = rnd.uniform(0.0f, 2.0f) * pif(); 
        particle.materialIndex = materialIndex;
        particle.radius = 1.0f;
        particle.emissive = 0.0f;
        particle.color = c;
        int r = rnd.integer(0, 15);
        particle.color.r = unorm8::fromBits(r + particle.color.r.bits());
        particle.color.g = unorm8::fromBits(r + particle.color.g.bits());
        particle.color.b  = unorm8::fromBits(r + particle.color.b.bits());
        physicsData.angularVelocity = 0;//rnd.uniform(-1.0f, 1.0f) * (360.0f * units::degrees()) / (5.0f * units::seconds());

        // Unique values every 5m for the lowest frequencies
        const Vector3int32 intPos(particle.position * ((1 << 16) / (5 * units::meters())));
        const float acceptProbability = pow(max(0.0f, noise.sampleFloat(intPos.x, intPos.y, intPos.z, 5)), 2.0f);

        if (rnd.uniform() > acceptProbability) {
            // Reject this puff
            --i;
        }
    }

    // Ensure that this matches
    m_physicsData.resize(m_particle.size());
}

void ParticleSystemModel::setSurfaceScatter
   (const ParseOBJ&         parseObj, 
    const Color4&           color, 
    const Matrix4&          transform, 
    int                     materialIndex) {

    const Color4unorm8 c(Color4(color.r, color.g, color.b, pow(color.a, 1.0f / SmokeVolumeSet::PARTICLE_GAMMA)));
    const int numParticles = 500;
    const float radius = 3.0f;

    // Push out along normal
    const float explode = radius * 0.25f;

    Random rnd(10000, false);
    m_particle.resize(0);
    m_physicsData.resize(0);
    AABox bounds;

    const Array<Point3>& vertex = parseObj.vertexArray;
    Array<int>    index;

    // Extract the mesh
    for (ParseOBJ::GroupTable::Iterator git = parseObj.groupTable.begin(); git.isValid(); ++git) {
        const shared_ptr<ParseOBJ::Group>& group = git->value;
        for (ParseOBJ::MeshTable::Iterator mit = group->meshTable.begin(); mit.isValid(); ++mit) {
            const shared_ptr<ParseOBJ::Mesh>& mesh = mit->value;
            for (int f = 0; f < mesh->faceArray.size(); ++f) {
                const ParseOBJ::Face& face = mesh->faceArray[f];
                for (int v = 0; v < 3; ++v) {
                    index.append(face[v].vertex);
                }
            }  // face
        } // mesh
    } // group

    // Construct a mesh
    MeshShape mesh(vertex, index);
}

#endif

} // G3D

