#include "PlayerEntity.h"

#include "PhysicsScene.h"

// Print lots of debugging info
//#define TRACE_COLLISIONS

// Show collision geometry
//#define SHOW_COLLISIONS


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


shared_ptr<Entity> PlayerEntity::create 
(const String&                           name,
 Scene*                                  scene,
 const CFrame&                           position,
 const shared_ptr<Model>&                model) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    shared_ptr<PlayerEntity> playerEntity(new PlayerEntity());

    // Initialize each base class, which parses its own fields
    playerEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
    playerEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
    playerEntity->PlayerEntity::init(Vector3::zero(), Sphere(1.0f));
 
    return playerEntity;
}


void PlayerEntity::init(AnyTableReader& propertyTable) {
    Vector3 v;
    propertyTable.getIfPresent("velocity", v);
    Sphere s(1.5f);
    propertyTable.getIfPresent("collisionSphere", s);

    init(v, s);
}


void PlayerEntity::init(const Vector3& velocity, const Sphere& collisionProxy) {
    m_velocity = velocity;
    m_collisionProxySphere = collisionProxy;
    m_desiredOSVelocity     = Vector3::zero();
    m_desiredYawVelocity    = 0;
    m_desiredPitchVelocity  = 0;
    m_heading               = 0;
    m_headTilt              = 0;
}


Any PlayerEntity::toAny(const bool forceAll) const {
    Any a = VisibleEntity::toAny(forceAll);
    a.setName("PlayerEntity");

    a["velocity"] = m_velocity;
    a["collisionSphere"] = m_collisionProxySphere;

    return a;
}
    
 
void PlayerEntity::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
    VisibleEntity::onPose(surfaceArray);
}


/** Maximum coordinate values for the player ship */
//static const Point3 MAX_POS(10, 5, 0);
void PlayerEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    // Do not call Entity::onSimulation; that will override with spline animation

    if (! isNaN(deltaTime) && (deltaTime > 0)) {
        m_previousFrame = m_frame;
    }
    simulatePose(absoluteTime, deltaTime);

    //m_velocity = m_frame.vectorToWorldSpace(m_desiredOSVelocity);
    //m_frame.translation += m_velocity * (float)deltaTime;
    if (! isNaN(deltaTime)) {
        slideMove(deltaTime);
        m_heading           += m_desiredYawVelocity * (float)deltaTime;
        m_frame.rotation     = Matrix3::fromAxisAngle(Vector3::unitY(), m_heading);

        m_headTilt = clamp(m_headTilt + m_desiredPitchVelocity, -80 * units::degrees(), 80 * units::degrees());
    }

}


void PlayerEntity::getConservativeCollisionTris(Array<Tri>& triArray, const Vector3& velocity, float deltaTime) const {
    Sphere nearby = collisionProxy();
    nearby.radius += velocity.length() * deltaTime;
    ((PhysicsScene*)m_scene)->staticIntersectSphere(nearby, triArray);

#   ifdef SHOW_COLLISIONS
        //debugDraw(new SphereShape(nearby), 0, Color4::clear(), Color3::black());
        debugDraw(new MeshShape(triArray), 0, Color3::cyan(), Color3::blue());
#   endif
}


bool PlayerEntity::findFirstCollision
(const Array<Tri>&    triArray, 
 const Vector3&       velocity, 
 float&               stepTime, 
 Vector3&             collisionNormal,
 Point3&              collisionPoint) const {

    bool collision = false;

    const Sphere& startSphere = collisionProxy();
    for (int t = 0; t < triArray.size(); ++t) {

        const Tri& tri = triArray[t];
        const CPUVertexArray& cpuVertexArray = ((PhysicsScene*)m_scene)->vertexArrayOfCollisionTree();
        Triangle triangle(tri.position(cpuVertexArray,0),tri.position(cpuVertexArray,1), tri.position(cpuVertexArray,2));
        Vector3 C;
        const float d = 
            CollisionDetection::collisionTimeForMovingSphereFixedTriangle
            (startSphere, velocity, triangle, C);
        
        if (d < stepTime) {
            // Found a new collision sooner than the previous one.
            const Vector3& centerAtCollisionTime = startSphere.center + velocity * d;
             
            const Vector3& delta = centerAtCollisionTime - C;

            // Distance from sphere to collision point; if this is less than the sphere radius,
            // the collision was interpenetarat
            const float r = delta.length();
            const Vector3& n = delta / r;

            static const float epsilon = 0.000001f;
            const bool interpenetration = (r < startSphere.radius - epsilon);
            const bool rightDirection   = (dot(n, velocity) < -epsilon);

            if (interpenetration || rightDirection) {
                // Normal to the sphere at the collision point
                collisionNormal = n;
                collisionPoint  = C;
                stepTime        = d;
                collision       = true;
            }
        }
    }

#   ifdef SHOW_COLLISIONS
        if (collision) {
            if (collisionNormal.y < 0.99f) {
                //runSimulation = false;
                const float duration = 1.0f;
                debugDraw(new SphereShape(Sphere(collisionPoint, 0.1f)), duration, Color3::red(), Color4::clear());
                debugDraw(new ArrowShape(collisionPoint, collisionNormal), duration, Color3::red(), Color4::clear());
            }
        }
#   endif

    return collision;
}


