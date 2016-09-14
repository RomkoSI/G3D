/**
  \file GLG3D/NativeTriTree.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2016-09-11
*/
#pragma once

#include "G3D/platform.h"
#include <functional>
#include "G3D/Color3.h"
#include "G3D/AABox.h"
#include "G3D/MemoryManager.h"
#include "G3D/SmallArray.h"
#include "G3D/Triangle.h"
#include "GLG3D/TriTreeBase.h"
#include "GLG3D/Component.h"
#ifndef _MSC_VER
#include <stdint.h>
#endif

namespace G3D {
/**
 \brief Native C++ implementation of a static bounding interval hierarchy
        that is very good for box queries and OK for ray-triangle.

 The BIH is a tree in which each node is an axis-aligned box
 containing up to three child nodes: elements in the negative half
 space of a splitting plane, elements in the positive half space, and
 elements spaning both sides.  When constructing the tree, spanning
 elements can either be inserted at a spanning node or split and
 inserted into the child nodes.  The presence of a splitting plane
 allows early-out ray intersection like a kd-tree and the bounding
 boxes allow relatively tight tree pruning, like a bounding volume
 hierarchy.

 Various algorithms are implemented for choosing the splitting plane
 that trade between ray-intersection performance and tree-building
 performance.

 @cite Watcher and Keller, Instant Ray Tracing: The Bounding Interval Hierarchy, EGSR 2006
 http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.87.4612
*/
class NativeTriTree : public TriTreeBase {
public:
    using TriTreeBase::intersectRay;

    /**
      \breif A 3D Ray optimized for ray casting, optionally limited to a positive subsegment of the ray.

      \sa Ray
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
          "Fast Ray / Axis-Aligned Bounding Box Overlap Tests using Ray Slopes" 
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
           Creates a Ray from a origin and a (nonzero) unit direction.
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
           \brief Returns the closest point on the Ray to point.
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
         Returns the closest distance between point and the Ray
         */
        float distance(const Point3& point) const {
            return (closestPoint(point) - point).magnitude();
        }

        /**
         Returns the point where the Ray and plane intersect.  If there
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
         Ray-triangle intersection for a 1-sided triangle.  Fastest version.
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


    enum SplitAlgorithm {
        /** Produce nodes with approximately equal shape by splitting
            nodes half-way across the bounds of their contents
            (similar to an oct-tree).  Fastest method for building the
            tree. */
        MEAN_EXTENT, 

        /** Split nodes so that children have about the same surface area. */
        MEDIAN_AREA,

        /** Split nodes so that children have about the same number of
            triangles.*/
        MEDIAN_COUNT, 

        /** Split nodes so that they have approximately equal
            intersection times, according to a Surface Area Heuristic.
            Theory indicates that this gives the highest performance
            for ray intersection, although that may not be the case
            for specific scenes and rays.*/
        SAH};

    class Settings {
    public:
        /*
        \param computePrevPosition If true, compute the
        CPUVertexArray::prevPosition array that is then used to
        compute Surfel::prevPosition.  This requires more space in
        memory but allows the reconstruction of motion vectors for
        post-processed motion blur.  Note that NativeTriTree::intersect only
        traces against the current time, however. */
        bool               computePrevPosition;

        SplitAlgorithm     algorithm;

        /** Fraction of the bounding box surface area that one polygon is allowed
            to contribute before it is forced to be stored at an internal node.
            Set to inf() to disable placing triangles at internal nodes.

            1/6 = a triangle cutting across an entire cube will be placed at
            that node, if it spans the splitting plane.            
        */
        float              maxAreaFraction;

        /** Put approximately this many triangles at each leaf.  Some
           leaves may have more because no suitable splitting plane
           could be found. */
        int                valuesPerLeaf;

        /** SAH uses an approximation to the published heuristic to
            reduce splitting from \f$O(n^2)\f$ to
            \f$O(n)\f$.  When the number of Tris to be
            divided at a node falls below accurateSAHCountThreshold,
            it switches to the full heuristic for increased accuracy.

            Set to zero to always use the accurate method and
            <code>std::numeric_limits<int>::max()</code> to always use
            the fast method.*/
        int                accurateSAHCountThreshold;

        inline Settings() : 
            algorithm(MEAN_EXTENT), 
            maxAreaFraction(1.0f / 11.0f), 
            valuesPerLeaf(4),
            accurateSAHCountThreshold(125) {}
    };

    static const char* algorithmName(SplitAlgorithm s);

    class Stats {
    public:
        int numLeaves;

        /** Total triangles stored, after splitting */
        int numTris;

        int numNodes;

        int shallowestLeaf;

        /** Shallowest node that contains more than the minimum number of Tris.*/
        int shallowestNodeOverMin;

        float averageValuesPerLeaf;

        /** Deepest leaf */
        int depth;

        /** Max tris per node of any node */
        int largestNode;

        Stats() : numLeaves(0), numTris(0), numNodes(0), shallowestLeaf(100000),
                  shallowestNodeOverMin(100000), averageValuesPerLeaf(0), 
                  depth(0), largestNode(0) {}
    };

private:
    /** A convex polygon formed by repeatedly clipping a Tri with axis-aligned planes */
    class Poly {
    private:
        /** Pointer into the NativeTriTree::m_triArray */
        const Tri*             m_source;
        Vector3                m_low;
        Vector3                m_high;
        float                  m_area;

