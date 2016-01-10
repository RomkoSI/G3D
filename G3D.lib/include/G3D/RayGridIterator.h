#ifndef G3D_RayGridIterator_h
#define G3D_RayGridIterator_h

#include "G3D/platform.h"
#include "G3D/Vector3int32.h"
#include "G3D/Ray.h"
#include "G3D/Vector3.h"

namespace G3D {

/** 
    Computes conservative line raster/voxelization across a grid for
    use in walking a grid spatial data structure or or voxel scene
    searching for intersections.  At each iteration, the iterator
    steps exactly one cell in exactly one dimension.

    Example of this iterator applied to ray-primitive intersection in a
    grid:

\code
bool firstRayIntersection(const Ray& ray, Value*& value, float& distance) const {

    for (RayGridIterator it(ray, cellSize); inBounds(it.index); ++it) {
        // Search for an intersection within this grid cell
        const Cell& c = cell(it.index);
        float maxdistance = min(distance, t.tExit);
        if (c.firstRayIntersection(ray, value, maxdistance)) {
            distance = maxdistance;
            return true;
        }
    }
}
\endcode

\sa CollisionDetection, PointHashGrid, Ray, Intersect

 */
class RayGridIterator {
protected:

    /** Extent of the grid in each dimension, in grid cell units.*/
    Vector3int32  m_numCells;

    /** Current grid cell m_index */
    Vector3int32  m_index;
    
    /** Sign of the direction that the ray moves along each axis; +/-1
       or 0 */
    Vector3int32  m_step;

    /** Size of one cell in units of t along each axis. */
    Vector3       m_tDelta;

    /** Distance along the ray of the first intersection with the
       current cell (i.e., that given by m_index). Zero for the cell that contains the ray origin. */
    float         m_enterDistance;

    /** Distance along the ray to the intersection with the next grid
       cell.  enterDistance and m_exitDistance can be used to bracket ray
       ray-primitive intersection tests within a cell. 
       */ 
    Vector3       m_exitDistance;

    /** The axis along which the ray entered the cell; X = 0, Y = 1, Z = 2.  This is always set to X for the cell that contains the ray origin. */
    int           m_enterAxis;

    /** The original ray */
    Ray           m_ray;

    /** Size of each cell along each axis */
    Vector3       m_cellSize;

    /** True if index() refers to a valid cell inside the grid.  This
        is usually employed as the loop termination condition.*/
    bool          m_insideGrid;

    /** The value that the index will take on along each boundary when
        it just leaves the grid. */
    Vector3int32  m_boundaryIndex;

    /** True if this cell contains the ray origin */
    bool          m_containsRayOrigin;

public:

    /** \copydoc m_ray */
    const Ray& ray() const {
        return m_ray;
    }

    /** \copydoc m_numCells */
    Vector3int32 numCells() const {
        return m_numCells;
    }

    /** \copydoc m_enterAxis */
    int enterAxis() const {
        return m_enterAxis;
    }

    /** Outward-facing normal to the current grid cell along the
       partition just entered.  Initially zero. */
    Vector3int32 enterNormal() const {
        Vector3int32 normal(0,0,0);
        normal[m_enterAxis] = -m_step[m_enterAxis];
        return normal;
    }

    /** \copydoc m_cellSize */
    const Vector3& cellSize() const {
        return m_cellSize;
    }

    /** Location where the ray entered the current grid cell */
    Point3 enterPoint() const {
        return m_ray.origin() + m_enterDistance * m_ray.direction();
    }

    /** Location where the ray exits the current grid cell */
    Point3 exitPoint() const {
        return m_ray.origin() + m_exitDistance.min() * m_ray.direction();
    }

    /** \copydoc m_enterDistance */
    float enterDistance() const {
        return m_enterDistance;
    }

    /** Distance from the ray origin to the exit point in this cell. */
    float exitDistance() const {
        return m_exitDistance.min();
    }

    /** \copydoc m_step */
    const Vector3int32& step() const {
        return m_step;
    }

    /** \copydoc m_index */
    const Vector3int32& index() const {
        return m_index;
    }

    /** \copydoc m_tDelta */
    const Vector3& tDelta() const {
        return m_tDelta;
    }

    /** \copydoc m_insideGrid */
    bool insideGrid() const {
        return m_insideGrid;
    }

    /** \copydoc m_containsRayOrigin */
    bool containsRayOrigin() const {
        return m_containsRayOrigin;
    }

    /** 
        \brief Initialize the iterator to the first grid cell hit by
        the ray and precompute traversal variables.

        The grid is assumed to have a corner at (0,0,0) and extend
        along the canonical axes.  For intersections grids transformed
        by a rigid body transformation, first transform the ray into
        the grid's object space with CFrame::rayToObjectSpace.

        If the ray never passes through the grid, insideGrid() will
        be false immediately after intialization.

        If using for 2D iteration, set <code>numCells.z = 1</code> and
        <code>ray.origin().z = 0.5</code>

        \param cellSize The extent of one cell

        \param minBoundsLocation The location of the lowest corner of grid cell minBoundsCellIndex along each axis.  
        This translates the grid relative to the ray's coordinate frame.

        \param minBoundsCellIndex The index of the first grid cell.  This allows
        operation with grids defined on negative indices.  This translates all
        grid indices.
    */
    RayGridIterator
    (Ray                    ray, 
     const Vector3int32&    numCells, 
     const Vector3&         cellSize            = Vector3(1,1,1),
     const Point3&          minBoundsLocation = Point3(0,0,0),
     const Point3int32&     minBoundsCellIndex  = Point3int32(0,0,0));

    /** Increment the iterator, moving to the next grid cell */
    RayGridIterator& operator++() {
        // Find the axis of the closest partition along the ray
        if (m_exitDistance.x < m_exitDistance.y) { 
            if (m_exitDistance.x < m_exitDistance.z) {
                m_enterAxis = 0;
            } else { 
                m_enterAxis = 2;
            } 
        } else if (m_exitDistance.y < m_exitDistance.z) { 
            m_enterAxis = 1;
        } else {
            m_enterAxis = 2;
        }

        m_enterDistance              = m_exitDistance[m_enterAxis];
        m_index[m_enterAxis]        += m_step[m_enterAxis];
        m_exitDistance[m_enterAxis] += m_tDelta[m_enterAxis];
        
        // If the index just hit the boundary exit, we have
        // permanently exited the grid.
        m_insideGrid = m_insideGrid && 
            (m_index[m_enterAxis] != m_boundaryIndex[m_enterAxis]);

        m_containsRayOrigin = false;

        return *this;
    }
};

}

#endif
