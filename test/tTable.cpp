#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#include <map>

#if 1//defined(G3D_WINDOWS) && (_MSC_VER >= 1300)
#   define HAS_HASH_MAP
#endif

#ifdef HAS_HASH_MAP
#    ifdef __GNUC__
//     Tricks to make hash_map work as if it was part of stl on GCC
#        include <ext/hash_map>
         namespace std {
             using namespace __gnu_cxx;

         } // std
             namespace __gnu_cxx {                                                                         
                 template<> struct hash< String > { 
                     size_t operator()( const String& x ) const {
                         return hash< const char* >()( x.c_str() );                                              
                     }                                                                                         
                 };
             } // __gnu_cxx
#    else
#       if defined(_MSC_VER) && (_MSC_VER >= 1900)
#           define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1
#       endif
#        include <hash_map>
#    endif
#   if defined(_MSC_VER) && (_MSC_VER >= 1400)
        using stdext::hash_map;
#   else
        using std::hash_map;
#   endif
#endif

// For demonstrating VC8 bug
#ifdef HAS_HASH_MAP
hash_map<int, int> x;
#endif

class TableKey {
public:
    int value;
    
    inline bool operator==(const TableKey& other) const {
        return value == other.value;
    }
    
    inline bool operator!=(const TableKey& other) const {
        return value != other.value;
    }

    inline unsigned int hashCode() const {
        return 0;
    }
};

template <>
struct HashTrait<TableKey*> {
    static size_t hashCode(const TableKey* key) { return key->hashCode(); }
};

class TableKeyWithCustomHashStruct {
public:
    int data;
    TableKeyWithCustomHashStruct() { }
    TableKeyWithCustomHashStruct(int data) : data(data) { }
};

struct TableKeyCustomHashStruct {
    static size_t hashCode(const TableKeyWithCustomHashStruct& key) { 
        return static_cast<size_t>(key.data); 
    }
};

bool operator==(const TableKeyWithCustomHashStruct& lhs, const TableKeyWithCustomHashStruct& rhs) {
    return (lhs.data == rhs.data);
}


void testTable() {

    printf("G3D::Table  ");

    // Test ops involving HashCode / lookup for a table with a key
    // that uses a custom hashing struct
    {
        Table<TableKeyWithCustomHashStruct, int, TableKeyCustomHashStruct> table;
        bool exists;
        int val;

        table.set(1, 1);
        table.set(2, 2);
        table.set(3, 3);

        table.remove(2);

        val = table.get(3);
        testAssert(val == 3);

        exists = table.get(1, val);
        testAssert(exists && val == 1);
        exists = table.get(2, val);
        testAssert(!exists);
        exists = table.get(3, val);
        testAssert(exists && val == 3);

        exists = table.containsKey(1);
        testAssert(exists);
        exists = table.containsKey(2);
        testAssert(!exists);

        table.remove(1);
        table.remove(3);

        testAssert(table.size() == 0);
    }


    // Basic get/set
    {
        Table<int, int> table;
    
        table.set(10, 20);
        table.set(3, 1);
        table.set(1, 4);

        testAssert(table[10] == 20);
        testAssert(table[3] == 1);
        testAssert(table[1] == 4);
        testAssert(table.containsKey(10));
        testAssert(!table.containsKey(0));

        testAssert(table.debugGetDeepestBucketSize() == 1);
    }

    // Test hash collisions
    {
        TableKey        x[6];
        Table<TableKey*, int> table;
        for (int i = 0; i < 6; ++i) {
            x[i].value = i;
            table.set(x + i, i);
        }

        testAssert(table.size() == 6);
        testAssert(table.debugGetDeepestBucketSize() == 6);
        testAssert(table.debugGetNumBuckets() == 10);
    }

    // Test that all supported default key hashes compile
    {
        Table<int, int> table;
    }
    {
        Table<uint32, int> table;
    }
    {
        Table<uint64, int> table;
    }
    {
        Table<void*, int> table;
    }
    {
        Table<String, int> table;
    }
    {
        Table<const String, int> table;
    }

    {
        Table<GEvent, int> table;
    }
    {
        Table<GKey, int> table;
    }
    {
        Table<Sampler, int> table;
    }
    {
        Table<VertexBuffer*, int> table;
    }
    {
        Table<AABox, int> table;
    }
    {
        typedef _internal::Indirector<AABox> AABoxIndrctr;
        Table<AABoxIndrctr, int> table;
    }
    {
        Table<NetAddress, int> table;
    }
    {
        Table<Sphere, int> table;
    }
    {
        Table<Triangle, int> table;
    }
    {
        Table<Vector2, int> table;
    }
    {
        Table<Vector3, int> table;
    }
    {
        Table<Vector4, int> table;
    }
    {
        Table<Vector4int8, int> table;
    }
    {
        Table<WrapMode, int> table;
    }

    printf("passed\n");
}


