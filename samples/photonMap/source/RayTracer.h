#ifndef RayTracer_h
#define RayTracer_h

#include <G3D/G3DAll.h>
#include "Photon.h"
#include "PhotonMap.h"

#define USE_FAST_POINT_HASH_GRID 1

class RayTracer : public ReferenceCountedObject {
public:

#if USE_FAST_POINT_HASH_GRID
    typedef FastPointHashGrid<Photon, Photon> PhotonMap;
#else
    typedef ::PhotonMap PhotonMap;
#endif
    
    class Settings {
    public:
        class PhotonMapSettings {
        public:
            int                 numEmitted;
            int                 numBounces;

            /** Radius of effect of a single, perfectly-specularly reflected photon */
            float               minGatherRadius;

            /** Radius of effect of a photon that has undergone many dim, diffuse bounces */
            float               maxGatherRadius;

            /** Typically on the range [0, 1].  Larger numbers mean that photons
                get big very quickly as they go through diffuse bounces.
               Small numbers keep photons tight through multiple diffuse bounces. */
            float               radiusBroadeningRate;

            bool operator==(const PhotonMapSettings& other) const {
                return
                    (numEmitted == other.numEmitted) &&
                    (numBounces == other.numBounces) &&
                    (maxGatherRadius == other.maxGatherRadius) &&
                    (minGatherRadius == other.minGatherRadius) &&
                    (radiusBroadeningRate == other.radiusBroadeningRate);
            }

            bool operator!=(const PhotonMapSettings& other) const {
                return ! (*this == other);
            }

            PhotonMapSettings();
        };

        int                 width;
        int                 height;
        bool                multithreaded;
        bool                useTree;

        int                 sqrtNumPrimaryRays;
        int                 numBackwardBounces;

        /** If true, cast a ray from each photon to the points that it shades to
            ensure that it is still a good estimator at those locations. This prevents
            most light leaks but will substantially slow down shading. */
        bool                checkFinalVisibility;

        PhotonMapSettings   photon;
            
        Settings();
    };

    class Stats {
    public:
        int                 lights;

        int                 triangles;

        /** width x height */
        int                 pixels;

        float               buildTriTreeTimeMilliseconds;
        float               photonTraceTimeMilliseconds;
        float               buildPhotonMapTimeMilliseconds;
        float               rayTraceTimeMilliseconds;

        int                 storedPhotons;

        Stats();
    };

protected:

    class ThreadData {
    public:
        shared_ptr<Random>  rnd;
        Array<Tri>          localTri;
    };

    /** Array of data so that each threadID may
        have its own and avoid needed locks. */
    Array< ThreadData >     m_threadData;

    // The following are only valid during a call to render()
    shared_ptr<Image>        m_image;
    LightingEnvironment m_lighting;
    shared_ptr<Camera>       m_camera;
    Settings                 m_settings;

    RealTime                 m_triTreeUpdateTime;
    TriTree                  m_triTree;
    /** Time for the last tree build */
    RealTime                 m_buildTriTreeTimeMilliseconds;

    typedef Array<Photon> PhotonList;
    /** Per-thread list that is then moved into m_photonMap when tracing is done */
    Array<PhotonList>        m_photonList;
    /** Time to copy the m_photonList into the m_photonMap */
    RealTime                 m_buildPhotonMapTimeMilliseconds;
    RealTime                 m_photonMapUpdateTime;
    PhotonMap                m_photonMap;
    RealTime                 m_photonTraceTimeMilliseconds;

    /** Total emissive power of all global illumination-producing lights. */
    Power3                   m_totalIndirectProducingLightPower;

    /** The scene */
    shared_ptr<Scene>        m_scene;

    RayTracer();

    void maybeUpdateTree();

    /** Note that this does not update if the photon map gather radius
        has changed, so the results will be right but may be
        inefficient. */
    void maybeUpdatePhotonMap();

    /** Called from GThread::runConcurrently2D(), which is invoked in traceAllPixels() */
    void traceOnePixel(int x, int y, int threadID);

