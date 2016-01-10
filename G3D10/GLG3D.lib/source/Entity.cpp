/*
  \file GLG3D.lib/source/Entity.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2012-07-27
  \edited  2014-07-13

  Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "GLG3D/Entity.h"
#include "G3D/Box.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "G3D/Ray.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Draw.h"
#include "GLG3D/SceneVisualizationSettings.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GFont.h"

namespace G3D {

Entity::Entity() : m_scene(NULL), m_movedSinceLoad(false), m_lastBoundsTime(0), m_lastChangeTime(0), m_canChange(true), m_shouldBeSaved(true) {}


void Entity::init
   (const String& name,
    Scene*             scene,
    AnyTableReader&    propertyTable) {

    // Track may need this to have a name
    m_name = name;

    m_sourceAny = propertyTable.any();

    shared_ptr<Track> track;
    Any c;
    if (propertyTable.getIfPresent("track", c)) {
        track = Track::create(this, scene, c);
    }

    bool canChange = true, shouldBeSaved = true;
    propertyTable.getIfPresent("canChange", canChange);
    propertyTable.getIfPresent("shouldBeSaved", shouldBeSaved);

    CFrame frame;
    propertyTable.getIfPresent("frame", frame);

    init(name, scene, frame, track, canChange, shouldBeSaved);

    CFrame previousFrame;
    propertyTable.getIfPresent("previousFrame", previousFrame);
    m_previousFrame = previousFrame;
}


void Entity::init
   (const String&          name,
    Scene*                      scene,
    const CFrame&               frame,
    const shared_ptr<Track>&    track,
    bool                        canChange,
    bool                        shouldBeSaved) {

    m_name          = name;
    m_canChange     = canChange;
    m_shouldBeSaved = shouldBeSaved;
    m_track         = track;
    m_scene         = scene;

    if (notNull(track)) {
        m_previousFrame = m_frame = m_track->computeFrame(0);
    } else {
        m_frame = m_previousFrame = frame;
    }
    
    m_lastChangeTime = System::time();
    if (! m_canChange && notNull(m_track)) {
        debugAssertM(false, "Track specified for an Entity that cannot change.");
    }
}


void Entity::setFrame(const CFrame& f) {
    if (m_frame != f) {
        m_lastChangeTime = System::time();
        debugAssert(m_lastChangeTime > 0.0);
        m_frame = f;
        m_movedSinceLoad = true;
    }
}


void Entity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    // Update the previous frame if time is elapsing (if time is paused then
    // we do not update the previous frame, since that indicates that the camera
    // is inspecting objects in motion).  Note that deltaTime may be NaN, indicating that
    // a discontinuous time change occured.
    if (isNaN(deltaTime) || (deltaTime != 0)) {
        // Even if this object has m_canChange == false, we have to update
        // the position because the Scene might be in editing mode.
        m_previousFrame = m_frame;
    }

    if (notNull(m_track)) {
        m_frame = m_track->computeFrame(absoluteTime);
        if (m_frame != m_previousFrame) {
            m_lastChangeTime = System::time();
        }
    }
}


void Entity::setFrameSpline(const PhysicsFrameSpline& spline) {
    shared_ptr<SplineTrack> c = dynamic_pointer_cast<SplineTrack>(m_track);
    if (isNull(c)) {
        c = shared_ptr<SplineTrack>(new SplineTrack());
        setTrack(shared_ptr<Track>(c));
    }
    c->setSpline(spline);
}


void Entity::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
}


void Entity::getLastBounds(AABox& box) const {
    box = m_lastAABoxBounds;
}


void Entity::getLastBounds(Box& box) const {
    box = m_lastBoxBounds;
}


void Entity::getLastBounds(Sphere& sphere) const {
    sphere = m_lastSphereBounds;
}


bool Entity::intersectBounds(const Ray& R, float& maxDistance, Model::HitInfo& info) const {
    if (m_lastAABoxBounds.isEmpty()) {
        return false;
    }
    
    float t = R.intersectionTime(m_lastBoxBounds);
    if (t < maxDistance) {
        maxDistance = t;
        return true;
    } else {
        return false;
    }
}


bool Entity::intersect(const Ray& R, float& maxDistance, Model::HitInfo& info) const {
    return intersectBounds(R, maxDistance, info);
}



Any Entity::toAny(const bool forceAll) const {
    Any a = m_sourceAny;
    AnyTableReader oldValues(m_sourceAny);

    debugAssert(! a.isNil());
    if (a.isNil()) {
        // Fallback for release mode failure
        return a;
    }

    if (m_movedSinceLoad) {
        a["frame"] = m_frame;

        CoordinateFrame oldPreviousFrame;
        if (forceAll || (oldValues.getIfPresent("previousFrame", oldPreviousFrame) && oldPreviousFrame != m_previousFrame)) {
            a["previousFrame"] = m_previousFrame;
        }
    }

    const shared_ptr<SplineTrack>& splineTrack = dynamic_pointer_cast<SplineTrack>(m_track);
    if (notNull(splineTrack) && splineTrack->changed()) {
        // Update the spline
        const PhysicsFrameSpline& spline = splineTrack->spline();
        if (spline.control.size() == 1) {
            // Write out in short form for the single control point
            const PhysicsFrame& p = spline.control[0];
            if (p.rotation == Quat()) {
                // No rotation
                a["track"] = p.translation;
            } else {
                // Full coordinate frame
                a["track"] = CFrame(p);
            }
        } else {
            // Write the full spline
            a["track"] = spline;
        }
    }

    a.setName("Entity");
    return a;
}


void Entity::visualize(RenderDevice* rd, bool isSelected, const class SceneVisualizationSettings& s, const shared_ptr<GFont>& font, const shared_ptr<Camera>& camera) {
    if (s.showEntitySphereBounds) {
        Draw::sphere(m_lastSphereBounds, rd, Color4(0.1f, 0.5f, 0.8f, 0.1f), Color3::white());
    }

    if (s.showEntityBoxBounds) {
        Draw::box(m_lastBoxBounds, rd, Color4(0.1f, 0.5f, 0.8f, 0.1f), Color3::white());
    }

    if (s.showEntityBoxBoundArray) {
        for (int b = 0; b < m_lastBoxBoundArray.size(); ++b) {
            Draw::box(m_lastBoxBoundArray[b], rd, Color4(0.1f, 0.8f, 0.5f, 0.1f), Color3::green());
        }
    }
   
    if (s.showEntityNames) {
        font->draw3DBillboard(rd, m_name, m_lastAABoxBounds.center() - camera->frame().lookVector() * m_lastSphereBounds.radius, m_lastAABoxBounds.extent().length() * 0.1f, Color3::black());
    }
}


void Entity::makeGUI(GuiPane* pane, GApp* app) {
}

} // namespace G3D