        /** Preallocate space for several vertices
            to avoid heap allocation per-poly.*/
        SmallArray<Vector3, 4> m_vertex;

        /** Called from split */
        inline void addVertex(const Vector3& v) {
            m_vertex.append(v);
            m_low  = m_low.min(v);
            m_high = m_high.max(v);
        }

        /** Called from split
            Due to floating point roundoff, redundant vertices and 
            sliver triangles some times get generated; avoid that.
        */
        inline void addIfNewVertex(const Vector3& v) {
            if ((m_vertex.size() == 0) || (m_vertex.last() != v)) {
                addVertex(v);
            }
        }

        /** Called from split to recompute m_area */
        void computeArea();

    public:

        Poly();

        Poly(const CPUVertexArray& vertexArray, const Tri* t);

        /** Original triangle from which this was created */
        inline const Tri* source() const {
            return m_source;
        }

        /** Bounding box low end */
        inline const Vector3& low() const {
            return m_low;
        }

        /** Bounding box high end */
        inline const Vector3& high() const {
            return m_high;
        }

        /** Surface area */
        inline float area() const {
            return m_area;
        }

        /** Render this poly using a triangle fan. Inefficient; only 
            intended for debugging.*/
        void draw(class RenderDevice* rd, const CPUVertexArray& vertexArray) const;
    
        /** Splits this at position @a offset on @a axis and appends the
            one or two pieces to the appropriate arrays. 

            If this spans the splitting plane and has area >= minSpanArea,
            then this is added to largeSpanArray instead.  Choose
            minSpanArea = inf() to prevent this case from ever arising.
            Choose minSpanArea = 0 to force all spanning polys to fall into
            largeSpanArray.        
        */
        void split
        (Vector3::Axis axis,
         float         offset,
         float         minSpanArea,
         Array<Poly>&  lowArray,
         Array<Poly>&  highArray,
         Array<Poly>&  largeSpanArray) const;

        static AABox computeBounds(const Array<Poly>& array);

        inline void getBounds(AABox& b) const {
            b.set(m_low, m_high);
        }
    };

    static inline Color3 chooseColor(const void* ptr) {
        return Color3::pastelMap(uint32(intptr_t(ptr)));
    }

    /** Compares whether the high() of one Poly is less than the high() of another */
    class HighComparator {
    private:
        
        Vector3::Axis sortAxis;
        
    public:
        
        HighComparator(Vector3::Axis a) : sortAxis(a) {}
        
        inline bool operator()(const Poly& A, const Poly& B) const {
            return A.high()[sortAxis] < B.high()[sortAxis];
        }
    };


    /** Does not have to be deleted; has no constructor. */
    struct ValueArray {
        /** Pointers into the NativeTriTree's 

            Each Tri may extend out of the Node's bounds, because it has been
            split. That does not affect performance because the time
            to compute an intersection is independent of the area
            of the Tri, and by proceeding in splitting-plane order
            the probability dependent on the area outside the
            bounds is zero. */
        const Tri**      data;

        int              size;

        /** Bounds on the part of valueArray that is within bounds,
            for internal nodes that contain triangles. Undefined
            for nodes without triangles.*/
        AABox            bounds;
    };
    
    class Node {
    private:
        
        /** Bounds on this node and all of its children */
        AABox            bounds;

        /** Position along the split axis */
        float            splitLocation;
        
        /** Stores the child pointer in the high bits and the split
            axis in the low 2 bits. 

            Because they are always two children, we only store a
            pointer to the first.*/
        uintptr_t        packedChildAxis;

        ValueArray*      valueArray;

        /** 0 = node below split location, 1 = node above split
            location.  At an internal node, both are non-NULL,
            at a leaf, both are NULL. 
        */
        inline Node& __fastcall child(int i) const {
            debugAssert(i >= 0 && i <= 1);
            return reinterpret_cast<Node*>(packedChildAxis & ~3)[i];
        }

        inline Vector3::Axis splitAxis() const {
            return static_cast<Vector3::Axis>(packedChildAxis & 3);
        }

        inline bool isLeaf() const {
            return (packedChildAxis < 4);
        }

        void setValueArray(const Array<Poly>& src, const shared_ptr<MemoryManager>& mm);

        /** Returns true if the split that divided the originals into
            low and high did not effectively reduce the number of
            underlying source Tris.*/
        bool badSplit(int numOriginalSources, int numLow, int numHigh);

        /** Split these polys at the current splitLocation and
            splitAxis, and then recurse into children.  Assumes bounds
            has been set.
            
            Called from the constructor. */
        void split(Array<Poly>& original, const Settings& settings, 
                   const shared_ptr<MemoryManager>& mm);

        /** Called from the constructor to choose a splitting plane
            and axis. Assumes that this->bounds is already set. */
        float chooseSplitLocation(Array<Poly>& source, const Settings& settings, 
                                  Vector3::Axis axis);

