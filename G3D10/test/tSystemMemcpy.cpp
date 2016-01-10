#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;



void perfSystemMemcpy() {
    printf("----------------------------------------------------------\n");

    // Number of memory sizes to test
    static const int M = 8;

    //  Repeats per memory size
    static const int trials = 300;

    size_t size[M];
    for (int i = 0; i < M; ++i) {
		size[i] = 1024 * (size_t)::pow((float)(i + 1), 4);
    }

    printf("System::memcpy Performance:\n");
    printf("  Measured in cycles/kb at various copy sizes\n\n");
    uint64 native[M], g3d[M];

    for (int m = 0; m < M; ++m) {
        int n = (int)size[m];
        void* m1 = System::alignedMalloc(n, 16);
        void* m2 = System::alignedMalloc(n, 16);

        testAssertM((((long)m1) % 16) == 0, "Memory is not aligned correctly");
        testAssertM((((long)m2) % 16) == 0, "Memory is not aligned correctly");

        // First iteration just primes the system
        ::memcpy(m1, m2, n);
        System::beginCycleCount(native[m]);
            for (int j = 0; j < trials; ++j) {
                ::memcpy(m1, m2, n);
            }
        System::endCycleCount(native[m]);

        System::memcpy(m1, m2, n);
        System::beginCycleCount(g3d[m]);
            for (int j = 0; j < trials; ++j) {
                System::memcpy(m1, m2, n);
            }
        System::endCycleCount(g3d[m]);

        System::alignedFree(m1);
        System::alignedFree(m2);
    }


    printf("         Size       ");
    for (int i = 0; i < M; ++i) {
        printf("%6dk", (int)size[i] / 1024);
    }
    printf("\n");

    printf("    ::memcpy        ");
    for (int m = 0; m < M; ++m) {
        double k = trials * (double)size[m] / 1024;
        printf(" %6d", (int)(native[m] / k));
    }
    printf("\n");

    printf("    System::memcpy* ");
    for (int m = 0; m < M; ++m) {
        double k = trials * (double)size[m] / 1024;
        printf(" %6d", (int)(g3d[m] / k));
    }
    printf("\n        --------------------------------------------------\n");
    printf("    Outcome         ");
    for (int m = 0; m < M; ++m) {
        if (g3d[m] <= native[m] * 1.1) {
            printf("    ok ");
        } else {
            printf("   FAIL");
        }
    }
    printf("\n");    

    if (System::hasSSE2() && System::hasMMX()) {
        printf("      * MMX on this machine\n");
    } else if (System::hasSSE() && System::hasMMX()) {
        printf("      * MMX on this machine\n");
    } else {
        printf("      * memcpy on this machine\n");
    }
    printf("\n");

}


void testSystemMemcpy() {
    printf("System::memcpy ");
	static const int k = 50000;
	static uint8 a[k];
	static uint8 b[k];

	int i;
	
	for (i = 0; i < k; ++i) {
		a[i] = i & 255;
	}

	System::memcpy(b, a, k);

	for (i = 0; i < k; ++i) {
		testAssert(b[i] == (i & 255));
		testAssert(a[i] == (i & 255));
	}
    printf("passed\n");

}

