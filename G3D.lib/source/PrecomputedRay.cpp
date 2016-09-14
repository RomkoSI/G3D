/**
 @file PrecomputedRay.cpp 
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2002-07-12
 @edited  2016-09-16
 */

#include "G3D/platform.h"
#include "G3D/PrecomputedRay.h"
#include "G3D/Plane.h"
#include "G3D/Sphere.h"
#include "G3D/CollisionDetection.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

namespace G3D {

PrecomputedRay& PrecomputedRay::operator=(const Ray& ray) {
    set(ray.origin(), ray.direction(), ray.minDistance(), ray.maxDistance());
    return *this;
}


PrecomputedRay::PrecomputedRay(const Ray& ray) {
    *this = ray;
}


PrecomputedRay::operator Ray() const {
    return Ray(m_origin, m_direction, m_minDistance, m_maxDistance);
}


void PrecomputedRay::set(const Vector3& origin, const Vector3& direction, float mn, float mx) {
    debugAssert(mn >= 0.0 && mn < mx);
    m_minDistance = mn;
    m_maxDistance = mx;
    m_origin = origin;
    m_direction = direction;
    debugAssert(direction.isUnit());
    
    m_invDirection = Vector3::one() / direction;
    
    // ray slope
    ibyj = m_direction.x * m_invDirection.y;
    jbyi = m_direction.y * m_invDirection.x;
    jbyk = m_direction.y * m_invDirection.z;
    kbyj = m_direction.z * m_invDirection.y;
    ibyk = m_direction.x * m_invDirection.z;
    kbyi = m_direction.z * m_invDirection.x;
    
    // precomputed terms
    c_xy = m_origin.y - jbyi * m_origin.x;
    c_xz = m_origin.z - kbyi * m_origin.x;
    c_yx = m_origin.x - ibyj * m_origin.y;
    c_yz = m_origin.z - kbyj * m_origin.y;
    c_zx = m_origin.x - ibyk * m_origin.z;
    c_zy = m_origin.y - jbyk * m_origin.z;    
    
    // ray slope classification
    if (m_direction.x < 0) {
        if (m_direction.y < 0) {
            if (m_direction.z < 0) {
                classification = MMM;
            } else if (m_direction.z > 0) {
                classification = MMP;
            } else { //(m_direction.z >= 0)
                classification = MMO;
            }
        } else { //(m_direction.y >= 0)
            if (m_direction.z < 0) {
                if (m_direction.y == 0) {
                    classification = MOM;
                } else {
                    classification = MPM;
                }
            } else { //(m_direction.z >= 0)
                if ((m_direction.y == 0) && (m_direction.z == 0)) {
                    classification = MOO;
                } else if (m_direction.z == 0) {
                    classification = MPO;
                } else if (m_direction.y == 0) {
                    classification = MOP;
                } else {
                    classification = MPP;
                }
            }
        }
    } else { //(m_direction.x >= 0)
        if (m_direction.y < 0) {
            if (m_direction.z < 0) {
                if (m_direction.x == 0) {
                    classification = OMM;
                } else {
                    classification = PMM;
                }
            } else { //(m_direction.z >= 0)
                if ((m_direction.x == 0) && (m_direction.z == 0)) {
                    classification = OMO;
                } else if (m_direction.z == 0) {
                    classification = PMO;
                } else if (m_direction.x == 0) {
                    classification = OMP;
                } else {
                    classification = PMP;
                }
            }
        } else { //(m_direction.y >= 0)
            if (m_direction.z < 0) {
                if ((m_direction.x == 0) && (m_direction.y == 0)) {
                    classification = OOM;
                } else if (m_direction.x == 0) {
                    classification = OPM;
                } else if (m_direction.y == 0) {
                    classification = POM;
                } else {
                    classification = PPM;
                }
            } else { //(m_direction.z > 0)
                if (m_direction.x == 0) {
                    if (m_direction.y == 0) {
                        classification = OOP;
                    } else if (m_direction.z == 0) {
                        classification = OPO;
                    } else {
                        classification = OPP;
                    }
                } else {
                    if ((m_direction.y == 0) && (m_direction.z == 0)) {
                        classification = POO;
                    } else if (m_direction.y == 0) {
                        classification = POP;
                    } else if (m_direction.z == 0) {
                        classification = PPO;
                    } else {
                        classification = PPP;
                    }
                }
            }            
        }
    }
}


PrecomputedRay::PrecomputedRay(class BinaryInput& b) {
    deserialize(b);
}


void PrecomputedRay::serialize(class BinaryOutput& b) const {
    m_origin.serialize(b);
    m_direction.serialize(b);
    b.writeFloat32(m_minDistance);
    b.writeFloat32(m_maxDistance);
}


void PrecomputedRay::deserialize(class BinaryInput& b) {
    m_origin.deserialize(b);
    m_direction.deserialize(b);
    const float mn = b.readFloat32();
    const float mx = b.readFloat32();
    set(m_origin, m_direction, mn, mx);
}


PrecomputedRay PrecomputedRay::refract(
    const Vector3&  newOrigin,
    const Vector3&  normal,
    float           iInside,
    float           iOutside) const {

    const Vector3 D = m_direction.refractionDirection(normal, iInside, iOutside);
    return PrecomputedRay(newOrigin + (m_direction + normal * (float)sign(m_direction.dot(normal))) * 0.001f, D);
}


PrecomputedRay PrecomputedRay::reflect(
    const Vector3&  newOrigin,
    const Vector3&  normal) const {

    Vector3 D = m_direction.reflectionDirection(normal);
    return PrecomputedRay(newOrigin + (D + normal) * 0.001f, D);
}


Vector3 PrecomputedRay::intersection(const Plane& plane) const {
    float d;
    Vector3 normal = plane.normal();
    plane.getEquation(normal, d);
    float rate = m_direction.dot(normal);

    if (rate >= 0.0f) {
        return Vector3::inf();
    } else {
        float t = -(d + m_origin.dot(normal)) / rate;
        if (t < m_minDistance || t > m_maxDistance) {
            return Vector3::inf();
        }
        return m_origin + m_direction * t;
    }
}


float PrecomputedRay::intersectionTime(const class Sphere& sphere, bool solid) const {
    Vector3 dummy;
    const float t = CollisionDetection::collisionTimeForMovingPointFixedSphere(
            m_origin, m_direction, sphere, dummy, dummy, solid);
    if ((t < m_minDistance) || (t > m_maxDistance)) {
        return finf();
    } else {
        return t;
    }
}


float PrecomputedRay::intersectionTime(const class Plane& plane) const {
    Vector3 dummy;
    const float t = CollisionDetection::collisionTimeForMovingPointFixedPlane(
            m_origin, m_direction, plane, dummy);
    if ((t < m_minDistance) || (t > m_maxDistance)) {
        return finf();
    } else {
        return t;
    }
}


float PrecomputedRay::intersectionTime(const class Box& box) const {
    Vector3 dummy;
    float time = CollisionDetection::collisionTimeForMovingPointFixedBox(
            m_origin, m_direction, box, dummy);

    if ((time == finf()) && (box.contains(m_origin))) {
        return 0.0f;
    } else if (time < m_minDistance || time > m_maxDistance) {
        return finf();
    } else {
        return time;
    }
}


float PrecomputedRay::intersectionTime(const class AABox& box) const {
    Vector3 dummy;
    bool inside;
    const float t = CollisionDetection::collisionTimeForMovingPointFixedAABox(
            m_origin, m_direction, box, dummy, inside);

    if ((t == finf()) && inside) {
        return 0.0f;
    } else if ((t < m_minDistance) || (t > m_maxDistance)) {
        return finf();
    } else {
        return t;
    }
}

} // G3D