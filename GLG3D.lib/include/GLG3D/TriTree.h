/**
  \file GLG3D/TriTree.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2012-10-04
*/
#ifndef G3D_TriTree_h
#define G3D_TriTree_h

#include "G3D/platform.h"
#include "G3D/Vector3.h"
#include "G3D/Color3.h"
#include "G3D/AABox.h"
#include "G3D/Ray.h"
#include "G3D/MemoryManager.h"
#include "G3D/Array.h"
#include "G3D/SmallArray.h"
#include "G3D/Intersect.h"
#include "G3D/CollisionDetection.h"
#include "GLG3D/Tri.h"
#include "GLG3D/Component.h"
#include "GLG3D/CPUVertexArray.h"
#ifndef _MSC_VER
#include <stdint.h>
#endif

namespace G3D {
class Surface;
class Surfel;
class Material;

/** 
 \brief Static bounding interval hierarchy for Ray-Tri intersections.

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
class TriTree {
public:
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
        /** Pointer into the TriTree::m_triArray */
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
        /** Pointers into the TriTree's 

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

        void setValueArray(const Array<Poly>& src, const MemoryManager::Ref& mm);

        /** Returns true if the split that divided the originals into
            low and high did not effectively reduce the number of
            underlying source Tris.*/
        bool badSplit(int numOriginalSources, int numLow, int numHigh);

        /** Split these polys at the current splitLocation and
            splitAxis, and then recurse into children.  Assumes bounds
            has been set.
            
            Called from the constructor. */
        void split(Array<Poly>& original, const Settings& settings, 
                   const MemoryManager::Ref& mm);

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
             const MemoryManager::Ref& mm);

        /** Call in lieu of delete to remove children.  Caller must
            free the Node itself.*/
        void destroy(const MemoryManager::Ref& mm);

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
        (const TriTree&  triTree,
         const Ray&      ray,
         Tri::Intersector& intersectCallback, 
         float&          distance,
         bool            exitOnAnyHit,
         bool            twoSided) const;
    };

    /** Memory manager used to allocate Nodes and Tri arrays. */
    MemoryManager::Ref   m_memoryManager;

    /** Allocated with m_memoryManager */
    Node*                m_root;

    /** All Tris in the tree.  These are stored here to reduce the
        memory footprint of duplicated Tris throughout the tree. There
        appears to be no significant performance overhead from the
        pointer jump of not storing Tris directly in the nodes. */
    Array<Tri>           m_triArray;



    /** All vertices referenced by the Tris in the TriTree */
    CPUVertexArray       m_cpuVertexArray;

public:

    TriTree();

    ~TriTree();

    /** Array access to the stored Tris */
    inline const Tri& operator[](int i) const {
        debugAssert(i >= 0 && i < m_triArray.size());
        return m_triArray[i];
    }

    void clear();

    /** Walk the entire tree, computing statistics */
    Stats stats(int valuesPerNode) const;

    /** The arrays will be copied, the underlying Array<Tri> is guaranteed to be in the same order as the parameter Tri Array.
        Zero area triangles are retained in the array, but will never be collided with. */
    void setContents(const Array<Tri>& triArray, const CPUVertexArray& vertexArray, const Settings& settings = Settings());
    
    /** Uses Surface::getTris to extract the triangles from each
        surface and then is identical to the other setContents,
        without the extra Tri copying.

        \param computePrevPosition If true, compute the
        CPUVertexArray::prevPosition array that is then used to
        compute Surfel::prevPosition.  This requires more space in
        memory but allows the reconstruction of motion vectors for
        post-processed motion blur.  Note that TriTree::intersect only
        traces against the current time, however.
    */
    void setContents
    (const Array<shared_ptr<Surface> >& surfaceArray,
     ImageStorage newStorage = COPY_TO_CPU, 
     const Settings& settings = Settings(),
     bool computePrevPosition = false);

    int size() const {
        return m_triArray.size();
    }

    /** Returns true if there was an intersection.

        Example:
        <pre>
           Tri::Intersector intersector;
           float distance = inf();
           if (tree.intersectRay(ray, intersector, distance)) {
               Vector3 position, normal;
               Vector2 texCoord;
               intersector.getResult(position, normal, texCoord);
               ...
           }
        </pre>

        \param exitOnAnyHit If true, return any intersection, not the first (faster for shadow rays)
        \param twoSided If true, both sides of triangles are tested for intersections. If a back face is hit,
        the normal will not automatically be flipped when passed to the Intersector (note that 
        the default implementation of Tri::Intersector::getResult will flip the normal; don't call that or 
        check backface if you need to know which side was hit.)
     */
    bool intersectRay
    (const Ray& ray,
     Tri::Intersector& intersectCallback, 
     float& distance,
     bool exitOnAnyHit = false,
     bool twoSided = false) const;

    /** Returns the surfel hit, or NULL if none */
    shared_ptr<Surfel> intersectRay
    (const Ray& ray,
     float& distance,
     bool exitOnAnyHit = false,
     bool twoSided = false) const;

    /** Returns all triangles that intersect or are contained within
        the sphere (technically, this is a ball intersection). */
    void intersectSphere
    (const Sphere& sphere,
     Array<Tri>&   triArray) const;

    /** Returns all triangles that intersect or are contained within
        the box. */
    void intersectBox
    (const AABox&  box,
     Array<Tri>&   triArray) const;

    /** Render the tree for debugging and visualization purposes. 
        Inefficent.

        \param level Show the nodes at or above this level of the tree, where 0 = root

        \param showBoxes Render bounding boxes for internal nodes at level == @a level

        \param minNodeSize Do not render triangles at nodes with fewer than this many triangles.
        Set to zero to disable, set to a high number to spot poorly-constructed nodes

    */
    void draw(RenderDevice* rd, int level, bool showBoxes = true, int minNodeSize = 0);


    /** \deprecated */
    const CPUVertexArray& getCPUVertexArray() const {
        return m_cpuVertexArray;
    }

    const CPUVertexArray& cpuVertexArray() const {
        return m_cpuVertexArray;
    }
};

}
#endif
