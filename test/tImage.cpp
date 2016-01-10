#include "G3D/G3DAll.h"
#include "testassert.h"

static void testImageLoading() {
    // Perform basic image file loading tests
    shared_ptr<Image> img = Image::fromFile("ImageTest/test-image.bmp");
    testAssert(notNull(img) && img->format() == ImageFormat::RGB8());
    img = Image::fromFile("ImageTest/test-image.png");
    testAssert(notNull(img) && img->format() == ImageFormat::RGB8());
    img = Image::fromFile("ImageTest/test-image.jpg");
    testAssert(notNull(img) && img->format() == ImageFormat::RGB8());
    img = Image::fromFile("ImageTest/test-image.tga");
    testAssert(notNull(img) && img->format() == ImageFormat::RGB8());
    img = Image::fromFile("ImageTest/test-image.exr");
    testAssert(notNull(img) && img->format() == ImageFormat::RGBA32F());
}

void testImage() {

    printf("Image  ");

    // Test loading image files
    testImageLoading();

    shared_ptr<Image> im = Image::create(10, 10, ImageFormat::RGB32F());

    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            im->set(x, y, Color3((float)x, y/10.0f, 0.5f));
        }
    }
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            Color3 c((float)x, y/10.0f, 0.5f);
            Color3 d = im->get<Color3>(x, y);
            testAssert(c.fuzzyEq(d));
        }
    }

    printf("passed\n");
}
