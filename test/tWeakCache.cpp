#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

class CacheTest : public ReferenceCountedObject {
public:
    static int count;
    int x;
    CacheTest() {
        ++count;
    }
    ~CacheTest() {
        --count;
    }
};
int CacheTest::count = 0;
typedef shared_ptr<CacheTest> CacheTestRef;

void testWeakCache() {
    WeakCache<String, CacheTestRef> cache;

    testAssert(CacheTest::count == 0);
    CacheTestRef x(new CacheTest());
    testAssert(CacheTest::count == 1);

    cache.set("x", x);
    testAssert(CacheTest::count == 1);
    CacheTestRef y(new CacheTest());
    CacheTestRef z(new CacheTest());
    testAssert(CacheTest::count == 3);
    
    cache.set("y", y);
    
    testAssert(cache["x"] == x);
    testAssert(cache["y"] == y);
    testAssert(isNull(cache["q"]));
    
    x.reset();
    testAssert(CacheTest::count == 2);
    testAssert(isNull(cache["x"]));

    cache.set("y", z);
    y.reset();
    testAssert(cache["y"] == z);

    cache.remove("y");
}