template<class K, class V>
void perfTest(const char* description, const K* keys, const V* vals, int M) {
    uint64 tableSet = 0, tableGet = 0, tableRemove = 0;
    uint64 mapSet = 0, mapGet = 0, mapRemove = 0;
#   ifdef HAS_HASH_MAP
    uint64 hashMapSet = 0, hashMapGet = 0, hashMapRemove = 0;
#   endif

    uint64 overhead = 0;

    // Run many times to filter out startup behavior
    for (int j = 0; j < 3; ++j) {

        // There's a little overhead just for the loop and reading 
        // the values from the arrays.  Take this into account when
        // counting cycles.
        System::beginCycleCount(overhead);
        K k; V v;
        for (int i = 0; i < M; ++i) {
            k = keys[i];
            v = vals[i];
        }
        System::endCycleCount(overhead);

        {Table<K, V> t;
        System::beginCycleCount(tableSet);
        for (int i = 0; i < M; ++i) {
            t.set(keys[i], vals[i]);
        }
        System::endCycleCount(tableSet);
        
        System::beginCycleCount(tableGet);
        for (int i = 0; i < M; ++i) {
            v=t[keys[i]];
        }
        System::endCycleCount(tableGet);

        System::beginCycleCount(tableRemove);
        for (int i = 0; i < M; ++i) {
            t.remove(keys[i]);
        }
        System::endCycleCount(tableRemove);
        }

        /////////////////////////////////

        {std::map<K, V> t;
        System::beginCycleCount(mapSet);
        for (int i = 0; i < M; ++i) {
            t[keys[i]] = vals[i];
        }
        System::endCycleCount(mapSet);
        
        System::beginCycleCount(mapGet);
        for (int i = 0; i < M; ++i) {
            v=t[keys[i]];
        }
        System::endCycleCount(mapGet);

        System::beginCycleCount(mapRemove);
        for (int i = 0; i < M; ++i) {
            t.erase(keys[i]);
        }
        System::endCycleCount(mapRemove);
        }

        /////////////////////////////////

#       ifdef HAS_HASH_MAP
        {hash_map<K, V> t;
        System::beginCycleCount(hashMapSet);
        for (int i = 0; i < M; ++i) {
            t[keys[i]] = vals[i];
        }
        System::endCycleCount(hashMapSet);
        
        System::beginCycleCount(hashMapGet);
        for (int i = 0; i < M; ++i) {
            v=t[keys[i]];
        }
        System::endCycleCount(hashMapGet);

        System::beginCycleCount(hashMapRemove);
        for (int i = 0; i < M; ++i) {
            t.erase(keys[i]);
        }
        System::endCycleCount(hashMapRemove);
        }
#       endif
    }

    tableSet -= overhead;
    if (tableGet < overhead) {
        tableGet = 0;
    } else {
        tableGet -= overhead;
    }
    tableRemove -= overhead;

    mapSet -= overhead;
    mapGet -= overhead;
    mapRemove -= overhead;

    float N = (float)M;
    printf("%s\n", description);
    bool G3Dwin = 
        (tableSet <= mapSet) &&
        (tableGet <= mapGet) &&
        (tableRemove <= mapRemove);
    printf("Table         %9.1f  %9.1f  %9.1f   %s\n", 
           (float)tableSet / N, (float)tableGet / N, (float)tableRemove / N,
           G3Dwin ? " ok " : "FAIL"); 
#   ifdef HAS_HASH_MAP
    printf("hash_map      %9.1f  %9.1f  %9.1f\n", (float)hashMapSet / N, (float)hashMapGet / N, (float)hashMapRemove / N); 
#   endif
    printf("std::map      %9.1f  %9.1f  %9.1f\n", (float)mapSet / N, (float)mapGet / N, (float)mapRemove / N); 
    printf("\n");
}


void perfTable() {
    printf("                          [times in cycles]\n");
    printf("                   insert       fetch     remove    outcome\n");

    const int M = 300;
    {
        int keys[M];
        int vals[M];
        for (int i = 0; i < M; ++i) {
            keys[i] = i * 2;
            vals[i] = i;
        }
        perfTest<int, int>("int,int", keys, vals, M);
    }

    {
        String keys[M];
        int vals[M];
        for (int i = 0; i < M; ++i) {
            keys[i] = format("%d", i * 2);
            vals[i] = i;
        }
        perfTest<String, int>("string, int", keys, vals, M);
    }

    {
        int keys[M];
        String vals[M];
        for (int i = 0; i < M; ++i) {
            keys[i] = i * 2;
            vals[i] = format("%d", i);
        }
        perfTest<int, String>("int, string", keys, vals, M);
    }

    {
        String keys[M];
        String vals[M];
        for (int i = 0; i < M; ++i) {
            keys[i] = format("%d", i * 2);
            vals[i] = format("%d", i);
        }
        perfTest<String, String>("string, string", keys, vals, M);
    }
}
