/**
 \file G3D/PrecomputedRay.h
 
 PrecomputedRay class
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2002-07-12
 @edited  2016-09-14
 */

#pragma once

#include "G3D/platform.h"
#include "G3D/Vector3.h"
#include "G3D/Triangle.h"

namespace G3D {

class Ray;

/**
  A 3D Ray optimized for AABox and Tri intersection,
  optionally limited to a positive subsegment of the ray.

  \sa G3D::Ray, G3D::LineSegment, G3D::Line
 */
class PrecomputedRay {
private:
    friend class Intersect;

    // The order of the first four member variables is guaranteed and may not be changed

    Point3  m_origin;

    float   m_minDistance;

    /** Unit length */
    Vector3 m_direction;

    float   m_maxDistance;

    /** 1.0 / direction */
    Vector3 m_invDirection;

    
    /** The following are for the "ray slope" optimization from
      "Fast PrecomputedRay / Axis-Aligned Bounding Box Overlap Tests using PrecomputedRay Slopes" 
      by Martin Eisemann, Thorsten Grosch, Stefan M��ller and Marcus Magnor
      Computer Graphics Lab, TU Braunschweig, Germany and
      University of Koblenz-Landau, Germany */
    enum Classification {MMM, MMP, MPM, MPP, PMM, PMP, PPM, PPP, POO, MOO, OPO, OMO, OOP, OOM, OMM, OMP, OPM, OPP, MOM, MOP, POM, POP, MMO, MPO, PMO, PPO};    

    Classification classification;

    /** ray slope */
    float ibyj, jbyi, kbyj, jbyk, ibyk, kbyi;

    /** Precomputed components */
    float c_xy, c_xz, c_yx, c_yz, c_zx, c_zy;

public:

    /** \param direction Assumed to have unit length */
    void set(const Point3& origin, const Vector3& direction, float minDistance = 0.0f, float maxDistance = finf());

    PrecomputedRay& operator=(const Ray& ray);

    explicit PrecomputedRay(const Ray& ray);

    operator Ray() const;

    float minDistance() const {
        return m_minDistance;
    }

    float maxDistance() const {
        return m_maxDistance;
    }

    const Point3& origin() const {
        return m_origin;
    }
    
    /** Unit direction vector. */
    const Vector3& direction() const {
        return m_direction;
    }

    /** Component-wise inverse of direction vector.  May have inf() components */
    const Vector3& invDirection() const {
        return m_invDirection;
    }
    
    PrecomputedRay() {
        set(Point3::zero(), Vector3::unitX());
    }

    /** \param direction Assumed to have unit length */
    PrecomputedRay(const Point3& origin, const Vector3& direction, float minDistance = 0.0f, float maxDistance = finf()) {
        set(origin, direction, minDistance, maxDistance);
    }

    PrecomputedRay(class BinaryInput& b);
    
    void serialize(class BinaryOutput& b) const;

    void deserialize(class BinaryInput& b);
    
    /**
       Creates a PrecomputedRay from a origin and a (nonzero) unit direction.
    */
    static PrecomputedRay fromOriginAndDirection(const Point3& point, const Vector3& direction, float minDistance = 0.0f, float maxDistance = finf()) {
        return PrecomputedRay(point, direction, minDistance, maxDistance);
    }
    
    /** Returns a new ray which has the same direction but an origin
        advanced along direction by @a distance
        
        
        The min and max distance of the ray are unmodified. */
    PrecomputedRay bumpedRay(float distance) const {
        return PrecomputedRay(m_origin + m_direction * distance, m_direction, m_minDistance, m_maxDistance);
    }

    /** Returns a new ray which has the same direction but an origin
        advanced by \a distance * \a bumpDirection.
        
        The min and max distance of the ray are unmodified. */
    PrecomputedRay bumpedRay(float distance, const Vector3& bumpDirection) const {
        return PrecomputedRay(m_origin + bumpDirection * distance, m_direction, m_minDistance, m_maxDistance);
    }

