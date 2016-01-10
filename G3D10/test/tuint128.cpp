#include "G3D/G3DAll.h"
#include "testassert.h"
void testuint128() {

	uint128 a(0,0);
	uint128 b(0,0);

    uint32 hiHi;
    uint32 loHi;
    uint32 hiLo;
    uint32 loLo;

	for (int i = 0; i < 1000; ++i) {
        hiHi = (uint32) (uniformRandom(0.0, 1.0) * 0xFFFFFFFF);
        loHi = (uint32) (uniformRandom(0.0, 1.0) * 0xFFFFFFFF);
        hiLo = (uint32) (uniformRandom(0.0, 1.0) * 0xFFFFFFFF);
        loLo = (uint32) (uniformRandom(0.0, 1.0) * 0xFFFFFFFF);

		a = uint128((uint64(hiHi) << 32) + loHi, (uint64(hiLo) << 32) + loLo);
		b = uint128(0, 0);

        // Test multiplication against equivalent addition
        for(int j = 1; j < 10000; ++j) {        
            uint128 c(a.hi, a.lo);
            c *= uint128(0, j);
            b += a;
            testAssert(b == c);
        }

        // Test multiplication by 1
        b = a;
		a *= uint128(0, 1);
		testAssert(a == b);

        // Test addition of 0
        a += uint128(0, 0);
        testAssert(a == b);

        // Test left shift against equivalent addition
        uint128 c = a;
        c <<= 1;
		a += a;
        testAssert(a == c);

        // Test right shift against unsigned division. C and B should be equal unless the top bit of b was a 1.
        if(!(b.hi >> 63)) {
            c >>= 1;
            testAssert(c == b);
        }

        // Test multiplication by 2
		b *= uint128(0, 2);
		testAssert(a == b);

        // Test multiplication by 0
        a *= uint128(0, 0);
        testAssert(a == uint128(0, 0));
	}
}
