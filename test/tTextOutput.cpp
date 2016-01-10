#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void perfTextOutput() {
    printf("TextOutput\n");

    // printf
    {
        TextOutput t;
        char buf[2048];

        uint64 tt, tf, tp;
        const int N = 5000;

        System::beginCycleCount(tp);
        for (int i = 0; i < N; ++i){
            sprintf(buf, "%d, %d, %d\n", i, i + 1, i + 2);
        }
        System::endCycleCount(tp);

        String s;
        System::beginCycleCount(tf);
        for (int i = 0; i < N; ++i){
            s = format("%d, %d, %d\n", i, i + 1, i + 2);
        }
        System::endCycleCount(tf);

        System::beginCycleCount(tt);
        for (int i = 0; i < N; ++i){
            t.printf("%d, %d, %d\n", i, i + 1, i + 2);
        }
        System::endCycleCount(tt);
        t.commitString(s);

        int k = 3;
        printf(" Cycles to print int32\n");
        printf("   sprintf                    %g\n", (double)tp / (k * N));
        printf("   format                     %g\n", (double)tf / (k * N));
        printf("   TextOutput::printf         %g\n", (double)tt / (k * N));
        printf("\n");
    }
    printf("\n\n");
//    while(true);
}


