#include "RayTracer.h"
#include "App.h"

static const float BUMP_EPSILON = 0.5f * units::millimeters();

/** Returns a point slightly offset from the surfel on the side that traceDirection points into */
static Point3 bump(const shared_ptr<Surfel>& surfel, const Vector3& traceDirection) {
    return surfel->position + surfel->geometricNormal * (sign(traceDirection.dot(surfel->geometricNormal)) * BUMP_EPSILON);
}

typedef float AveragePower;

RayTracer::Settings::Settings() :  
    width(160), 
    height(90),
    multithreaded(true),
    useTree(true),
    sqrtNumPrimaryRays(1),
    numBackwardBounces(5),
    checkFinalVisibility(false) {

#   ifdef G3D_DEBUG
        // If we're debugging, then by default we probably don't want threads
        multithreaded = false;
#   endif
}


RayTracer::Settings::PhotonMapSettings::PhotonMapSettings() :
    numEmitted(500000),
    numBounces(4),
    minGatherRadius(1 * units::centimeters()),
    maxGatherRadius(40 * units::centimeters()),
    radiusBroadeningRate(0.4f) {}


RayTracer::Stats::Stats() : 
    lights(0),
    triangles(0),
    pixels(0),
    buildTriTreeTimeMilliseconds(0),
    photonTraceTimeMilliseconds(0),
    buildPhotonMapTimeMilliseconds(0),
    rayTraceTimeMilliseconds(0),
    storedPhotons(0) {}


RayTracer::RayTracer() : m_triTreeUpdateTime(0), m_photonMapUpdateTime(0) {
    m_threadData.resize(System::numCores());
    for (int i = 0; i < m_threadData.size(); ++i) {
        // Use a different seed for each and do not be threadsafe
        m_threadData[i].rnd = shared_ptr<Random>(new Random(i, false));
    }
}


RayTracer::~RayTracer() {
}


shared_ptr<RayTracer> RayTracer::create(const shared_ptr<Scene>& scene) {
    shared_ptr<RayTracer> ptr(new RayTracer());
    ptr->m_scene = scene;
    return ptr;
}


/** Total indirect illumination emitted by this source */
static AveragePower indirectPower(const shared_ptr<Light>& light) {
    if (light->producesIndirectIllumination()) {
        return light->emittedPower().average();
    } else {
        return 0.0f;
    }
}


void RayTracer::computeTotalIndirectProducingLightPower() {
    m_totalIndirectProducingLightPower = Power3::zero();

    const Array< shared_ptr<Light> >& lightArray = m_lighting.lightArray;

    for (int i = 0; i < lightArray.size(); ++i) {
        const shared_ptr<Light>& light = lightArray[i];
        if (light->producesIndirectIllumination()) {
            m_totalIndirectProducingLightPower += light->emittedPower();
        }
    }
}


void RayTracer::maybeUpdatePhotonMap() {
    if ((m_scene->lastVisibleChangeTime() > m_photonMapUpdateTime) ||
        (m_scene->lastLightChangeTime() > m_photonMapUpdateTime)) {
        app->drawMessage("Tracing Photons");

        m_photonMap.clear(m_settings.photon.maxGatherRadius, 26500);
        m_buildPhotonMapTimeMilliseconds = 0;
        m_photonTraceTimeMilliseconds    = 0;

        
        computeTotalIndirectProducingLightPower();
            
        if ((m_settings.photon.numEmitted > 0) && m_totalIndirectProducingLightPower.nonZero()) {

            const int numThreads = m_settings.multithreaded ? System::numCores() : 1;
            m_photonList.clear();
            m_photonList.resize(numThreads);
            
            {
                const RealTime start = System::time();
                
                GThread::runConcurrently2D
                    (Point2int32(0, 0),
                     Point2int32(1, m_settings.photon.numEmitted),
                     this,
                     &RayTracer::traceOnePhoton,
                     numThreads);
                
                m_photonTraceTimeMilliseconds = float((System::time() - start) / units::milliseconds());
            }
            
            {
                const RealTime start = System::time();
                
                for (int t = 0; t < numThreads; ++t) {
                    m_photonMap.insert(m_photonList[t]);
                }
                
                m_buildPhotonMapTimeMilliseconds = float((System::time() - start) / units::milliseconds());
            }
        
        } // If compute photons

        // Record the time at which we updated the photon map
        m_photonMapUpdateTime = System::time();

        m_photonMap.debugPrintStatistics();
    }
}