    /**
       \brief Returns the closest point on the PrecomputedRay to point.
    */
    Point3 closestPoint(const Point3& point) const {
        float t = m_direction.dot(point - m_origin);
        if (t < m_minDistance) {
            return m_origin + m_direction * m_minDistance;
        } else if (t > m_maxDistance) {
            return m_origin + m_direction * m_maxDistance;
        } else {
            return m_origin + m_direction * t;
        }
    }

    /**
     Returns the closest distance between point and the PrecomputedRay
     */
    float distance(const Point3& point) const {
        return (closestPoint(point) - point).magnitude();
    }

    /**
     Returns the point where the PrecomputedRay and plane intersect.  If there
     is no intersection, returns a point at infinity.

      Planes are considered one-sided, so the ray will not intersect
      a plane where the normal faces in the traveling direction.
    */
    Point3 intersection(const class Plane& plane) const;

    /**
     Returns the distance until intersection with the sphere or the (solid) ball bounded by the sphere.
     Will be 0 if inside the sphere, inf if there is no intersection.

     The ray direction is <B>not</B> normalized.  If the ray direction
     has unit length, the distance from the origin to intersection
     is equal to the time.  If the direction does not have unit length,
     the distance = time * direction.length().

     See also the G3D::CollisionDetection "movingPoint" methods,
     which give more information about the intersection.

     \param solid If true, rays inside the sphere immediately intersect (good for collision detection).  If false, they hit the opposite side of the sphere (good for ray tracing).
     */
    float intersectionTime(const class Sphere& sphere, bool solid = false) const;

    float intersectionTime(const class Plane& plane) const;

    float intersectionTime(const class Box& box) const;

    float intersectionTime(const class AABox& box) const;

    /**
     The three extra arguments are the weights of vertices 0, 1, and 2
     at the intersection point; they are useful for texture mapping
     and interpolated normals.
     */
    float intersectionTime(
        const Vector3& v0, const Vector3& v1, const Vector3& v2,
        const Vector3& edge01, const Vector3& edge02,
        float& w0, float& w1, float& w2) const;

     /**
     PrecomputedRay-triangle intersection for a 1-sided triangle.  Fastest version.
       @cite http://www.acm.org/jgt/papers/MollerTrumbore97/
       http://www.graphics.cornell.edu/pubs/1997/MT97.html
     */
    float intersectionTime(
        const Point3& vert0,
        const Point3& vert1,
        const Point3& vert2,
        const Vector3& edge01,
        const Vector3& edge02) const;


    float intersectionTime(
        const Point3& vert0,
        const Point3& vert1,
        const Point3& vert2) const {

        return intersectionTime(vert0, vert1, vert2, vert1 - vert0, vert2 - vert0);
    }


    float intersectionTime(
        const Point3&  vert0,
        const Point3&  vert1,
        const Point3&  vert2,
        float&         w0,
        float&         w1,
        float&         w2) const {

        return intersectionTime(vert0, vert1, vert2, vert1 - vert0, vert2 - vert0, w0, w1, w2);
    }


    /* One-sided triangle 
       */
    float intersectionTime(const Triangle& triangle) const {
        return intersectionTime(
            triangle.vertex(0), triangle.vertex(1), triangle.vertex(2),
            triangle.edge01(), triangle.edge02());
    }


    float intersectionTime
    (const Triangle& triangle,
     float&         w0,
     float&         w1,
     float&         w2) const {
        return intersectionTime(triangle.vertex(0), triangle.vertex(1), triangle.vertex(2),
            triangle.edge01(), triangle.edge02(), w0, w1, w2);
    }

    /** Refracts about the normal
        using G3D::Vector3::refractionDirection
        and bumps the ray slightly from the newOrigin. 
        
        Sets the min distance to zero and the max distance to infinity.
        */
    PrecomputedRay refract(
        const Vector3&  newOrigin,
        const Vector3&  normal,
        float           iInside,
        float           iOutside) const;

