#ifndef PlayerEntity_h
#define PlayerEntity_h

#include <G3D/G3DAll.h>

class PlayerEntity : public VisibleEntity {
protected:

    Vector3         m_velocity;
    Vector3         m_desiredOSVelocity;
    /** In object-space */
    Sphere          m_collisionProxySphere;

    // Radians/s
    float           m_desiredYawVelocity;
    float           m_desiredPitchVelocity;

    // Radians
    float           m_heading;
    /** Unused for rendering, for use by a fps cam. */
    float           m_headTilt;

    PlayerEntity() {}

#ifdef G3D_OSX
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
    void init(AnyTableReader& propertyTable);
    
    void init(const Vector3& velocity, const Sphere& collisionSphere);
#ifdef G3D_OSX
    #pragma clang diagnostic pop
#endif

public:


    /** \brief Computes all triangles that could be hit during a
        slideMove with the current \a velocity, allowing that the
        velocity may be decreased along some axes during movement.

        Called from slideMove(). */
    void getConservativeCollisionTris(Array<Tri>& triArray, const Vector3& velocity, float deltaTime) const;
    
    /** Finds the first collision between m_collisionProxySphere
        travelling with \a velocity and the triArray.  Travels for at
        most \a stepTime, and updates \a stepTime with the
        collision time if there is one.  Returns true if there is a
        collision before the end of the original \a stepTime.

        \param collisionNormal Inward-pointing normal to the sphere at
        the collision time (separating axis).
    */
    bool findFirstCollision
    (const Array<Tri>&      triArray, 
     const Vector3&         velocity, 
     float&                 stepTime, 
     Vector3&               collisionNormal,
     Point3&                collisionPoint) const;

    /** Moves linearly for deltaTime using the current
     m_desiredLinearVelocity, decreasing velocity as needed to avoid
     collisions.  Called from onSimulation(). */
    void slideMove(SimTime deltaTime);


    /** In world space */
    Sphere collisionProxy() const {
        return Sphere(m_frame.pointToWorldSpace(m_collisionProxySphere.center), m_collisionProxySphere.radius);
    }

    /** In radians... not used for rendering, use for first-person cameras */
    float headTilt() const {
        return m_headTilt;
    }

    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                           name,
     Scene*                                  scene,
     AnyTableReader&                         propertyTable,
     const ModelTable&                       modelTable);

    /** For programmatic construction at runtime */
    static shared_ptr<Entity> create 
    (const String&                           name,
     Scene*                                  scene,
     const CFrame&                           position,
     const shared_ptr<Model>&                model);

    void setDesiredOSVelocity(const Vector3& objectSpaceVelocity) {
        m_desiredOSVelocity = objectSpaceVelocity;
    }

    const Vector3& desiredOSVelocity() {
        return m_desiredOSVelocity;
    }

    void setDesiredAngularVelocity(const float y, const float p) {
        m_desiredYawVelocity    = y;
        m_desiredPitchVelocity  = p;
    }

    /** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "PlayerEntity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray) override;

    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

};

#endif
