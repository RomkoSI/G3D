/**
 @file Ray.cpp 
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2002-07-12
 @edited  2016-09-14


  G3D Innovation Engine
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */

#include "G3D/platform.h"
#include "G3D/Ray.h"
#include "G3D/Plane.h"
#include "G3D/Sphere.h"
#include "G3D/CollisionDetection.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

namespace G3D {

void Ray::set(const Vector3& origin, const Vector3& direction, float mn, float mx) {
    debugAssert(mn >= 0.0 && mn < mx);
    m_minDistance = mn;
    m_maxDistance = mx;
    m_origin = origin;
    m_direction = direction;
    debugAssert(direction.isUnit());
}


Ray::Ray(class BinaryInput& b) {
    deserialize(b);
}


void Ray::serialize(class BinaryOutput& b) const {
    m_origin.serialize(b);
    m_direction.serialize(b);
    b.writeFloat32(m_minDistance);
    b.writeFloat32(m_maxDistance);
}


void Ray::deserialize(class BinaryInput& b) {
    m_origin.deserialize(b);
    m_direction.deserialize(b);
    float mn = b.readFloat32();
    float mx = b.readFloat32();
    set(m_origin, m_direction, mn, mx);
}


Ray Ray::refract(
    const Vector3&  newOrigin,
    const Vector3&  normal,
    float           iInside,
    float           iOutside) const {

    const Vector3 D = m_direction.refractionDirection(normal, iInside, iOutside);
    return Ray(newOrigin + (m_direction + normal * (float)sign(m_direction.dot(normal))) * 0.001f, D);
}


Ray Ray::reflect(
    const Vector3&  newOrigin,
    const Vector3&  normal) const {

    Vector3 D = m_direction.reflectionDirection(normal);
    return Ray(newOrigin + (D + normal) * 0.001f, D);
}


Vector3 Ray::intersection(const Plane& plane) const {
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


float Ray::intersectionTime(const class Sphere& sphere, bool solid) const {
    Vector3 dummy;
    const float t = CollisionDetection::collisionTimeForMovingPointFixedSphere(
            m_origin, m_direction, sphere, dummy, dummy, solid);
    if ((t < m_minDistance) || (t > m_maxDistance)) {
        return finf();
    } else {
        return t;
    }
}


float Ray::intersectionTime(const class Plane& plane) const {
    Vector3 dummy;
    const float t = CollisionDetection::collisionTimeForMovingPointFixedPlane(
            m_origin, m_direction, plane, dummy);
    if ((t < m_minDistance) || (t > m_maxDistance)) {
        return finf();
    } else {
        return t;
    }
}


float Ray::intersectionTime(const class Box& box) const {
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


float Ray::intersectionTime(const class AABox& box) const {
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

}
