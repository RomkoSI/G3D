/** 
  \file Grid.h
 
  \maintainer Morgan McGuire
 
  \created 2014-10-24
  \edited  2014-10-24

  Copyright 2000-2015, Morgan McGuire, http://graphics.cs.williams.edu
  All rights reserved.
 */
#ifndef G3D_Grid_h
#define G3D_Grid_h

#include "platform.h"
#include "Vector3int32.h"
#include "ReferenceCount.h"
#include "WrapMode.h"
#include "g3dmath.h"

namespace G3D {

/** A dense 3D grid of templated Cells. 
    \sa Array, RayGridIterator, PointHashGrid, FastPointHashGrid */
template<typename Cell>
class Grid : public ReferenceCountedObject {
protected:

    Vector3int32                    m_size;

    /** m_cell[x][y][z] */
    Cell*                           m_cell;

    Cell                            m_empty;

    // Intentionally unimplemented
    Grid(const Grid& g);

    // Intentionally unimplemented
    Grid& operator=(const Grid& g);

public:

    /** \sa create */
    Grid() : m_cell(NULL) {
        clearAndResize(Vector3int32(1, 1, 1), Cell());
    }

    void setEmptyValue(const Cell& e) {
        m_empty = e;
    }

    /** The value to clear new cells to on clearAndResize and
        to return from get() when using WrapMode::ZERO */
    const Cell& emptyValue() const {
        return m_empty;
    }

    virtual ~Grid() {
        delete[] m_cell;
        m_cell = NULL;
    }

    void clearAndResize(const Vector3int32& size, const Cell& emptyValue) { 
        debugAssert((size.x > 0) && (size.y > 0) && (size.z > 0));
        delete[] m_cell;
        m_cell = new Cell[size.x * size.y * size.z];
        m_size = size;
        const int N = size.x * size.y * size.z;
        for (int i = 0; i < N; ++i) {
            m_cell[i] = emptyValue;
        }
    }

    void clearAndResize(const Vector3int32& size) { 
        clearAndResize(size, m_empty);
    }

    /** A grid can also be directly constructed as an object
        instead of a pointer...see the public constructor.*/
    static shared_ptr<Grid> create() { 
        return shared_ptr<Grid>(new Grid());
    }

    const Vector3int32& size() const {
        return m_size;
    }

    bool inBounds(const Point3int32& p) const {
        // Cast to unsigned to perform two-sided test
        return (uint32(p.x) < uint32(m_size.x)) && (uint32(p.y) < uint32(m_size.y)) && (uint32(p.z) < uint32(m_size.z));
    }

    /** Assumes that p is in bounds */
    const Cell& operator[](const Point3int32& p) const {
        debugAssert(notNull(m_cell));
        debugAssert(inBounds(p));
        return m_cell[p.x + m_size.x * (p.y + m_size.y * p.z)];
    }

    /** Assumes that p is in bounds */
    Cell& operator[](const Point3int32& p) {
        return const_cast<Cell&>(const_cast<const Grid&>(*this)[p]);
    }

    const Cell& get(const Point3int32& p, WrapMode m = WrapMode::ERROR) const {
        if (inBounds(p)) {
            return (*this)[p];
        } else {
            switch (m.value) {
            case WrapMode::ERROR:
                throw "Out of bounds";

            case WrapMode::ZERO:
            case WrapMode::IGNORE:
                return m_empty;

            case WrapMode::CLAMP:
                return (*this)[p.clamp(Point3int32(0, 0, 0), Point3int32(m_size.x - 1, m_size.y - 1, m_size.z - 1))];

            case WrapMode::TILE:
                return (*this)[p.wrap(m_size)];
            }
            return m_empty;
        }
    }

    void set(const Point3int32& p, const Cell& value, WrapMode m = WrapMode::ERROR) {
        if (inBounds(p)) {
            return (*this)[p] = value;
        } else {
            switch (m.value) {
            case WrapMode::ERROR:
                throw "Out of bounds";

            case WrapMode::ZERO:
            case WrapMode::IGNORE:
                break;

            case WrapMode::CLAMP:
                (*this)[p.clamp(Point3int32(0, 0, 0), Point3int32(m_size.x - 1, m_size.y - 1, m_size.z - 1))] = value;

            case WrapMode::TILE:
                (*this)[p.wrap(m_size)] = value;
            }
        }
    }
};

} // namespace 

#endif
