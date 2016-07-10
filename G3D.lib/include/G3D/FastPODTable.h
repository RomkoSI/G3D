/**
 \file G3D/FastPODTable.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2014-07-09
 \edited  2016-07-09

 G3D Innovation Engine
 Copyright 2002-2016, Morgan McGuire.
 All rights reserved.
*/
#pragma once

#include "G3D/platform.h"
#include "G3D/HashTrait.h"
#include "G3D/EqualsTrait.h"
#include "G3D/MemoryManager.h"
#include "G3D/ReferenceCount.h"

namespace G3D {

namespace _internal {
/** Will be initialized to all 0's when allocated by grow().  The
    specific contents are assigned by template
    specialization/metaprogramming below. */
template<typename Key, typename Value, bool valueIsPOD> struct FastPODTable_Entry;

/** Will be initialized to all 0's when allocated by grow() */
template<typename Key, typename Value>
struct FastPODTable_Entry<Key, Value, true> {
    typedef         Value StoredValueType;

    Key             key;

    StoredValueType value;
    
    bool            m_inUse;
    
    bool inUse() const {
        return m_inUse;
    }

    void releaseValue() {
        m_inUse = false;
    }

    Value& valueRef() {
        return value;
    }

    const Value& valueRef() const {
        return value;
    }

    static StoredValueType newStoredValue() {
        return Value();
    }
};


template<typename Key, typename Value>
struct FastPODTable_Entry<Key, Value, false> {
    typedef         Value* StoredValueType;

    Key             key;
    
    /**
       Pointers are used so that the m_slot array will be tightly
       packed in memory even if sizeof(Value) is large.  This is
       nullptr if the Entry is not currently being used.
    */
    StoredValueType value;
    
    bool inUse() const {
        return notNull(value);
    }

    void releaseValue() {
        delete value;
    }

    Value& valueRef() {
        return *value;
    }

    const Value& valueRef() const {
        return *value;
    }

    static StoredValueType newStoredValue() {
        return new Value();
    }
};

} // namespace internal


/** 
    \brief A sparse grid optimized for the performance of read and
    write operations.  Implemented with an open-addressing hash table
    that does not support remove. This is substantially faster than
    Table<Point3int16, Value>.  Implemented with quadratic probing.
    
    Uses Vector4int16 instead of Point3int16 for 64-bit alignment
    alignment purposes (although the w component <i>is</i> properly
    hashed; it can be used to encode other indexing data such as
    orientation without degrading performance).

    \param Key Must be plain-old-data (POD), i.e., not have a
    destructor, since it will be memset rather than constructed.

    \param valueAssignmentIsFast Set to true only if Value is
    relatively small (e.g., 128 bytes or less), the assignment
    operator is efficient when applied to it (e.g., G3D::Array
    assignment is slow), and that is a POD type.  
    Default is true.
*/
template<typename Key, typename Value, class HashFunc = HashTrait<Key>, class EqualsFunc = EqualsTrait<Key>, bool valueIsSimplePOD = false>
class FastPODTable {
protected:
    // This implementation uses template metaprogramming to switch the
    // implementation between using inlined values and pointers to
    // values depending on whether value is a "simple POD" type, such
    // as int, that is fast to inline, or something complex like Array
    // that should not be frequently copied.  See
    // http://en.wikibooks.org/wiki/C%2B%2B_Programming/Templates/Template_Meta-Programming
    // for a reference to the techniques used here.  To USE the FastPODTable
    // class the caller need not be aware of these techniques.
    //
    // The obfuscating drawbacks of this style of programming and the
    // general limitations of this class are offset by the performance
    // advantages of having a highly-optimized add-only table for
    // critical applications such as sparse voxel storage, photon
    // mapping, and symbol tables.

    /** The type of this class */
    typedef FastPODTable<Key, Value, HashFunc, EqualsFunc, valueIsSimplePOD> MyType;

    /** Number of slots to allocate for every entry used.  Load factor
        is 1 / SLOTS_PER_ENTRY.  This was tuned for performance. */
    static const int SLOTS_PER_ENTRY = 3;

    /** The type of the individual slots in the hash table */
    typedef _internal::FastPODTable_Entry<Key, Value, valueIsSimplePOD> Entry;

