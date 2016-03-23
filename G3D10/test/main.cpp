/**
 \file test/main.cpp

 This file runs unit conformance and performance tests for G3D.  
 To write a new test, add a file named t<class>.cpp to the project
 and provide two entry points: test<class> and perf<class> (even if they are
 empty).

 Call those two methods from main() in main.cpp.

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2002-01-01
 \edited  2013-01-06
 */

#include "G3D/G3D.h"
#include "GLG3D/GLG3D.h"
#ifdef main
#    undef main
#endif

using namespace G3D;
#include <iostream>

#ifdef G3D_WINDOWS
#    include "conio.h"
#else
#    include "curses.h"
#endif
#include <string>
#include "testassert.h"

using G3D::uint64;
using G3D::uint32;
using G3D::uint8;

//#define RUN_SLOW_TESTS

// Forward declarations
void testImageConvert();
void testImage();

void perfArray();
void testArray();
void testSmallArray();

void testFileSystem();

void testMatrix();
void testMatrix4();
void testMatrix3();
void perfMatrix3();

void testSpeedLoad();

void testZip();

void testuint128();

void testCollisionDetection();
void perfCollisionDetection();

void testWeakCache();
void testCallback();

void testSpline();

void teststring();

void testGChunk();

void testQuat();

void perfKDTree();
void testKDTree();

void testSphere();

void testAABox();

void testReliableConduit(NetworkDevice*);

void perfSystemMemcpy();
void testSystemMemcpy();

void perfSystemMemset();
void testSystemMemset();

void testMap2D();

void testReferenceCount();

void testRandom();

void perfTextOutput();

void testMeshAlgTangentSpace();

void perfQueue();
void testQueue();

void testBinaryIO();
void testHugeBinaryIO();
void perfBinaryIO();

void testTextInput();
void testTextInput2();

void testTable();
void testAdjacency();

void perfTable();

void testAtomicInt32();

void testCoordinateFrame();

void testGThread();

void testfilter();

void testAny();


void testunorm8();
void testsnorm8();
void testunorm16();
void testsnorm16();


void testPointHashGrid();
void perfPointHashGrid();

void perfHashTrait();

void testFullRender(bool generateGoldStandard);

void testTableTable() {

    // Test making tables out of tables
    typedef Table<String, int> StringTable;
    Table<int, StringTable> table;

    table.set(3, StringTable());
    table.set(0, StringTable());
    table[3].set("Hello", 3);
}

void testCamera() {
    printf("Camera...");
    shared_ptr<Camera> camera = Camera::create();
    camera->setFrame(CFrame());
    camera->setFieldOfView(toRadians(90), FOVDirection::HORIZONTAL);
    camera->setNearPlaneZ(-1);
    camera->setFarPlaneZ(-100);

    Rect2D viewport = Rect2D::xywh(0,0,200,100);
    Array<Plane> plane;
    camera->getClipPlanes(viewport, plane);
    testAssertM(plane.size() == 6, "Missing far plane");
    testAssertM(plane[0].fuzzyContains(Vector3(0,0,-1)), plane[0].center().toString());
    testAssertM(plane[0].normal() == Vector3(0,0,-1), plane[0].normal().toString());

    testAssertM(plane[5].fuzzyContains(Vector3(0,0,-100)), plane[1].center().toString());
    testAssertM(plane[5].normal() == Vector3(0,0,1), plane[1].normal().toString());

    testAssertM(plane[1].normal().fuzzyEq(Vector3(-1,0,-1).direction()), plane[1].normal().toString());
    testAssertM(plane[2].normal().fuzzyEq(Vector3(1,0,-1).direction()), plane[2].normal().toString());

    // top
    testAssertM(plane[3].normal().fuzzyEq(Vector3(0,-0.894427f,-0.447214f).direction()), plane[3].normal().toString());
    testAssertM(plane[4].normal().fuzzyEq(Vector3(0,0.894427f,-0.447214f).direction()), plane[4].normal().toString());

    Vector3 ll, lr, ul, ur;
    camera->getNearViewportCorners(viewport, ur, ul, ll, lr);
    testAssertM(ur == Vector3(1, 0.5, -1), ur.toString());
    testAssertM(lr == Vector3(1, -0.5, -1), lr.toString());
    testAssertM(ll == Vector3(-1, -0.5, -1), ll.toString());
    testAssertM(ul == Vector3(-1, 0.5, -1), ul.toString());
    printf("passed\n");
}


