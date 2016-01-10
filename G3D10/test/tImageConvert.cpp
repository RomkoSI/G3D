#include "G3D/G3DAll.h"
#include "testassert.h"

static void printBoard(const Color3unorm8* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
	if (b[i] == Color3unorm8::zero())
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

static void printBoard(const Color4unorm8* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
	if (Color3unorm8(b[i].r,b[i].g,b[i].b) == Color3unorm8::zero())
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

static void printBoard(const Color3* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
        if (b[i] == Color3::zero())
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

static void printBoard(const Color4* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
        if (b[i].rgb() == Color3::zero()) {
	        printf("o ");
        } else {
	        printf("x ");
        }
        if ( (i+1) % 8 == 0) {
	        printf("\n");
        }
    }
    printf("\n");
}

#define RECONCAST reinterpret_cast<const void*>
#define RECAST reinterpret_cast<void*>



void testImageConvert() {

    printf("G3D::ImageFormat  ");

    // Set up the checkerboard

    const int S = 8;
    Color3   rgb32f[S * S];
    Color3  _rgb32f[S * S];
    const Color3 black(Color3::black());
    const Color3 white(Color3::white());

    for (int i = 0; i < S * S; ++i) {
	if ( isEven(i/8 + i)) {
	    rgb32f[i] = black;
	} else {
	    rgb32f[i] = white;
	}
    }


    Color3unorm8   rgb8[S * S];
    Color3unorm8  _rgb8[S * S];

    Color3unorm8   bgr8[S * S];
    Color3unorm8  _bgr8[S * S];

    Color4        rgba32f[S * S];;
    Color4       _rgba32f[S * S];

    // rgb32f <--> rgba32f
    Array<const void*> input;
    Array<void*> output;

    input.append(RECONCAST(rgb32f));
    output.append(RECAST(rgba32f));
    ImageFormat::convert(input, S, S, ImageFormat::RGB32F(), 0,
			 output, ImageFormat::RGBA32F(), 0, false);
    {
        input.clear();
        output.clear();
        input.append(RECONCAST(rgba32f));
        output.append(RECAST(rgb8));

	    // rgba32f <--> rgb8
	    ImageFormat::convert(input, S, S, ImageFormat::RGBA32F(), 0,
			         output, ImageFormat::RGB8(), 0, false);
	    {
            input.clear();
            output.clear();
            input.append(RECONCAST(rgb8));
            output.append(RECAST(bgr8));

	        // rgb8 <--> bgr8
	        ImageFormat::convert(input, S, S, ImageFormat::RGB8(), 0,
				     output, ImageFormat::BGR8(), 0, false);

            input.clear();
            output.clear();
            input.append(RECONCAST(bgr8));
            output.append(RECAST(_rgb8));

	        ImageFormat::convert(input, S, S, ImageFormat::BGR8(), 0,
				     output, ImageFormat::RGB8(), 0, false);
	    }
        input.clear();
        output.clear();
        input.append(RECONCAST(_rgb8));
        output.append(RECAST(_rgba32f));

	    ImageFormat::convert(input, S, S, ImageFormat::RGB8(), 0,
			         output, ImageFormat::RGBA32F(), 0, false);
    }
    input.clear();
    output.clear();
    input.append(RECONCAST(_rgba32f));
    output.append(RECAST(_rgb32f));

    ImageFormat::convert(input, S, S, ImageFormat::RGBA32F(), 0,
			 output, ImageFormat::RGB32F(), 0, false);



    // Compare

    for (int i = 0; i < 64; ++i) {
	if (Color3(rgb32f[i]) != Color3(_rgb32f[i])) {
	    printf("No match at position i = %d \n", i);
	    printf("failed. \n");
	}
    }



    printf("passed\n");
}


void perfTest() {
    printf("ImageFormat::convert performance test is not implemented.");

}