        float chooseMedianAreaSplitLocation(Array<Poly>& original, 
                                            Vector3::Axis axis);

        struct FloatHashTrait {
            static size_t hashCode(float k) { return *reinterpret_cast<size_t*>(&k); }
        };

        float chooseSAHSplitLocation(Array<Poly>& source, Vector3::Axis axis, const Settings& settings);

        float chooseSAHSplitLocationAccurate(Array<Poly>& source, 
                                         Vector3::Axis axis, const Settings& settings);

        float chooseSAHSplitLocationFast(Array<Poly>& source, Vector3::Axis axis, const Settings& settings);

        /** The SAHCost of tracing against just this array. */
        static float SAHCost(int size, float area, float containingArea);

        /** Returns the cost of splitting at this location under the
            surface area heuristic.

          Surface area heuristic: Trace time ~= boxIntersectTime +
          triIntersectTime * num * polyArea / boxArea
                      
        */
        static float SAHCost(Vector3::Axis axis, float offset,
                             const Array<Poly>& original, float containingArea, 
                             const Settings& settings);

        /** Called from intersect to determine which child the ray hits first.

             There are three cases to consider:
             
              1. the ray can start on one side of the splitting plane and never enter the other,
              2. the ray can start on one side and enter the other, and
              3. the ray can travel exactly down the splitting plane
        */
        inline void __fastcall computeTraversalOrder(const Ray& ray, int& firstChild, int& secondChild) const {
            const Vector3::Axis axis = splitAxis();
            const float origin = ray.origin()[axis];
            const float direction = ray.direction()[axis];

            if (origin < splitLocation) {
        
                // The ray starts on the small side
                firstChild = 0;
        
                if (direction > 0) {
                    // The ray will eventually reach the other side
                    secondChild = 1;
                }
        
            } else if (origin > splitLocation) {
        
                // The ray starts on the large side
                firstChild = 1;
        
                if (direction < 0) {
                    // The ray will eventually reach the other side
                    secondChild = 0;
                }
            } else {

                // The ray starts *on* the splitting plane
                if (direction < 0) {
                    // ...and goes to the small side
                    firstChild = 0;
                } else if (direction > 0) {
                    // ...and goes to the large side
                    firstChild = 1;
                } else {
                    // ...and travels in the splitting plane.  The order is
                    // arbitrary
                    firstChild  = 0;
                    secondChild = 1;
                }
            }
        }

    public:

        Node(Array<Poly>& originals, const Settings& settings, 
             const shared_ptr<MemoryManager>& mm);

        /** Call in lieu of delete to remove children.  Caller must
            free the Node itself.*/
        void destroy(const shared_ptr<MemoryManager>& mm);

        void draw(RenderDevice* rd, const CPUVertexArray& vertexArray, int level, bool showBoxes, int minNodeSize) const;

        /** Append all contained triangles that intersect this to triArray. Assumes that this node 
            does intersect the box. 

            \param alreadyAdded Since nodes do not have unique ownership of triangles, this set is needed
            to avoid adding duplicates to the triArray.
          */  
        void intersectBox(const AABox& box, const CPUVertexArray& vertexArray, Array<Tri>& triArray, Set<Tri*>& alreadyAdded) const;
        void intersectSphere(const Sphere& sphere, const CPUVertexArray& vertexArray, Array<Tri>& triArray, Set<Tri*>& alreadyAdded) const;

        void print(const String& indent) const;

        void getStats(Stats& s, int level, int valuesPerNode) const;

        bool __fastcall intersectRay
        (const NativeTriTree&                     triTree,
         const Ray&                         ray,
         float                              maxDistance,
         Hit&                               hit,
         IntersectRayOptions                options) const;
    };

    /** Memory manager used to allocate Nodes and Tri arrays. */
    shared_ptr<MemoryManager>   m_memoryManager;

    /** Allocated with m_memoryManager */
    Node*                m_root;
    
public:

    NativeTriTree();

    ~NativeTriTree();

    virtual void clear() override;

    /** Walk the entire tree, computing statistics */
    Stats stats(int valuesPerNode) const;
        
    virtual void intersectSphere
        (const Sphere&                      sphere,
         Array<Tri>&                        triArray) const override;

    virtual void rebuild() override;

    virtual bool intersectRay
        (const Ray&                         ray, 
         Hit&                               hit,
         IntersectRayOptions                options         = IntersectRayOptions(0)) const override;

    virtual void intersectBox
        (const AABox&                       box,
         Array<Tri>&                        results) const override;

    /** Render the tree for debugging and visualization purposes. 
        Inefficent.

        \param level Show the nodes at or above this level of the tree, where 0 = root

        \param showBoxes Render bounding boxes for internal nodes at level == @a level

        \param minNodeSize Do not render triangles at nodes with fewer than this many triangles.
        Set to zero to disable, set to a high number to spot poorly-constructed nodes

    */
    void draw(RenderDevice* rd, int level, bool showBoxes = true, int minNodeSize = 0);

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

inline float NativeTriTree::PrecomputedRay::intersectionTime(
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


inline float NativeTriTree::PrecomputedRay::intersectionTime
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
}