void RayTracer::emitPhoton(Random& rnd, Photon& photon) {
    const Array< shared_ptr<Light> >& lightArray = m_lighting.lightArray;

    debugAssertM(lightArray.size() > 0, "Scene must have lights");
    debugAssert(m_totalIndirectProducingLightPower.nonZero());

    int i = 0;
    for (AveragePower p = rnd.uniform(0, m_totalIndirectProducingLightPower.average()) - indirectPower(lightArray[0]);
         (i < lightArray.size() - 1) &&
         (p > 0.0f);
         p -= indirectPower(lightArray[i])) {
        ++i;
    }

    const shared_ptr<Light>& light = lightArray[i];

    // Choose light i
    photon.power = 
        (m_totalIndirectProducingLightPower.average() / m_settings.photon.numEmitted) * 
        (light->emittedPower() / light->emittedPower().average());

    photon.position         = light->frame().translation;
    photon.wi               = -light->randomEmissionDirection(rnd);
    photon.effectRadius     = 0.0f;
}


float RayTracer::photonEffectRadius(float probabilityHint) const {
    // Map low probability to max radius, and follow a root curve

    // Larger exponents mean faster falloff
    const float k = m_settings.photon.radiusBroadeningRate;

    const float a = pow(clamp(probabilityHint, 0.0f, 1.0f), k);
    const float r = lerp(m_settings.photon.maxGatherRadius, m_settings.photon.minGatherRadius, a);
    return r;
}


void RayTracer::traceOnePhoton(int ignoreX, int ignoreY, int threadID) {
    ThreadData& threadData(m_threadData[threadID]);

    Photon photon;
    emitPhoton(*threadData.rnd, photon);
    
    const float MIN_POWER_THRESHOLD = 0.001f / m_settings.photon.numEmitted;

    float probabilityHint = 1.0;

    for (int numBounces = 0; 
         (numBounces < m_settings.photon.numBounces) && 
         (photon.power.sum() > MIN_POWER_THRESHOLD);
         ++numBounces) {
        // Find the first surface
        float distance = finf();
        const shared_ptr<Surfel>& surfel = castRay(photon.position, -photon.wi, distance, false);

        if (isNull(surfel)) {
            return;
        }

        // Store the photon (if this is not the first bounce and it is
        // not a purely specular surface)
        if ((numBounces > 0) && surfel->nonZeroFiniteScattering()) {
            photon.effectRadius = photonEffectRadius(probabilityHint);

            // Update photon position. Store it slightly before it hit the surface
            // to improve filtering later.
            photon.position = surfel->position + photon.wi * min(photon.effectRadius, distance) / 4;

            // Store a copy of this photon
            m_photonList[threadID].append(photon);
        }

        // Scatter
        Color3  weight;
        Vector3 wo;
        float   probabilityScale;
        surfel->scatter(PathDirection::SOURCE_TO_EYE, photon.wi, true, *threadData.rnd, weight, wo, probabilityScale);

        probabilityHint *= probabilityScale;

        // Update photon power and direction
        photon.power *= weight;
        photon.wi = -wo;

        photon.position = bump(surfel, wo);
    }
}


void RayTracer::maybeUpdateTree() {
    if (m_scene->lastVisibleChangeTime() > m_triTreeUpdateTime) {
        app->drawMessage("Building Spatial Data Structure");
        Array< shared_ptr<Surface> > surface;
        m_scene->onPose(surface);

        const RealTime start = System::time();
        m_triTree.setContents(surface);
        m_buildTriTreeTimeMilliseconds = float((System::time() - start) / units::milliseconds());

        m_triTreeUpdateTime = System::time();
    }
}


shared_ptr<Image> RayTracer::render
(const Settings&                     settings, 
 const LightingEnvironment&          lighting, 
 const shared_ptr<Camera>&           camera,
 Stats&                              stats) {

    RealTime start;
    debugAssert(notNull(camera));

    if (m_settings.photon != settings.photon) {
        // Photon settings changed; reset the photon map
        m_photonMapUpdateTime = 0;
    }

    m_camera   = camera;
    m_lighting = lighting;
    m_settings = settings;

    maybeUpdateTree();
    maybeUpdatePhotonMap();

    app->drawMessage("Tracing Backward Rays");

    stats.buildTriTreeTimeMilliseconds   = (float)m_buildTriTreeTimeMilliseconds;
    stats.photonTraceTimeMilliseconds    = (float)m_photonTraceTimeMilliseconds;
    stats.buildPhotonMapTimeMilliseconds = (float)m_buildPhotonMapTimeMilliseconds;
 
    // Allocate the image
    m_image = Image::create(settings.width, settings.height, ImageFormat::RGB32F());

    // Render the image
    start = System::time();
    const int numThreads = settings.multithreaded ? GThread::NUM_CORES : 1;
    traceAllPixels(numThreads);
    stats.rayTraceTimeMilliseconds = float((System::time() - start) / units::milliseconds());

    shared_ptr<Image> temp(m_image);

    stats.lights        = m_lighting.lightArray.size();
    stats.pixels        = settings.width * settings.height;
    stats.triangles     = m_triTree.size();
    stats.storedPhotons = m_photonMap.size();

    // Reset pointers to NULL to allow garbage collection
    m_camera.reset();
    m_image.reset();

    return temp;
}


