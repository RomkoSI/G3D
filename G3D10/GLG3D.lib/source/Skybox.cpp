#include "G3D/Any.h"
#include "GLG3D/Skybox.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Surface.h"
#include "GLG3D/SkyboxSurface.h"
#include "GLG3D/Scene.h"

namespace G3D {

void Skybox::init(AnyTableReader& propertyTable) {

    Array<shared_ptr<Texture> > k;
    Array<SimTime>  t;
    SimTime         f = 1.0f;
    SplineExtrapolationMode e = SplineExtrapolationMode::CLAMP;

    if (propertyTable.containsUnread("texture")) {

        k.append(Texture::create(Texture::Specification(propertyTable["texture"], true, Texture::DIM_CUBE_MAP)));
        t.append(0);

    } else {

        Array<Any> ar;
        propertyTable.getIfPresent("keyframeArray",     ar);
        k.resize(ar.size());
        for (int i = 0; i < ar.size(); ++i) {
            k[i] = Texture::create(Texture::Specification(ar[i], true, Texture::DIM_CUBE_MAP));
        }
        propertyTable.getIfPresent("timeArray",         t);
        propertyTable.getIfPresent("finalInterval",     f);
        propertyTable.getIfPresent("extrapolationMode", e);

        propertyTable.any().verify(t.size() == k.size(), "Must have same number of time and keyframe elements");
        propertyTable.any().verify
            ((m_extrapolationMode == SplineExtrapolationMode::CLAMP) || 
             (m_extrapolationMode == SplineExtrapolationMode::CYCLIC), 
             "Only CYCLIC and CLAMP extrapolation modes supported");
    }

    init(k, t, f, e);
}


void Skybox::init(const Array<shared_ptr<Texture> >& k, const Array<SimTime>& t, SimTime f, SplineExtrapolationMode e) {

    alwaysAssertM
        ((e == SplineExtrapolationMode::CLAMP) || 
            (e == SplineExtrapolationMode::CYCLIC), 
            "Only CYCLIC and CLAMP extrapolation modes supported");

    m_keyframeArray = k;
    m_timeArray = t;
    m_finalInterval = f;
    m_extrapolationMode = e;
}


shared_ptr<Entity> Skybox::create 
   (const String&                           name,
    Scene*                                  scene,
    AnyTableReader&                         propertyTable,
    const ModelTable&                       modelTable) {

    (void)modelTable;

    const shared_ptr<Skybox> e(new Skybox());
    e->Entity::init(name, scene, propertyTable);
    e->Skybox::init(propertyTable);
    propertyTable.verifyDone();

    return e;
}


shared_ptr<Skybox> Skybox::create
   (const String& name,
    Scene*                                  scene,
    const Array<shared_ptr<Texture> >&      k, 
    const Array<SimTime>&                   t, 
    SimTime                                 f, 
    SplineExtrapolationMode                 e,
    bool                                    canChange,
    bool                                    shouldBeSaved) {

    shared_ptr<Skybox> m(new Skybox());

    m->Entity::init(name, scene, CFrame(), shared_ptr<Entity::Track>(), canChange, shouldBeSaved);
    m->Skybox::init(k, t, f, e);
     
    return m;
}


Any Skybox::toAny(const bool forceAll) const {
    Any a = Entity::toAny(forceAll);
    a.setName("Skybox");
    return a;
}


void Skybox::onPose(Array< shared_ptr<Surface> >& surfaceArray) {
    SimTime now = m_scene->time();

    int i = 0, j = 0;
    float alpha = 0;

    if (((m_extrapolationMode == SplineExtrapolationMode::CLAMP) && (now < m_timeArray[0])) || (m_timeArray.size() == 1)) {
        alpha = 0;
        i = 0;
        j = i;
    } else if ((m_extrapolationMode == SplineExtrapolationMode::CLAMP) && (now >= m_timeArray.last())) {
        alpha = 0;
        i = m_timeArray.size() - 1;
        j = i;
    } else {
        // General case lerp

        const SimTime totalCycleTime = m_timeArray.last() + m_finalInterval;
        if ((now >= totalCycleTime) && (m_extrapolationMode == SplineExtrapolationMode::CYCLIC)) {
            // Bring within total time
            now -= floor(now / totalCycleTime) * totalCycleTime;
        }

        // Find the first keyframe before  the current time
        for (i = 1; (i < m_timeArray.size()) && (m_timeArray[i] <= now); ++i) { }
        --i;

        // Now find the keyframe after it
        j = (i + 1) % m_timeArray.size();

        const SimTime intervalStart = m_timeArray[i];
        // If we wrapped around, use the final interval
        const SimTime intervalEnd   = (j < i) ? (m_finalInterval + intervalStart) : m_timeArray[j];
        alpha = float((now - intervalStart) / (intervalEnd - intervalStart));
    }

    surfaceArray.append(SkyboxSurface::create(m_keyframeArray[i], m_keyframeArray[j], alpha));
}


} // namespace G3D
