/**
   This is an updated version of the OpenGL starter code for the book
   Computer Graphics: Principles and Practice 3rd edition
   http://cgpp.net

   See also minGlut2, minGlutGLSL, and starter projects in G3D.

   This code sample shows how to use GLUT to initialize OpenGL.  This
   provides fewer features and much less support than the regular G3D
   initialization, but provides a convenient minimal base when
   tracking down GPU driver bugs or other issues for which you would
   like to rule out G3D's interactions.

   The provided Image class allows simple setPixel() access and can be
   saved to disk or shown on screen (both gamma corrected) for
   debugging.  You can also make direct OpenGL calls.

   Note that this project requires linking to GLUT, which is not
   formally distributed with G3D but is part of OS X and most Linux
   distributions.  The Windows version of GLUT is included in this
   directory.
 */
#include <G3D/G3D.h>      // Just for the linker pragmas, Color3, and Vector2
#include <GLG3D/GLG3D.h>  // Just for the linker pragmas
#include "supportclasses.h"
#ifdef G3D_64BIT
#   error This project does not support 64-bit builds because it has no 64-bit GLUT.
#endif

// Make Microsoft Windows programs start from the main() entry point
G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    // Sample program
    ::Image im(200, 300);
    for (int i = 0; i < im.width(); ++i) {
        for (int j = 0; j < im.height(); ++j) {
            // Checkerboard gradient
            im.set(i, j, (((i + j) & 1) == 0) ? Color3::white() * float(j) / float(im.height()) : Color3::black());
        }
    }

    // Save to disk:
    im.save("output.ppm");

    // Show on screen (never returns):
    im.display();

    return 0;
}