void RayTracer::traceAllPixels(int numThreads) {

    for (int i = 0; i < m_lighting.lightArray.size(); ++i) {
        const shared_ptr<Light> light(m_lighting.lightArray[i]);
        debugAssertM(light->frame().translation.isFinite(), 
            format("Light %s is not at a finite location", light->name().c_str()));
    }

    GThread::runConcurrently2D
        (Point2int32(0, 0),
         Point2int32(m_image->width(), m_image->height()),
         this,
         &RayTracer::traceOnePixel,
         numThreads);    
}


shared_ptr<Surfel> RayTracer::castRay(const Point3& X, const Vector3& w, float& distance, bool anyHit) const {
    // Distance from P to X
    shared_ptr<Surfel> surfel;

    const Ray ray(X, w);
    if (m_settings.useTree) {

        // Treat the triTree as a tree
        surfel = m_triTree.intersectRay(ray, distance, anyHit);

    } else {

        // Treat the triTree as an array
        Tri::Intersector intersector;
        for (int t = 0; t < m_triTree.size(); ++t) {
            const Tri& tri = m_triTree[t];
            intersector(ray, m_triTree.cpuVertexArray(), tri, anyHit, distance);
        }

        surfel = intersector.surfel();
    }

    return surfel;
}


void RayTracer::traceOnePixel(int x, int y, int threadID) {

    ThreadData& threadData(m_threadData[threadID]);

    Radiance3 L;

    if (m_settings.sqrtNumPrimaryRays == 1) {
        // Through pixel center
        L = traceOnePrimaryRay(float(x) + 0.5f, float(y) + 0.5f, threadData); 
    } else {
        const float denom = 1.0f / m_settings.sqrtNumPrimaryRays;
        for (int i = 0; i < m_settings.sqrtNumPrimaryRays; ++i) {
            for (int j = 0; j < m_settings.sqrtNumPrimaryRays; ++j) {
                // Place the sample taps in a centered regular grid
                L += traceOnePrimaryRay(float(x) + (i + 0.5f) * denom, float(y) + (j + 0.5f) * denom, threadData); 
            }
        }
        L /= square((float)m_settings.sqrtNumPrimaryRays);
    }

    m_image->set(x, y, L);
}


static const Radiance3 BACKGROUND_RADIANCE(0.5f);
Radiance3 RayTracer::traceOnePrimaryRay(float x, float y, ThreadData& threadData) {
    // Ray P, -wi
    const Ray& primaryRay = m_camera->worldRay(x, y, Rect2D::xywh(0, 0, (float)m_image->width(), (float)m_image->height()));
    return L_in(primaryRay.origin(), primaryRay.direction(), threadData, m_settings.numBackwardBounces);
}


Radiance3 RayTracer::L_in(const Point3& X, const Vector3& wi, ThreadData& threadData, int bouncesLeft) const {
    if (bouncesLeft == 0) {
        // We aren't allowed to trace farther, so estimate from the
        // environment map
        return BACKGROUND_RADIANCE;
    }

    // Surface hit by the primary ray (at X)
    float maxDistance = finf();
    shared_ptr<Surfel> surfel(castRay(X, wi, maxDistance));

    if (notNull(surfel)) {
        // Hit something
        return L_out(surfel, -wi, threadData, bouncesLeft);
    } else {
        // Hit background
        return BACKGROUND_RADIANCE;
    }
}

Radiance3 RayTracer::L_out(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData, int bouncesLeft) const {
    return surfel->emittedRadiance(wo) + L_scattered(surfel, wo, threadData, bouncesLeft - 1);
}


Radiance3 RayTracer::L_scattered(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData, int bouncesLeft) const {
    return
        L_direct(surfel, wo, threadData) + 
        L_indirectImpulses(surfel, wo, threadData, bouncesLeft) +
        L_indirectDiffuse(surfel, wo, threadData);
}


