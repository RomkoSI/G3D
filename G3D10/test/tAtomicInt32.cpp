#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testAtomicInt32() {
    printf("G3D::AtomicInt32 ");

    // Test first in the absence of threads.
    {
        AtomicInt32 a(1);
        testAssert(a.value() == 1);

        a = 1;
        a.increment();
        testAssert(a.value() == 2);

        a = 2;
        a.decrement();
        testAssert(a.value() == 1);
		testAssert(a.decrement() == 0);

        a = 10;
        testAssert(a.value() == 10);

        a = 10;
        a.compareAndSet(10, 1);
        testAssert(a.value() == 1);

        a = 1;
        a.compareAndSet(10, 15);
        testAssert(a.value() == 1);

        a = 1;
        a.add(5);
        testAssert(a.value() == 6);

        a = 6;
        a.sub(3);
        testAssert(a.value() == 3);

        a = 6;
        a.sub(-3);
        testAssert(a.value() == 9);
    }

    printf("passed\n");
}