    /** Constant used as a return value for findSlot */
    enum {NOT_FOUND = -1};

    /** Hash table slots */
    Entry*                              m_slot;

    /** Number of elements in m_slot */
    int                                 m_numSlots;

    /** Number of elements in m_slot currently storing valid Value pointers */
    int                                 m_usedSlots;

    int                                 m_initialSlots;

    /** A default "empty" value; nullptr if Entry stores pointers */
    typename Entry::StoredValueType     m_empty;

    shared_ptr<MemoryManager>           m_memoryManager;

    /** Step to the next index while probing for a slot. 
    \param probeDistance Number of times probed.  0 on the first call.*/
    inline void probe(int& i, int& probeDistance) const {
        ++probeDistance;

        // Linear probing (not used, but handy for debugging sometimes)
        // i = (i + 1) & (m_numSlots - 1);

        // Quadratic probing: (http://en.wikipedia.org/wiki/Quadratic_probing)
        i = (i + ((probeDistance + square(probeDistance)) >> 1)) & (m_numSlots - 1);
    }


    inline int hashCode(const Key& key) const {
        return int(HashFunc::hashCode(key));
    }


    /** Double the size of the table (must be a power of two size)*/
    void grow() {
        static bool inGrow = false;
        alwaysAssertM(! inGrow, "Re-entrant call to grow");
        inGrow = true;
        
        Entry* oldSlot = m_slot;
        const int oldNumSlots = m_numSlots;
        
        m_numSlots = 2 * oldNumSlots;
        alwaysAssertM(isPow2(m_numSlots), "Number of slots must be a power of 2");
        
        m_slot = (Entry*)System::malloc(sizeof(Entry) * m_numSlots);
        alwaysAssertM(m_slot != nullptr, "out of memory");
        System::memset(m_slot, 0, sizeof(Entry) * m_numSlots);
        
        // getCreate during copy will re-create all elements
        m_usedSlots = 0;
        
        // Insert all elements
        for (int i = 0; i < oldNumSlots; ++i) {
            Entry& e = oldSlot[i];
            
            if (e.inUse()) {
                // This element is in use, so we should re-insert a copy of it
                // and assign it to its current value.  This call should never
                // trigger a call back to grow().
                const int j = findSlot(e.key, true, e.value, true);
                alwaysAssertM(j != NOT_FOUND, "Internal error during grow");

                // e.value is now either a pointer that is now owned
                // by slot j or a POD value that will never be
                // referenced again and can be freed.
            }
        }
        
        System::free(oldSlot);
        inGrow = false;
    }


    /**
       Returns the index of the slot for key.  If key is not in the
       table and createIfNotFound is true, then this creates the slot.
       If key is not in the table and createIfNotFound is false, then
       this returns NOT_FOUND;

       \param valueToUse if a spot is created and useValue true, this is used as the value (allows the implementation of
       grow to re-insert existing values).
     */
    int findSlot(const Key& key, bool createIfNotFound, const typename Entry::StoredValueType& valueToUse, bool useValue) {

        int i = hashCode(key) & (m_numSlots - 1);
        int probeDistance = 0;

        while (true) {
            Entry& e = m_slot[i];
            if (! m_slot[i].inUse()) {
                if (createIfNotFound) {
                    // We found an empty location where this key should have been
                    if (m_usedSlots * SLOTS_PER_ENTRY < m_numSlots) {
                        
                        // Use this slot
                        ++m_usedSlots;
                        
                        e.key = key;
                        if (useValue) {
                            e.value = valueToUse;
                        } else {
                            e.value = Entry::newStoredValue();
                        }
                    
                        // Return a reference to the index
                        return i;
                    } else {
                        // We're running out of slots
                        grow();
                    
                        // Resume inserting, starting from the new position for this key
                        i = hashCode(key) & (m_numSlots - 1);
                        probeDistance = 0;
                    }
                } else {
                    return NOT_FOUND;
                }
            } else if (EqualsFunc::equals(e.key, key)) {
                // Found the existing value
                return i;
            } else {
                // Something else was in this slot...probe forward
                probe(i, probeDistance);
                
                // We should never have to probe more than N/2 slots 
                // with quadratic programming and a power-of-two table.
                debugAssertM(probeDistance < m_numSlots / 2, "Probed too far without finding a hit");
            }
        }
    }

    
    void getStats(int& longestProbe, float& averageProbe) const {
        double probeSum = 0;
        longestProbe = 0;
        
        int count = 0;
        // Find the total distance probed for all m_usedSlots.
        for (int s = 0; s < m_numSlots; ++s) {
            const Entry& e = m_slot[s];
            if (e.inUse()) {
                // See where this block should have been
                int probeDistance = 0;
                int i = hashCode(e.key) & (m_numSlots - 1);
                while (i != s) {
                    probe(i, probeDistance);
                }
                ++probeDistance;
                probeSum += probeDistance;
                longestProbe = max(longestProbe, probeDistance);
                ++count;
            }
        }
        
        debugAssert(count == m_usedSlots);
        if (m_usedSlots == 0) {
            averageProbe = 0.0;
        } else {
            averageProbe = float(probeSum / m_usedSlots);
        }
    }