void testConvexPolygon2D() {
    printf("ConvexPolygon2D\n");
    Array<Vector2> v;
    v.append(Vector2(0, 0), Vector2(1,1), Vector2(2, 0));
    ConvexPolygon2D C(v);
    testAssert(! C.contains(Vector2(10, 2)));
    testAssert(C.contains(Vector2(1, 0.5)));
    printf("  passed\n");
}

void testBox2D() {
    printf("Box2D\n");
    Box2D b(Vector2(0,0), Vector2(2,3));
    testAssert(b.contains(Vector2(0,0)));
    testAssert(b.contains(Vector2(2,3)));
    testAssert(b.contains(Vector2(1,1.5)));
    testAssert(! b.contains(Vector2(-1,1.5)));
    testAssert(! b.contains(Vector2(3,1.5)));
    testAssert(! b.contains(Vector2(1,-1.5)));
    testAssert(! b.contains(Vector2(1,4)));
}


void testWildcards() {
    printf("filenameContainsWildcards\n");
    testAssert(!filenameContainsWildcards("file1.exe"));
	testAssert(filenameContainsWildcards("file?.exe"));
	testAssert(filenameContainsWildcards("f*.exe"));
	testAssert(filenameContainsWildcards("f*.e?e"));
	testAssert(filenameContainsWildcards("*1.exe"));
	testAssert(filenameContainsWildcards("?ile1.exe"));
}

void testFuzzy() {
    printf("Fuzzy Comparisons\n");
    Vector3 v(0.00124764f, -0.000569403f, 0.002096f);
    testAssert(! v.isZero());

    Vector3 z(0.00000001f, -0.000000001f, 0.0000000001f);
    testAssert(z.isZero());
}


void testBox() {
    printf("Box\n");
    Box box = Box(Vector3(0,0,0), Vector3(1,1,1));

    testAssert(box.contains(Vector3(0,0,0)));
    testAssert(box.contains(Vector3(1,1,1)));
    testAssert(box.contains(Vector3(0.5,0.5,0.5)));
    testAssert(! box.contains(Vector3(1.5,0.5,0.5)));
    testAssert(! box.contains(Vector3(0.5,1.5,0.5)));
    testAssert(! box.contains(Vector3(0.5,0.5,1.5)));
    testAssert(! box.contains(-Vector3(0.5,0.5,0.5)));
    testAssert(! box.contains(-Vector3(1.5,0.5,0.5)));
    testAssert(! box.contains(-Vector3(0.5,1.5,0.5)));
    testAssert(! box.contains(-Vector3(0.5,0.5,1.5)));


    /*

      2--------3
     / :      /|
    /  :     / |
   6--------7  |
   |   :    |  |
   |   0....|..1
   | /      | /
   |/       |/
   4--------5

    y  
    ^ 
    |
    |-->x
  z/
  
  */

    Vector3 v0, v1, v2, v3, n1, n2;

    v0 = box.corner(0);
    v1 = box.corner(1);
    v2 = box.corner(2);
    v3 = box.corner(3);

    testAssert(v0 == Vector3(0,0,0));
    testAssert(v1 == Vector3(1,0,0));
    testAssert(v2 == Vector3(0,1,0));
    testAssert(v3 == Vector3(1,1,0));

    Vector3 n[2] = {Vector3(0,0,-1), Vector3(1,0,0)};
    (void)n;

    int i;
    for (i = 0; i < 2; ++i) {
        box.getFaceCorners(i, v0, v1, v2, v3);
        n1 = (v1 - v0).cross(v3 - v0);
        n2 = (v2 - v1).cross(v0 - v1);

        testAssert(n1 == n2);
        testAssert(n1 == n[i]);
    }

}



