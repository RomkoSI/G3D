#include "PlayerEntity.h"

shared_ptr<Entity> PlayerEntity::create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      loadOptions) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    shared_ptr<PlayerEntity> playerEntity(new PlayerEntity());

    // Initialize each base class, which parses its own fields
    playerEntity->Entity::init(name, scene, propertyTable);
    playerEntity->VisibleEntity::init(propertyTable, modelTable);
    playerEntity->PlayerEntity::init(propertyTable);

    // Verify that all fields were read by the base classes
    propertyTable.verifyDone();

    return playerEntity;
}


shared_ptr<PlayerEntity> PlayerEntity::create 
(const String&                           name,
 Scene*                                  scene,
 const CFrame&                           position,
 const shared_ptr<Model>&                model) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    shared_ptr<PlayerEntity> playerEntity(new PlayerEntity());

    // Initialize each base class, which parses its own fields
    playerEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
    playerEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
    playerEntity->PlayerEntity::init(Vector3::zero());
 
    return playerEntity;
}


void PlayerEntity::init(AnyTableReader& propertyTable) {
    Vector3 v;
    propertyTable.getIfPresent("velocity", v);
    init(v);
}


void PlayerEntity::init(const Vector3& velocity) {
    m_maxOSVelocity = Vector3(30, 30, 90);
    // Reach max velocity over a short duration
    m_maxOSAcceleration = m_maxOSVelocity / (Vector3(0.3f, 0.3f, 1.5f) * units::seconds());

    m_velocity = velocity;
}


Any PlayerEntity::toAny(const bool forceAll) const {
    Any a = VisibleEntity::toAny(forceAll);
    a.setName("PlayerEntity");

    a["velocity"] = m_velocity;

    return a;
}
    
 
void PlayerEntity::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
    VisibleEntity::onPose(surfaceArray);
}


static Vector3 minMagnitude(const Vector3& desired, const Vector3& maxVal) {
    return sign(desired) * min(abs(desired), maxVal);
}


/** Maximum coordinate values for the player ship */
static const Point3 MAX_POS(20, 10, 0);
void PlayerEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    // Do not call Entity::onSimulation; that will override with spline animation

    if (! (isNaN(deltaTime) || (deltaTime == 0))) {
        m_previousFrame = m_frame;
    }

    simulatePose(absoluteTime, deltaTime);

    if (deltaTime > 0) {
        // This particular game setup is like Afterburner, where the ship is locked
        // and math is simplified by not allowing true rotation. For free flight,
        // we'd have to actually compute the object to world transformations and
        // deal with the interaction between rotation and translation.

        Vector3 osVelocity = m_velocity;//m_frame.vectorToObjectSpace(m_velocity);

        // Clamp desire to what is allowed by this object's own forces, but allow
        // it to exceed "max velocity" due to external forces
        const Vector3& desiredOSImpulse = minMagnitude(m_desiredOSVelocity, m_maxOSVelocity) - osVelocity;

        // Clamp impulse in object space (work with impulses to avoid dividing and
        // then multiplying by deltaTime, which could be a small number and hurt precision).
        const Vector3& osImpulse = minMagnitude(desiredOSImpulse, m_maxOSAcceleration * float(deltaTime));
    
        // Accelerate in object space
        osVelocity += osImpulse;
        m_velocity = osVelocity;//m_frame.vectorToWorldSpace(osVelocity);
        m_frame.translation += m_velocity * (float)deltaTime;

        // Tilt based on object space velocity
        const float maxRoll = 50 * units::degrees();
        float osRoll = maxRoll * -m_velocity.x / m_maxOSVelocity.x;

        const float maxPitch = 45 * units::degrees();
        float osPitch = maxPitch * m_velocity.y / m_maxOSVelocity.y;
        m_frame.rotation = Matrix3::fromAxisAngle(Vector3::unitX(), osPitch) * Matrix3::fromAxisAngle(Vector3::unitZ(), osRoll);

        m_frame.translation = m_frame.translation.clamp(-MAX_POS, MAX_POS);
    }
}
