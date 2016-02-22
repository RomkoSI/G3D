/*
  \file GLG3D.lib/source/MarkerEntity.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2013-01-01
  \edited  2013-04-13
 */
#include "GLG3D/MarkerEntity.h"
#include "G3D/Any.h"
#include "G3D/Ray.h"
#include "GLG3D/Draw.h"
#include "GLG3D/Scene.h"
#include "GLG3D/GFont.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SceneVisualizationSettings.h"

namespace G3D {

static shared_ptr<GFont> s_font;

MarkerEntity::MarkerEntity() : m_color(Color3::white()) {}

shared_ptr<MarkerEntity> MarkerEntity::create(const String& name) {
    
    Any a(Any::TABLE);
    AnyTableReader reader(a);
    return dynamic_pointer_cast<MarkerEntity>(create(name, NULL, reader, ModelTable()));
}


void MarkerEntity::init(AnyTableReader& propertyTable) {
    Array<Box> boxArray;
    propertyTable.getIfPresent("osBoxArray", boxArray);

    Color3 color = Color3::white();
    propertyTable.getIfPresent("color", color);

    init(boxArray, color);
}


void MarkerEntity::init(const Array<Box>& boxArray, const Color3& color) {
    m_osBoxArray = boxArray;
    m_color = color;
}


shared_ptr<Entity> MarkerEntity::create 
   (const String&                      name,
    Scene*                             scene,
    AnyTableReader&                    propertyTable,
    const ModelTable&                  modelTable,
    const Scene::LoadOptions&          options) {

    shared_ptr<MarkerEntity> m(new MarkerEntity());
    m->Entity::init(name, scene, propertyTable);
    m->MarkerEntity::init(propertyTable);
    propertyTable.verifyDone();
    return m;
}


shared_ptr<MarkerEntity> MarkerEntity::create
   (const String& name,
    Scene*                                  scene,
    const Array<Box>&                       osBoxArray,
    const Color3&                           color,
    const CFrame&                           frame,
    const shared_ptr<Track>&                track,
    bool                                    canChange,
    bool                                    shouldBeSaved) {

    shared_ptr<MarkerEntity> m(new MarkerEntity());

    m->Entity::init(name, scene, frame, track, canChange, shouldBeSaved);
    m->MarkerEntity::init(osBoxArray, color);
     
    return m;
}


Any MarkerEntity::toAny(const bool forceAll) const {
    Any a = Entity::toAny(forceAll);
    a.setName("MarkerEntity");
    a["color"] = m_color;
    a["osBoxArray"] = m_osBoxArray;

    return a;
}


void MarkerEntity::visualize(RenderDevice* rd, bool isSelected, const SceneVisualizationSettings& s, const shared_ptr<GFont>& font, const shared_ptr<Camera>& camera) {
    Entity::visualize(rd, isSelected, s, font, camera);

    if (s.showMarkers || isSelected) {

        // Copy before it is mutated
        const CFrame old = rd->objectToWorldMatrix();
        rd->setObjectToWorldMatrix(m_frame);
        for (int b = 0; b < m_osBoxArray.size(); ++b) {
            const Box& osBox = m_osBoxArray[b];
            Draw::box(osBox, rd, Color4(m_color, 0.25f), m_color);
        }
        rd->setObjectToWorldMatrix(old);

        font->draw3DBillboard(rd, m_name, m_lastAABoxBounds.center(), m_lastAABoxBounds.extent().length() * 0.1f, m_color);
    } else {
    }
}


void MarkerEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    Entity::onSimulation(absoluteTime, deltaTime);

    if (m_lastAABoxBounds.isEmpty() || (m_lastChangeTime > m_lastBoundsTime)) {

        if (m_osBoxArray.size() == 0) {
            // No bounds
            m_lastAABoxBounds  = AABox(m_frame.translation);
            m_lastBoxBounds    = m_lastAABoxBounds;
            m_lastSphereBounds = Sphere(m_frame.translation, 0);
            m_lastObjectSpaceAABoxBounds = AABox(Point3::zero());
        } else {
            if (m_osBoxArray.size() == 1) {
                // Tightly fit the one box
                m_osBoxArray[0].getBounds(m_lastObjectSpaceAABoxBounds);
                m_lastBoxBounds = m_frame.toWorldSpace(m_osBoxArray[0]);
                m_lastBoxBoundArray.resize(1);
                m_lastBoxBoundArray.last() = m_lastBoxBounds;
                m_lastBoxBounds.getBounds(m_lastAABoxBounds);
            } else {
                // General case.  Compute world space AA bounds
                m_lastObjectSpaceAABoxBounds = AABox();
                m_lastAABoxBounds = AABox();
                m_lastBoxBoundArray.fastClear();
                for (int b = 0; b < m_osBoxArray.size(); ++b) {
                    const Box& osBox = m_osBoxArray[b];

                    m_lastObjectSpaceAABoxBounds.merge(osBox);

                    const Box& wsBox = m_frame.toWorldSpace(osBox);
                    m_lastBoxBoundArray.next() = wsBox;
                    m_lastAABoxBounds.merge(wsBox);
                }
                m_lastBoxBounds = m_lastAABoxBounds;
            }

            m_lastSphereBounds = Sphere(m_lastBoxBounds.center(), m_lastBoxBounds.extent().length());
        }

        m_lastBoundsTime = System::time();
    }
}


bool MarkerEntity::intersectBounds(const Ray& R, float& maxDistance, Model::HitInfo& info) const {
    if (m_lastAABoxBounds.isEmpty()) {
        return false;
    }
    
    float t = R.intersectionTime(m_lastBoxBounds);
    if (t < maxDistance) {
        info.set(shared_ptr<Model>(), dynamic_pointer_cast<MarkerEntity>(const_cast<MarkerEntity*>(this)->shared_from_this()));
        maxDistance = t;
        return true;
    } else {
        return false;
    }
}


bool MarkerEntity::intersect(const Ray& R, float& maxDistance, Model::HitInfo& info) const {
    // Find the first intersection
    bool hitAnything = false;
    const Ray& osRay = m_frame.toObjectSpace(R);

    for (int b = 0; b < m_osBoxArray.size(); ++b) {
        const Box& osBox = m_osBoxArray[b];

        const float d = osRay.intersectionTime(osBox);

        if (d < maxDistance) {
            maxDistance = d;
            hitAnything = true;
            info.set(shared_ptr<Model>(), dynamic_pointer_cast<MarkerEntity>(const_cast<MarkerEntity*>(this)->shared_from_this()));
        }
    }

    return hitAnything;
}

} // G3D
