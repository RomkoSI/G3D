/**
 @file MD2Model_load.cpp

 Code for loading an MD2Model

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 @created 2003-08-07
 @edited  2010-04-19
 */

#include "GLG3D/MD2Model.h"
#include "G3D/BinaryInput.h"
#include "G3D/Log.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"

namespace G3D {
Vector3 MD2Model::normalTable[162];


class MD2ModelHeader {
public:
    int magic; 
    int version; 
    int skinWidth; 
    int skinHeight; 
    int frameSize; 
    int numSkins; 
    int numVertices; 
    int numTexCoords; 
    int numTriangles; 
    int numGlCommands; 
    int numFrames; 
    int offsetSkins; 
    int offsetTexCoords; 
    int offsetTriangles; 
    int offsetFrames; 
    int offsetGlCommands; 
    int offsetEnd;

    void deserialize(BinaryInput& b) {
        magic               = b.readInt32();
        version             = b.readInt32(); 
        skinWidth           = b.readInt32(); 
        skinHeight          = b.readInt32(); 
        frameSize           = b.readInt32(); 
        numSkins            = b.readInt32(); 
        numVertices         = b.readInt32(); 
        numTexCoords        = b.readInt32(); 
        numTriangles        = b.readInt32(); 
        numGlCommands       = b.readInt32(); 
        numFrames           = b.readInt32(); 
        offsetSkins         = b.readInt32(); 
        offsetTexCoords     = b.readInt32(); 
        offsetTriangles     = b.readInt32(); 
        offsetFrames        = b.readInt32(); 
        offsetGlCommands    = b.readInt32(); 
        offsetEnd           = b.readInt32();

    }
};


struct MD2Frame {
public:
   Vector3          scale;
   Vector3          translate;
   String      name;

