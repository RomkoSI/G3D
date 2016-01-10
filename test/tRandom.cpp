#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testRandom() {
    printf("Random number generators ");

    int num0 = 0;
    int num1 = 0;
    for (int i = 0; i < 10000; ++i) {
        switch (Random::common().integer(0, 1)) {
        case 0:
            ++num0;
            break;

        case 1:
            ++num1;
            break;

        default:
            testAssertM(false, "Random number outside the range [0, 1] from iRandom(0,1)");
        }
    }
    int difference = iAbs(num0 - num1);
    testAssertM(difference < 300, "Integer random number generator appears skewed.");
    (void)difference;

    for (int i = 0; i < 100; ++i) {
        double r = uniformRandom(0, 1);
        testAssert(r >= 0.0);
        testAssert(r <= 1.0);
        (void)r;
    }


    // Triangle.randomPoint
    Triangle tri(Vector3(0,0,0), Vector3(1,0,0), Vector3(0,1,0));
    for (int i = 0; i < 1000; ++i) {
        Vector3 p = tri.randomPoint();
        testAssert(p.z == 0);
        testAssert(p.x <= 1.0 - p.y);
        testAssert(p.x >= 0);
        testAssert(p.y >= 0);
        testAssert(p.x <= 1.0);
        testAssert(p.y <= 1.0);
    }


    for (int i = 0; i < 100; ++i) {
         Vector3 point = tri.randomPoint();
         testAssert(CollisionDetection::isPointInsideTriangle(
                 tri.vertex(0),
                 tri.vertex(1),
                 tri.vertex(2),
                 tri.normal(),
                 point));
    }

    printf("passed\n");
}