    /** Called from traceOnePixel */
    Radiance3 traceOnePrimaryRay(float x, float y, ThreadData& threadData);

    /** Called from render().  Writes to m_image. */
    void traceAllPixels(int numThreads);

    void computeTotalIndirectProducingLightPower();

    void emitPhoton(Random& rnd, Photon& photon);
    
    void traceOnePhoton(int ignorex, int ignorey, int threadID);

    /** Computes the radius of the effect of a single photon based on the a priori path probability. */
    float photonEffectRadius(float probabiltyHint) const;

    /**
       \param X The ray origin in world space
       \param w The ray direction in world space
       \param distance Don't trace farther than this, and return the actual distance traced in this variable
       \param anyHit If true, return any surface hit, even if it is not the first
       \return The surfel hit, or NULL if none was hit
     */
    shared_ptr<Surfel> castRay(const Point3& X, const Vector3& w, float& distance, bool anyHit = false) const;
    
    ///////////////////////////////////////////////////

    /** Incident light at X propagating in direction -wi     
       \f[ \Li(X, \wi) = L_o(Y, -\wi) \f] 
       where \f$Y = X + t\wi\f$ for distance \f$t\f$ to the next surface along the ray. 
    */
    Radiance3 L_in(const Point3& X, const Vector3& wi, ThreadData& threadData, int bouncesLeft) const;

    /** Outgoing light at X propagating in direction wo 
        \f[ L_{\mathrm{o},\mathrm{scattered}}(X,\wo) = \int_{\sphere} \Li(X, \wi) \cdot f_X(\wi, \wo)\cdot |\wi \cdot \n|~ d\wi \f]

        \f[ L_{\mathrm{o},\mathrm{scattered}}(X,\wo) = L_{\mathrm{o},\mathrm{direct}}(X,\wo) + L_{\mathrm{o},\mathrm{indirectImpulses}}(X,\wo) + L_{\mathrm{o},\mathrm{indirectDiffuse}}(X,\wo) \f]
    */
    Radiance3 L_out(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData, int bouncesLeft) const;

    /** Component of L_out due to scattered light (vs. emitted)     
      \f[  L_{\mathrm{o}, \mathrm{direct}}(X, \wo) = \sum_{Y \in \mathrm{lights}} \beta(X, Y) \cdot f_X(\wi, \wo) \cdot |\wi \cdot \n| \f]
      for \f$\wi = \sphere(Y - X)\f$, where \f$\beta\f$ is biradiance.
    */
    Radiance3 L_scattered(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData, int bouncesLeft) const;

    /** Component of L_scattered due to direct illumination */
    Radiance3 L_direct(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData) const;

    /** Component of L_scattered due to indirect illumination that scattered specularly */
    Radiance3 L_indirectImpulses(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData, int bouncesLeft) const;

    /** Component of L_scattered due to indirect illumination that scattered diffusely (i.e., non-specularly) */
    Radiance3 L_indirectDiffuse(const shared_ptr<Surfel>& surfel, const Vector3& wo, ThreadData& threadData) const;
    
    /** Returns true if there is unobstructed line of sight from Y to X.*/
    bool visible(const Point3& Y, const Point3& X, bool shadowRay = false) const;

    /** Gather triangles within @a gatherSphere that are not in the plane cullPosition, cullNormal 
    Clears the @a localTri array at start*/
    void getNearbyTris(const Point3& cullPosition, const Vector3& cullNormal, const Sphere& gatherSphere, Array<Tri>& localTri) const;

public:

    ~RayTracer();

    static shared_ptr<RayTracer> create(const shared_ptr<Scene>& scene);

    /** Render the specified image */
    shared_ptr<Image> render
    (const Settings&                     settings, 
     const LightingEnvironment&     lighting, 
     const shared_ptr<Camera>&           camera,
     Stats&                              stats);

    void debugDrawPhotons(RenderDevice* rd);
    void debugDrawPhotonMap(RenderDevice* rd);
    
};

#endif


