#include "G3D/platform.h"
#include "G3D/AABox.h"
#include "G3D/RayGridIterator.h"
#include "G3D/CollisionDetection.h"

namespace G3D {


RayGridIterator::RayGridIterator
(Ray                    ray, 
 const Vector3int32&    numCells, 
 const Vector3&         cellSize,
 const Point3&          gridOrigin,
 const Point3int32&     gridOriginIndex) :
    m_numCells(numCells),
    m_enterDistance(0.0f),
    m_enterAxis(0),
    m_ray(ray), 
    m_cellSize(cellSize),
    m_insideGrid(true),
    m_containsRayOrigin(true) {
    
    if (gridOrigin.nonZero()) {
        // Change to the grid's reference frame
        ray = Ray::fromOriginAndDirection(ray.origin() - gridOrigin, ray.direction());
    }

    //////////////////////////////////////////////////////////////////////
    // See if the ray begins inside the box
    const AABox gridBounds(Vector3::zero(), Vector3(numCells) * cellSize);

    bool startsOutside = false;
    bool inside = false;
    Point3 startLocation = ray.origin();

    const bool passesThroughGrid =
        CollisionDetection::rayAABox
        (ray, Vector3(1,1,1) / ray.direction(),
         gridBounds, gridBounds.center(),
         square(gridBounds.extent().length() * 0.5f),
         startLocation,
         inside);

    if (! inside) {
        if (passesThroughGrid) {
            // Back up slightly so that we immediately hit the
            // start location.  The precision here is tricky--if
            // the ray strikes at a very glancing angle, we need
            // to move a large distance along the ray to enter the
            // grid.  If the ray strikes head on, we only need to
            // move a small distance.
            m_enterDistance = (ray.origin() - startLocation).length() - 0.0001f;
            startLocation = ray.origin() + ray.direction() * m_enterDistance;
            startsOutside = true;
        } else {
            // The ray never hits the grid
            m_insideGrid = false;
        }
    }

    //////////////////////////////////////////////////////////////////////
    // Find the per-iteration variables
    for (int a = 0; a < 3; ++a) {
        m_index[a]  = (int32)floor(startLocation[a] / cellSize[a]);
        m_tDelta[a] = cellSize[a] / fabs(ray.direction()[a]);

        m_step[a]   = (int32)sign(ray.direction()[a]);

        // Distance to the edge fo the cell along the ray direction
        float d = startLocation[a] - m_index[a] * cellSize[a];
        if (m_step[a] > 0) {
            // Measure from the other edge
            d = cellSize[a] - d;

            // Exit on the high side
            m_boundaryIndex[a] = m_numCells[a];
        } else {
            // Exit on the low side (or never)
            m_boundaryIndex[a] = -1;
        }
        debugAssert(d >= 0 && d <= cellSize[a]);

        if (ray.direction()[a] != 0) {
            m_exitDistance[a] = d / fabs(ray.direction()[a]) + m_enterDistance;
        } else {
            // Ray is parallel to this partition axis.
            // Avoid dividing by zero, which could be NaN if d == 0
            m_exitDistance[a] = (float)inf();
        }
    }

    if (gridOriginIndex.nonZero()) {
        // Offset the grid coordinates
        m_boundaryIndex += gridOriginIndex;
        m_index += gridOriginIndex;
    }

    if (startsOutside) {
        // Let the increment operator bring us into the first cell
        // so that the starting axis is initialized correctly.
        ++(*this);
    }
}



}
