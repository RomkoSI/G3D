/*
  \file GLG3D.lib/source/Entity_Track.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2013-02-20
  \edited  2013-02-23
 */
#include "G3D/Any.h"
#include "GLG3D/Entity.h"
#include "GLG3D/Scene.h"

namespace G3D {
using namespace _internal;

namespace _internal {

class CombineTrack : public Entity::Track {
protected:
    friend class Entity::Track;

    shared_ptr<Entity::Track>      m_rotation;
    shared_ptr<Entity::Track>      m_translation;

    CombineTrack(const shared_ptr<Entity::Track>& r, const shared_ptr<Entity::Track>& t) : 
      m_rotation(r), m_translation(t) {}

public:
    
    virtual CFrame computeFrame(SimTime time) const override {
        return CFrame
            (m_rotation->computeFrame(time).rotation, 
             m_translation->computeFrame(time).translation);
    }

};


class TransformTrack : public Entity::Track {
protected:
    friend class Entity::Track;

    shared_ptr<Entity::Track>      m_a;
    shared_ptr<Entity::Track>      m_b;

    TransformTrack(const shared_ptr<Entity::Track>& a, const shared_ptr<Entity::Track>& b) : 
      m_a(a), m_b(b) {}

public:

    virtual CFrame computeFrame(SimTime time) const override {
        return m_a->computeFrame(time) * m_b->computeFrame(time); 
    }
};


class OrbitTrack : public Entity::Track {
protected:
    friend class Entity::Track;

    float							m_radius;
    float							m_period;

	OrbitTrack(float r, float p) : m_radius(r), m_period(p) {}

public:

    virtual CFrame computeFrame(SimTime time) const override {
		const float angle = float(twoPi() * time) / m_period;
		return CFrame::fromXYZYPRRadians(sin(angle) * m_radius, 0, cos(angle) * m_radius, angle);
    }
};


class TrackEntityTrack : public Entity::Track {
protected:
    friend class Entity::Track;

    const String&      m_entityName;
    Scene*                  m_scene;

    TrackEntityTrack(const String& n, Scene* scene) : m_entityName(n), m_scene(scene) {}

public:

    virtual CFrame computeFrame(SimTime time) const override {
        shared_ptr<Entity> e = m_scene->entity(m_entityName);
        if (notNull(e)) {
            return e->frame();
        } else {
            // Maybe during initialization and the other entity does not yet exist
            return CFrame();
        }
    }
};


class LookAtTrack : public Entity::Track {
protected:
    friend class Entity::Track;

    shared_ptr<Entity::Track>      m_base;
    shared_ptr<Entity::Track>      m_target;
    const Vector3               m_up;

    LookAtTrack(const shared_ptr<Entity::Track>& base, const shared_ptr<Entity::Track>& target, const Vector3& up) : 
        m_base(base), m_target(target), m_up(up) {}

public:

    virtual CFrame computeFrame(SimTime time) const override {
        CFrame f = m_base->computeFrame(time);
        f.lookAt(m_target->computeFrame(time).translation, m_up);
        return f;
    }
};


class TimeShiftTrack : public Entity::Track {
protected:
    friend class Entity::Track;

    shared_ptr<Entity::Track>           m_track;
    SimTime                             m_dt;

    TimeShiftTrack(const shared_ptr<Entity::Track>& c, const SimTime dt) : 
      m_track(c), m_dt(dt) {}

public:
    
