/**
\file GLG3D/source/ArticulatedModel_hair.cpp

\author Michael Mara, http://illuminationcodified.com
\created 2015-09-02
\edited  2015-09-02

Copyright 2000-2015, Morgan McGuire.
All rights reserved.
*/
#include <GLG3D/ArticulatedModel.h>

namespace G3D {

void ArticulatedModel::loadHAIR(const Specification& specification) {
    Part*     part = addPart("root");
    Geometry* geom = addGeometry("geom");   
    
    BinaryInput bi(specification.filename, G3D_LITTLE_ENDIAN);
    // Interspersed comments are taken directly from the description of the format on the HAIR site (http://www.cemyuksel.com/research/hairmodels/)

    // Bytes 0 - 3	Must be "HAIR" in ascii code(48 41 49 52)
    const char letter_H = bi.readUInt8();
    const char letter_A = bi.readUInt8();
    const char letter_I = bi.readUInt8();
    const char letter_R = bi.readUInt8();
    alwaysAssertM((letter_H == 'H') && 
        (letter_A == 'A' ) &&
        (letter_I == 'I') &&
        (letter_R == 'R'), "Malformed .HAIR file, must begin with ASCII code 'HAIR'");

    // Bytes 4 - 7	Number of hair strands as unsigned int
    uint32 strandCount = bi.readUInt32();

    // Bytes 8 - 11	Total number of points of all strands as unsigned int
    uint32 pointCount = bi.readUInt32();

    // Bytes 12 - 15	Bit array of data in the file
    uint32 flags = bi.readUInt32();

    // Bit - 0 is 1 if the file has segments array.
    bool hasSeparateSegmentCounts = (flags & (1 << 0)) != 0;

    // Bit - 1 is 1 if the file has points array(this bit must be 1).
    bool hasPoints = (flags & (1 << 1)) != 0;
    alwaysAssertM(hasPoints, "Malformed .hair file, must have bit 1 set in bytes 12-15");

    // Bit - 2 is 1 if the file has thickness array.
    bool hasThickness = (flags & (1 << 2)) != 0;

    // Bit - 3 is 1 if the file has transparency array.
    bool hasTransparency = (flags & (1 << 3)) != 0;

    // Bit - 4 is 1 if the file has color array.
    bool hasColor = (flags & (1 << 4)) != 0;

    // Bit - 5 to Bit - 31 are reserved for future extension(must be 0).
    
    // Bytes 16 - 19	Default number of segments of hair strands as unsigned int
    // If the file does not have a segments array, this default value is used.
    uint32 defaultSegmentCount = bi.readUInt32();

    // Bytes 20 - 23	Default thickness hair strands as float
    // If the file does not have a thickness array, this default value is used.
    float defaultThickness = bi.readFloat32();

    // Bytes 24 - 27	Default transparency hair strands as float
    // If the file does not have a transparency array, this default value is used.
    float defaultTransparency = bi.readFloat32();

    // Bytes 28 - 39	Default color hair strands as float array of size 3
    // If the file does not have a thickness array, this default value is used.
    const Color3& defaultColor = bi.readColor3();
    // Bytes 40 - 127	File information as char array of size 88 in ascii
    const String& fileInfo = bi.readFixedLengthString(88);
    G3D::debugPrintf("Loading hair model with this file information: %s\n", fileInfo.c_str());

    // Segments array (unsigned short)
    // This array keeps the number of segments of each hair strand. 
    Array<unsigned short> segmentsArray;
    if (hasSeparateSegmentCounts) {
        segmentsArray.resize(strandCount);
        bi.readBytes(segmentsArray.getCArray(), strandCount * sizeof(unsigned short));
    }

    // Points array (float)
    // This array keeps the 3D positions each of hair strand point.
    alwaysAssertM(System::machineEndian() == G3DEndian::G3D_LITTLE_ENDIAN,
        "Hair loading code assumes little endian");
    Array<Point3> pointArray;
    pointArray.resize(pointCount);
    bi.readBytes(pointArray.getCArray(), pointCount * sizeof(Point3));

    // Thickness array (float)
    // This array keeps the thickness of hair strands at point locations, 
    // therefore the size of this array is equal to the number of points.
    Array<float> thicknessArray;
    if (hasThickness) {
        thicknessArray.resize(pointCount);
        bi.readBytes(thicknessArray.getCArray(), pointCount * sizeof(float));
    }

    // Transparency array (float)
    // This array keeps the transparency of hair strands at point locations, 
    // therefore the size of this array is equal to the number of points.
    Array<float> transparencyArray;
    if (hasTransparency) {
        transparencyArray.resize(pointCount);
        bi.readBytes(transparencyArray.getCArray(), pointCount * sizeof(float));
    }

    // Color array (Color3)
    // This array keeps the color of hair strands at point locations, 
    // therefore the size of this array is three times the number of points.
    Array<Color3> colorArray;
    if (hasColor) {
        colorArray.resize(pointCount);
        bi.readBytes(colorArray.getCArray(), pointCount * sizeof(Color3));
    }

    if (false) {
        // TODO: This is debugging code, make it a HairOption
        alwaysAssertM(! hasSeparateSegmentCounts, "Can't use this code path for separate segment counts");
        alwaysAssertM(segmentsArray.size() == 0, "I expected this to be zero");
        const int pointsPerStrand = pointCount / strandCount;
        static const int strandsToShow = 5000;
        pointCount = pointsPerStrand * strandsToShow;
        strandCount = strandsToShow;
        pointArray.resize(pointCount);
        if (hasThickness) { thicknessArray.resize(pointCount); }
        if (hasTransparency) { transparencyArray.resize(pointCount); }
        if (hasColor) { colorArray.resize(pointCount); }
    }

    // File is now completely read
    const int sideCount = specification.hairOptions.sideCount;

    geom->cpuVertexArray.hasTangent = false;
    geom->cpuVertexArray.hasTexCoord0 = false;
    geom->cpuVertexArray.hasTexCoord1 = false;
    geom->cpuVertexArray.hasBones = false;

    // OPT: We could reduce the model size in memory by not adding vertex colors if they
    // are the same across the entire model, by just modifying the material directly

    const bool needsVertexColors = (hasTransparency || hasColor) && ! specification.stripVertexColors;
    geom->cpuVertexArray.hasVertexColors = needsVertexColors;

    UniversalMaterial::Specification s;
    if (needsVertexColors) {
        geom->cpuVertexArray.vertexColors.resize(pointArray.size() * sideCount);
        
        for (int i = 0; i < pointArray.size(); ++i) {
            const Color4 color(hasColor ? colorArray[i] : defaultColor, hasTransparency ? transparencyArray[i] : defaultTransparency);
            for (int j = 0; j < sideCount; ++j) {
                geom->cpuVertexArray.vertexColors[i * sideCount + j] = color;
            }
        }
    } else {
        s.setLambertian(Texture::Specification(Color4(defaultColor, defaultTransparency)));
    }

    const shared_ptr<UniversalMaterial>& material = UniversalMaterial::create(s);
    Mesh* mesh;
    if (! specification.hairOptions.separateSurfacePerStrand) {
        mesh = addMesh("mesh", part, geom);
        mesh->material = material;
    }
    
    geom->cpuVertexArray.vertex.resize(pointArray.size() * sideCount);
    int currentPointIndex = 0;
    for (int i = 0; i < int(strandCount); ++i) {
        if (specification.hairOptions.separateSurfacePerStrand) {
            mesh = addMesh(format("strand%d", i), part, geom);
            mesh->material = material;
        }
        const int segmentCount = hasSeparateSegmentCounts ? segmentsArray[i] : defaultSegmentCount;
        alwaysAssertM(segmentCount > 1, "Our hair loader currently cannot handle degenerate strands");
        for (int j = 0; j <= segmentCount; ++j) {

            const float rawThickness = hasThickness ? thicknessArray[currentPointIndex] : defaultThickness;
            const float thickness    = rawThickness * specification.hairOptions.strandRadiusMultiplier;
            const Point3& currentPoint = pointArray[currentPointIndex];
            Vector3 forwardVector;
            Vector3 backwardVector;
            if (j == 0) {
                const Point3& nextPoint = pointArray[currentPointIndex + 1];
                forwardVector = backwardVector = (nextPoint - currentPoint).direction();
            } else if (j == segmentCount) {
                const Point3& prevPoint = pointArray[currentPointIndex - 1];
                forwardVector = backwardVector = (currentPoint - prevPoint).direction();
            } else {
                const Point3& nextPoint = pointArray[currentPointIndex + 1];
                const Point3& prevPoint = pointArray[currentPointIndex - 1];
                forwardVector  = (nextPoint - currentPoint).direction();
                backwardVector = (currentPoint - prevPoint).direction();
            }
            const Vector3& tangentDirection = (forwardVector + backwardVector).direction();
            
            Vector3 upVector = Vector3::unitY();
            if (abs(tangentDirection.dot(upVector)) > 0.99f) {
                upVector = Vector3::unitX();
            }

            const Vector3& baseNormal = tangentDirection.unitCross(upVector);
            for (int k = 0; k < sideCount; ++k) {
                CPUVertexArray::Vertex& v = geom->cpuVertexArray.vertex[currentPointIndex * sideCount + k];

                // Rotate around the axis of direction
                v.normal    = Matrix3::fromAxisAngle(tangentDirection, k * 2.0f * pif() / sideCount) * baseNormal;
                v.position  = currentPoint + v.normal * thickness;
                v.texCoord0 = Point2(0.0f, float(j) / float(segmentCount));
                v.tangent   = Vector4(tangentDirection, 1.0f);
            }

            if (j > 0) {
                int currStart = currentPointIndex * sideCount;
                int prevStart = currStart - sideCount;
                for (int k = 0; k < sideCount; ++k) {
                    const int next = (k + 1) % sideCount;
                    mesh->cpuIndexArray.append(prevStart + k, prevStart + next, currStart + k);
                    mesh->cpuIndexArray.append(prevStart + next, currStart + next, currStart + k);
                }
            }

            ++currentPointIndex;
        }
    }
    debugPrintf("Done parsing hair with %d hairs and %d polygons\n", strandCount, mesh->cpuIndexArray.size() / 3);
}

} // namespace G3D