    // Intentionally unimplemented
    MyType& operator=(const MyType& other);
    FastPODTable(const MyType& other);
    
public:

    /** \param expectedSize Number of key-value pairs that are
        expected to be stored in this table. */
    FastPODTable(int expectedSize = 16) : 
        m_slot(nullptr),
        m_numSlots(0), 
        m_usedSlots(0),
        m_initialSlots(ceilPow2(unsigned(expectedSize * SLOTS_PER_ENTRY))) {

        alwaysAssertM(expectedSize > 0, "expectedSize must be positive");
        debugAssert(m_initialSlots > 0);
        debugAssert(isPow2(m_initialSlots));
        // Wipe the pointer or value
        System::memset(&m_empty, 0, sizeof(typename Entry::StoredValueType));

        // Initial allocation
        m_numSlots = m_initialSlots;
        m_slot = (Entry*)System::malloc(sizeof(Entry) * m_numSlots);
        alwaysAssertM(m_slot != nullptr, "out of memory");
        System::memset(m_slot, 0, sizeof(Entry) * m_numSlots);
        
        debugAssert(SLOTS_PER_ENTRY > 2);
    }


    /** Returns the number of key-value pairs stored. */
    int size() const {
        return m_usedSlots;
    }


    /** Removes all elements and shrinks table to its initial size. */
    void clear() {
        for (int i = 0; i < m_numSlots; ++i) {
            m_slot[i].releaseValue();
        }

        if (m_numSlots > m_initialSlots) {
            // Shrink
            System::free(m_slot);
            m_numSlots = m_initialSlots;
            m_slot = (Entry*)System::malloc(sizeof(Entry) * m_numSlots);
            alwaysAssertM(m_slot != nullptr, "out of memory");
        }

        System::memset(m_slot, 0, sizeof(Entry) * m_numSlots);
        m_usedSlots = 0;
    }

    void clearAndSetMemoryManager(shared_ptr<MemoryManager> memoryManager) {
        clear();
        // TODO: actually use the memory manager elsewhere
        m_memoryManager = memoryManager;
    }


    /** Returns the size of everything in this table (not counting
        objects reference from Value%s by pointers). 

        \param valueSizeFunction A function that computes the size of a Value.
        If nullptr, sizeof(Value) is used.


        Example for a table of Arrays, where it is desirable to recurse
        into the arrays:

        \code
        size_t arraySize(const Array<X>& y) {
           return y.sizeInMemory();
        }

        FastPODTable<int, Array<X> > table;
        ...
        size_t s = table.sizeInMemory(&arraySize);
        \endcode
    */
    size_t sizeInMemory(size_t (*valueSizeFunction)(const Value&) = nullptr) const {
        size_t s = (sizeof(Entry) * (m_numSlots - m_usedSlots)) + sizeof(MyType);
        
        for (int i = 0; i < m_numSlots; ++i) {
            if (m_slot[i].inUse()) {
                if (notNull(valueSizeFunction)) {
                    s += valueSizeFunction(m_slot[i].valueRef());
                } else {
                    s += sizeof(Entry);
                }
            }
        }
        return s;
    }


    /** Leaves slot memory allocated but removes the contents. */
    void fastClear() {
        for (int i = 0; i < m_numSlots; ++i) {
            m_slot[i].releaseValue();
        }
        
        System::memset(m_slot, 0, sizeof(Entry) * m_numSlots);
        m_usedSlots = 0;
    }


