/**
  \file GLG3D/EmbreeTriTree.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2016-09-14
  \edited  2016-09-14

  G3D Innovation Engine
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
*/
#include "G3D/platform.h"

#ifdef G3D_WINDOWS
#include <xmmintrin.h> 
#include <pmmintrin.h> 
#include "GLG3D/EmbreeTriTree.h"
#include "G3D/Stopwatch.h"
#include "embree/rtcore.h"
#include "embree/rtcore_ray.h"
#include <atomic>

namespace G3D {

void EmbreeTriTree::apiConvert(const Ray& ray, RTCRay& rtcRay) {
    (Point3&)rtcRay.org = ray.origin();
    (Vector3&)rtcRay.dir = ray.direction();
    rtcRay.tnear  = ray.minDistance(); 
    rtcRay.tfar   = ray.maxDistance();
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID; 
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID; 
    rtcRay.mask   = 0xFFFFFFFF; 
    rtcRay.time   = 0.0f;
}


void EmbreeTriTree::apiConvert(const RTCRay& rtcRay, const Vector3& normal, int triIndex, TriTreeBase::Hit& hit) {
    hit.triIndex = triIndex;
    hit.u = rtcRay.u;
    hit.v = rtcRay.v;
    hit.backface = normal.dot((const Vector3&)rtcRay.Ng) > 0.0f;
    hit.distance = rtcRay.tfar;
}


EmbreeTriTree::EmbreeTriTree() : m_scene(nullptr), m_device(nullptr) {
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON); 
    m_device = rtcNewDevice(nullptr);
    alwaysAssertM(m_device, "Embree intialization failed");
}


EmbreeTriTree::~EmbreeTriTree() {
    clear();
    rtcDeleteDevice(m_device);
    m_device = nullptr;
}


void EmbreeTriTree::clear() {
    TriTreeBase::clear();
    m_alphaTriangleArray.clear();
    m_opaqueTriangleArray.clear();
    if (m_scene) {
        rtcDeleteScene(m_scene);     
        m_scene = nullptr;
    }
}


void EmbreeTriTree::rebuild() {
    m_alphaTriangleArray.clear();
    m_opaqueTriangleArray.clear();
    if (m_scene) {
        rtcDeleteScene(m_scene);     
        m_scene = nullptr;
    }

    if (m_vertexArray.size() == 0) {
        return;
    }

    // RTC_SCENE_HIGH_QUALITY had no impact on trace performance
    m_scene = rtcDeviceNewScene(m_device, RTC_SCENE_STATIC, RTCAlgorithmFlags(RTC_INTERSECT1 | RTC_INTERSECT_STREAM | RTC_SCENE_INCOHERENT));

    // Reserve the expected/worst case of all opaque to speed allocations
    // when growing the array (negligible impact, less than ~1%)
    m_opaqueTriangleArray.resize(m_triArray.size());
    m_opaqueTriangleArray.fastClear();

    // Separate out the triangles based on the presence of alpha. Running on multiple 
    // threads gives negligible gain.
    {
        tbb::spin_mutex alphaLock, opaqueLock;
        const Tri* triCArray = m_triArray.getCArray();
		tbb::parallel_for(tbb::blocked_range<size_t>(0, m_triArray.size(), 64), [&](const tbb::blocked_range<size_t>& r) {
			const size_t start = r.begin();
			const size_t end   = r.end();

            for (size_t t = start; t < end; ++t) {
                const Tri& tri = triCArray[t];
                const RTCTriangle rtcTri(tri.index[0], tri.index[1], tri.index[2], int(t));

                if (tri.hasPartialCoverage()) {
                    tbb::spin_mutex::scoped_lock lock(alphaLock);
                    m_alphaTriangleArray.append(rtcTri);
                } else {
                    tbb::spin_mutex::scoped_lock lock(opaqueLock);
                    m_opaqueTriangleArray.append(rtcTri);
                }
            }
        });
    }

    m_opaqueGeomID = rtcNewTriangleMesh(m_scene, RTC_GEOMETRY_STATIC, m_opaqueTriangleArray.size(), m_vertexArray.size(), 1);
    if (m_opaqueTriangleArray.size() > 0) {
        rtcSetBuffer(m_scene, m_opaqueGeomID, RTC_VERTEX_BUFFER, &m_vertexArray.vertex[0].position, 0, sizeof(CPUVertexArray::Vertex));
        rtcSetBuffer(m_scene, m_opaqueGeomID, RTC_INDEX_BUFFER, &m_opaqueTriangleArray[0].i0, 0, sizeof(RTCTriangle));
    }

    m_alphaGeomID  = rtcNewTriangleMesh(m_scene, RTC_GEOMETRY_STATIC, m_alphaTriangleArray.size(), m_vertexArray.size(), 1);
    if (m_alphaTriangleArray.size() > 0) {
        rtcSetBuffer(m_scene, m_alphaGeomID, RTC_VERTEX_BUFFER, &m_vertexArray.vertex[0].position, 0, sizeof(CPUVertexArray::Vertex));
        rtcSetBuffer(m_scene, m_alphaGeomID, RTC_INDEX_BUFFER, &m_alphaTriangleArray[0].i0, 0, sizeof(RTCTriangle));
    }

    // Register the generic filter function adapter
    rtcSetOcclusionFilterFunctionN(m_scene, m_alphaGeomID, FilterAdapter::rtcFilterFuncN);
    rtcSetIntersectionFilterFunctionN(m_scene, m_alphaGeomID, FilterAdapter::rtcFilterFuncN);

    // Register the back pointer to the tree
    rtcSetUserData(m_scene, m_opaqueGeomID, this);
    rtcSetUserData(m_scene, m_alphaGeomID,  this);

    rtcCommit(m_scene);
}