void testAABoxCollision() {
    printf("intersectionTimeForMovingPointFixedAABox\n");

    Vector3 boxlocation, aaboxlocation, normal;

    for (int i = 0; i < 1000; ++i) {

        Vector3 pt1 = Vector3::random() * uniformRandom(0, 10);
        Vector3 vel1 = Vector3::random();

        Vector3 low = Vector3::random() * 5;
        Vector3 extent(uniformRandom(0,4), uniformRandom(0,4), uniformRandom(0,4));
        AABox aabox(low, low + extent);
        Box   box = aabox;

        double boxTime = CollisionDetection::collisionTimeForMovingPointFixedBox(
            pt1, vel1, box, boxlocation, normal);
        (void)boxTime;

        double aaTime = CollisionDetection::collisionTimeForMovingPointFixedAABox(
            pt1, vel1, aabox, aaboxlocation);
        (void)aaTime;

        Ray ray = Ray::fromOriginAndDirection(pt1, vel1);
        double rayboxTime = ray.intersectionTime(box);
        (void)rayboxTime;

        double rayaaTime = ray.intersectionTime(aabox);
        (void)rayaaTime;

        testAssert(fuzzyEq(boxTime, aaTime));
        if (boxTime < finf()) {
            testAssert(boxlocation.fuzzyEq(aaboxlocation));
        }

        testAssert(fuzzyEq(rayboxTime, rayaaTime));
    }
}


void testPlane() {
    printf("Plane\n");
    {
        Plane p(Vector3(1, 0, 0),
                Vector3(0, 1, 0),
                Vector3(0, 0, 0));

        Vector3 n = p.normal();
        testAssert(n == Vector3(0,0,1));
    }

    {
        Plane p(Vector3(4, 6, .1f),
                Vector3(-.2f, 6, .1f),
                Vector3(-.2f, 6, -.1f));

        Vector3 n = p.normal();
        testAssert(n.fuzzyEq(Vector3(0,-1,0)));
    }

    {
        Plane p(Vector4(1,0,0,0),
                Vector4(0,1,0,0),
                Vector4(0,0,0,1));
        Vector3 n = p.normal();
        testAssert(n.fuzzyEq(Vector3(0,0,1)));
    }

    {
        Plane p(
                Vector4(0,0,0,1),
                Vector4(1,0,0,0),
                Vector4(0,1,0,0));
        Vector3 n = p.normal();
        testAssert(n.fuzzyEq(Vector3(0,0,1)));
    }

    {
        Plane p(Vector4(0,1,0,0),
                Vector4(0,0,0,1),
                Vector4(1,0,0,0));
        Vector3 n = p.normal();
        testAssert(n.fuzzyEq(Vector3(0,0,1)));
    }
}



class A {
public:
    int x;

    A() : x(0) {
        printf("Default constructor\n");
    }

    A(int y) : x(y) {
        printf("Construct %d\n", x);
    }

    A(const A& a) : x(a.x) {
        printf("Copy %d\n", x);
    }

    A& operator=(const A& other) {
        printf("Assign %d\n", other.x);
        x = other.x;
        return *this;
    }

    ~A() {
        printf("Destruct %d\n", x);
    }
};





