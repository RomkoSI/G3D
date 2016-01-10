#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

using G3D::uint32;

static void testAABoxCulledBy() {
    const uint32 b00000000 = 0;
    const uint32 b00000001 = 1;
    const uint32 b00000010 = 2;
    const uint32 b00000011 = 3;
    const uint32 b00000100 = 4;
    const uint32 b00000101 = 5;
    const uint32 b00000110 = 6;
    const uint32 b00000111 = 7;
    const uint32 b00001000 = 8;

    // Avoid unused variable warnings
    (void)b00000000;
    (void)b00000001;
    (void)b00000010;
    (void)b00000011;
    (void)b00000100;
    (void)b00000101;
    (void)b00000110;
    (void)b00000111;
    (void)b00001000;

    printf("AABox::culledBy\n");

    Array<Plane> planes;

    // Planes at +/- 1
    planes.append(Plane(Vector3(-1,0,0), Vector3(1,0,0)));
    planes.append(Plane(Vector3(1,0,0), Vector3(-1,0,0)));

    AABox box(Vector3(-0.5, 0, 0), Vector3(0.5, 1, 1));
    
    uint32 parentMask, childMask;
    int index = 0;
    bool culled;

    // Contained case
    parentMask = (uint32)-1; childMask = 0; index = 0;
    culled = box.culledBy(planes, index, parentMask, childMask);
    testAssert(index == -1);
    testAssert(! culled);
    testAssert(childMask == b00000000);

    // Positive straddle
    box = AABox(Vector3(0.5, 0, 0), Vector3(1.5, 1, 1));
    parentMask = (uint32)-1; childMask = 0; index = 0;
    culled = box.culledBy(planes, index, parentMask, childMask);
    testAssert(index == -1);
    testAssert(! culled);
    testAssert(childMask == b00000001);
    
    // Negative straddle
    box = AABox(Vector3(-1.5, 0, 0), Vector3(0.5, 1, 1));
    parentMask = (uint32)-1; childMask = 0; index = 0;
    culled = box.culledBy(planes, index, parentMask, childMask);
    testAssert(index == -1);
    testAssert(! culled);
    testAssert(childMask == b00000010);

    // Full straddle
    box = AABox(Vector3(-1.5, 0, 0), Vector3(1.5, 1, 1));
    parentMask = (uint32)-1; childMask = 0; index = 0;
    culled = box.culledBy(planes, index, parentMask, childMask);
    testAssert(index == -1);
    testAssert(! culled);
    testAssert(childMask == b00000011);

    // Negative culled 
    box = AABox(Vector3(-2.5, 0, 0), Vector3(-1.5, 1, 1));
    parentMask = (uint32)-1; childMask = 0; index = 0;
    culled = box.culledBy(planes, index, parentMask, childMask);
    testAssert(index == 1);
    testAssert(culled);

    // Positive culled 
    box = AABox(Vector3(1.5, 0, 0), Vector3(2.5, 1, 1));
    parentMask = (uint32)-1; childMask = 0; index = 0;
    culled = box.culledBy(planes, index, parentMask, childMask);
    testAssert(index == 0);
    testAssert(culled);

    shared_ptr<Camera> camera = Camera::create();
    camera->getClipPlanes(Rect2D::xywh(0,0,640,480), planes);
}

void testAABox() {
	printf("AABox ");

	testAABoxCulledBy();

	printf("passed\n");
}
