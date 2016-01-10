#include <G3D/G3DAll.h>
#include "testassert.h"

void testSpeedLoad() {
    debugPrintf("SpeedLoad...");
    {
        BinaryOutput b("speedload-test.bin", G3D_LITTLE_ENDIAN);
        SpeedLoad::writeHeader(b, "Chunk1");
        SpeedLoad::writeHeader(b, "Chunk2");
        SpeedLoad::writeHeader(b, "Chunk3");
        b.commit();
    }
    {
        BinaryInput b("speedload-test.bin", G3D_LITTLE_ENDIAN);
        SpeedLoad::readHeader(b, "Chunk1");
        SpeedLoad::readHeader(b, "Chunk2");
        SpeedLoad::readHeader(b, "Chunk3");
    }
    debugPrintf("Passed\n");
}