void measureNormalizationPerformance() {
    printf("----------------------------------------------------------\n");
    uint64 raw = 0, opt = 0, overhead = 0;
    int n = 1024 * 1024;

    double y;
    Vector3 x = Vector3(10,-20,3);

    int i, j;

    for (j = 0; j < 2; ++j) {
        x = Vector3(10,-20,3);
		y = 0;
        System::beginCycleCount(overhead);
        for (i = n - 1; i >= 0; --i) {
            x.z = (float)i;
            y += x.z;
        }
        System::endCycleCount(overhead);
    }

    x = Vector3(10,-20,3);
    y = 0;
    System::beginCycleCount(raw);
    for (i = n - 1; i >= 0; --i) {
        x.z = (float)i;
        y += x.direction().z;
        y += x.direction().z;
        y += x.direction().z;
    }
    System::endCycleCount(raw);
    
    x = Vector3(10,-20,3);
    y = 0;
    System::beginCycleCount(opt);
    for (i = n - 1; i >= 0; --i) {
        x.z = (float)i;
        y += x.fastDirection().z;
        y += x.fastDirection().z;
        y += x.fastDirection().z;
    }
    System::endCycleCount(opt);

    double r = (double)raw;
    double o = (double)opt;
    double h = (double)overhead;

    printf("%g %g %g\n", r-h, o-h, h);

    printf("Vector3::direction():               %d cycles\n", (int)((r-h)/(n*3.0)));
    printf("Vector3::fastDirection():           %d cycles\n", (int)((o-h)/(n*3.0)));

}



void testFloat() {
    printf("float...");
    /* changed from "nan" by ekern.  does this work on windows? */
    double x = nan();
    bool b1  = (x < 0.0);
    bool b2  = (x >= 0.0);
    bool b3  = !(b1 || b2);
    (void)b1;
    (void)b2;
    (void)b3;
    testAssert(isNaN(nan()));
    testAssert(! isNaN(4));
    testAssert(! isNaN(0));
    testAssert(! isNaN(inf()));
    testAssert(! isNaN(-inf()));
    testAssert(! isFinite(nan()));
    testAssert(! isFinite(-inf()));
    testAssert(! isFinite(inf()));
    testAssert(isFinite(0.0f));
		    
    printf("  passed\n");
}


void testglFormatOf() {
    printf("glFormatOf...");

    testAssert(glFormatOf(Color3) == GL_FLOAT);
    testAssert(glFormatOf(Color3unorm8) == GL_UNSIGNED_BYTE);
    testAssert(glFormatOf(Vector3int16) == GL_SHORT);
    testAssert(glFormatOf(float) == GL_FLOAT);
    testAssert(glFormatOf(int16) == GL_SHORT);
    testAssert(glFormatOf(int32) == GL_INT);

    testAssert(sizeOfGLFormat(GL_FLOAT) == 4);
    printf("passed\n");
}



void testSwizzle() {
    Vector4 v1(1,2,3,4);
    Vector2 v2;

    v2 = v1.xy() + v1.yz();
}


void testSphere() {
    printf("Sphere...");
    Sphere a(Vector3(0,3,0), 2);
    Sphere b(Vector3(0,2,0), 0.5f);

    testAssert(a.contains(b));
    testAssert(! b.contains(a));

    Sphere s = a;
    s.merge(b);
    testAssert(s == a);

    Sphere c(Vector3(1,0,0), 2);
    s = a;
    s.merge(c);
    testAssert(s.contains(a));
    testAssert(s.contains(c));

    printf("passed\n");
}



void testCoordinateFrame() {
    printf("CoordinateFrame ");

    {
        // Easy case
        CoordinateFrame c;
        c.lookAt(Vector3(-1, 0, -1));
        float h = c.getHeading();
        testAssert(fuzzyEq(h, G3D::pif() / 4));
        (void)h;
    }

    // Test getHeading at a variety of angles
    for (int i = -175; i <= 175; i += 5) {
        CoordinateFrame c;
        float h = c.getHeading();
        testAssert(h == 0);

        c.rotation = Matrix3::fromAxisAngle(Vector3::unitY(), toRadians(i));

        h = c.getHeading();
        testAssert(fuzzyEq(h, toRadians(i)));
    }

    printf("passed\n");
}

void measureRDPushPopPerformance(RenderDevice* rd) {
    uint64 identityCycles;

    int N = 500;
    rd->pushState();
    rd->popState();

    System::beginCycleCount(identityCycles);
    for (int i = 0; i < N; ++i) {
        rd->pushState();
        rd->popState();
    }
    System::endCycleCount(identityCycles);

    printf("RenderDevice::push+pop:             %g cycles\n", identityCycles / (double)N);
}

