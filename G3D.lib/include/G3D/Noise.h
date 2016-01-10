/**
   \file Noise.h
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 G3D Library
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_Noise_h
#define G3D_Noise_h

#include "G3D/g3dmath.h"

namespace G3D {

/** 
  \brief 3D fixed point Perlin noise generator.

 Ported from Ken Perlin's Java INoise implementation. 

 \cite Copyright 2002 Ken Perlin
 \cite http://mrl.nyu.edu/~perlin/noise/

 Example:
 \code
    Noise n;
    GImage im(256, 256, 1);
    for (int y = 0; y < im.height(); ++y) {
        for (int x = 0; x < im.width(); ++x) {
            im.pixel1(x, y) = Color1unorm8(n.sampleUint8(x << 12, y << 12, 0));
        }
    }
    im.save("noise.png");

 \endcode

 \sa G3D::Random
*/
class Noise {
private:
    static int fadeArray[256];
    static int p[512];

    static int fade(int t) {
        const int t0 = fadeArray[t >> 8];
        const int t1 = fadeArray[min(255, (t >> 8) + 1)];
        return t0 + ( (t & 255) * (t1 - t0) >> 8 );
    }

    static double f(double t) { 
        return t * t * t * (t * (t * 6 - 15) + 10); 
    }

    void init();

    static int lerp(int t, int a, int b) { 
        return a + (t * (b - a) >> 12); 
    }

    static int grad(int hash, int x, int y, int z) {
        int h = hash&15, u = h<8?x:y, v = h<4?y:h==12||h==14?x:z;
        return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
    }

public:

    /** Not threadsafe */
    Noise() {
        init();
    }

    static Noise& common();

    /** 
        Returns numbers between -2^16 and 2^16.

        Arguments should be on the order of 2^16
        
        Threadsafe. */
    int sample(int x, int y, int z) {
        int X = (x >> 16) & 255;
        int Y = (y >> 16) & 255;
        int Z = (z >> 16) & 255;
        int N = 1 << 16;

        x &= N - 1; 
        y &= N - 1; 
        z &= N - 1;

        int u  = fade(x);
        int v  = fade(y);
        int w  = fade(z);
        int A  = p[X  ]+Y;
        int AA = p[A]+Z;
        int AB = p[A+1]+Z;
        int B  = p[X+1]+Y;
        int BA = p[B]+Z;
        int BB = p[B+1]+Z;

        return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x   , y   , z   ),  
                                    grad(p[BA  ], x-N , y   , z   )), 
                            lerp(u, grad(p[AB  ], x   , y-N , z   ),  
                                 grad(p[BB  ], x-N , y-N , z   ))),
                    lerp(v, lerp(u, grad(p[AA+1], x   , y   , z-N ),  
                                 grad(p[BA+1], x-N , y   , z-N )), 
                         lerp(u, grad(p[AB+1], x   , y-N , z-N ),
                              grad(p[BB+1], x-N , y-N , z-N ))));
    }

    /** Returns numbers on the range [0, 255].

        Arguments should be on the order of 2^16
        
        Threadsafe. */
    uint8 sampleUint8(int x, int y, int z) {
        const int v = sample(x, y, z);
        return (v + (1 << 16)) >> 9;
    }

    /** Returns numbers on the range [-1, 1] for a single octave of noise.
        Each octave adds 0.5 of the range of the previous one.  An infinite
        number of octaves is bounded by [-2, 2].

        Unique values occur at coordinates that are multiples of 2^16 for
        the lowest frequency octave.
        Between those the noise smoothly varies.
        
        Threadsafe. */
    float sampleFloat(int x, int y, int z, int numOctaves = 1);
};

}

#endif