    virtual CFrame computeFrame(SimTime time) const override {
        return m_track->computeFrame(time + m_dt);
    }

};

} // namespace _internal


void Entity::Track::VariableTable::set(const String& id, const shared_ptr<Entity::Track>& val) {
    debugAssert(notNull(val));
    variable.set(id, val);
}


shared_ptr<Entity::Track> Entity::Track::VariableTable::operator[](const String& id) const {
    const shared_ptr<Entity::Track>* valPtr = variable.getPointer(id);

    if (isNull(valPtr)) {
        if (isNull(parent)) {
            // Variable not found.  We do not allow Entity names here because it would prevent static 
            // checking of ID's...the full list of Entitys is unknown when parsing their Tracks.
            return shared_ptr<Entity::Track>();
        } else {
            // Defer to parent
            return (*parent)[id];
        }
    } else {
        // Return the value found
        return *valPtr;
    }
}


shared_ptr<Entity::Track> Entity::Track::create(Entity* entity, Scene* scene, const Any& a) {
    VariableTable table;
    return create(entity, scene, a, table);
}


shared_ptr<Entity::Track> Entity::Track::create(Entity* entity, Scene* scene, const Any& a, const VariableTable& variableTable) {
    if (a.type() == Any::STRING) {
        // This must be an id
        const shared_ptr<Entity::Track>& c = variableTable[a.string()];
        if (isNull(c)) {
            a.verify(false, "");
        }
        return c;
    }

    if ((beginsWith(a.name(), "PhysicsFrameSpline")) ||
        (beginsWith(a.name(), "PFrameSpline")) ||
        (beginsWith(a.name(), "Point3")) || 
        (beginsWith(a.name(), "Vector3")) || 
        (beginsWith(a.name(), "Matrix3")) || 
        (beginsWith(a.name(), "Matrix4")) || 
        (beginsWith(a.name(), "CFrame")) || 
        (beginsWith(a.name(), "PFrame")) || 
        (beginsWith(a.name(), "UprightSpline")) || 
        (beginsWith(a.name(), "CoordinateFrame")) || 
        (beginsWith(a.name(), "PhysicsFrame"))) {

        return Entity::SplineTrack::create(a);

    } else if (a.name() == "entity") {

        // Name of an Entity
        const String& targetName = a[0].string();
        alwaysAssertM(notNull(scene) && notNull(entity), "entity() Track requires non-NULL Scene and Entity");
        debugAssert(targetName != "");
        scene->setOrder(targetName, entity->name());
        return shared_ptr<Entity::Track>(new TrackEntityTrack(targetName, scene));

    } else if (a.name() == "transform") {

        return shared_ptr<Entity::Track>
            (new TransformTrack(create(entity, scene, a[0], variableTable), 
                                create(entity, scene, a[1], variableTable)));

    } else if (a.name() == "follow") {

        a.verify(false, "follow Tracks are unimplemented");
        return shared_ptr<Entity::Track>();
        // return shared_ptr<Entity::Track>(new TransformTrack(create(a[0]), create(a[1])));

	} else if (a.name() == "orbit") {

        return shared_ptr<Entity::Track>(new OrbitTrack(a[0], a[1]));

    } else if (a.name() == "combine") {

        return shared_ptr<Entity::Track>
            (new CombineTrack(create(entity, scene, a[0], variableTable), 
                                   create(entity, scene, a[1], variableTable)));

    } else if (a.name() == "lookAt") {

        return shared_ptr<Entity::Track>
            (new LookAtTrack(create(entity, scene, a[0], variableTable), 
                                  create(entity, scene, a[1], variableTable), (a.size() > 2) ? Vector3(a[2]) : Vector3::unitY()));

    } else if (a.name() == "timeShift") {

        const shared_ptr<Track>& p = create(entity, scene, a[0], variableTable);
        a.verify(notNull(dynamic_pointer_cast<SplineTrack>(p)) || notNull(dynamic_pointer_cast<OrbitTrack>(p)), "timeShift() requires a PhysicsFrameSpline or orbit");
        return shared_ptr<Entity::Track>(new TimeShiftTrack(p, float(a[1].number())));

    } else if (a.name() == "with") {

        // Create a new variable table and recurse
        VariableTable extendedTable(&variableTable);

        const Any& vars = a[0];
        for (Table<String, Any>::Iterator it = vars.table().begin(); it.isValid(); ++it) {
            // Note that if Any allowed iteration through its table in definition order, then
            // we could implement Scheme LET* instead of LET here.
            extendedTable.set(it->key, create(entity, scene, it->value, variableTable));
        }

        return create(entity, scene, a[1], extendedTable);

    } else {

        // Some failure
        a.verify(false, "Unrecognized Entity::Track type");
        return shared_ptr<Entity::Track>();

    }
}

}