void PlayerEntity::slideMove(SimTime timeLeft) { 
    static const float epsilon = 0.0001f;

    // Use constant velocity gravity (!)
    alwaysAssertM(((PhysicsScene*)m_scene)->gravity().x == 0.0f && ((PhysicsScene*)m_scene)->gravity().z == 0.0f, 
                            "We assume gravity points along the y axis to simplify implementation");
    
    m_desiredOSVelocity.y = max( ((PhysicsScene*)m_scene)->gravity().y, m_desiredOSVelocity.y + ((PhysicsScene*)m_scene)->gravity().y);
    // Initial velocity
    Vector3 velocity = frame().vectorToWorldSpace(m_desiredOSVelocity) + ((PhysicsScene*)m_scene)->gravity();

    Array<Tri> triArray;
    getConservativeCollisionTris(triArray, velocity, (float)timeLeft);
    
    // Trivial implementation that ignores collisions:
#   if 0
        m_frame.translation += velocity  * timeLeft;
        return;
#   endif

    // Keep simulating until we run out of time or velocity, at which point
    // no further movement is possible.
#   ifdef TRACE_COLLISIONS
        debugPrintf("================================\n");
        debugPrintf("Initial velocity = %s; position = %s\n", velocity.toString().c_str(),  m_frame.translation.toString().c_str());
#   endif
    int iterations = 0;
    while ((timeLeft > epsilon) && (velocity.length() > epsilon)) {
        float stepTime = float(timeLeft);
        Vector3 collisionNormal;
        Point3 collisionPoint;

        const bool collided = 
            findFirstCollision(triArray, velocity, stepTime, collisionNormal, collisionPoint);
#       ifdef TRACE_COLLISIONS
            debugPrintf("  stepTime = %f\n", stepTime);
#       endif
        // Advance to just before the collision
        stepTime = max(0.0f, stepTime - epsilon * 0.5f);
        m_frame.translation += velocity * stepTime;

        // Early out of loop when debugging
        //if (! runSimulation) { return; }
        
        if (collided) {
#           ifdef TRACE_COLLISIONS
                debugPrintf("  Collision C=%s, n=%s; position after=%s)\n", 
                            collisionPoint.toString().c_str(),
                            collisionNormal.toString().c_str(),
                            m_frame.translation.toString().c_str());
#           endif
            if (collisionProxy().contains(collisionPoint)) {
                // Interpenetration. This is bad because the
                // rest of the code assumes no interpenetration and
                // uses that to rise up steps.  Place the sphere
                // adjacent to the triangle and eliminate all velocity
                // towards the triangle.
                m_frame.translation = collisionPoint + collisionNormal * (m_collisionProxySphere.radius + epsilon * 2.0f);

                
#               ifdef TRACE_COLLISIONS
                    debugPrintf("  Interpenetration detected.  Position after = %s\n",
                        m_frame.translation.toString().c_str());
#               endif
            }
                
            // Constrain the velocity by subtracting the component
            // into the collision normal
            const Vector3& vPerp = collisionNormal * collisionNormal.dot(velocity);
            const Vector3& vPar  = velocity - vPerp;
#           ifdef SHOW_COLLISIONS
            if (collisionNormal.y < 0.95f) {
                float duration = 1.0f;
                debugDraw(new ArrowShape(collisionPoint, velocity), duration, Color3::green());
                debugDraw(new ArrowShape(collisionPoint, vPerp),    duration, Color3::yellow());
                debugDraw(new ArrowShape(collisionPoint, vPar),     duration, Color3::blue());
                if (duration == finf()) {
                    // Pause so we can see the result
                    runSimulation = false;
                }
            }
#           endif
            velocity = vPar;

#           ifdef TRACE_COLLISIONS
                debugPrintf("  velocity after collision = %s\n", velocity.toString().c_str());
#           endif
        }
#       ifdef TRACE_COLLISIONS
            debugPrintf("  --------------\n");
#       endif

        ++iterations;
        timeLeft -= stepTime;           
    }
    //screenPrintf("%d collision iterations", iterations);
}