   void deserialize(BinaryInput& b) {
       scale.deserialize(b);
       translate.deserialize(b);
       name = b.readString(16);
   }
};


void MD2Model::Part::reset() {
    _textureFilenames.clear();
    keyFrame.clear();
    primitiveArray.clear();
    indexArray.clear();
    _texCoordArray.clear();
    faceArray.clear();
    vertexArray.clear();
    edgeArray.clear();
}


void MD2Model::Part::load(const String& filename, float resize) {

    resize *= 0.55f;

    // If models are being reloaded it is dangerous to trust the interpolation cache.
    interpolatedModel = NULL;

    alwaysAssertM(FileSystem::exists(filename), String("Can't find \"") + filename + "\"");

    setNormalTable();

    // Clear out
    reset();

    BinaryInput b(filename, G3D_LITTLE_ENDIAN);

    MD2ModelHeader header;

    header.deserialize(b);
    debugAssert(header.version == 8);
    debugAssert(header.numVertices <= 4096);

    keyFrame.resize(header.numFrames);
    Array<Vector3> frameMin;
    frameMin.resize(header.numFrames); 
    Array<Vector3> frameMax;
    frameMax.resize(header.numFrames);
    Array<double>  frameRad;
    frameRad.resize(header.numFrames);

    texCoordScale.x = 1.0f / header.skinWidth;
    texCoordScale.y = 1.0f / header.skinHeight;

    Vector3 min  = Vector3::inf();
    Vector3 max  = -Vector3::inf();
    double  rad  = 0;

    if (header.numVertices < 3) {
        Log::common()->printf("\n*****************\nWarning: \"%s\" is corrupted and is not being loaded.\n", filename.c_str());
        return;
    }

    loadTextureFilenames(b, header.numSkins, header.offsetSkins);

    for (int f = 0; f < keyFrame.size(); ++f) {
        MD2Frame md2Frame;

        b.setPosition(header.offsetFrames + f * header.frameSize);
        md2Frame.deserialize(b);

        // Read the vertices for the frame
        keyFrame[f].vertexArray.resize(header.numVertices);
        keyFrame[f].normalArray.resize(header.numVertices);

        // Per-pose bounds
        Vector3 min_1  = Vector3::inf();
        Vector3 max_1  = -Vector3::inf();
        double  rad_1  = 0;

        // Quake's axes are permuted and scaled
        double scale[3]   = {-.07, .07, -.07};
        int    permute[3] = {2, 0, 1};
        int v, i;
        for (v = 0; v < header.numVertices; ++v) {

            Vector3& vertex = keyFrame[f].vertexArray[v];
            for (i = 0; i < 3; ++i) {
                vertex[permute[i]] = (b.readUInt8() * md2Frame.scale[i] + md2Frame.translate[i]) * float(scale[permute[i]]);
            }

            vertex *= resize;

            uint8 normalIndex = b.readUInt8();
            debugAssertM(normalIndex < 162, "Illegal canonical normal index in file");
            keyFrame[f].normalArray[v] = iClamp(normalIndex, 0, 161);

            min_1 = min_1.min(vertex);
            max_1 = max_1.max(vertex);

            if (vertex.squaredMagnitude() > rad_1) {
                rad_1 = vertex.squaredMagnitude();
            }
        }

        frameMin[f] = min_1;
        frameMax[f] = max_1;
        frameRad[f] = sqrt(rad_1);

        min = min.min(min_1);
        max = max.max(max_1);

        if (rad_1 > rad) {
            rad = rad_1;
        }
    }

    // Compute per-animation bounds based on frame bounds
    for (int a = 0; a < JUMP; ++a) {
        const int first = animationTable[a].first;
        const int last  = animationTable[a].last;

        if ((first < header.numFrames) && (last < header.numFrames)) {
            Vector3 min = frameMin[first];
            Vector3 max = frameMax[first];
            double rad  = frameRad[first];

            for (int i = first + 1; i <= last; ++i) {
                min = min.min(frameMin[i]);
                max = max.max(frameMax[i]);
                rad = G3D::max(rad, frameRad[i]);
            }

            animationBoundingBox[a]    = AABox(min, max);

            // Sometimes the sphere bounding the box is tighter than the one we calculated.
            const float boxRadSq = (max-min).squaredMagnitude() * 0.25f;

            if (boxRadSq >= square(rad)) {
                animationBoundingSphere[a] = Sphere(Vector3::zero(), (float)rad);
            } else {
                animationBoundingSphere[a] = Sphere((max + min) * 0.5f, sqrt(boxRadSq));
            }

        } else {
            // This animation is not supported by this model
            animationBoundingBox[a]    = AABox(Vector3::zero(), Vector3::zero());
            animationBoundingSphere[a] = Sphere(Vector3::zero(), 0);
        }
    }

    animationBoundingBox[JUMP] = animationBoundingBox[JUMP_DOWN];
    animationBoundingSphere[JUMP] = animationBoundingSphere[JUMP_DOWN];

    boundingBox    = AABox(min, max);
    boundingSphere = Sphere(Vector3::zero(), (float)sqrt(rad));

    // Load the texture coords
    Array<Vector2int16> fileTexCoords;
    fileTexCoords.resize(header.numTexCoords);
    b.setPosition(header.offsetTexCoords);
    for (int t = 0; t < fileTexCoords.size(); ++t) {
        fileTexCoords[t].x = b.readUInt16();
        fileTexCoords[t].y = b.readUInt16();
    }

    // The indices for the texture coords (which don't match the
    // vertex indices originally).
    indexArray.resize(header.numTriangles * 3);
    Array<Vector2int16> index_texCoordArray;
    index_texCoordArray.resize(indexArray.size());

    // Read the triangles, reversing them to get triangle list order
    b.setPosition(header.offsetTriangles);
    for (int t = header.numTriangles - 1; t >= 0; --t) {

        for (int i = 2; i >= 0; --i) {
            indexArray[t * 3 + i] = b.readUInt16();
        }

        for (int i = 2; i >= 0; --i) {
            index_texCoordArray[t * 3 + i] = fileTexCoords[b.readUInt16()];
        }
    }

    computeTexCoords(index_texCoordArray);

    // Read the primitives
    {
        primitiveArray.clear();
        b.setPosition(header.offsetGlCommands);
        
        int n = b.readInt32();

        while (n != 0) {
            Primitive& primitive = primitiveArray.next();

            if (n > 0) {
                primitive.type = PrimitiveType::TRIANGLE_STRIP;
            } else {
                primitive.type = PrimitiveType::TRIANGLE_FAN;
                n = -n;
            }

            primitive.pvertexArray.resize(n);

            Array<Primitive::PVertex>&  pvertex = primitive.pvertexArray;

            for (int i = 0; i < pvertex.size(); ++i) {
                pvertex[i].texCoord.x = b.readFloat32();
                pvertex[i].texCoord.y = b.readFloat32();
                pvertex[i].index      = b.readInt32();
            }

            n = b.readInt32();
        }
    }


    MeshAlg::computeAdjacency(keyFrame[0].vertexArray, indexArray, faceArray, edgeArray, vertexArray);
    weldedFaceArray = faceArray;
    weldedEdgeArray = edgeArray;
    weldedVertexArray = vertexArray;
    MeshAlg::weldAdjacency(keyFrame[0].vertexArray, weldedFaceArray, weldedEdgeArray, weldedVertexArray);

    numBoundaryEdges = MeshAlg::countBoundaryEdges(edgeArray);
    numWeldedBoundaryEdges = MeshAlg::countBoundaryEdges(weldedEdgeArray);

    shared_ptr<VertexBuffer> indexBuffer = 
        VertexBuffer::create(indexArray.size() * sizeof(int), VertexBuffer::WRITE_ONCE);
    indexVAR = IndexStream(indexArray, indexBuffer);
}


void MD2Model::Part::loadTextureFilenames(BinaryInput& b, int num, int offset) {

    _textureFilenames.resize(num);
    b.setPosition(offset);
    for (int t = 0; t < num; ++t) {
        _textureFilenames[t] = b.readString();
    }
}


void MD2Model::Part::computeTexCoords(
    const Array<Vector2int16>&   inCoords) {

    int numVertices = keyFrame[0].vertexArray.size();

    // Table mapping original vertex indices to alternative locations
    // for that vertex (corresponding to different texture coords).
    // A zero length array means a vertex that was not yet seen.
    Array< Array<int> > cloneListArray;
    cloneListArray.resize(numVertices);

    _texCoordArray.resize(numVertices);

    // Walk through the index array and inCoords array
    for (int i = 0; i < indexArray.size(); ++i) {
        // Texture coords
        const Vector2int16& coords = inCoords[i];

        // Vertex index
        const int v = indexArray[i];

        // cloneList[vertex index] = vertex index of clone
        Array<int>& cloneList = cloneListArray[v];

        if (cloneList.size() == 0) {
            // We've never seen this vertex before, so assign it
            // the texture coordinates we already have.
            cloneList.append(v);
            _texCoordArray[v] = coords;

        } else {
            bool foundMatch = false;

            Vector2 v2coords(coords);
            // Walk through the clones and see if one has the same tex coords
            for (int c = 0; c < cloneList.size(); ++c) {
                const int clone = cloneList[c];

                if (_texCoordArray[clone] == v2coords) {
                    // Found a match
                    foundMatch = true;

                    // Replace the index with the clone's index
                    indexArray[i] = clone;

                    // The texture coordinates are already assigned
                    break;
                }
            }

            if (! foundMatch) {
                // We have a new vertex.  Add it to the list of vertices
                // cloned from the original.
                const int clone = numVertices;
                cloneList.append(clone);

                // Overwrite the index array entry
                indexArray[i] = clone;

                // Add the texture coordinates
                _texCoordArray.append(coords);

                // Clone the vertex in every key pose.
                for (int k = 0; k < keyFrame.size(); ++k) {
                    keyFrame[k].vertexArray.append(keyFrame[k].vertexArray[v]);
                    keyFrame[k].normalArray.append(keyFrame[k].normalArray[v]);
                }

                ++numVertices;
            }
        }
    }

    // Rescale the texture coordinates
    for (int i = 0; i < _texCoordArray.size(); ++i) {
        _texCoordArray[i] *= texCoordScale;
    }
}


void MD2Model::setNormalTable() {
    if (normalTable[0].y != 0) {
        // The table has already been initialized
        return;
    }

    // Initialize the global table
    normalTable[  0] = Vector3(0.000000f, 0.850651f, 0.525731f);
    normalTable[  1] = Vector3(-0.238856f, 0.864188f, 0.442863f);
    normalTable[  2] = Vector3(0.000000f, 0.955423f, 0.295242f);
    normalTable[  3] = Vector3(-0.500000f, 0.809017f, 0.309017f);
    normalTable[  4] = Vector3(-0.262866f, 0.951056f, 0.162460f);
    normalTable[  5] = Vector3(0.000000f, 1.000000f, 0.000000f);
    normalTable[  6] = Vector3(-0.850651f, 0.525731f, 0.000000f);
    normalTable[  7] = Vector3(-0.716567f, 0.681718f, 0.147621f);
    normalTable[  8] = Vector3(-0.716567f, 0.681718f, -0.147621f);
    normalTable[  9] = Vector3(-0.525731f, 0.850651f, 0.000000f);
    normalTable[ 10] = Vector3(-0.500000f, 0.809017f, -0.309017f);
    normalTable[ 11] = Vector3(0.000000f, 0.850651f, -0.525731f);
    normalTable[ 12] = Vector3(0.000000f, 0.955423f, -0.295242f);
    normalTable[ 13] = Vector3(-0.238856f, 0.864188f, -0.442863f);
    normalTable[ 14] = Vector3(-0.262866f, 0.951056f, -0.162460f);
    normalTable[ 15] = Vector3(-0.147621f, 0.716567f, 0.681718f);
    normalTable[ 16] = Vector3(-0.309017f, 0.500000f, 0.809017f);
    normalTable[ 17] = Vector3(-0.425325f, 0.688191f, 0.587785f);
    normalTable[ 18] = Vector3(-0.525731f, 0.000000f, 0.850651f);
    normalTable[ 19] = Vector3(-0.442863f, 0.238856f, 0.864188f);
    normalTable[ 20] = Vector3(-0.681718f, 0.147621f, 0.716567f);
    normalTable[ 21] = Vector3(-0.587785f, 0.425325f, 0.688191f);
    normalTable[ 22] = Vector3(-0.809017f, 0.309017f, 0.500000f);
    normalTable[ 23] = Vector3(-0.864188f, 0.442863f, 0.238856f);
    normalTable[ 24] = Vector3(-0.688191f, 0.587785f, 0.425325f);
    normalTable[ 25] = Vector3(-0.681718f, -0.147621f, 0.716567f);
    normalTable[ 26] = Vector3(-0.809017f, -0.309017f, 0.500000f);
    normalTable[ 27] = Vector3(-0.850651f, 0.000000f, 0.525731f);
    normalTable[ 28] = Vector3(-0.850651f, -0.525731f, 0.000000f);
    normalTable[ 29] = Vector3(-0.864188f, -0.442863f, 0.238856f);
    normalTable[ 30] = Vector3(-0.955423f, -0.295242f, 0.000000f);
    normalTable[ 31] = Vector3(-0.951056f, -0.162460f, 0.262866f);
    normalTable[ 32] = Vector3(-1.000000f, 0.000000f, 0.000000f);
    normalTable[ 33] = Vector3(-0.955423f, 0.295242f, 0.000000f);
    normalTable[ 34] = Vector3(-0.951056f, 0.162460f, 0.262866f);
    normalTable[ 35] = Vector3(-0.864188f, 0.442863f, -0.238856f);
    normalTable[ 36] = Vector3(-0.951056f, 0.162460f, -0.262866f);
    normalTable[ 37] = Vector3(-0.809017f, 0.309017f, -0.500000f);
    normalTable[ 38] = Vector3(-0.864188f, -0.442863f, -0.238856f);
    normalTable[ 39] = Vector3(-0.951056f, -0.162460f, -0.262866f);
    normalTable[ 40] = Vector3(-0.809017f, -0.309017f, -0.500000f);
    normalTable[ 41] = Vector3(-0.525731f, 0.000000f, -0.850651f);
    normalTable[ 42] = Vector3(-0.681718f, 0.147621f, -0.716567f);
    normalTable[ 43] = Vector3(-0.681718f, -0.147621f, -0.716567f);
    normalTable[ 44] = Vector3(-0.850651f, 0.000000f, -0.525731f);
    normalTable[ 45] = Vector3(-0.688191f, 0.587785f, -0.425325f);
    normalTable[ 46] = Vector3(-0.442863f, 0.238856f, -0.864188f);
    normalTable[ 47] = Vector3(-0.587785f, 0.425325f, -0.688191f);
    normalTable[ 48] = Vector3(-0.309017f, 0.500000f, -0.809017f);
    normalTable[ 49] = Vector3(-0.147621f, 0.716567f, -0.681718f);
    normalTable[ 50] = Vector3(-0.425325f, 0.688191f, -0.587785f);
    normalTable[ 51] = Vector3(-0.295242f, 0.000000f, -0.955423f);
    normalTable[ 52] = Vector3(0.000000f, 0.000000f, -1.000000f);
    normalTable[ 53] = Vector3(-0.162460f, 0.262866f, -0.951056f);
    normalTable[ 54] = Vector3(0.525731f, 0.000000f, -0.850651f);
    normalTable[ 55] = Vector3(0.295242f, 0.000000f, -0.955423f);
    normalTable[ 56] = Vector3(0.442863f, 0.238856f, -0.864188f);
    normalTable[ 57] = Vector3(0.162460f, 0.262866f, -0.951056f);
    normalTable[ 58] = Vector3(0.309017f, 0.500000f, -0.809017f);
    normalTable[ 59] = Vector3(0.147621f, 0.716567f, -0.681718f);
    normalTable[ 60] = Vector3(0.000000f, 0.525731f, -0.850651f);
    normalTable[ 61] = Vector3(-0.442863f, -0.238856f, -0.864188f);
    normalTable[ 62] = Vector3(-0.309017f, -0.500000f, -0.809017f);
    normalTable[ 63] = Vector3(-0.162460f, -0.262866f, -0.951056f);
    normalTable[ 64] = Vector3(0.000000f, -0.850651f, -0.525731f);
    normalTable[ 65] = Vector3(-0.147621f, -0.716567f, -0.681718f);
    normalTable[ 66] = Vector3(0.147621f, -0.716567f, -0.681718f);
    normalTable[ 67] = Vector3(0.000000f, -0.525731f, -0.850651f);
    normalTable[ 68] = Vector3(0.309017f, -0.500000f, -0.809017f);
    normalTable[ 69] = Vector3(0.442863f, -0.238856f, -0.864188f);
    normalTable[ 70] = Vector3(0.162460f, -0.262866f, -0.951056f);
    normalTable[ 71] = Vector3(-0.716567f, -0.681718f, -0.147621f);
    normalTable[ 72] = Vector3(-0.500000f, -0.809017f, -0.309017f);
    normalTable[ 73] = Vector3(-0.688191f, -0.587785f, -0.425325f);
    normalTable[ 74] = Vector3(-0.238856f, -0.864188f, -0.442863f);
    normalTable[ 75] = Vector3(-0.425325f, -0.688191f, -0.587785f);
    normalTable[ 76] = Vector3(-0.587785f, -0.425325f, -0.688191f);
    normalTable[ 77] = Vector3(-0.716567f, -0.681718f, 0.147621f);
    normalTable[ 78] = Vector3(-0.500000f, -0.809017f, 0.309017f);
    normalTable[ 79] = Vector3(-0.525731f, -0.850651f, 0.000000f);
    normalTable[ 80] = Vector3(0.000000f, -0.850651f, 0.525731f);
    normalTable[ 81] = Vector3(-0.238856f, -0.864188f, 0.442863f);
    normalTable[ 82] = Vector3(0.000000f, -0.955423f, 0.295242f);
    normalTable[ 83] = Vector3(-0.262866f, -0.951056f, 0.162460f);
    normalTable[ 84] = Vector3(0.000000f, -1.000000f, 0.000000f);
    normalTable[ 85] = Vector3(0.000000f, -0.955423f, -0.295242f);
    normalTable[ 86] = Vector3(-0.262866f, -0.951056f, -0.162460f);
    normalTable[ 87] = Vector3(0.238856f, -0.864188f, 0.442863f);
    normalTable[ 88] = Vector3(0.500000f, -0.809017f, 0.309017f);
    normalTable[ 89] = Vector3(0.262866f, -0.951056f, 0.162460f);
    normalTable[ 90] = Vector3(0.850651f, -0.525731f, 0.000000f);
    normalTable[ 91] = Vector3(0.716567f, -0.681718f, 0.147621f);
    normalTable[ 92] = Vector3(0.716567f, -0.681718f, -0.147621f);
    normalTable[ 93] = Vector3(0.525731f, -0.850651f, 0.000000f);
    normalTable[ 94] = Vector3(0.500000f, -0.809017f, -0.309017f);
    normalTable[ 95] = Vector3(0.238856f, -0.864188f, -0.442863f);
    normalTable[ 96] = Vector3(0.262866f, -0.951056f, -0.162460f);
    normalTable[ 97] = Vector3(0.864188f, -0.442863f, -0.238856f);
    normalTable[ 98] = Vector3(0.809017f, -0.309017f, -0.500000f);
    normalTable[ 99] = Vector3(0.688191f, -0.587785f, -0.425325f);
    normalTable[100] = Vector3(0.681718f, -0.147621f, -0.716567f);
    normalTable[101] = Vector3(0.587785f, -0.425325f, -0.688191f);
    normalTable[102] = Vector3(0.425325f, -0.688191f, -0.587785f);
    normalTable[103] = Vector3(0.955423f, -0.295242f, 0.000000f);
    normalTable[104] = Vector3(1.000000f, 0.000000f, 0.000000f);
    normalTable[105] = Vector3(0.951056f, -0.162460f, -0.262866f);
    normalTable[106] = Vector3(0.850651f, 0.525731f, 0.000000f);
    normalTable[107] = Vector3(0.955423f, 0.295242f, 0.000000f);
    normalTable[108] = Vector3(0.864188f, 0.442863f, -0.238856f);
    normalTable[109] = Vector3(0.951056f, 0.162460f, -0.262866f);
    normalTable[110] = Vector3(0.809017f, 0.309017f, -0.500000f);
    normalTable[111] = Vector3(0.681718f, 0.147621f, -0.716567f);
    normalTable[112] = Vector3(0.850651f, 0.000000f, -0.525731f);
    normalTable[113] = Vector3(0.864188f, -0.442863f, 0.238856f);
    normalTable[114] = Vector3(0.809017f, -0.309017f, 0.500000f);
    normalTable[115] = Vector3(0.951056f, -0.162460f, 0.262866f);
    normalTable[116] = Vector3(0.525731f, 0.000000f, 0.850651f);
    normalTable[117] = Vector3(0.681718f, -0.147621f, 0.716567f);
    normalTable[118] = Vector3(0.681718f, 0.147621f, 0.716567f);
    normalTable[119] = Vector3(0.850651f, 0.000000f, 0.525731f);
    normalTable[120] = Vector3(0.809017f, 0.309017f, 0.500000f);
    normalTable[121] = Vector3(0.864188f, 0.442863f, 0.238856f);
    normalTable[122] = Vector3(0.951056f, 0.162460f, 0.262866f);
    normalTable[123] = Vector3(0.442863f, 0.238856f, 0.864188f);
    normalTable[124] = Vector3(0.309017f, 0.500000f, 0.809017f);
    normalTable[125] = Vector3(0.587785f, 0.425325f, 0.688191f);
    normalTable[126] = Vector3(0.147621f, 0.716567f, 0.681718f);
    normalTable[127] = Vector3(0.238856f, 0.864188f, 0.442863f);
    normalTable[128] = Vector3(0.425325f, 0.688191f, 0.587785f);
    normalTable[129] = Vector3(0.500000f, 0.809017f, 0.309017f);
    normalTable[130] = Vector3(0.716567f, 0.681718f, 0.147621f);
    normalTable[131] = Vector3(0.688191f, 0.587785f, 0.425325f);
    normalTable[132] = Vector3(0.262866f, 0.951056f, 0.162460f);
    normalTable[133] = Vector3(0.238856f, 0.864188f, -0.442863f);
    normalTable[134] = Vector3(0.262866f, 0.951056f, -0.162460f);
    normalTable[135] = Vector3(0.500000f, 0.809017f, -0.309017f);
    normalTable[136] = Vector3(0.716567f, 0.681718f, -0.147621f);
    normalTable[137] = Vector3(0.525731f, 0.850651f, 0.000000f);
    normalTable[138] = Vector3(0.688191f, 0.587785f, -0.425325f);
    normalTable[139] = Vector3(0.425325f, 0.688191f, -0.587785f);
    normalTable[140] = Vector3(0.587785f, 0.425325f, -0.688191f);
    normalTable[141] = Vector3(-0.295242f, 0.000000f, 0.955423f);
    normalTable[142] = Vector3(-0.162460f, 0.262866f, 0.951056f);
    normalTable[143] = Vector3(0.000000f, 0.000000f, 1.000000f);
    normalTable[144] = Vector3(0.000000f, 0.525731f, 0.850651f);
    normalTable[145] = Vector3(0.295242f, 0.000000f, 0.955423f);
    normalTable[146] = Vector3(0.162460f, 0.262866f, 0.951056f);
    normalTable[147] = Vector3(-0.442863f, -0.238856f, 0.864188f);
    normalTable[148] = Vector3(-0.162460f, -0.262866f, 0.951056f);
    normalTable[149] = Vector3(-0.309017f, -0.500000f, 0.809017f);
    normalTable[150] = Vector3(0.442863f, -0.238856f, 0.864188f);
    normalTable[151] = Vector3(0.162460f, -0.262866f, 0.951056f);
    normalTable[152] = Vector3(0.309017f, -0.500000f, 0.809017f);
    normalTable[153] = Vector3(-0.147621f, -0.716567f, 0.681718f);
    normalTable[154] = Vector3(0.147621f, -0.716567f, 0.681718f);
    normalTable[155] = Vector3(0.000000f, -0.525731f, 0.850651f);
    normalTable[156] = Vector3(-0.587785f, -0.425325f, 0.688191f);
    normalTable[157] = Vector3(-0.425325f, -0.688191f, 0.587785f);
    normalTable[158] = Vector3(-0.688191f, -0.587785f, 0.425325f);
    normalTable[159] = Vector3(0.688191f, -0.587785f, 0.425325f);
    normalTable[160] = Vector3(0.425325f, -0.688191f, 0.587785f);
    normalTable[161] = Vector3(0.587785f, -0.425325f, 0.688191f);
}

}
