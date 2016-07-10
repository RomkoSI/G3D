/**
 \file G3D/source/Parse3DS.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-08-12
 \edited  2011-08-12
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#include "G3D/Parse3DS.h"
#include "G3D/BinaryInput.h"
#include "G3D/Log.h"
#include "G3D/CoordinateFrame.h"

namespace G3D {


void Parse3DS::parse(BinaryInput& bi, const String& basePath) {
    b = &bi;

    G3DEndian oldEndian = bi.endian();

    bi.setEndian(G3D_LITTLE_ENDIAN);
    currentRotation = Matrix3::identity();
    
    fileVersion     = 0;
    meshVersion     = 0;
    currentObject   = -1;
    currentMaterial = -1;
    
    ChunkHeader chunk = readChunkHeader();
    
    alwaysAssertM(chunk.id == MAIN3DS, "Not a 3DS file!");
    processChunk(chunk);

    bi.setEndian(oldEndian);

    b = nullptr;
}


Vector3 Parse3DS::read3DSVector() {
    Vector3 v;

    v.x = -b->readFloat32();
    v.z = b->readFloat32();
    v.y = b->readFloat32();

    return v;
}


Parse3DS::ChunkHeader Parse3DS::readChunkHeader() {
    ChunkHeader c;
    c.begin  = (int)b->getPosition();
    c.id     = (ChunkHeaderID)b->readUInt16();
    c.length = b->readInt32();
    c.end    = c.length + c.begin;
    return c;
}



void Parse3DS::processMapChunk
(Map& map,
 const ChunkHeader& mapChunkHeader) {

    // Parse all sub-chunks
    while (b->getPosition() < mapChunkHeader.end) {
        ChunkHeader curChunkHeader = readChunkHeader();
        switch (curChunkHeader.id) {
        case MAT_MAP_FILENAME:
            map.filename = b->readString();
            break;

        case INT_PCT:
            map.pct = b->readUInt8() / 100.0f;
            break;

        case MAT_MAP_TILING:
            map.flags = b->readUInt16();
            break;

        case MAT_MAP_USCALE:
            map.scale.x = b->readFloat32();
            break;

        case MAT_MAP_VSCALE:
            map.scale.y = b->readFloat32();
            break;

        case MAT_MAP_UOFFSET:
            map.offset.x = b->readFloat32();
            break;

        case MAT_MAP_VOFFSET:
            map.offset.y = b->readFloat32();
            break;

        default:
            //debugPrintf("Skipped unknown chunk 0x%x\n", curChunkHeader.id);
            ;
        }

        if (b->getPosition() != curChunkHeader.end) {
            /*
            logPrintf("Skipping %lld bytes of chunk 0x%x\n", 
                curChunkHeader.end - b->getPosition(),
                curChunkHeader.id);
                */
        }

        // Jump to the end of the chunk
        b->setPosition(curChunkHeader.end);
    }
}


void Parse3DS::processMaterialChunk
(UniversalMaterial& material,
 const ChunkHeader& materialChunkHeader) {

    // Parse all sub-chunks
    while (b->getPosition() < materialChunkHeader.end) {
        ChunkHeader curChunkHeader = readChunkHeader();

        switch (curChunkHeader.id) {

        // EDITMATERIAL subchunks
        case MATNAME:                    
            material.name = b->readString();
            materialNameToIndex.set(material.name, currentMaterial);
            break;

        case MATAMBIENT:
            break;

        case MATDIFFUSE:
            material.diffuse = read3DSColor();
            break;

        case MATSPECULAR:
            material.specular = read3DSColor();
            break;

        case MATSHININESS:
            material.shininess = read3DSPct();
            break;

        case MATSHIN2PCT:
            material.shininessStrength = read3DSPct();
            break;

        case MATTRANSPARENCY:
            material.transparency = read3DSPct();
            break;

        case MATTWOSIDE:
            // Carries no data.  The presence of this chunk always means two-sided.
            material.twoSided = true;
            break;

        case MATTEXTUREMAP1:
            processMapChunk(material.texture1, curChunkHeader);
            break;

        case MATTEXTUREMAP2:
            processMapChunk(material.texture2, curChunkHeader);
            break;

        case MATBUMPMAP:
            processMapChunk(material.bumpMap, curChunkHeader);
            break;

        case MATGLOSSYMAP:
            //processMapChunk(material.specMap, curChunkHeader);
            break;


        default:
            //debugPrintf("Skipped unknown chunk 0x%x\n", curChunkHeader.id);
            ;
        }

        if (b->getPosition() != curChunkHeader.end) {
            /*
            logPrintf("Skipping %lld bytes of chunk 0x%x\n", 
                curChunkHeader.end - b->getPosition(),
                curChunkHeader.id);
                */
        }

        // Jump to the end of the chunk
        b->setPosition(curChunkHeader.end);
    }
}