void testGLight() {
    shared_ptr<Light> L = Light::point("Light", Vector3(1,2,3), Color3::white(), 1,0,0);
    Sphere s;
    
    s = L->effectSphere();
    testAssert(s.contains(Vector3(1,2,3)));
    testAssert(s.contains(Vector3(0,0,0)));
    testAssert(s.contains(Vector3(100,100,100)));

    {
        shared_ptr<Light> L = Light::point("Light", Vector3(1,2,3), Color3::white(), 1,0,1);
        Sphere s;
        
        s = L->effectSphere();
        testAssert(s.contains(Vector3(1,2,3)));
        testAssert(s.contains(Vector3(1,1,3)));
        testAssert(! s.contains(Vector3(100,100,100)));
    }
}


void testLineSegment2D() {
    LineSegment2D A = LineSegment2D::fromTwoPoints(Vector2(1,1), Vector2(2,2));
    LineSegment2D B = LineSegment2D::fromTwoPoints(Vector2(2,1), Vector2(1,2));
    LineSegment2D C = LineSegment2D::fromTwoPoints(Vector2(2,1), Vector2(3,-1));
    LineSegment2D D = LineSegment2D::fromTwoPoints(Vector2(1,1.2f), Vector2(2,1.2f));

    Vector2 i0 = A.intersection(B);
    testAssert(i0.fuzzyEq(Vector2(1.5f, 1.5f)));

    Vector2 i1 = A.intersection(C);
    testAssert(i1 == Vector2::inf());

    Vector2 i2 = D.intersection(A);
    testAssert(i2.fuzzyEq(Vector2(1.2f, 1.2f)));
}


void perfHashTrait() {
    printf("Hash functions for Vector3:\n");

    const int N = 1000000;
    const Vector3 v(100, 32, 0.11f);
    {
        const RealTime start = System::time();
        size_t h = 0;
        for (int i = 0; i < N; ++i) {
            h += v.hashCode();
        }
        (void)h;
        printf("Vector3::hashCode:  %f\n", System::time() - start);
    }
    {
        const RealTime start = System::time();
        size_t h = 0;
        for (int i = 0; i < N; ++i) {
            h += Crypto::crc32(&v, sizeof(v));
        }
        (void)h;
        printf("Crypto::crc32:  %f\n", System::time() - start);
    }
    {
        const RealTime start = System::time();

        size_t h = 0;
        for (int i = 0; i < N; ++i) {
            h += Crypto::md5(&v, sizeof(v))[0];
        }
        (void)h;
        printf("Crypto::md5:  %f\n", System::time() - start);
    }
    {
        const RealTime start = System::time();

        size_t h = 0;
        for (int i = 0; i < N; ++i) {
            Vector4 w(v.x, v.y, v.z, 0);
            h += HashTrait<uint128>::hashCode(*(uint128*)&w);
        }
        (void)h;
        printf("HashTrait<uint128>:  %f\n", System::time() - start);
    }
}

void testCompassDirection() {
    printf("CompassDirection...");
    CompassDirection a(90);
    CompassDirection b(110);
    testAssert((a - b).compassDegrees() == -20); 
    testAssert((b - a).compassDegrees() ==  20); 

    a = CompassDirection(30);
    b = CompassDirection(-30);
    testAssert((b - a).compassDegrees() == -60); 
    testAssert((a - b).compassDegrees() == 60); 

    testAssert((-b).value() == 30); 
    testAssert(CompassDirection(365).value() == 5); 
    testAssert(CompassDirection(355).value() == 355); 
    printf("passed\n");
}

