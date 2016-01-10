#include "G3D/G3DAll.h"
#include "testassert.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testHugeBinaryIO() {
    printf("BinaryOutput Large Files\n");
	if (FileSystem::exists("huge.bin", false)) {
#if defined(G3D_WINDOWS)
        system("del huge.bin");
#elif defined(G3D_LINUX)
		system("rm huge.bin");
#elif defined(G3D_OSX)
		system("rm huge.bin");
#endif
    }

    size_t testSize = 1024 * 1024 * 600;

    size_t stepSize = 1024 * 1024 * 2;
    uint8* giantBuffer = new uint8[stepSize];
    testAssert(giantBuffer);

    {
        BinaryOutput b("huge.bin", G3D_LITTLE_ENDIAN);
        for (int i = 0; i < (int)testSize / (int)stepSize; ++i) {
            b.writeBytes(giantBuffer, (int)stepSize);
        }
        b.commit();
    }

    printf("BinaryInput Large Files\n");

    {
        BinaryInput b("huge.bin", G3D_LITTLE_ENDIAN);

        for (int i = 0; i < (int)testSize / (int)stepSize; ++i) {
            b.readBytes(giantBuffer, (int)stepSize);
        }
    }

    delete giantBuffer;

	if (FileSystem::exists("huge.bin", false)) {
#if defined(G3D_WINDOWS)
        system("del huge.bin");
#elif defined(G3D_LINUX)
		system("rm huge.bin");
#elif defined(G3D_OSX)
		system("rm huge.bin");
#endif
    }
}

static void testBitSerialization() {
    printf("Bit Serialization\n");
    uint8 x[100];

    {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);

        b.beginBits();
            b.writeBits(0, 1);
            b.writeBits(1, 1);
        b.endBits();

        b.commit(x);

        testAssert(x[0] == 2);
    }

    {
        BinaryInput b(x, 1, G3D_LITTLE_ENDIAN);
        b.beginBits();
            
            uint8 a = b.readBits(1);
            testAssert(a == 0);
            
            a = b.readBits(1);
            testAssert(a == 1);
        b.endBits();
    }

    {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
        b.beginBits();
            b.writeBits(0xF1234567, 32);
        b.endBits();

        b.commit(x);

        testAssert(x[0] == 0x67);
        testAssert(x[1] == 0x45);
        testAssert(x[2] == 0x23);
        testAssert(x[3] == 0xF1);
    }

    {
        BinaryInput b(x, 4, G3D_LITTLE_ENDIAN);
        b.beginBits();
            
            uint8 a = b.readBits(8);
            testAssert(a == 0x67);
            
            a = b.readBits(8);
            testAssert(a == 0x45);
            
            a = b.readBits(8);
            testAssert(a == 0x23);

            a = b.readBits(8);
            testAssert(a == 0xF1);

        b.endBits();
    }

    {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);

        b.beginBits();
            b.writeBits(0, 3);
            b.writeBits(3, 3);
            b.writeBits(4, 3);
            b.writeBits(7, 3);
        b.endBits();

        b.commit(x);
    }

    {
        BinaryInput b(x, 2, G3D_LITTLE_ENDIAN);
        b.beginBits();
            
            uint8 a = b.readBits(3);
            testAssert(a == 0);
            
            a = b.readBits(3);
            testAssert(a == 3);

            a = b.readBits(3);
            testAssert(a == 4);

            a = b.readBits(3);
            testAssert(a == 7);
        b.endBits();
    }

}


static void testCompression() {
    printf("BinaryInput & BinaryOutput\n");
    BinaryOutput f("out.t", G3D_LITTLE_ENDIAN);

    for (int i = 0; i < 100; ++i) {
        f.writeUInt32(1234);
        f.writeFloat64(1.234);
    }
    f.compress();
    f.commit();

    BinaryInput g("out.t", G3D_LITTLE_ENDIAN, true);
    for (int k = 0; k < 100; ++k) {
        uint32 i = g.readUInt32();
        testAssert(i == 1234); (void)i;
        double j = g.readFloat64();
        testAssert(j == 1.234); (void)j;
    }
}


static void measureSerializerPerformance() {
    Array<uint8> x;
    x.resize(1024);
    RealTime t0 = System::time();
    Matrix4 M(Matrix4::identity());
    
    for (int i = 0; i < 100; ++i) {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
        b.writeInt32(1);
        b.writeInt32(2);
        b.writeInt32(8);
        M.serialize(b);
        b.commit(x.getCArray());
    }
    RealTime reallocTime = (System::time() - t0) / 100.0;
    printf("BinaryOutput time with re-allocation: %gs\n", reallocTime);

    BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
    t0 = System::time();    
    for (int i = 0; i < 100; ++i) {
        b.writeInt32(1);
        b.writeInt32(2);
        b.writeInt32(8);
        M.serialize(b);
        b.commit(x.getCArray());
        b.reset();
    }
    RealTime resetTime = (System::time() - t0) / 100.0;
    printf("BinaryOutput time with BinaryOutput::reset: %gs\n\n", resetTime);
    
}