void Parse3DS::processObjectChunk
(Object&                     object,
 const Parse3DS::ChunkHeader& objectChunkHeader) {

    object.name = b->readString();

    // Parse all sub-chunks
    while (b->getPosition() < objectChunkHeader.end) {
        ChunkHeader curChunkHeader = readChunkHeader();

        switch (curChunkHeader.id) {
        case OBJTRIMESH:
            processTriMeshChunk(object, curChunkHeader);
            break;

        default:
            ;
            //debugPrintf("Skipped unknown chunk 0x%x\n", curChunkHeader.id);
        }

        if (b->getPosition() != curChunkHeader.end) {
            /*
            logPrintf("Skipping %lld bytes of chunk 0x%x\n", 
                curChunkHeader.end - b->getPosition(),
                curChunkHeader.id);
                */
        }

        // Jump to the end of the chunk
        b->setPosition(curChunkHeader.end);
    }
}


void Parse3DS::processTriMeshChunk
(Object& object,
 const Parse3DS::ChunkHeader& objectChunkHeader) {

    bool alreadyWarned = false;
    (void) alreadyWarned;
    // Parse all sub-chunks
    while (b->getPosition() < objectChunkHeader.end) {
        ChunkHeader curChunkHeader = readChunkHeader();

        switch (curChunkHeader.id) {
        case TRIVERT:
            {
                int n = b->readUInt16();

                // Read the vertices
                object.vertexArray.resize(n);
                for (int v = 0; v < n; ++v) {
                    object.vertexArray[v] = read3DSVector();
                    if (! object.vertexArray[v].isFinite()) {
#                       ifdef G3D_DEBUG
                            if (! alreadyWarned) {
                                debugPrintf("Warning: infinite vertex while loading 3DS file!\n");
                                alreadyWarned = true;
                            }
#                       endif
                        object.vertexArray[v] = Vector3::zero();
                    }
                }
                debugAssert(b->getPosition() == curChunkHeader.end);
            }
            break;

        case TRIFACE:
            {
                int n = b->readUInt16();
                object.indexArray.resize(n * 3);
                for (int i = 0; i < n; ++i) {
                    // Indices are in clockwise winding order
                    for (int v = 0; v < 3; ++ v) {
                        object.indexArray[i * 3 + v] = b->readUInt16();
                    }

                    G3D::uint16 flags = b->readUInt16();
                    (void)flags;

                    /*
                    Here's the only documentation I've found on the flags:

                    this number is is a binary number which expands to 3 values.
                     for example 0x0006 would expand to 110 binary. The value should be
                     read as 1 1 0 .This value can be found in 3d-studio ascii files as
                     AB:1 BC:1 AC:0 .Which probably indicated the order of the vertices.
                     For example AB:1 would be a normal line from A to b-> But AB:0 would
                     mean a line from B to A.

                     bit 0       AC visibility
                     bit 1       BC visibility
                     bit 2       AB visibility
                     bit 3       Mapping (if there is mapping for this face)
                     bit 4-8   0 (not used ?)
                     bit 9-10  x (chaotic ???)
                     bit 11-12 0 (not used ?)
                     bit 13      face selected in selection 3
                     bit 14      face selected in selection 2
                     bit 15      face selected in selection 1
                   */
                }
            }
            // The face chunk can contain TRIFACEMAT chunks (TODO: make into a separate face parser)
            processTriMeshChunk(object, curChunkHeader);
            break;

        case TRIFACEMAT:
            {
                // Name of the material
                debugAssert(object.faceMatArray.size() >= 0);
                FaceMat& faceMat = object.faceMatArray.next();
                faceMat.materialName = b->readString();
                faceMat.faceIndexArray.resize(b->readUInt16());
                debugAssert(faceMat.faceIndexArray.size() >= 0);

                for (int i = 0; i < faceMat.faceIndexArray.size(); ++i) {
                    faceMat.faceIndexArray[i] = b->readUInt16();
                }
            }
            break;

        case TRI_TEXCOORDS:
            {
                int n = b->readUInt16();
                if (n == object.vertexArray.size()) {
                    Array<Vector2>& texCoord = object.texCoordArray;
                    texCoord.resize(n);

                    for (int v = 0; v < texCoord.size(); ++v) {
                        // Y texture coords are flipped relative to G3D
                        texCoord[v].x = b->readFloat32();
                        texCoord[v].y = 1.0f - b->readFloat32();
                    }
                } else {
                    // Wrong number of vertices!
                    logPrintf("WARNING: encountered bad number of vertices in TRIUV chunk.");
                }
            }
            break;

        case TRISMOOTH:
            // TODO: smoothing groups
            break;

        case TRIMATRIX: 
            {
                // Coordinate frame.  Convert to G3D coordinates
                // by swapping y and z and then negating the x.
                float c[12];
                for (int i = 0; i < 12; ++i) {
                    c[i] = b->readFloat32();
                }

                // Note that this transformation has *already*
                // been applied to the vertices.
       
                object.cframe =
                    Matrix4( c[0],  c[3],  c[6], -c[9],
                             c[1],  c[4],  c[7],  c[11],
                             c[2],  c[5],  c[8],  c[10],
                             0,      0,      0,     1);
                         
                /*
                debugPrintf("%s\n", object.name.c_str());
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        debugPrintf("%3.3f ", object.cframe[r][c]);
                    }
                    debugPrintf("\n");
                }
                debugPrintf("\n");
                */
            }
            break;

        default:
            ;
            debugPrintf("Skipped unknown chunk 0x%x\n", curChunkHeader.id);
        }

        if (b->getPosition() != curChunkHeader.end) {
            /*
            logPrintf("Skipping %lld bytes of chunk 0x%x\n", 
                curChunkHeader.end - b->getPosition(),
                curChunkHeader.id);
                */
        }

        // Jump to the end of the chunk
        b->setPosition(curChunkHeader.end);
    }
}


