/**
  \file GLG3D/TriTree.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2009-06-10
  \edited  2012-09-02
*/

#include "G3D/AreaMemoryManager.h"
#include "GLG3D/TriTree.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"
#include "GLG3D/Surface.h"
#include "GLG3D/UniversalSurfel.h"

namespace G3D {


const char* TriTree::algorithmName(SplitAlgorithm s) {
    const char* n[] = {"Mean extent", "Median area", "Median count", "SAH"};
    return n[s];
}


void TriTree::intersectSphere
(const Sphere& sphere,
 Array<Tri>&   triArray) const {

    if (m_root) {
        Set<Tri*> alreadyAdded;
        m_root->intersectSphere(sphere, m_cpuVertexArray, triArray, alreadyAdded);
    }

}


void TriTree::intersectBox
(const AABox&  box,
 Array<Tri>&   triArray) const {
    if (m_root) {
        Set<Tri*> alreadyAdded;
        m_root->intersectBox(box, m_cpuVertexArray, triArray, alreadyAdded);
    }

}



void TriTree::setContents
(const Array<shared_ptr<Surface> >& surfaceArray, 
 ImageStorage                       newStorage, 
 const Settings&                    settings,
 bool                               computePrevPosition) {
    static const float epsilon = 0.000001f;
    clear();
    Surface::getTris(surfaceArray, m_cpuVertexArray, m_triArray, computePrevPosition);
   
    if (newStorage != IMAGE_STORAGE_CURRENT) {
        for (int i = 0; i < m_triArray.size(); ++i) {
            const Tri& tri = m_triArray[i];
            shared_ptr<Material> material = tri.material();
            material->setStorage(newStorage);
        }
    }

    Array<Poly> source;
    // Don't add 0 area triangles to source
    for (int i = 0; i < m_triArray.size(); ++i) {
        if (m_triArray[i].area() > epsilon) {
            source.append(Poly(m_cpuVertexArray, &m_triArray[i]));
        }
    }
    
    if (source.size() > 0) {
        m_memoryManager = AreaMemoryManager::create();
        m_root = new (m_memoryManager->alloc(sizeof(Node))) Node(source, settings, m_memoryManager);
    }

    //alwaysAssertM(m_triArray.size() == m_triArray.capacity(), "Allocated too much memory for the Tri Array");
    alwaysAssertM(m_cpuVertexArray.vertex.size() == m_cpuVertexArray.vertex.capacity(), 
                  "Allocated too much memory for the vertex array");
}

/** Returns true if \a ray hits \a box.

   \param maxTime The routine <i>may</i> return false if an intersection exists but lies after maxTime*/
inline static bool __fastcall intersect(const Ray& ray, const AABox& box, float maxTime) {
    //  Enabling the more exact test actually hurts performance on test scenes
    //    float t;
    //    return Intersect::rayAABox(ray, box, t) && (t < maxTime);
    return Intersect::rayAABox(ray, box); 
}
    
void TriTree::Node::setValueArray(const Array<Poly>& src, const shared_ptr<MemoryManager>& mm) {
    if (src.size() == 0) {
        return;
    }
    
    Vector3 lo = src[0].low();
    Vector3 hi = src[0].high();

    valueArray = reinterpret_cast<ValueArray*>(mm->alloc(sizeof(ValueArray)));

    valueArray->size = src.size();
    valueArray->data = reinterpret_cast<const Tri**>(mm->alloc(sizeof(const Tri*) * valueArray->size));
    for (int i = 0; i < valueArray->size; ++i) {
        const Poly& t = src[i];
        
        debugAssert(t.area() > 0.0f);
        
        valueArray->data[i] = t.source();
        debugAssert(isValidHeapPointer(valueArray->data[i]));

#       ifdef G3D_DEBUG
            // Try dereferencing the tri to verify that it is legal
            (void)valueArray->data[i]->getIndex(0); 
#       endif

        // Update bounds on the value array
        lo = lo.min(t.low());
        hi = hi.max(t.high());
    }
    
    valueArray->bounds = AABox(lo, hi);
}

        
bool TriTree::Node::badSplit(int numOriginalSources, int numLow, int numHigh) {
    debugAssert(numHigh <= numOriginalSources);
    debugAssert(numLow <= numOriginalSources);
    return
        (numLow == 0) || (numHigh == 0) ||
        (numLow == numOriginalSources) || (numHigh == numOriginalSources) ||
        (numLow + numHigh > numOriginalSources * 1.8f);
}


void TriTree::Node::split(Array<Poly>& original, const Settings& settings, const shared_ptr<MemoryManager>& mm) {
    // Order in which we'd like to split along axes
    Vector3::Axis preferredAxis[3];
    const Vector3& extent = bounds.extent();
    preferredAxis[0] = extent.primaryAxis();
    preferredAxis[1] = Vector3::Axis((preferredAxis[0] + 1) % 3);
    preferredAxis[2] = Vector3::Axis((preferredAxis[1] + 1) % 3);
    
    // Make the preference order the extent ranking order
    if (extent[preferredAxis[2]] > extent[preferredAxis[1]]) {
        Vector3::Axis temp = preferredAxis[2];
        preferredAxis[2] = preferredAxis[1];
        preferredAxis[1] = temp;
    }
    
    Array<Poly> lowArray, highArray, spanArray;
    for (int i = 0; i < 3; ++i) {
        lowArray.fastClear(); highArray.fastClear(); spanArray.fastClear();
        
        Vector3::Axis axis = preferredAxis[i];
        splitLocation = chooseSplitLocation(original, settings, axis);
        
        // Once an underlying triangle's underlying area from
        // all of the original triangles exceeds that of (on
        // average) one face of the bounding box, just insert
        // the triangle because otherwise it is being
        // multiplied at every split.
        const float maxArea = bounds.area() * settings.maxAreaFraction;
        for (int j = 0; j < original.size(); ++j) {
            original[j].split(axis, splitLocation, maxArea, lowArray, highArray, spanArray);
        }
        
        if (badSplit(original.size(), lowArray.size(), highArray.size())) {
            if (i == 2) {
                // No split effectively reduced the number
                // of triangles, so make this node a leaf
                setValueArray(original, mm);
            }
        } else {
            // This was a good split
            setValueArray(spanArray, mm);
            
            // Create child nodes
            Node* ptr = (Node*) mm->alloc(sizeof(Node) * 2);
            
            alwaysAssertM((uintptr_t(ptr) & 3) == 0, 
                          format("Pointer is not a multiple of four bytes: %d", (int)(intptr_t)ptr));
            packedChildAxis = reinterpret_cast<uintptr_t>(ptr) | static_cast<uintptr_t>(axis);

            new (ptr) Node(lowArray, settings, mm);
            new (ptr + 1) Node(highArray, settings, mm);
            return;
        }
    }
}


void TriTree::Node::destroy(const shared_ptr<MemoryManager>& mm) {
    // Destroy children
    if (! isLeaf()) {
        for (int i = 0; i < 2; ++i) {
            child(i).destroy(mm);
        }

        mm->free(& child(0));
        packedChildAxis = 0;
    }

    // Destroy value array
    if (valueArray) {
        mm->free(valueArray);
        valueArray = NULL;
    }
}


float TriTree::Node::chooseSplitLocation(Array<Poly>& source, const Settings& settings, Vector3::Axis axis) {
    switch (settings.algorithm) {
    case MEAN_EXTENT:
        return bounds.center()[axis];
        
    case MEDIAN_AREA:
        return chooseMedianAreaSplitLocation(source, axis);
        
    case MEDIAN_COUNT:
        source.sort(HighComparator(axis));
        return source[(source.size() - 1) / 2].high()[axis];
        
    case SAH:
        return chooseSAHSplitLocation(source, axis, settings);
        
    default:
        alwaysAssertM(false, "Fell through switch");
        return 0.0f;
    }
}


float TriTree::Node::chooseMedianAreaSplitLocation(Array<Poly>& original, Vector3::Axis axis) {
    original.sort( HighComparator(axis) );
    
    // Total area of all originals
    float area = 0.0f;
    for (int i = 0; i < original.size(); ++i) {
        area += original[i].area();
    }
    debugAssert(area > 0);
    
    // Find the half-area point.  We need a little epsilon in
    // the comparison because of incremental floating point
    // error when accumulating areas.
    area /= 2.0f;
    const float epsilon = 0.0001f;
    for (int i = 0; i < original.size(); ++i) {
        area -= original[i].area();
        if (area <= epsilon) {
            return original[i].high()[axis];
        }
    }
    debugAssertM(false, "Could not find half-way point");
    return 0.0f;
}


float TriTree::Node::chooseSAHSplitLocation(Array<Poly>& source, Vector3::Axis axis, const Settings& settings) {
    if (source.size() <= settings.accurateSAHCountThreshold) {
        return chooseSAHSplitLocationAccurate(source, axis, settings);
    } else {
        return chooseSAHSplitLocationFast(source, axis, settings);
    }
}


float TriTree::Node::chooseSAHSplitLocationFast(Array<Poly>& source, Vector3::Axis axis, const Settings& settings) {
    (void)settings;
    source.sort(HighComparator(axis));

    // Find the unique splitting candidates in order; only consider high bounds
    Array<float> splitPosition;

    {
        splitPosition.append(source[0].high()[axis]);
        float c = splitPosition.last();
        for (int i = 1; i < source.size(); ++i) {
            const float h = source[i].high()[axis];
            if (h > c) {
                c = h;
                splitPosition.append(h);
            }
        }
        // Remove the last; it is the high end of the entire set and
        // is not eligible as a splitting position.
        splitPosition.popDiscard();
    }

    // Find the one-sided cost of each position by a sweep.
    const int S = splitPosition.size();
    Array<float> highCost;
    highCost.resize(S);
    const float containingArea = bounds.area();

    // Sweep from above for the high-side cost
    {
        int i = source.size() - 1;
        Vector3 low  = source[i].low();
        Vector3 high = source[i].high();
        --i;
        // Iterate over splitting planes
        for (int s = S - 1; s >= 0; --s) {
            const float h = splitPosition[s];
            while (source[i].high()[axis] > h) {
                low  = low.min(source[i].low());
                high = high.max(source[i].high());
                --i;
            }
            highCost[s] = (source.size() - i) * AABox(low, high).area() / containingArea;
        }
    }

    // Must put at least this many triangles on each side to consider a split
    const int minTrisPerSide = source.size() / 5;

    // Sweep from below for the low-side cost, tracking the best as we go
    float lowestCost         = (float)inf();
    float lowestCostPosition = (float)inf();
    {
        int i = 0;
        Vector3 low  = source[i].low();
        Vector3 high = source[i].high();
        ++i;
        // Iterate over splitting planes
        for (int s = 0; s < S; ++s) {
            const float h = splitPosition[s];
            while (source[i].high()[axis] <= h) {
                low  = low.min(source[i].low());
                high = high.max(source[i].high());
                ++i;
            }
            const float lowCost = i * AABox(low, high).area() / containingArea;

            // If there's a index big range within which all costs are
            // all similar, then prefer splits closer to the median to
            // keep the tree balanced.  This helps avoid splits that
            // chop very few points off the end as well
            const float bias = 0.1f * square(i - source.size() * 0.5f);

            const float avoidSmall = 
                ((i < minTrisPerSide) || (source.size() - i < minTrisPerSide)) ? 100.0f : 0.0f;

            const float cost = lowCost + highCost[s] + bias + avoidSmall;
            if (cost < lowestCost) {
                lowestCost = cost;
                lowestCostPosition = h;
            }
        }
    }

    return lowestCostPosition;
}


float TriTree::Node::chooseSAHSplitLocationAccurate(Array<Poly>& source, Vector3::Axis axis, const Settings& settings) {
    // Get the unique potential split locations
    
    Set<float, FloatHashTrait> positionSet;
    positionSet.clearAndSetMemoryManager(AreaMemoryManager::create(sizeof(float) * 2 * source.size() + 200));
    for (int i = 0; i < source.size(); ++i) {
        positionSet.insert(source[i].low()[axis]);
        positionSet.insert(source[i].high()[axis]);
    }
    
    Array<float> position;
    positionSet.getMembers(position);
    positionSet.clear();
    
    int lowestCostIndex = 0;
    float lowestCost = (float)inf();
    //debugPrintf("\nChoosing split:\n");
    for (int i = 0; i < position.size(); ++i) {
        float cost = SAHCost(axis, position[i], source, bounds.area(), settings);
        //debugPrintf("  pos = %f, cost = %f\n", position[i], cost);
        if (cost < lowestCost) {
            lowestCost = cost;
            lowestCostIndex = i;
        }
    }
    
    return position[lowestCostIndex];
}


float TriTree::Node::SAHCost(int size, float area, float containingArea) {
    static const float boxIntersectTime = 5;
    static const float triIntersectTime = 1;
    
    if (size == 0) {
        return 0;
    } else {
        return triIntersectTime * size * area / containingArea + boxIntersectTime;
    }
}


float TriTree::Node::SAHCost(Vector3::Axis axis, float offset, const Array<Poly>& original, float containingArea, const Settings& settings) {
    // TODO: pass these arrays in for thread-safing
    static Array<Poly> lowArray, highArray, spanArray;
    
    lowArray.fastClear();
    highArray.fastClear();
    spanArray.fastClear();
    for (int j = 0; j < original.size(); ++j) {
        original[j].split(axis, offset, settings.maxAreaFraction * containingArea, lowArray, highArray, spanArray);
    }
    
    const float L = SAHCost(lowArray.size(),  Poly::computeBounds(lowArray).area(),  containingArea);
    const float S = SAHCost(spanArray.size(), Poly::computeBounds(spanArray).area(), containingArea);
    const float H = SAHCost(highArray.size(), Poly::computeBounds(highArray).area(), containingArea);
    //debugPrintf("    L = %f, S = %f, H = %f\n", L, S, H);
    return L + S + H;
}


bool __fastcall TriTree::Node::intersectRay
(const TriTree&      triTree,
 const Ray&          ray,
 Tri::Intersector&   intersectCallback, 
 float&              distance,
 bool                exitOnAnyHit,
 bool                twoSided) const {

    bool hit = false;
    
    // Don't bother paying the bounding box intersection at
    // leaves, since we have to pay it again below.
    if (! isLeaf() && ! intersect(ray, bounds, distance)) {
        // The ray doesn't hit this node, so it can't hit the
        // children of the node either--stop searching.
        return false;
    }
    
    enum {NONE = -1};
    
    Vector3::Axis axis = splitAxis();

    int firstChild = NONE, secondChild = NONE;
    if (! isLeaf()) {
        computeTraversalOrder(ray, firstChild, secondChild);
    }
    
    // Test on the side closer to the ray origin.
    if (firstChild != NONE) {
        hit = child(firstChild).intersectRay(triTree, ray, intersectCallback, distance, exitOnAnyHit, twoSided) || hit;
        if (exitOnAnyHit && hit) {
            return true;
        }
    }
    
    // Test the contents of the node. If the value array is
    // really small, don't waste time on the bounds
    // intersection, just run the ray-triangle intersection.
    if (valueArray &&
        (valueArray->size > 0) && 
        intersect(ray, valueArray->bounds, distance)) {

        // Test for intersection against every object at this node.
        for (int v = 0; v < valueArray->size; ++v) { 
            bool justHit = intersectCallback(ray, triTree.m_cpuVertexArray, *(valueArray->data[v]), twoSided, distance);
            
            if (justHit) {
                hit = true;
                // Pointer arithmetic to find what index in the tri tree array this triangle was.
                // The data array is a set of pointers into the triArray.
                intersectCallback.primitiveIndex = int(valueArray->data[v] - triTree.m_triArray.getCArray());
                intersectCallback.cpuVertexArray = &triTree.m_cpuVertexArray;
                if (exitOnAnyHit) {
                    return true;
                }
            }
            
            
        }        
    }
    
    // Test on the side farther from the ray origin.
    if (secondChild != NONE) {
        
        if (ray.direction()[axis] != 0.0f) {
            // See if there was an intersection before hitting the splitting plane.  
            // If so, there is no need to look on the far side and recursion terminates. 
            // This test makes about a factor of two improvement in performance.
            const float distanceToSplittingPlane =
                (splitLocation - ray.origin()[axis]) * ray.invDirection()[axis];

            if (distanceToSplittingPlane > distance) {
                // We aren't going to hit anything else before hitting the splitting plane,
                // so don't bother looking on the far side of the splitting plane at the other
                // child.
                return hit;
            }
        }
        
        hit = child(secondChild).intersectRay(triTree, ray, intersectCallback, distance, exitOnAnyHit, twoSided) || hit;
    }

    return hit;
}


TriTree::Node::Node(Array<Poly>& originals, const Settings& settings, const shared_ptr<MemoryManager>& mm) : 
    bounds(Poly::computeBounds(originals)), 
    splitLocation(0),
    packedChildAxis(0),
    valueArray(NULL) {
    
    debugAssert(originals.size() > 0);
    
    if (originals.size() <= settings.valuesPerLeaf) {
        setValueArray(originals, mm);
        return;
    }
    
    split(originals, settings, mm);
    
    debugAssert((valueArray == NULL) ||
                bounds.contains(valueArray->bounds));
}


#if 0
static void draw(RenderDevice* rd, const AABox& m_box, const Color4& color) {
    // Shrink boxes slightly to avoid z-fighting
    static const Vector3 epsilon = Vector3::one() * 0.0001f;
    const Vector3& A = m_box.low() + epsilon;
    const Vector3& B = m_box.high() - epsilon;
    Draw::box(AABox(A.min(B), B.max(A)), rd, color, Color3::black());
}
#endif


void TriTree::Node::intersectSphere(const Sphere& sphere, const CPUVertexArray& vertexArray, Array<Tri>& triArray, Set<Tri*>& alreadyAdded) const {
    if (! bounds.intersects(sphere)) {
        return;
    }

    // Add the triangles at this node
    if (valueArray && valueArray->bounds.intersects(sphere)) {
        for (int v = 0; v < valueArray->size; ++v) {
            Tri* tri = const_cast<Tri*>(valueArray->data[v]);
            if (! alreadyAdded.contains(tri)) {
                if ((tri->area() > 0) && CollisionDetection::fixedSolidSphereIntersectsFixedTriangle(sphere, Triangle(tri->position(vertexArray, 0), 
                                                                                       tri->position(vertexArray, 1), tri->position(vertexArray, 2)))) {
                    triArray.append(*tri);
                    alreadyAdded.insert(tri);
                }
            }
        }
    }

    // Recurse into children
    if (! isLeaf()) {
        for (int c = 0; c < 2; ++c) {
            child(c).intersectSphere(sphere, vertexArray, triArray, alreadyAdded);
        }
    }
}


void TriTree::Node::intersectBox(const AABox& box,  const CPUVertexArray& vertexArray, Array<Tri>& triArray, Set<Tri*>& alreadyAdded) const {
    if (! bounds.intersects(box)) {
        return;
    }

    // Add the triangles at this node
    if (valueArray && valueArray->bounds.intersects(box)) {
        for (int v = 0; v < valueArray->size; ++v) {
            Tri* tri = const_cast<Tri*>(valueArray->data[v]);
            if (! alreadyAdded.contains(tri)) {
                if ((tri->area() > 0) && CollisionDetection::fixedSolidBoxIntersectsFixedTriangle(box, Triangle(tri->position(vertexArray, 0), 
                                                                                 tri->position(vertexArray, 1), tri->position(vertexArray, 2)))) {
                    triArray.append(*tri);
                    alreadyAdded.insert(tri);
                }
            }
        }
    }

    // Recurse into children
    if (! isLeaf()) {
        for (int c = 0; c < 2; ++c) {
            child(c).intersectBox(box, vertexArray, triArray, alreadyAdded);
        }
    }
}


void TriTree::Node::draw(RenderDevice* rd, const CPUVertexArray& vertexArray, int level, bool showBoxes, int minNodeSize) const {
	/*
    if (valueArray && (valueArray->size > minNodeSize)) {
        static const Vector3 epsilon = Vector3::one() * 0.001f;
        // Grow clip-bounds slightly to show objects right on the edge
        glClipToBox(AABox(bounds.low() - epsilon, bounds.high() + epsilon));
        
        rd->setColor(chooseColor(this));
        for (int p = 0; p < 2; ++p) {
            rd->beginPrimitive(PrimitiveType::TRIANGLES);
            for (int t = 0; t < valueArray->size; ++t) {
                rd->setNormal(valueArray->data[t]->normal(vertexArray));
                for (int v = 0; v < 3; ++v) {
                    rd->sendVertex(valueArray->data[t]->position(vertexArray, v));
                }
            }
            rd->endPrimitive();
            
            rd->setColor(Color3::black());
            rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
            rd->setPolygonOffset(-0.1f);
        }
        rd->setRenderMode(RenderDevice::RENDER_SOLID);
        rd->setPolygonOffset(0);
        
        glDisableAllClipping();
    }
    
    // We need to show that there are further children at this terminal draw level
    if ((level == 0) && showBoxes) {
        if (valueArray == NULL) {
            G3D::draw(rd, bounds, chooseColor(this));
        } else if (! isLeaf()) {
            // Don't cover up the triangles inside
            G3D::draw(rd, bounds, Color4(chooseColor(this), 0.5f));
        }
    }
    
    // Recurse if not at the terminal level
    if ((level > 0) && ! isLeaf()) {
        for (int c = 0; c < 2; ++c) {
            child(c).draw(rd, vertexArray, level - 1, showBoxes, minNodeSize);
        }
    }
	*/
}


void TriTree::Node::print(const String& indent) const {
    debugPrintf("%sbounds = [%s, %s]", indent.c_str(), 
                bounds.low().toString().c_str(), bounds.high().toString().c_str());
    if (valueArray) {
        debugPrintf(" N = %d\n", valueArray->size);
    }
    
    if (! isLeaf()) {
        debugPrintf("\n");
        for (int i = 0; i < 2; ++i) {
            child(i).print(indent + " ");
        }
    }
}


void TriTree::Node::getStats(Stats& s, int level, int valuesPerNode) const {    
    int n = (valueArray) ? valueArray->size : 0;
    s.numTris += n;
    ++s.numNodes;
    s.depth = max(s.depth, level);
    s.largestNode = max(s.largestNode, n);
    
    if (valueArray && (valueArray->size > valuesPerNode)) {
        s.shallowestNodeOverMin = min(s.shallowestNodeOverMin, level);
    }
    
    if (isLeaf()) {
        ++s.numLeaves;
        s.averageValuesPerLeaf += n;
        s.shallowestLeaf = min(s.shallowestLeaf, level);
    } else {
        for (int c = 0; c < 2; ++c) {
            child(c).getStats(s, level + 1, valuesPerNode);
        }
    }            
}


TriTree::TriTree() : m_root(NULL) {}


TriTree::~TriTree() {
    clear();
}


/** Walk the entire tree, computing statistics */
TriTree::Stats TriTree::stats(int valuesPerNode) const {
    Stats s;
    if (m_root) {
        m_root->getStats(s, 0, valuesPerNode);
        s.averageValuesPerLeaf /= s.numLeaves;
    } else {
        s.shallowestLeaf = 0;
        s.shallowestNodeOverMin = 0;
    }
    return s;
}


void TriTree::clear() {
    if (m_root) {
        m_root->destroy(m_memoryManager);
        m_memoryManager->free(m_root);
        m_root = NULL;
        m_triArray.fastClear();
        m_cpuVertexArray.clear();
        m_memoryManager.reset();
    }
}


void TriTree::setContents(const Array<Tri>& triArray, const CPUVertexArray& vertexArray, const Settings& settings) {
    static const float epsilon = 0.000001f;
    clear();
    
    Array<Poly> source;
    // Copy the source data 

    // Copy the vertex array
    m_cpuVertexArray.copyFrom(vertexArray);
    
    // Copy the tri array
    m_triArray.copyFrom(triArray);

    
    for (int i = 0; i < triArray.size(); ++i) {
        if (triArray[i].area() > epsilon) {
            source.append(Poly(m_cpuVertexArray, &m_triArray[i]));
        }
    }
    
    if (source.size() > 0) {
        m_memoryManager = AreaMemoryManager::create();
        m_root = new (m_memoryManager->alloc(sizeof(Node))) Node(source, settings, m_memoryManager);
    }

    alwaysAssertM(m_triArray.size() == m_triArray.capacity(), "Allocated too much memory for the Tri Array");
    alwaysAssertM(m_cpuVertexArray.vertex.size() == m_cpuVertexArray.vertex.capacity(), "Allocated too much memory for the vertex array");
}


void TriTree::draw(RenderDevice* rd, int level, bool showBoxes, int minNodeSize) {
    if (m_root) {
        rd->setCullFace(CullFace::NONE);
        m_root->draw(rd, m_cpuVertexArray, level, showBoxes, minNodeSize);
    }
}


shared_ptr<Surfel> TriTree::intersectRay
(const Ray&            ray,
 float&                distance,
 bool                  exitOnAnyHit,
 bool                  twoSided) const {

    Tri::Intersector intersector;
    if (intersectRay(ray, intersector, distance, exitOnAnyHit, twoSided)) {
        return intersector.tri->material()->sample(intersector);
    } else {
        return shared_ptr<Surfel>();
    }
}


bool TriTree::intersectRay
(const Ray&            ray,
 Tri::Intersector&     intersectCallback, 
 float&                distance,
 bool                  exitOnAnyHit,
 bool                  twoSided) const {

    bool hit = false;
    if (m_root != NULL) {
        hit = m_root->intersectRay(*this, ray, intersectCallback, distance, exitOnAnyHit, twoSided);
    }
    return hit;
}

}
