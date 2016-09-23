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
    // G3D::Ray is a strict subset of rtcRay in terms of memory
    // layout because rtcRay has alignment padding, so directly copy
    (Ray&)rtcRay = ray;
    rtcRay.tnear  = ray.minDistance(); 
    rtcRay.tfar   = ray.maxDistance();
    // rtcRay.instID = RTC_INVALID_GEOMETRY_ID;  // uncomment if instances are required
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID; 
    // rtcRay.primID = RTC_INVALID_GEOMETRY_ID;  // 
    // rtcRay.mask   = 0xFFFFFFFF;               // uncomment if we need ray masking
    // rtcRay.time   = 0.0f;                     // uncomment if we need motion blur
}


void EmbreeTriTree::apiConvert(const RTCRay& rtcRay, int triIndex, TriTreeBase::Hit& hit) {
    hit.triIndex = triIndex;
    hit.u = rtcRay.u;
    hit.v = rtcRay.v;
    hit.backface = (rtcRay.Ng[0] * rtcRay.dir[0]) + (rtcRay.Ng[1] * rtcRay.dir[1]) + (rtcRay.Ng[2] * rtcRay.dir[2]) < 0.0f;      
    hit.distance = rtcRay.tfar;
}


void EmbreeTriTree::apiConvertOcclusion(const RTCRay& rtcRay, TriTreeBase::Hit& hit) {
    hit.triIndex = 0;
    hit.u = 0;
    hit.v = 0;
    hit.backface = false;        
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
    m_alphaTriangleArray.fastClear();
    m_opaqueTriangleArray.fastClear();
    if (m_scene) {
        rtcDeleteScene(m_scene);     
        m_scene = nullptr;
    }

    if (m_vertexArray.size() == 0) {
        return;
    }

    // RTC_SCENE_HIGH_QUALITY had no impact on trace performance
    m_scene = rtcDeviceNewScene(m_device, RTC_SCENE_STATIC, RTCAlgorithmFlags(RTC_INTERSECT1 | RTC_INTERSECT_STREAM | RTC_SCENE_INCOHERENT));

	Stopwatch timer;
	timer.tick();

    // Separate out the triangles based on the presence of alpha. Running on multiple 
    // threads actually gives negligible gain for Sponza-size scenes.

    // Reserve the expected/worst case of all opaque to speed allocations
    // and allow atomic append.
    m_alphaTriangleArray.resize(m_triArray.size());
    m_opaqueTriangleArray.resize(m_triArray.size());
	{
		std::atomic<int> alphaCount = 0;
		std::atomic<int> opaqueCount = 0;

		const Tri* triCArray = m_triArray.getCArray();
		tbb::parallel_for(tbb::blocked_range<size_t>(0, m_triArray.size(), 64), [&](const tbb::blocked_range<size_t>& r) {
			const size_t start = r.begin();
			const size_t end = r.end();
			static const size_t NUM_LOCAL_PER_THREAD_TRIS = 64;

			size_t numAlpha = 0;
			size_t numOpaque = 0;
			RTCTriangle local_alpha[NUM_LOCAL_PER_THREAD_TRIS];
			RTCTriangle local_opaque[NUM_LOCAL_PER_THREAD_TRIS];

			for (size_t t = start; t < end; ++t) {
				const Tri& tri = triCArray[t];
				// sort triangle into either the alpha or opaque queue
				if (tri.hasPartialCoverage()) {
					local_alpha[numAlpha++] = RTCTriangle(tri.index[0], tri.index[1], tri.index[2], int(t));
                } else {
					local_opaque[numOpaque++] = RTCTriangle(tri.index[0], tri.index[1], tri.index[2], int(t));
                }

				// flush local 'alpha' queue if needed
				if (numAlpha == NUM_LOCAL_PER_THREAD_TRIS) {
					const size_t index = (size_t)alphaCount.fetch_add((int)numAlpha);
					memcpy(&m_alphaTriangleArray[index], local_alpha, sizeof(RTCTriangle)*numAlpha);
					numAlpha = 0;
				}

				// flush local 'alpha' queue if needed
				if (numOpaque == NUM_LOCAL_PER_THREAD_TRIS) {
					const size_t index = (size_t)opaqueCount.fetch_add((int)numOpaque);
					memcpy(&m_opaqueTriangleArray[index], local_opaque, sizeof(RTCTriangle)*numOpaque);
					numOpaque = 0;
				}
			}

			// flush all non-empty queues
			if (numAlpha > 0) {
				const size_t index = (size_t)alphaCount.fetch_add((int)numAlpha);
				memcpy(&m_alphaTriangleArray[index], local_alpha, sizeof(RTCTriangle)*numAlpha);
				numAlpha = 0;
			}

			if (numOpaque > 0) {
				const size_t index = (size_t)opaqueCount.fetch_add((int)numOpaque);
				memcpy(&m_opaqueTriangleArray[index], local_opaque, sizeof(RTCTriangle)*numOpaque);
				numOpaque = 0;
			}
		});

        // Shrink the size (but not capacity) to fit
        m_alphaTriangleArray.resize(alphaCount, false);
        m_opaqueTriangleArray.resize(opaqueCount, false);
	}

	timer.tock();

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
    rtcSetOcclusionFilterFunctionN(m_scene, m_alphaGeomID,    FilterAdapter::rtcFilterFuncN);
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
                    // Embree's normals are backwards compared to G3D's CCW
                    RTCRayN_Ng_x(ray, N, r)   = -normal.x;
                    RTCRayN_Ng_y(ray, N, r)   = -normal.y;
                    RTCRayN_Ng_z(ray, N, r)   = -normal.z;
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
		// Hit
        if (occlusionOnly) {
            apiConvertOcclusion(rtcRay, hit);
        } else {
            const int triIndex = ((rtcRay.geomID == m_alphaGeomID) ? m_alphaTriangleArray : m_opaqueTriangleArray)[rtcRay.primID].triIndex;
            apiConvert(rtcRay, triIndex, hit);
        }
        return true;
    } else {
		// Miss
        return false;
    }
}

    
void EmbreeTriTree::intersectRays
   (const Array<Ray>&                  rays,
    Array<Hit>&                        results,
    IntersectRayOptions                options) const {

    results.resize(rays.size());    

    const bool occlusionOnly = (options & OCCLUSION_TEST_ONLY) != 0;
  
    FilterAdapter adapter(options);
    RTCIntersectContext context;
    context.flags = RTCIntersectFlags(0);
    if ((options & COHERENT_RAY_HINT) != 0) {
        context.flags = RTCIntersectFlags(context.flags | RTC_INTERSECT_COHERENT);
    } else {
        context.flags = RTCIntersectFlags(context.flags | RTC_INTERSECT_INCOHERENT);
    }
    context.userRayExt = (void*)&adapter;

    // Using raw pointers instead of C++ arrays gave no performance increase for the code below
    static const size_t grainSize = 64;
	tbb::parallel_for(tbb::blocked_range<size_t>(0, rays.size(), grainSize), [&](const tbb::blocked_range<size_t>& r) {
		const size_t start = r.begin();
		const size_t end   = r.end();

		// Subblock-based processing to keep the rays in cache gives a ~6% improvement over
		// converting the entire blocked_range at once.
		static const size_t BLOCK_SIZE = 64;
		for (size_t r = start; r < end; r += BLOCK_SIZE) {
			RTCRay rtcRays[BLOCK_SIZE];
			const size_t numRays = min(BLOCK_SIZE, end - r);

			for (size_t i = 0; i < numRays; ++i) {
				apiConvert(rays[r + i], rtcRays[i]);
			}

			(occlusionOnly ? rtcOccluded1M : rtcIntersect1M)(m_scene, &context, rtcRays, numRays, sizeof(RTCRay));

			for (size_t i = 0; i < numRays; ++i) {
				RTCRay& rtcRay = rtcRays[i];
				if (rtcRay.geomID != RTC_INVALID_GEOMETRY_ID) {
					// Hit
					if (occlusionOnly) {
						apiConvertOcclusion(rtcRay, results[r + i]);
					} else {
						const int triIndex = ((rtcRay.geomID == m_alphaGeomID) ? m_alphaTriangleArray : m_opaqueTriangleArray)[rtcRay.primID].triIndex;
						apiConvert(rtcRay, triIndex, results[r + i]);
					}					
				} else {
					// Miss
					results[r + i].triIndex = Hit::NONE;
				}
			} // for each ray
		} // for 
	}); // parallel for

}

} // G3D

#endif