int main(int argc, char* argv[]) {
    bool generateGoldStandard = false;
    if (argc > 1) {
        const String flag = argv[1];
        generateGoldStandard = (flag == "--override");
    }

    char x[2000];
    getcwd(x, sizeof(x));
    
    testAssertM(FileSystem::exists("apiTest.zip", false), 
                 format("Tests are being run from the wrong directory.  cwd = %s", x));

    RenderDevice* renderDevice = NULL;

    G3D::String s;
    System::describeSystem(s);
    printf("%s\n", s.c_str());

    NetworkDevice::instance()->describeSystem(s);
    printf("%s\n", s.c_str());


    OSWindow::Settings settings;
    settings.width = 800;
    settings.height = 600;
    settings.alphaBits = 0;
    settings.rgbBits = 8;
    settings.stencilBits = 0;
    settings.msaaSamples = 1;
        

#    ifndef _DEBUG
        printf("Performance analysis:\n\n");

        perfSystemMemcpy();
        perfSystemMemset();
        printf("%s\n", System::mallocStatus().c_str());

        perfArray();

        perfBinaryIO();

        perfTable();

        perfHashTrait();

        perfCollisionDetection();

        perfQueue();

        perfMatrix3();

        perfTextOutput();

        measureNormalizationPerformance();

        if (! renderDevice) {
            renderDevice = new RenderDevice();
        }
    
        renderDevice->init(settings);

        if (renderDevice) {
            renderDevice->describeSystem(s);
            printf("%s\n", s.c_str());
        }

        perfPointHashGrid();

        measureRDPushPopPerformance(renderDevice);
        
        perfKDTree();

        if (renderDevice) {
            renderDevice->cleanup();
            delete renderDevice;
            renderDevice = NULL;
        }

#ifndef G3D_OSX
        testFullRender(generateGoldStandard);
#endif

#       ifdef G3D_WINDOWS
            // Pause so that we can see the values in the debugger
//	        getch();
#       endif

#   else

    printf("\n\nTests:\n\n");

    testCompassDirection();

    testunorm16();
    testunorm8();
    testsnorm8();
    testsnorm16();

    testFloat();

    teststring();

    testImage();

    testMatrix();

    testAny();

    testBinaryIO();

    testSpeedLoad();

    testReliableConduit(NetworkDevice::instance());

    testFileSystem();

    testCollisionDetection();  

    testTextInput();
    testTextInput2();
    printf("  passed\n");

    testSphere();

    testImageConvert();

    testLineSegment2D();

    if (! renderDevice) {
        renderDevice = new RenderDevice();
        renderDevice->init(settings);
    }

    if (renderDevice) {
        testKDTree();
        testGLight();
    }

    if (renderDevice) {
        renderDevice->cleanup();
        delete renderDevice;
        renderDevice = NULL;
    }

    testZip();

    testMap2D();

    testfilter();

    testArray();

    testSmallArray();

    testSpline();

    testMatrix3();
    testMatrix4();

    testTable();

    testTableTable();  

    testCoordinateFrame();

    testQuat();

    testReferenceCount();

    testAtomicInt32();

    testGThread();
    
    testWeakCache();
    
    testSystemMemset();

    testSystemMemcpy();

    testuint128();

    testQueue();

    testMeshAlgTangentSpace();

    testConvexPolygon2D();

    testPlane();
    printf("  passed\n");

    testAABox();

    testRandom();
    printf("  passed\n");

    testAABoxCollision();
    printf("  passed\n");
    testAdjacency();
    printf("  passed\n");
    testWildcards();
    printf("  passed\n");

    
    testRandom();

    testFuzzy();
    printf("  passed\n");

    testBox();
    printf("  passed\n");

    testBox2D();
    printf("  passed\n");

    testglFormatOf();
    printf("  passed\n");
    testSwizzle();

    testCamera();

    testCallback();

    testPointHashGrid();

#   ifdef RUN_SLOW_TESTS
        testHugeBinaryIO();
        printf("  passed\n");
#   endif

    printf("%s\n", System::mallocStatus().c_str());
    System::resetMallocPerformanceCounters();

    printf("\nAll tests succeeded.\n");
#endif

    if (renderDevice) {
        renderDevice->cleanup();
        delete renderDevice;
    }
    
    NetworkDevice::cleanup();

    return 0;
}

