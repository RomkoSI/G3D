#ifndef FastPointHashGrid_h
#define FastPointHashGrid_h

#include "G3D/platform.h"
#include "G3D/HashTrait.h"
#include "G3D/PositionTrait.h"
#include "G3D/Array.h"
#include "G3D/Vector4int16.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "FastPODTable.h"

#ifdef CURRENT
#undef CURRENT
#endif

namespace G3D {

/** 
    A multiset of values (i.e., with duplicates allowed) indexed
    efficiently by spatial location.

    This is optimized for 64-bit processors.  It generally performs at
    least as well as PointHashGrid (although it does not support
    remove operations), is about 2x faster to build, and has a simpler
    structure that is more amenable to serialization.

    \sa PointHashGrid, PointKDTree, FastPODTable
 */
template< typename Value, class PosFunc = PositionTrait<Value> >
class FastPointHashGrid {
public:

    typedef Array<Value, 15> ValueArray;

protected:

    typedef FastPODTable<Vector4int16, ValueArray, HashTrait<Vector4int16>, EqualsTrait<Vector4int16>, false> TableType;
    
    TableType*     m_table;
    float          m_metersPerCell;
    float          m_cellsPerMeter;
    int            m_size;

    inline Vector4int16 toCell(const Vector3& pos) const {
        return Vector4int16
            (int16(iFloor(pos.x * m_cellsPerMeter)),
             int16(iFloor(pos.y * m_cellsPerMeter)), 
             int16(iFloor(pos.z * m_cellsPerMeter)),
             0);
    }

    /** Increase this value if the cost of iterating over cells seems
        high.
    
     Decrease this value if the cost of rejecting points that are
     outside of a box seems high
    
     Currently the best sphere gather performance seems to be when the
     cell width is slightly smaller than the radius of the gather
     sphere, so that one can expect between 27 and 64 cells to be
     gathered.
     */
    static float gatherRadiusToCellWidth(float r) {
        return r * 0.75f;
    }

public:

    FastPointHashGrid(float gatherRadiusHint = 0.5f, int expectedNumCells = 16) : 
        m_table(NULL),
        m_metersPerCell(gatherRadiusToCellWidth(gatherRadiusHint)),
        m_cellsPerMeter(1.0f / m_metersPerCell),
        m_size(0) {

        alwaysAssertM(expectedNumCells > 0, "expectedNumCells must be positive");
        clear(gatherRadiusHint, expectedNumCells);
    }

    /** 
        Removes all elements but does not release the underlying structure.
     */
    void fastClear() {
        m_table->clear();
        m_size = 0;
    }


    float cellWidth() const {
        return m_metersPerCell;
    }


    /** 
        If the same value is inserted multiple times, it will appear
        multiple times (which is usually desirable).
     */
    void insert(const Value& v) {
        Point3 pos;
        PosFunc::getPosition(v, pos);

        // Map to the scale of the grid
        const Vector4int16 ipos = toCell(pos);

        // Get the corresponding cell from the table (allocating if needed)
        ValueArray& array = (*m_table)[ipos];
        array.append(v);
        ++m_size;
    }


    void insert(const Array<Value>& array) {
        for (int i = 0; i < array.size(); ++i) {
            insert(array[i]);
        }
    }


    /** Actual number of grid cells currently allocated. \sa size() */
    int numCells() const {
        return m_table->size();
    }


    TableType* underlyingTable() const {
        return m_table;
    }

    
    enum { CURRENT = 0 };

    /**
       \param gatherRadiusHint     If CURRENT, use the current cell width
       \param newExpectedNumCells  If CURRENT, use the current expected number of cells
     */
    void clear(float gatherRadiusHint = CURRENT, int newExpectedNumCells = CURRENT) {
        float newCellWidth = gatherRadiusToCellWidth(gatherRadiusHint);
        if (gatherRadiusHint == CURRENT) {
            newCellWidth = m_metersPerCell;
        }

        if (newExpectedNumCells == CURRENT) {
            debugAssertM(notNull(m_table), "Cannot use newExpectedNumCells = CURRENT during intialization");
            newExpectedNumCells = max(m_table->size(), 16);
        }

        debugAssert(newExpectedNumCells > 0);

        delete m_table;
        m_table = new TableType(newExpectedNumCells);

        m_cellsPerMeter = 1.0f / newCellWidth;
        m_metersPerCell = newCellWidth;
        m_size = 0;
    }