void Parse3DS::processChunk(const Parse3DS::ChunkHeader& parentChunkHeader) {

    // Parse all sub-chunks
    while (b->getPosition() < parentChunkHeader.end) {
        ChunkHeader curChunkHeader = readChunkHeader();

        switch (curChunkHeader.id) {
        case M3D_VERSION:
            fileVersion = b->readUInt16();
            debugAssertM(fileVersion == 3, "Unsupported 3DS file version");
            break;


        case EDIT3DS:
            processChunk(curChunkHeader);
            break;

            case MESH_VERSION:
                meshVersion = b->readUInt16();
                if (meshVersion != 3) {
                    logPrintf("Unsupported 3DS mesh version (%d)\n", meshVersion);
                }
                break;


            case EDIT_CONFIG1:
            case EDIT_CONFIG2:
            case EDIT_VIEW_P1:
            case EDIT_VIEW_P2:
            case EDIT_VIEW_P3:
            case EDIT_VIEW1: 
            case EDIT_BACKGR: 
            case EDIT_AMBIENT:
                // These are the configuration of 3DS Max itself;
                // window positions, etc.  Ignore them when loading
                // a model.
                break;

            case EDITMATERIAL:
                currentMaterial = materialArray.size();
                processMaterialChunk(materialArray.next(), curChunkHeader);
                currentMaterial = -1;
                break;


            case EDITOBJECT:
                processObjectChunk(objectArray.next(), curChunkHeader);
                break;

        case EDITKEYFRAME:
            processChunk(curChunkHeader);
            break;

            // Subchunks of EDITKEYFRAME            
            case KFSPOTLIGHT:
                break;

            case KFFRAMES:
                startFrame = b->readUInt32();
                endFrame   = b->readUInt32();
                //logPrintf("\nStart frame = %d, end frame = %d\n\n", startFrame, endFrame);
                processChunk(curChunkHeader);
                break;

            case KFMESHINFO:
                currentRotation = Matrix3::identity();
                currentScale = Vector3(1,1,1);
                currentTranslation = Vector3::zero();
                currentPivot = Vector3::zero();

                processChunk(curChunkHeader);

                // Copy the keyframe information
                if (currentObject != -1) {
                    debugAssert(isFinite(currentRotation.determinant()));
                    CoordinateFrame cframe(currentRotation, currentTranslation + currentPivot);
                    objectArray[currentObject].keyframe = Matrix4(cframe);
                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            objectArray[currentObject].keyframe[r][c] *= currentScale[c];
                        }
                    }
                }
                break;

                // Subchunks of KFMESHINFO
                case KFNAME:
                    {
                        String name = b->readString();                    
                        b->readUInt16(); 
                        b->readUInt16();
                        int hierarchyIndex = b->readUInt16();
                        // hierarchyIndex == -1 (0xFFFF) means "root object"
                        //logPrintf("\n\"%s\", %d\n\n", name.c_str(), hierarchyIndex);

                        // Find the current object
                        currentObject = -1;

                        if (name != "$$$DUMMY") {
                            for (int i = 0; i < objectArray.size(); ++i) {
                                if (name == objectArray[i].name) {
                                    currentObject = i;
                                    break;
                                }
                            }
                        }

                        if (currentObject != -1) {
                            objectArray[currentObject].hierarchyIndex = hierarchyIndex;
                        }
                    }
                    break;

                case KFPIVOT:
                    {
                        currentPivot = read3DSVector();
                        //debugPrintf("pivot = %s\n", currentPivot.toString().c_str());
                    }
                    break;

                case KFTRANSLATION:
                    currentTranslation = readLin3Track();
                    //debugPrintf("translation = %s\n", currentTranslation.toString().c_str());
                    break;

                case KFSCALE:
                    currentScale = readLin3Track();
                    // The scale will have the x-coordinate flipped since our 
                    // code always negates the x-axis (assuming it is reading a point).
                    currentScale.x *= -1;
                    //debugPrintf("scale = %s\n", currentScale.toString().c_str());
                    break;

                case KFROTATION:
                    currentRotation = readRotTrack();
                    break;

                case KFHIERARCHY:
                    if (currentObject != -1) {
                        objectArray[currentObject].nodeID = b->readUInt16();
                    }
                    break;
        default:
            //debugPrintf("Skipped unknown chunk 0x%x\n", curChunkHeader.id);
            ;
        }

        if (b->getPosition() != curChunkHeader.end) {
            /*
            logPrintf("Skipping %lld bytes of chunk 0x%x\n", 
                curChunkHeader.end - b->getPosition(),
                curChunkHeader.id);
                */
        }

        // Jump to the end of the chunk
        b->setPosition(curChunkHeader.end);
    }
}
            

