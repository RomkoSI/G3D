#include "G3D/Map2D.h"
#include "testassert.h"

using namespace G3D;

void testBicubic() {
    typedef Map2D<float, float> FloatMap;

    FloatMap::Ref map = FloatMap::create(4, 4);

    // Trivial cases; fit a linear function
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4; ++y) {
            map->set(x, y, (float)x);
        }
    }

    float c = map->bicubic(1.5f, 1.5f);
    testAssert(fuzzyEq(c, 1.5f));

    c = map->bicubic(1.0f, 1.0f);
    testAssert(fuzzyEq(c, 1.0f));

    c = map->bicubic(1.9f, 1.5f);
    testAssert(fuzzyEq(c, 1.9f));

    // Trivial cases; fit a much bigger linear function
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4; ++y) {
            map->set(x, y, x * 100000.0f);
        }
    }

    c = map->bicubic(1.5f, 1.5f);
    testAssert(fuzzyEq(c, 1.5f * 100000));

    c = map->bicubic(1.0f, 1.0f);
    testAssert(fuzzyEq(c, 1.0f * 100000));

    c = map->bicubic(1.9f, 1.5f);
    testAssert(fuzzyEq(c, 1.9f * 100000));

}


void testMap2D() {
    testBicubic();
}