EmbreeTriTree::FilterAdapter::FilterAdapter(IntersectRayOptions options) : options(options) { }

void EmbreeTriTree::FilterAdapter::rtcFilterFuncN
    (int*                        valid, 
    void*                       userDataPtr, 
    const RTCIntersectContext*  context,
    RTCRayN*                    ray,
    const RTCHitN*              potentialHit,
    const size_t                N) {

    const FilterAdapter&    adapter     = *(FilterAdapter*)context->userRayExt;
    const EmbreeTriTree&    tree        = *(EmbreeTriTree*)userDataPtr;
    const CPUVertexArray&   vertexArray = tree.m_vertexArray;

    for (int r = 0; r < N; ++r) {
        // Ignore already-masked out rays
        if (valid[r] != -1) { continue; }

        // Non-unit normal of the ray hit...which may be a backface!
        const Vector3   normal     (-RTCHitN_Ng_x(potentialHit, N, r), -RTCHitN_Ng_y(potentialHit, N, r), -RTCHitN_Ng_z(potentialHit, N, r));
        const Vector3   direction  (RTCRayN_dir_x(ray, N, r), RTCRayN_dir_y(ray, N, r), RTCRayN_dir_z(ray, N, r));
        const int       primID     = RTCHitN_primID(potentialHit, N, r);
#               ifdef G3D_DEBUG
        {
            const int   geomID     = RTCHitN_geomID(potentialHit, N, r);
            debugAssert(geomID == tree.m_alphaGeomID);
        }
#               endif

        const Tri&      tri        = tree.m_triArray[tree.m_alphaTriangleArray[primID].triIndex];
                
        if (((adapter.options & TriTreeBase::DO_NOT_CULL_BACKFACES) == 0) && (! tri.twoSided()) && (normal.dot(direction) >= 0.0f)) {
            // Failed the backface test. Exit now to avoid the cost of alpha test
            valid[r] = 0;
        } else {

            const float     u      = RTCHitN_u(potentialHit, N, r);
            const float     v      = RTCHitN_v(potentialHit, N, r);
            const Point3    origin (RTCRayN_org_x(ray, N, r), RTCRayN_org_y(ray, N, r), RTCRayN_org_z(ray, N, r));

            const bool alphaTest = ((adapter.options & TriTreeBase::NO_PARTIAL_COVERAGE_TEST) == 0);
            const float alphaThreshold = ((adapter.options & TriTreeBase::PARTIAL_COVERAGE_THRESHOLD_ZERO) != 0) ? 1.0f : 0.5f;

            // Alpha test
            if (alphaTest && ! tri.intersectionAlphaTest(vertexArray, u, v, alphaThreshold)) {
                // Failed the alpha test
                valid[r] = 0;
            } else {
                RTCRayN_geomID(ray, N, r) = RTCHitN_geomID(potentialHit, N, r);

                if ((adapter.options & OCCLUSION_TEST_ONLY) == 0) {
                    RTCRayN_primID(ray, N, r) = primID;
                    RTCRayN_u(ray, N, r)      = u;
                    RTCRayN_v(ray, N, r)      = v;
                    RTCRayN_tfar(ray, N, r)   = RTCHitN_t(potentialHit, N, r);
                    RTCRayN_instID(ray, N, r) = RTCHitN_instID(potentialHit, N, r);
                    RTCRayN_Ng_x(ray, N, r)   = normal.x;
                    RTCRayN_Ng_y(ray, N, r)   = normal.y;
                    RTCRayN_Ng_z(ray, N, r)   = normal.z;
                }
            }
        } // if
    } // for
} // rtcFilterFuncN