    /** Returns the number of elements. \sa numCells */
    int size() const {
        return m_size;
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    class Iterator {
    private:
        friend class FastPointHashGrid<Value, PosFunc>;

        typename TableType::Iterator m_it;
        
        ValueArray*   m_list;
        int          m_index;

        Iterator(const class FastPointHashGrid<Value, PosFunc>* phg) :
            m_it(phg->m_table->begin()),
            m_list(NULL),
            m_index(0) {

            // Advance to the first non-empty element
            while (m_it.isValid() && (isNull(m_list) || (m_list->size() == 0))) {
                ++m_it;
                m_list = &m_it.value();
            }
        }

    public:

        Iterator(const Iterator& it) : m_it(it.m_it), m_list(it.m_list), m_index(it.m_index) {}

        bool isValid() const {
            return m_it.isValid();
        }
        
        const Value& value() const {
            return (*m_list)[m_index];
        }

        const Value& operator*() const {
            return value();
        }

        const Value* operator->() const {
            return &value();
        }

        Iterator& operator++() {
            debugAssert(m_it.isValid());
            ++m_index;

            if (m_index == m_list->size()) {
                m_index = 0;
                do {
                    ++m_it;
                    m_list = &m_it.value();
                } while (m_it.isValid() && (isNull(m_list) || (m_list->size() == 0)));
            }

            return *this;
        }

    }; // Iterator

    Iterator begin() const {
        return Iterator(this);
    }

    /////////////////////////////////////////////////////////////////////////////////////////
 
    class CellIterator {
    private:
        friend class FastPointHashGrid<Value, PosFunc>;

        const float m_scale;
        typename TableType::Iterator m_it;

        CellIterator(const FastPointHashGrid<Value, PosFunc>* phg) :
            m_scale(phg->m_metersPerCell),
            m_it(phg->m_table->begin()) {
        }

    public:
        
        bool isValid() const {
            return m_it.isValid();
        }

        CellIterator& operator++() {
            ++m_it;
            return *this;
        }

        const ValueArray& valueArray() const {
            return m_it.value();
        }

        /** Bounds of this cell. */
        AABox bounds() const {
            const Vector4int16 key = m_it.key();
            const Point3 k(key.x, key.y, key.z);

            return AABox(k * m_scale,
                         (k + Vector3(1,1,1)) * m_scale);
        }

        /** The underlying key.  Exposed for debugging, profiling, and porting. */
        Vector4int16 key() const {
            return m_it.key();
        }
        
    };

    /** Iterates over non-empty cells, each of which contains a ValueArray. */
    CellIterator beginCell() {
        return CellIterator(this);
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    class BoxIterator {
    private:
        friend class FastPointHashGrid<Value, PosFunc>;

        const FastPointHashGrid<Value, PosFunc>* m_phg;

        /** Inclusive */
        const Vector4int16 m_low;

        /** Inclusive */
        const Vector4int16 m_high;

        bool               m_isValid;

        Vector4int16       m_currentCell;
        ValueArray*        m_currentList;
        int                m_currentListIndex;

        /** Advance to the next (dense) grid cell, which may be
            empty */
        void advanceCellDense() {
            debugAssert(m_isValid);
            ++m_currentCell.x;
            if (m_currentCell.x > m_high.x) {
                m_currentCell.x = m_low.x;
                ++m_currentCell.y;
                if (m_currentCell.y > m_high.y) {
                    m_currentCell.y = m_low.y;
                    ++m_currentCell.z;
                    if (m_currentCell.z > m_high.z) {
                        // Done with iteration
                        m_isValid = false;
                    }
                }
            }
        }

        /** Move on to the next non-empty cell. */
        void advanceCellSparse() {
            debugAssert(m_isValid);

            m_currentListIndex = 0;
            m_currentList = NULL;

            while (m_isValid && (isNull(m_currentList) || (m_currentList->size() == 0))) {
                advanceCellDense();
                m_currentList = m_phg->m_table->getPointer(m_currentCell);
            }
        }


        BoxIterator(const FastPointHashGrid<Value, PosFunc>* phg, const AABox& box) :
            m_phg(phg),
            m_low(phg->toCell(box.low())),
            m_high(phg->toCell(box.high())),
            m_isValid(true),
            m_currentList(NULL),
            m_currentListIndex(0) {

            m_currentCell = m_low;

            // Back up one, and then advance to the beginning (or fail)
            --m_currentCell.x;
            advanceCellSparse();
        }

    public:

        /** Returns true when the current value can be read.  i.e., 
            structure loops like:

            \code
            for (Grid::BoxIterator it = grid.begin(box); it.isValid(); ++it) { ... }
            \endcode
        */
        bool isValid() const {
            return m_isValid;
        }

        /** Advance to the next value */
        BoxIterator& operator++() {
            debugAssertM(isValid(), "Iterator is done");

            ++m_currentListIndex;
            if (m_currentListIndex == m_currentList->size()) {
                advanceCellSparse();
            }
            return *this;
        }

        const Value& value() {
            debugAssert(isValid());
            return (*m_currentList)[m_currentListIndex];
        }

        const Value& operator*() {
            return value();
        }

        const Value* operator->() {
            return &value();
        }
    };


    BoxIterator begin(const AABox& box) const {
        return BoxIterator(this, box);
    }

    /////////////////////////////////////////////////////////////////////////////////////////

    class SphereIterator {
    private:
        friend class FastPointHashGrid<Value, PosFunc>;

        Sphere         m_sphere;

        /** Has to always be one ahead */
        BoxIterator    m_boxIt;

        void advance() {
            debugAssert(m_boxIt.isValid());
            Point3 pos;
            do {
                ++m_boxIt;
                if (! m_boxIt.isValid()) {
                    return;
                }
                PosFunc::getPosition(m_boxIt.value(), pos);
            } while (! m_sphere.contains(pos));
        }


        SphereIterator(const FastPointHashGrid<Value, PosFunc>* phg, const Sphere& sphere) :
            m_sphere(sphere),
            m_boxIt(phg, AABox(sphere.center - Vector3(sphere.radius, sphere.radius, sphere.radius),
                               sphere.center + Vector3(sphere.radius, sphere.radius, sphere.radius))) {

            if (m_boxIt.isValid()) {
                // See if the first element is good
                Point3 pos;
                PosFunc::getPosition(m_boxIt.value(), pos);

                // Advance to the next entry in the sphere, or fail
                if (! m_sphere.contains(pos)) {
                    advance();
                }
            }
        }

    public:
        
        /** Returns true when the current value can be read.  i.e., 
         structure loops like:

         \code
         for (Grid::SphereIterator it = grid.begin(sphere); it.isValid(); ++it) { ... }
         \endcode
        */
        bool isValid() const {
            return m_boxIt.isValid();
        }

        /** Advance to the next value */
        SphereIterator& operator++() {
            debugAssertM(isValid(), "Iterator is done");

            advance();

            return *this;
        }

        const Value& value() {
            return m_boxIt.value();
        }

        const Value& operator*() {
            return value();
        }

        const Value* operator->() {
            return &value();
        }
    };

    /**
       Iterates over all values that are contained within the \a sphere.
     */
    SphereIterator begin(const Sphere& sphere) const {
        return SphereIterator(this, sphere);
    }


    void debugPrintStatistics() const {
        m_table->debugPrintStatus();

        int totalLen = 0;
        int maxLen = 0;
        for (typename TableType::Iterator cell = m_table->begin();
             cell.isValid();
             ++cell) {
            totalLen += cell.value().length();
            maxLen = max(maxLen, cell.value().length());
        }

        debugPrintf("Max cell size: %d\n", maxLen);
        debugPrintf("Average cell size: %f\n", totalLen / float(m_table->size()));
             
    }
};


} // namespace

#endif
