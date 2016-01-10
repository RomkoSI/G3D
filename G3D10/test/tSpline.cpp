#include <G3D/G3DAll.h>
#include "testassert.h"

// unit-interval spline
static void unitTests() {
    Spline<float> spline;

    spline.append(0.0, 5.0);
    spline.append(1.0, 10.0);
    spline.extrapolationMode = SplineExtrapolationMode::LINEAR;

    float d = spline.duration();
    testAssert(fuzzyEq(d, 1.0f));
    testAssert(spline.size() == 2);
    
    int i;
    float u;

    spline.computeIndex(0, i, u);
    testAssert(i == 0);
    testAssert(u == 0);

    spline.computeIndex(0.5, i, u);
    testAssert(i == 0);
    testAssert(fuzzyEq(u, 0.5f));

    spline.computeIndex(1, i, u);
    testAssert(i == 1);
    testAssert(u == 0);

    spline.computeIndex(-1, i, u);
    testAssert(i == -1);
    testAssert(u == 0);

    spline.computeIndex(-0.5, i, u);
    testAssert(i == -1);
    testAssert(fuzzyEq(u, 0.5f));

    // Cyclic tests
    spline.extrapolationMode = SplineExtrapolationMode::CYCLIC;

    spline.computeIndex(0, i, u);
    testAssert(i == 0);
    testAssert(u == 0);

    spline.computeIndex(0.5, i, u);
    testAssert(i == 0);
    testAssert(fuzzyEq(u, 0.5f));

    spline.computeIndex(1, i, u);
    testAssert(i == 1);
    testAssert(u == 0);

    spline.computeIndex(2, i, u);
    testAssert(i == 2);
    testAssert(u == 0);

    spline.computeIndex(1.5, i, u);
    testAssert(i == 1);
    testAssert(u == 0.5f);

    spline.computeIndex(-1, i, u);
    testAssert(i == -1);
    testAssert(u == 0);

    spline.computeIndex(-0.5, i, u);
    testAssert(i == -1);
    testAssert(fuzzyEq(u, 0.5f));
}


static void nonunitTests() {
    Spline<float> spline;

    spline.append(1.0, 5.0);
    spline.append(3.0, 10.0);
    spline.extrapolationMode = SplineExtrapolationMode::LINEAR;

    float d = spline.duration();
    testAssert(fuzzyEq(d, 2.0f));
    testAssert(spline.size() == 2);
    
    int i;
    float u;

    spline.computeIndex(1, i, u);
    testAssert(i == 0);
    testAssert(u == 0);

    spline.computeIndex(2, i, u);
    testAssert(i == 0);
    testAssert(fuzzyEq(u, 0.5f));

    spline.computeIndex(3, i, u);
    testAssert(i == 1);
    testAssert(u == 0);

    spline.computeIndex(-1, i, u);
    testAssert(i == -1);
    testAssert(u == 0);

    spline.computeIndex(0, i, u);
    testAssert(i == -1);
    testAssert(fuzzyEq(u, 0.5f));

    // Cyclic case
    spline.extrapolationMode = SplineExtrapolationMode::CYCLIC;
    spline.computeIndex(1, i, u);
    testAssert(i == 0);
    testAssert(u == 0);

    spline.computeIndex(2, i, u);
    testAssert(i == 0);
    testAssert(fuzzyEq(u, 0.5f));

    spline.computeIndex(3, i, u);
    testAssert(i == 1);
    testAssert(u == 0);

    float fi = spline.getFinalInterval();
    testAssert(fi == 2);

    spline.computeIndex(-1, i, u);
    testAssert(i == -1);
    testAssert(u == 0);

    spline.computeIndex(0, i, u);
    testAssert(i == -1);
    testAssert(fuzzyEq(u, 0.5f));
}

// Hard case: irregular intervals, cyclic spline
static void irregularTests() {
    Spline<float> spline;
    spline.extrapolationMode = SplineExtrapolationMode::CYCLIC;
    spline.append(1.0, 1.0);
    spline.append(2.0, 1.0);
    spline.append(4.0, 1.0);

    float fi = spline.getFinalInterval();
    testAssert(fuzzyEq(fi, 1.5f));

    testAssert(fuzzyEq(spline.duration(), 4.5f));

    float u;
    int i;

    spline.computeIndex(1.0, i, u);
    testAssert(i == 0);
    testAssert(fuzzyEq(u, 0));

    spline.computeIndex(2.0, i, u);
    testAssert(i == 1);
    testAssert(fuzzyEq(u, 0));

    spline.computeIndex(4.0, i, u);
    testAssert(i == 2);
    testAssert(fuzzyEq(u, 0));

    spline.computeIndex(5.5, i, u);
    testAssert(i == 3);
    testAssert(fuzzyEq(u, 0));

    spline.computeIndex(-0.5, i, u);
    testAssert(i == -1);
    testAssert(fuzzyEq(u, 0));

    spline.computeIndex(0.25, i, u);
    testAssert(i == -1);
    testAssert(fuzzyEq(u, 0.5f));

}

static void linearTest() {
    Spline<float> spline;

    spline.append(0.0, 0.0);
    spline.append(1.0, 1.0);
    spline.extrapolationMode = SplineExtrapolationMode::LINEAR;

    // Points on the line y=x
    int N = 11;
    for (int i = 0; i < N; ++i) {
        float t = i / (N - 1.0f);
        
        float v = spline.evaluate(t);
        
        testAssert(fuzzyEq(v, t));
    }

    // points on the line y = 1
    spline.control[0] = 1.0;
    spline.control[1] = 1.0;

    for (int i = 0; i < N; ++i) {
        float t = i / (N - 1.0f);
        
        float v = spline.evaluate(t);
        
        testAssert(fuzzyEq(v, 1.0f));
    }


    spline.time[0] = 0.0;
    spline.time[1] = 0.5;

    for (int i = 0; i < N; ++i) {
        float t = i / (N - 1.0f);
        
        float v = spline.evaluate(t);
        
        testAssert(fuzzyEq(v, 1.0f));
    }
}


static void curveTest() {
    Spline<float> spline;
    spline.extrapolationMode = SplineExtrapolationMode::LINEAR;

    spline.append(0, 0);
    spline.append(0.25, 0);
    spline.append(1.0, 1.0);

    float t, v;
    t = 1.0;    
    v = spline.evaluate(t);
    testAssert(fuzzyEq(v, 1.0f));

    t = 1.5;
    v = spline.evaluate(t);
//    testAssert(fuzzyEq(v, 1.66667));
}

void testSpline() {
    printf("Spline ");
    // Control point testing
    unitTests();
    nonunitTests();
    irregularTests();

    // Evaluate testing
    linearTest();
  
    curveTest();
    printf("passed\n");
}
