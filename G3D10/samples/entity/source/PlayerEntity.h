#ifndef PlayerEntity_h
#define PlayerEntity_h

#include <G3D/G3DAll.h>

class PlayerEntity : public VisibleEntity {
protected:

    // OS = object space

    Vector3           m_maxOSAcceleration;
    Vector3           m_maxOSVelocity;
    Vector3           m_desiredOSVelocity;

    /** In world space */
    Vector3           m_velocity;

    PlayerEntity() {}

    void init(AnyTableReader& propertyTable);

    void init(const Vector3& velocity);

public:

    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      loadOptions);

    /** For programmatic construction at runtime */
    static shared_ptr<PlayerEntity> create 
    (const String&                  name,
     Scene*                         scene,
     const CFrame&                  position,
     const shared_ptr<Model>&       model);

    void setDesiredOSVelocity(const Vector3& objectSpaceVelocity) {
        m_desiredOSVelocity = objectSpaceVelocity;
    }

    /** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "Entity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray) override;

    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

};

#endif
