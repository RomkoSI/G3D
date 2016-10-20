/* 
 G3D Library
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#ifndef G3D_OrderedTable_h
#define G3D_OrderedTable_h

#include "G3D/platform.h"
#include "G3D/Table.h"

namespace G3D {
/**
 \brief A Table that can be iterated through by index, preserves ordering, 
   and implicitly supports removal during iteration.

  Useful for creating arrays of Entity%s or other objects that 
  need efficient fetch by name or ID but also efficient, order-preserving
  iteration.

  Does not overload operator[] to avoid ambiguity when the Key
  is an integer type.
*/
template <class Key, class Value>
class OrderedTable {
private:
    typedef std::pair<Key, Value>  Pair;

    Table<Key, int>         m_indexTable;
    Array<Pair>             m_array;

public:

    void set(const Key& key, const Value& value) {
        bool created = false;
        int& index = m_indexTable.getCreate(key, created);
        if (created) {
            index = m_array.size();
            m_array.append(Pair(key, value));
        } else {
            // Overwrite the value
            m_array[index].second = value;
        }
    }

    /** Returns -1 if not found. O(1) amortized expected time. */
    int findIndexOfKey(const Key& key) const {
        const int* i = m_indexTable.getPointer(key);
        if (notNull(i)) {
            return *i;
        } else {
            return -1;
        }
    }

    /**
      Returns the index for this key (same as findIndexOfKey)
      \sa key */
    int index(const Key& key) const {
        return findIndexOfKey(key);
    }

    /** Returns -1 if not found. O(n) expected time. */
    int findIndexOfValue(const Value& value) const {
        for (int i = 0; i < m_array.size(); ++i) {
            if (m_array[i].value == value) {
                return i;
            }
        }
        return -1;
    }

    bool containsKey(const Key& key) const {
        return m_indexTable.containsKey(key);
    }

    bool containsValue(const Value& v) const {
        return findIndexOfValue(v) != -1;
    }

    /** Removes the element with this key and slides other elements down to fill the hole.
        O(n) time in the size of the table. */
    void removeKey(const Key& k) {
        int i = m_indexTable[k];
        m_indexTable.remove(k);
        m_array.remove(i);

        // Decrement successive indices
        while (i < m_array.size()) {
            --m_indexTable[m_array[i].first];
            ++i;
        }
    }

    /** Removes the element with this key by swapping it with the last 
        element.
        Amortized expected O(1) time. */
    void fastRemoveKey(const Key& k) {
        int i = m_indexTable[k];
        m_indexTable.remove(k);
        m_array.fastRemove(i);
        if (i < m_array.size()) {
            // Update the newly swapped-in value
            m_indexTable[m_array[i].first] = i;
        }
    }

    Value& valueFromKey(const Key& k) {
        return m_array[m_indexTable[k]].second;
    }

    const Value& valueFromKey(const Key& k) const {
        return const_cast<OrderedTable<Key, Value>*>(this)->valueFromKey(k);
    }

    Value& valueFromIndex(int i) {
        return m_array[i].second;
    }

    const Value& valueFromIndex(int i) const {
        return const_cast<OrderedTable<Key, Value>*>(this)->valueFromIndex(i);
    }

    /** Return the key for this index */
    const Key& key(int i) const {
        return m_array[i].first;
    }

    int size() const {
        return m_array.size();
    }

    void clear() {
        m_array.clear();
        m_indexTable.clear();
    }
};

} // namespace G3D

#endif