    virtual ~FastPODTable() {
        fastClear();
        System::free(m_slot);
        m_slot = nullptr;
    }


    /** 
        Returns a reference to the value at key, creating the value if
        it did not previously exist.
    */
    Value& operator[](const Key& key) {
        debugAssert(m_numSlots > 0);
        const int i = findSlot(key, true, m_empty, false);
        debugAssert(m_usedSlots > 0);
        return m_slot[i].valueRef();
    }


    /** Synonym for operator[] */
    Value& getCreate(const Key& key) {
        return (*this)[key];
    }    

    /** Returns a pointer to the value for the specified key, or nullptr
        if that key is empty.*/
    Value* getPointer(const Key& key) {
        const int i = findSlot(key, false, m_empty, false);
        if (i == NOT_FOUND) {
            return nullptr;
        } else {
            return m_slot[i].value;
        }
    }


    const Value* getPointer(const Key& key) const {
        return const_cast<MyType*>(this)->getPointer(key);
    }
   

    void debugPrintStatus() const {
        int longestProbe;
        float averageProbe;
        getStats(longestProbe, averageProbe);
        
        debugPrintf("SLOTS_PER_ENTRY = %d\n", SLOTS_PER_ENTRY);
        debugPrintf("numSlots = %d\nusedSlots = %d\nlongestProbe = %d\naverageProbe = %lf\nload factor = %f\n\n", 
                    m_numSlots, m_usedSlots, longestProbe, averageProbe, m_usedSlots / float(m_numSlots));
    }


    /** Asserts that the table is well-conditioned for performance */
    void debugCheckStatus() const {
        int longestProbe;
        float averageProbe;
        getStats(longestProbe, averageProbe);
        alwaysAssertM(averageProbe < 1.3f, format("Average probe distance is too high (%f).", averageProbe));
        alwaysAssertM(longestProbe < 20, "Longest probe distance is too high.");
    }

    /**
     C++ STL style iterator variable.  See begin().
     */
    class IteratorBase {
    protected:

        MyType*        m_table;

        /**
           Slot index.
        */
        int            m_index;
        
        void findNext() {
            do {
                ++m_index;
            } while ((m_index < m_table->m_numSlots) && (! m_table->m_slot[m_index].inUse()));
        }

        IteratorBase(MyType* table) : m_table(table), m_index(-1) {
            findNext();
        }

        virtual ~IteratorBase() {}

    public:

        IteratorBase(const IteratorBase& it) : m_table(it.m_table), m_index(it.m_index) {}

        const Key& key() const {
            return m_table->m_slot[m_index].key;
        }
        
        IteratorBase& operator++() {
            debugAssert(isValid());
            findNext();
            return *this;
        }

        bool isValid() const {
            return (m_index < m_table->m_numSlots);
        }

        /** Exposes the index of the slot storing this key-value pair for debugging, profiling, and porting purposes. */
        int slotIndex() const {
            return m_index;
        }
    };

    /** Exposes the total number of slots used for debugging, profiling, and porting purposes. */
    int numSlots() const {
        return m_numSlots;
    }

    class Iterator : public IteratorBase {
    private:
        friend class FastPODTable<Key, Value, HashFunc, EqualsFunc, valueIsSimplePOD>;

        Iterator(MyType* table) : IteratorBase(table) {}

    public:
        
        Iterator(const Iterator& it) : IteratorBase(it) {}

        Value& value() {
            return this->m_table->m_slot[this->m_index].valueRef();
        }

        const Value& value() const {
            return this->m_table->m_slot[this->m_index].valueRef();
        }
    };
    
    class ConstIterator : public IteratorBase {
    private:
        friend class FastPODTable<Key, Value, HashFunc, EqualsFunc, valueIsSimplePOD>;

        ConstIterator(MyType* table) : IteratorBase(table) {}

    public:

        const Value& value() {
            return this->m_table->m_slot[this->m_index].valueRef();
        }
    };


    ConstIterator begin() const {
        return ConstIterator(this);
    }

    Iterator begin() {
        return Iterator(this);
    }
};

} // G3D