void Parse3DS::readTCB() {
    enum {
        USE_TENSION    = 0x0001,
        USE_CONTINUITY = 0x0002,
        USE_BIAS       = 0x0004,
        USE_EASE_TO    = 0x0008,
        USE_EASE_FROM  = 0x0010
    };
    
    int tcbframe = b->readInt32();
    (void)tcbframe;
    int tcbflags = b->readUInt16();

    if (tcbflags & USE_TENSION) {
        float tcbtens = b->readFloat32();
        (void)tcbtens;
    }

    if (tcbflags & USE_CONTINUITY) {
        float tcbcont = b->readFloat32();
        (void)tcbcont;
    }

    if (tcbflags & USE_BIAS) {
        float tcbbias = b->readFloat32();
        (void)tcbbias;
    }

    if (tcbflags & USE_EASE_TO) {
        float tcbeaseto = b->readFloat32();
        (void)tcbeaseto;
    }

    if (tcbflags & USE_EASE_FROM) {
        float tcbeasefrom = b->readFloat32();
        (void)tcbeasefrom;
    }                                           
}


Vector3 Parse3DS::readLin3Track() {
    int trackflags = b->readUInt16();
    (void)trackflags;
    b->readUInt32();
    b->readUInt32();

    // Number of key frames
    int keys = b->readInt32();
    debugAssertM(keys <= 1, "Can only read 1 frame of animation");

    Vector3 vector;

    for (int k = 0; k < keys; ++k) {
        // Read but ignore the individual interpolation
        // parameters.
        readTCB();
        vector = read3DSVector();
    }

    return vector;
}


Matrix3 Parse3DS::readRotTrack() {
    int trackflags = b->readUInt16();
    (void)trackflags;
    b->readUInt32();
    b->readUInt32();

    int keys = b->readInt32();
    debugAssertM(keys == 1, "Can only read 1 frame of animation");
    float   angle = 0;
    Vector3 axis;
    for (int k = 0; k < keys; ++k) {
        readTCB();
        angle = b->readFloat32();
        axis  = read3DSVector();
    }

    if (axis.isZero()) {
        axis = Vector3::unitY();
        debugAssertM(fuzzyEq(angle, 0), "Zero axis rotation with non-zero angle!");
    }

    //logPrintf("Axis = %s, angle = %g\n\n", axis.toString().c_str(), angle);
    return Matrix3::fromAxisAngle(axis, angle);
}


Color3 Parse3DS::read3DSColor() {
    ChunkHeader curChunkHeader = readChunkHeader();
    Color3 color;

    switch (curChunkHeader.id) {
    case RGBF:
        color.r = b->readFloat32();
        color.g = b->readFloat32();
        color.b = b->readFloat32();
        break;

    case RGB24:
        color.r = b->readUInt8() / 255.0f;
        color.g = b->readUInt8() / 255.0f;
        color.b = b->readUInt8() / 255.0f;
        break;

    default:
        debugAssertM(false, format("Expected a color chunk, found: %d", curChunkHeader.id));
    }

    // Jump to the end of the chunk
    b->setPosition(curChunkHeader.end);
    return color;
}


float Parse3DS::read3DSPct() {
    ChunkHeader curChunkHeader = readChunkHeader();
    float f = 0.0f;

    switch (curChunkHeader.id) {
    case INT_PCT:
        f = b->readUInt16() / 100.0f;
        break;

    case FLOAT_PCT:
        f = b->readFloat32();
        break;
    default:
        debugAssertM(false, format("Expected a percent chunk, found: %d", curChunkHeader.id));
    }

    // Jump to the end of the chunk
    b->setPosition(curChunkHeader.end);
    return f;
}

} // namespace G3D