bool EmbreeTriTree::intersectRay
   (const Ray&                         ray,
    Hit&                               hit,
    IntersectRayOptions                options) const {
     
    // Because the streaming API is the only way to pass a context to the filter function, 
    // we must use rtcIntersect1M here with M = 1, even though rtcIntersect would otherwise
    // be a more natural fit for a single ray cast.
        
    const bool occlusionOnly      = (options & OCCLUSION_TEST_ONLY) != 0;

    // Set up the filter (e.g., alpha test) function        
    FilterAdapter adapter(options);
        
    RTCIntersectContext context;
    context.flags = RTCIntersectFlags(0);
    context.userRayExt = (void*)&adapter;
                
    RTCRay rtcRay;
    apiConvert(ray, rtcRay);

    (occlusionOnly ? rtcOccluded1M : rtcIntersect1M)(m_scene, &context, &rtcRay, 1, sizeof(RTCRay));

#       ifdef G3D_DEBUG
    {
        const RTCError error = rtcDeviceGetError(m_device);
        debugAssert(error == RTC_NO_ERROR);
    }
#       endif

    if (rtcRay.geomID != RTC_INVALID_GEOMETRY_ID) {
        int triIndex = 0;
            
        if (! occlusionOnly) {
            // Identify the source triangle
            triIndex = ((rtcRay.geomID == m_alphaGeomID) ? m_alphaTriangleArray : m_opaqueTriangleArray)[rtcRay.primID].triIndex;
        }

        apiConvert(rtcRay, occlusionOnly ? Vector3::zero() : m_triArray[triIndex].nonUnitNormal(m_vertexArray), triIndex, hit);
        return true;
    } else {
        return false;
    }
}

    
void EmbreeTriTree::intersectRays
   (const Array<Ray>&                  rays,
    Array<Hit>&                        results,
    IntersectRayOptions                options) const {

    Stopwatch conversionTimer;
    conversionTimer.tick();
    results.resize(rays.size());    

    const bool occlusionOnly = (options & OCCLUSION_TEST_ONLY) != 0;
  
    FilterAdapter adapter(options);
    const int numBatches = System::numCores();

    RTCIntersectContext context;
    context.flags = RTCIntersectFlags(0);
    if ((options & COHERENT_RAY_HINT) != 0) {
        context.flags = RTCIntersectFlags(context.flags | RTC_INTERSECT_COHERENT);
    } else {
        context.flags = RTCIntersectFlags(context.flags | RTC_INTERSECT_INCOHERENT);
    }
    context.userRayExt = (void*)&adapter;
    conversionTimer.tock();
    debugConversionOverheadTime = conversionTimer.elapsedTime();

    // Using raw pointers instead of C++ arrays gave no performance increase for the code below

    const size_t minStepSize = 32;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, rays.size(), minStepSize), [&](const tbb::blocked_range<size_t>& r) {
		const size_t start = r.begin();
		const size_t end   = r.end();

        // Making this array "static thread_local" didn't have any performance impact---G3D's allocator
        // is quite efficient.
        Array<RTCRay> rtcRays;
		rtcRays.resize(end - start);

		for (size_t r = start; r < end; ++r) {
			apiConvert(rays[r], rtcRays[r - start]);
		}

        (occlusionOnly ? rtcOccluded1M : rtcIntersect1M)(m_scene, &context, rtcRays.getCArray(), rtcRays.size(), sizeof(RTCRay));

#           ifdef G3D_DEBUG
		{
			const RTCError error = rtcDeviceGetError(m_device);
			debugAssert(error == RTC_NO_ERROR);
		}
#           endif

		for (size_t r = start; r < end; ++r) {
			RTCRay& rtcRay = rtcRays[r - start];

            if (rtcRay.geomID != RTC_INVALID_GEOMETRY_ID) {
                // Identify the source triangle
                int triIndex = 0;            
                if (! occlusionOnly) {
                    triIndex = ((rtcRay.geomID == m_alphaGeomID) ? m_alphaTriangleArray : m_opaqueTriangleArray)[rtcRay.primID].triIndex;
                }
                apiConvert(rtcRay, occlusionOnly ? Vector3::zero() : m_triArray[triIndex].nonUnitNormal(m_vertexArray), triIndex, results[r]);
			} else {
				results[r].triIndex = Hit::NONE;
			}
		} // for
	}); // parallel for
}

} // G3D

#endif