    /** Reflects about the normal
        using G3D::Vector3::reflectionDirection
        and bumps the ray slightly from
        the newOrigin. 
        
        Sets the min distance to zero and the max distance to infinity.
        */
    PrecomputedRay reflect(
        const Vector3&  newOrigin,
        const Vector3&  normal) const;
};


#define EPSILON 0.000001
#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2]; 

inline float PrecomputedRay::intersectionTime(
    const Point3& vert0,
    const Point3& vert1,
    const Point3& vert2,
    const Vector3& edge1,
    const Vector3& edge2) const {

    (void)vert1;
    (void)vert2;

    // Barycenteric coords
    float u, v;

    float tvec[3], pvec[3], qvec[3];
    
    // begin calculating determinant - also used to calculate U parameter
    CROSS(pvec, m_direction, edge2);
    
    // if determinant is near zero, ray lies in plane of triangle
    const float det = DOT(edge1, pvec);
    
    if (det < EPSILON) {
        return finf();
    }
    
    // calculate distance from vert0 to ray origin
    SUB(tvec, m_origin, vert0);
    
    // calculate U parameter and test bounds
    u = DOT(tvec, pvec);
    if ((u < 0.0f) || (u > det)) {
        // Hit the plane outside the triangle
        return finf();
    }
    
    // prepare to test V parameter
    CROSS(qvec, tvec, edge1);
    
    // calculate V parameter and test bounds
    v = DOT(m_direction, qvec);
    if ((v < 0.0f) || (u + v > det)) {
        // Hit the plane outside the triangle
        return finf();
    }
    
    // Case where we don't need correct (u, v):
    float t = DOT(edge2, qvec);
    
    if (t >= 0.0f) {
        // Note that det must be positive
        t = t / det;
        if (t < m_minDistance || t > m_maxDistance) {
            return finf();
        } else {
            return t;
        }
    } else {
        // We had to travel backwards in time to intersect
        return finf();
    }
}


inline float PrecomputedRay::intersectionTime
(const Point3&  vert0,
 const Point3&  vert1,
 const Point3&  vert2,
 const Vector3&  edge1,
 const Vector3&  edge2,
 float&         w0,
 float&         w1,
 float&         w2) const {

    (void)vert1;
    (void)vert2;

    // Barycenteric coords
    float u, v;

    float
        tvec[3], pvec[3], qvec[3];

    // begin calculating determinant - also used to calculate U parameter
    CROSS(pvec, m_direction, edge2);
    
    // if determinant is near zero, ray lies in plane of triangle
    const float det = DOT(edge1, pvec);
    
    if (det < EPSILON) {
        return finf();
    }
    
    // calculate distance from vert0 to ray origin
    SUB(tvec, m_origin, vert0);
    
    // calculate U parameter and test bounds
    u = DOT(tvec, pvec);
    if ((u < 0.0f) || (u > det)) {
        // Hit the plane outside the triangle
        return finf();
    }
    
    // prepare to test V parameter
    CROSS(qvec, tvec, edge1);
    
    // calculate V parameter and test bounds
    v = DOT(m_direction, qvec);
    if ((v < 0.0f) || (u + v > det)) {
        // Hit the plane outside the triangle
        return finf();
    }
    
    float t = DOT(edge2, qvec);
    
    if (t >= 0) {
        const float inv_det = 1.0f / det;
        t *= inv_det;

        if (t < m_minDistance || t > m_maxDistance) {
            return finf();
        }

        u *= inv_det;
        v *= inv_det;
        w0 = (1.0f - u - v);
        w1 = u;
        w2 = v;

        return t;
    } else {
        // We had to travel backwards in time to intersect
        return finf();
    }
}

#undef EPSILON
#undef CROSS
#undef DOT
#undef SUB

}// namespace
