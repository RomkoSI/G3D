#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testSystemMemset() {
    printf("System::memset");
    {
        static const int k = 100;
	    static uint8 a[k];
	    
	    int i;

	    for (i = 0; i < k; ++i) {
		    a[i] = i & 255;
	    }

	    System::memset(a, 4, k);

	    for (i = 0; i < k; ++i) {
		    testAssert(a[i] == 4);
	    }
    }

    {
        // Test the internal testAssertions
        for (int N = 100; N < 10000; N += 137) {

            void* x = System::malloc(N);
            System::memset(x, 0, N);
            x = System::realloc(x, N * 2);
            System::memset(x, 0, N*2);
            System::free(x);
        }
    }



    printf(" passed\n");
}


void perfSystemMemset() {
    printf("----------------------------------------------------------\n");

    // Number of memory sizes to test
    static const int M = 8;

    //  Repeats per memory size
    static const int trials = 300;

    size_t size[M];
    for (int i = 0; i < M; ++i) {
		size[i] = 1024 * (size_t)::pow((float)(i + 1), 4);
    }

    printf("System::memset Performance:\n");
    printf("  Measured in cycles/kb at various target sizes\n\n");
    uint64 native[M], g3d[M];

    for (int m = 0; m < M; ++m) {
        int n = (int)size[m];
        void* m1 = System::alignedMalloc(n, 16);

        testAssertM((((long)m1) % 16) == 0, "Memory is not aligned correctly");

        // First iteration just primes the system
        ::memset(m1, 0, n);
        System::beginCycleCount(native[m]);
            for (int j = 0; j < trials; ++j) {
                ::memset(m1, 0, n);
            }
        System::endCycleCount(native[m]);

        System::memset(m1, 0, n);
        System::beginCycleCount(g3d[m]);
            for (int j = 0; j < trials; ++j) {
                System::memset(m1, 0, n);
            }
        System::endCycleCount(g3d[m]);

        System::alignedFree(m1);
    }


    printf("         Size       ");
    for (int i = 0; i < M; ++i) {
        printf("%6dk", (int)size[i] / 1024);
    }
    printf("\n");

    printf("    ::memset        ");
    for (int m = 0; m < M; ++m) {
        double k = trials * (double)size[m] / 1024;
        printf(" %6d", (int)(native[m] / k));
    }
    printf("\n");

    printf("    System::memset* ");
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
        printf("      * memset on this machine\n");
    }
    printf("\n");

}