Radiance3 RayTracer::L_indirectImpulses(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData, int bouncesLeft) const {
    Surfel::ImpulseArray impulseArray;
    surfel->getImpulses(PathDirection::EYE_TO_SOURCE, wo, impulseArray);

    Radiance3 L;
    for (int i = 0; i < impulseArray.size(); ++i) {
        const Surfel::Impulse& impulse = impulseArray[i];
        L += L_in(bump(surfel, impulse.direction), impulse.direction, threadData, bouncesLeft) * impulse.magnitude;
    }

    return L;
}

static float coneFalloff(float distance, float invRadius) {
    debugAssert(distance * invRadius <= 1.0f);
    debugAssert(distance >= 0.0f);
    return 1.0f - distance * invRadius;
}

static float coneVolume(float r) {
    //  \int_0^r \int_0^2pi k(s) s dq ds
    //    = \int_0^r \int_0^2pi [1 - s/r] s dq ds
    //    = \int_0^r \int_0^2pi [s - s^2/r] dq ds
    //    = 2 pi \int_0^r [s - s^2/r] ds
    //    = 2 pi [r^2/2 - r^3/(3r)]
    //    = 2 pi [r^2/2 - r^2/3] 
    //    = pi r^2 [1 - 2/3] 
    //    = pi r^2 / 3
    return pif() * square(r) / 3.0f;
}


/** Fraction of the radius over which the cone is slanted.  1.0
    = cone,  0.0 = disk. */
static const float clampedConeHeight = 0.25f;

static float clampedConeFalloff(float distance, float invRadius) {
    debugAssert(distance * invRadius <= 1.0f);
    debugAssert(distance >= 0.0f);

    return min(clampedConeHeight, 1.0f - distance * invRadius);
}

static float clampedConeVolume(float r) {
    return
        (square(clampedConeHeight) - 3.0f * clampedConeHeight + 3.0f) *
        (clampedConeHeight * pif() * square(r)) / 3.0f;
}


bool RayTracer::visible(const Point3& Y, const Point3& X, bool shadowRay) const {
    Vector3 w(X - Y);
    float distance = w.length();
    w /= distance;
    
    distance -= BUMP_EPSILON * 2;

    shared_ptr<Surfel> surfel = castRay(Y + w * BUMP_EPSILON, w, distance, true);

    if (isNull(surfel)) {
        return true;
    } else if (shadowRay && notNull(surfel->surface) && ! surfel->surface->expressiveLightScatteringProperties.castsShadows) {
        // Re-cast the ray
        return visible(surfel->position, X, true);
    } else {
        // Hit a surface
        return false;
    }
}


/** Compute visibility from Y to X, considering only triangles in the provided array */
static bool visible(const Point3& Y, const Point3& X, const Array<Tri>& triangles, const CPUVertexArray& cpuVertexArray) {

    Vector3 delta(X - Y);
    float distance = delta.length();
    delta /= distance;
    const Ray ray(Y + delta * BUMP_EPSILON, delta);
    distance -= BUMP_EPSILON * 2;

    // Look for any intersection
    Tri::Intersector intersect;
    for (int i = 0; i < triangles.size(); ++i) {
        if (intersect(ray, cpuVertexArray, triangles[i], true, distance)) {
            return false;
        }
    }

    // There was no intersection
    return true;
}


/** Minimum thickness expected of surfaces when applying photon final
    visibility tests.  If this number is too high then there may
    be light leaks (which would happen anyway without the test).
    If it is too low then the tests will run more slowly
    but the result will be unchanged. */
static const float MIN_SURFACE_THICKNESS = 5 * units::centimeters();