/** Measures the overhead of using BinaryInput and 
   testing for endian-ness, which in practice
   is rarely used.*/
static void measureOverhead() {
    static const int REPEAT = 10;
    static const int N = 1024 * 10;
    uint8 buffer[N * sizeof(float)];
    BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);

    float f = 3.2f;
    RealTime t0 = System::time();
    for (int j = 0; j < REPEAT; ++j) {
        bo.reset();
        for (int i = 0; i < N; ++i) {
            bo.writeFloat32(f);
            f += 0.1f;
        }
    }
    RealTime botime = System::time() - t0;


    t0 = System::time();
    for (int j = 0; j < REPEAT; ++j) {
        uint8* b = buffer;
        for (int i = 0; i < N; ++i) {
            *(float*)(b) = f;
            b += sizeof(float);
            f += 0.1f;
        }
    }
    RealTime buffertime = System::time() - t0;
    printf("BinaryOutput::writeFloat32 x 1e6:     %f s\n", botime / (REPEAT * N) * 1e6);
    printf("Raw memory buffer float write x 1e6:  %f s\n", buffertime / (REPEAT * N) * 1e6);
}


void perfBinaryIO() {
    measureOverhead();
    measureSerializerPerformance();
}


void testBasicSerialization() {
    Vector3 tmp(-100.0f, -10.0f, 2.0f);
    Vector3int16 tmp2(100, -10, 2);

    {

        BinaryOutput bo("outfile.bin", G3D_LITTLE_ENDIAN);
        tmp.serialize(bo);
        tmp2.serialize(bo);
        bo.commit();

    }
    {
        BinaryInput bi("outfile.bin", G3D_LITTLE_ENDIAN);
        Vector3 alpha;
        Vector3int16 alpha2;
        alpha.deserialize(bi);
        alpha2.deserialize(bi);
        testAssertM(alpha == tmp, format("%s should be %s \n", alpha.toString().c_str(), tmp.toString().c_str()));
        testAssertM(alpha2 == tmp2, format("%s should be %s \n", alpha2.toString().c_str(), tmp2.toString().c_str()));
    }

}

static void testStringSerialization() {
    
    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        String src = "Hello";
        bo.writeString(src);
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);
        String dst = bi.readString();

        testAssert(bo.size() == 6);
        testAssert(bi.hasMore() == false);
        testAssert(src.length() == dst.length());
        testAssert(src == dst);
    }

    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        String src = "Hello";
        bo.writeString(src, (int)src.length());
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);
        String dst = bi.readString(src.length());

        testAssert(bo.size() == src.length());
        testAssert(bi.hasMore() == false);
        testAssert(src.length() == dst.length());
        testAssert(src == dst);
    }

    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        String src = "Hello";
        bo.writeBytes(src.c_str(), src.length());
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);
        String dst = bi.readString();

        testAssert(bo.size() == 5);
        testAssert(bi.hasMore() == false);
        testAssert(src.length() == dst.length());
        testAssert(src == dst);
    }

    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        String src = "Hello";
        bo.writeBytes(src.c_str(), src.length());
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);
        String dst = bi.readString(src.length());

        testAssert(bo.size() == 5);
        testAssert(bi.hasMore() == false);
        testAssert(src.length() == dst.length());
        testAssert(src == dst);
    }

    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        String src = "Hello";
        bo.writeUInt32((uint32)src.length() + 1);
        bo.writeString(src);
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);
        String dst = bi.readString32();

        testAssert(bo.size() == 10);
        testAssert(bi.hasMore() == false);
        testAssert(src.length() == dst.length());
        testAssert(src == dst);
    }

    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        String src = "Hello\n";
        bo.writeString(src);
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);
        String dst = bi.readStringNewline();

        testAssert(bo.size() == 7);
        testAssert(dst == "Hello");
        testAssert(bi.hasMore() == true); // did not consume trailing NULL since it is after newline
        testAssert(bi.readString() == String()); // should return empty string since last character is NULL
    }

    {
        uint8 data[1024];

        BinaryOutput bo("<memory>", G3D_LITTLE_ENDIAN);
        bo.writeString("Hello\n");
        bo.writeString("Hello2\r\n\n");
        bo.commit(data);

        BinaryInput bi(data, bo.size(), G3D_LITTLE_ENDIAN);

        String dest = bi.readStringNewline(); // read up to \n and consume \n leaving NULL
        testAssert(dest == "Hello"); 

        dest = bi.readStringNewline(); // consume NULL
        testAssert(dest == String()); 

        dest = bi.readStringNewline(); // read up to \r\n and consume \r\n leaving next \n
        testAssert(dest == "Hello2"); 

        dest = bi.readStringNewline(); // consume \n
        testAssert(dest == String()); 

        dest = bi.readStringNewline(); // consume NULL
        testAssert(dest == String()); 

        testAssert(bi.hasMore() == false);
    }

}

void testBinaryIO() {
    testStringSerialization();
    testBasicSerialization();
    testBitSerialization();
    testCompression();
}