Radiance3 RayTracer::L_indirectDiffuse
(const shared_ptr<Surfel>& surfel,
 const Vector3&            wo, 
 ThreadData&               threadData) const {

    if (! surfel->nonZeroFiniteScattering() || (m_photonMap.size() == 0)) {
        // Either no scattering or no photons; don't bother computing
        return Radiance3::zero();
    }

    const Point3& X = surfel->position;
        
    Radiance3 L;
    const Sphere gatherSphere(X, m_settings.photon.maxGatherRadius);

    // Extract local triangles for collision detection
    Array<Tri>& localTris = threadData.localTri;
    if (m_settings.checkFinalVisibility && (gatherSphere.radius >= MIN_SURFACE_THICKNESS)) {
        getNearbyTris(surfel->position, surfel->geometricNormal, gatherSphere, localTris);
    }

    // Use the tri tree if there are too many elements in the array
    const bool useTriTreeVisibility = (localTris.size() > 10);

    for (PhotonMap::SphereIterator photon = 
             m_photonMap.begin(gatherSphere);
         photon.isValid(); 
         ++photon) {
        debugAssert(gatherSphere.contains(photon->position));

        const Point3&  Y = photon->position;

        // Distance to the photon; affects falloff
        const float    s = (Y - X).length();

        // Radius of the photon's effect
        const float    r = photon->effectRadius;
        
        if (s < r) {
            // The falloff (smoothing) kernel will be non-zero.

            const Vector3&    wi = photon->wi;
            const Color3&     f  = surfel->finiteScatteringDensity(wi, wo);

            // Maybe check visibility; don't bother if there is no illumination
            if (m_settings.checkFinalVisibility && f.nonZero() && (s > MIN_SURFACE_THICKNESS)) {
                // The photon is an appreciable distance from the shading point.
                // Cast a visibility ray to ensure that it is actually a good estimator
                // of flux at this location and not, for example, on the other side
                // of a partition.  We should ideally be casting the ray back to the source
                // of the photon, but that information is no longer available.
                // Note that this code is similar to a VPL shadow ray or final gather ray.  
                // Unlike VPLs, we already have a measure of visibility from the light source (and have
                // enough photons to make shading robust to moving objects)...and the ray cast
                // is always over a fairly short distance and thus likely to be fast.
                // Offset far enough that curved surfaces don't occlude themselves.
                const Vector3& offset = surfel->geometricNormal * (s * 0.5f + BUMP_EPSILON);

                const bool isVisible =
                    useTriTreeVisibility ? 
                        visible(Y, X + offset) :
                        ::visible(Y, X + offset, localTris, m_triTree.cpuVertexArray());

                if (! isVisible) {
                    // Stop processing this photon's contribution to the surfel
                    continue;
                }
            }
            
            const Biradiance3& B = photon->power * clampedConeFalloff((X - Y).length(), 1.0f / r) / clampedConeVolume(r);

            L += B * f;
        }
    }
    
    return L;
}


void RayTracer::getNearbyTris(const Point3& cullPosition, const Vector3& cullNormal, const Sphere& gatherSphere, Array<Tri>& localTri) const {
    localTri.fastClear();
    m_triTree.intersectSphere(gatherSphere, localTri);

    // Remove any triangle whose plane contains the surfel, since it can't possibly
    // affect visibility.
    for (int i = 0; i < localTri.size(); ++i) {
        const Tri& tri = localTri[i];
        const Vector3& n = tri.normal(m_triTree.cpuVertexArray());
        const Point3&  V = tri.vertex(m_triTree.cpuVertexArray(), 0).position;
        const bool inPlane = ! tri.twoSided() && (n.dot(cullNormal) > 0.99f) && (abs(n.dot(V - cullPosition)) <= 0.001f);
        if (inPlane) {
            // Remove this triangle
            localTri.fastRemove(i);
            --i;
        }
    }
}


Radiance3 RayTracer::L_direct(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData) const {
    Radiance3 L;
    const Point3& X(surfel->position);
    const Vector3& n(surfel->shadingNormal);

    for (int i = 0; i < m_lighting.lightArray.size(); ++i) {
        const shared_ptr<Light> light(m_lighting.lightArray[i]);

        if (light->producesDirectIllumination()) {
            
            const Point3&  Y(light->frame().translation);            
            const Vector3& wi((Y - X).direction());
            
            debugAssertM(X.isFinite(), "The surface is not at a finite location");
            debugAssertM(Y.isFinite(), "The light is not at a finite location");
            
            if ((! light->castsShadows()) || visible(Y, X, true)) {
                const Color3&      f(surfel->finiteScatteringDensity(wi, wo));
                const Biradiance3& B(light->biradiance(X));

                L += f * B * abs(wi.dot(n));
                debugAssertM(L.isFinite(), "Non-finite radiance in L_direct");
            }
        }
    }

    return L;
}


void RayTracer::debugDrawPhotons(RenderDevice* rd) {
	SlowMesh mesh(PrimitiveType::POINTS);
    for (PhotonMap::Iterator photon = m_photonMap.begin(); photon.isValid(); ++photon) {
        debugAssert(photon->power.min() >= 0);
        mesh.setColor(photon->power / photon->power.max());
        mesh.makeVertex(photon->position);
    }
	mesh.render(rd);
}


void RayTracer::debugDrawPhotonMap(RenderDevice* rd) {
    rd->setObjectToWorldMatrix(CFrame());
    for (PhotonMap::CellIterator cell = m_photonMap.beginCell(); cell.isValid(); ++cell) {
        Draw::box(cell.bounds(), rd);
    }    
}
