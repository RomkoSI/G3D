/**
  @file bsp/q3bspLoad.cpp
  
  The BSP::load method.

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2003-05-25
  @edited  2010-02-30

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */ 

#include "GLG3D/BSPMAP.h"
#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"

#define TESSELLATION_LEVEL 10
#define G3D_LOAD_SCALE (0.03f)
namespace G3D {
namespace _BSPMAP {

/**
 Quake uses a different coordinate system than G3D. 
 This swizzles the quake coordinates so they match
 G3D ones.
 */
static void swizzle(Vector3& v) {
    float temp = v.y;
    v.y = v.z;
    v.z = -temp;
}


/**
 Only used in loading of Quake 3 maps
 */
class Q3BSPTexture {
public:
    char                name[64];
    int                 flags;
    int                 contents;
};


/**
 Only used in loading of Quake 3 maps
 */
class Q3BSPShader {
public:
    char                strName[64];
    int                 brushIndex;
    int                 shaderData;
};

class BSPLump {
public:

    int offset;
    int length;

    /**
     Dictated by the Quake III file format
     */
    enum Q3Lump {
        Q3_ENTITIES_LUMP = 0,
        Q3_TEXTURES_LUMP,
        Q3_PLANES_LUMP,
        Q3_NODES_LUMP,
        Q3_LEAVES_LUMP,
        Q3_LEAFFACES_LUMP,
        Q3_LEAFBRUSHES_LUMP,
        Q3_MODELS_LUMP,
        Q3_BRUSHES_LUMP,
        Q3_BRUSHSIDES_LUMP,
        Q3_VERTEXES_LUMP,
        Q3_MESHVERTEXES_LUMP,
        Q3_SHADERS_LUMP,
        Q3_FACES_LUMP,
        Q3_LIGHTMAPS_LUMP,
        Q3_LIGHTVOLUMES_LUMP,
        Q3_VISDATA_LUMP,
        Q3_MAX_LUMPS};

    /**
     Dictated by the Half-Life file format
     */
    enum HLLump {
        HL_ENTITIES_LUMP = 0,
        HL_PLANES_LUMP,
        HL_TEXTURES_LUMP,
        HL_VERTEXES_LUMP,
        HL_VISIBILITY_LUMP,
        HL_NODES_LUMP,
        HL_TEXINFO_LUMP,
        HL_FACES_LUMP,
        HL_LIGHTING_LUMP,
        HL_CLIPNODES_LUMP,
        HL_LEAFS_LUMP,
        HL_MARKSURFACES_LUMP,
        HL_EDGES_LUMP,
        HL_SUFEDGES_LUMP,
        HL_MODELS_LUMP,
        HL_MAX_LUMPS};
};

/**
 @param brighten Most Quake II textures are dark; this argument is a factor to make
 them brighter by.  Default = 1.0, normal intensity.  It is safe to call
 load multiple times-- the previously loaded model will be freed correctly.
*/
static shared_ptr<Texture> loadBrightTexture(const String& filename, float brighten = 1.0f) {
    //debugAssert(fileExists(filename));
    Texture::Preprocess preprocess;
    preprocess.modulate = Color4::one() * brighten;
    bool generateMipMaps = true;
    return Texture::fromFile(filename, ImageFormat::AUTO(), Texture::DIM_2D, generateMipMaps, preprocess, true);
}
/**
    Only used in loading.

    For type 1 faceArray (polygons), vertex and n_vertexes describe a set of vertices 
    that form a polygon. The set always contains a loop of vertices, and sometimes 
    also includes an additional vertex near the center of the polygon.
  <P>

    For type 2 faceArray (patches), vertex and n_vertexes describe a 2D rectangular 
    grid of control vertices with dimensions given by size. Within this rectangular 
    grid, regions of 3Å◊3 vertices represent biquadratic Bezier patches. Adjacent 
    patches share a line of three vertices. There are a total of (size[0] - 1) / 2 
    by (size[1] - 1) / 2 patches. Patches in the grid start at (i, j) given by: 
  <P>

    i = 2n, n in [ 0 .. (size[0] - 1) / 2 ), and
    <DT>j = 2m, m in [ 0 .. (size[1] - 1) / 2 ). 

  <P>
    For type 3 faceArray (meshes), meshvert and n_meshverts are used to describe the
    independent triangles that form the mesh. As with type 1 faceArray, every three
    meshverts describe a triangle, and each meshvert is an offset from the first
    vertex of the face, given by vertex. 

  <P>
    For type 4 faceArray (billboards), vertex describes the single vertex that 
    determines the location of the billboard. Billboards are used for effects 
    such as flares. Exactly how each billboard vertex is to be interpreted has 
    not been investigated. 

  <P>
    The lm_ variables are primarily used to deal with lightMap data. A face that 
    has a lightMap has a non-negative lm_index. For such a face, lm_index is the
    index of the image in the lightMaps lump that contains the lighting data for
    the face. The data in the lightMap image can be located using the rectangle 
    specified by lm_start and lm_size. 

  <P>
    For type 1 faceArray (polygons) only, lm_origin and lm_vecs can be used to compute
    the world-space positions corresponding to lightMap samples. These positions 
    can in turn be used to compute dynamic lighting across the face. 

  <P>
    None of the lm_ variables are used to compute texture coordinates for indexing
    into lightMaps. In fact, lightMap coordinates need not be computed. Instead, 
    lightMap coordinates are simply stored with the vertices used to describe each face. 

  @cite Description by Kekoa Proudfoot
 */
class Q3BSPFace {
public:
    int                 textureID;
    int                 effect;
    int                 type;
    int                 firstVertex;
    int                 vertexesCount;
    int                 firstMeshVertex;
    int                 meshVertexesCount;
    int                 lightMapID;
    int                 lightMapCorner[2];
    int                 lightMapSize[2];
    Vector3             lightMapPosition;
    Vector3             lightMapVectors[2];
    Vector3             normal;
    int                 patchSize[2];
};

///////////////////////////////////////////////////////////////////////////////////

MapRef Map::fromFile(const String& path, const String& fileName, float scale, String altLoad,
                     const String& defaultTextureFile) {
    if (altLoad == "") {
        altLoad = System::findDataFile("pak0.pk3", false);
        if (! FileSystem::exists(altLoad)) {
            altLoad = System::findDataFile("mini-pak0.pk3", false);
        }
#       ifdef G3D_WINDOWS
        for (int i = 0; ! FileSystem::exists(altLoad) && (i < FileSystem::drives().size()); ++i) {
            altLoad = FilePath::concat(FileSystem::drives()[i], "pak0.pk3");
        }
        for (int i = 0; ! FileSystem::exists(altLoad) && (i < FileSystem::drives().size()); ++i) {
            altLoad = FilePath::concat(FileSystem::drives()[i], "mini-pak0.pk3");
        }
#       endif
    }
    Map* m = new Map();
    if (m->load(pathConcat(path, ""), fileName, altLoad, defaultTextureFile)) {
        return shared_ptr<Map>(m);
    } else {
        delete m;
        return shared_ptr<Map>();
    }
}

bool Map::load
(const String&  resPath,
 const String&  filename,
 const String&  altPath,
 const String&  defaultTextureFile) {
    
    int supportedVersion[NUM_FILE_FORMATS + 1];
//    supportedVersion[Q1] = 23;
//    supportedVersion[Q2] = 38;
    supportedVersion[HL] = 30;
    supportedVersion[Q3] = 47;
    supportedVersion[NUM_FILE_FORMATS] = 0;

    String full = resPath + "maps/" + filename;

    if ((defaultTextureFile != "") && FileSystem::exists(defaultTextureFile)) {
        defaultTexture = loadBrightTexture(defaultTextureFile);
    } else {     
        defaultTexture = Texture::white();
    }

    if (! FileSystem::exists(full)) {
        debugAssertM(FileSystem::exists(full), "Could not find " + full);
        return false;
    }

    BinaryInput bi(full, G3D_LITTLE_ENDIAN);
    
    // Determine file type
    MapFileFormat mapFormat;
    int           version;

    loadVersion(bi, mapFormat, version);

    if (version > supportedVersion[mapFormat]) {
        return false;
    }

    switch (mapFormat) {
    case Q3:
        loadQ3(bi, resPath, altPath);
        break;

    case HL:
        loadHL(bi, resPath, altPath);
        break;

    default:
        return false;
    }

    // Check the integrity of what we just loaded
    verifyData();
    
    facesDrawn.resize(faceArray.size());

    m_bounds = AABox(staticModel.min, staticModel.max);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////

void Map::loadQ3(BinaryInput& bi, const String& resPath, const String& altPath) {
    BSPLump lumps[BSPLump::Q3_MAX_LUMPS];
    loadLumps        (bi, lumps, BSPLump::Q3_MAX_LUMPS);
    loadEntities     (bi, lumps[BSPLump::Q3_ENTITIES_LUMP]);
    loadVertices     (bi, lumps[BSPLump::Q3_VERTEXES_LUMP]);
    loadMeshVertices (bi, lumps[BSPLump::Q3_MESHVERTEXES_LUMP]);
    loadFaces        (bi, lumps[BSPLump::Q3_FACES_LUMP]);
    loadTextures     (resPath, altPath, bi, lumps[BSPLump::Q3_TEXTURES_LUMP]);
    loadLightMaps    (bi, lumps[BSPLump::Q3_LIGHTMAPS_LUMP]);
    loadNodes        (bi, lumps[BSPLump::Q3_NODES_LUMP]);
    loadQ3Leaves     (bi, lumps[BSPLump::Q3_LEAVES_LUMP]);
    loadLeafFaceArray(bi, lumps[BSPLump::Q3_LEAFFACES_LUMP]);
    loadBrushes      (bi, lumps[BSPLump::Q3_BRUSHES_LUMP]);
    loadBrushSides   (bi, lumps[BSPLump::Q3_BRUSHSIDES_LUMP]);
    loadLeafBrushes  (bi, lumps[BSPLump::Q3_LEAFBRUSHES_LUMP]);
    loadPlanes       (bi, lumps[BSPLump::Q3_PLANES_LUMP]);
    loadStaticModel  (bi, lumps[BSPLump::Q3_MODELS_LUMP]);
    loadDynamicModels(bi, lumps[BSPLump::Q3_MODELS_LUMP]);
    loadLightVolumes (bi, lumps[BSPLump::Q3_LIGHTVOLUMES_LUMP]);
    loadQ3VisData    (bi, lumps[BSPLump::Q3_VISDATA_LUMP]);
}


void Map::loadHL(BinaryInput& bi, const String& resPath, const String& altPath) {    
    BSPLump lumps[BSPLump::HL_MAX_LUMPS];
    loadLumps        (bi, lumps, BSPLump::HL_MAX_LUMPS);

    loadPlanes       (bi, lumps[BSPLump::HL_PLANES_LUMP]);
    loadHLLeaves     (bi, lumps[BSPLump::HL_LEAFS_LUMP]);
    loadVertices     (bi, lumps[BSPLump::HL_VERTEXES_LUMP]);
    loadNodes        (bi, lumps[BSPLump::HL_NODES_LUMP]);
    // TODO: texinfo
    loadFaces        (bi, lumps[BSPLump::HL_FACES_LUMP]);
    // TODO: clipnodes
    // TODO: marksurfaces
    // TODO: edges
    // TODO: surfedges
    loadStaticModel  (bi, lumps[BSPLump::HL_MODELS_LUMP]);

    loadEntities     (bi, lumps[BSPLump::HL_ENTITIES_LUMP]);
    loadTextures     (resPath, altPath, bi, lumps[BSPLump::HL_TEXTURES_LUMP]);

    loadLightMaps    (bi, lumps[BSPLump::HL_LIGHTING_LUMP]);

    loadHLVisData    (bi, lumps[BSPLump::HL_VISIBILITY_LUMP], lumps[BSPLump::HL_LEAFS_LUMP]);
}

///////////////////////////////////////////////////////////////////////////////////

// TODO: make all loading endian-independent

void Map::loadVersion(BinaryInput& bi, MapFileFormat& mapFormat, int& version) {

    String dummy = bi.readString(4);

    if (dummy == "IBSP") {
        mapFormat = Q3;
        version   = bi.readInt32();
        return;
    } else {
        // Half-life map
        bi.setPosition(bi.getPosition() - 4);
        mapFormat = HL;
        version   = bi.readInt32();
    }
}


void Map::loadLumps(
    BinaryInput&            bi,
    BSPLump*                lumps,
    int                     max) {

    for (int i = 0; i < max; ++i) {
        bi.readBytes(&lumps[i], sizeof(BSPLump));
    }
}


void Map::loadEntities(BinaryInput&    bi,const BSPLump& lump){
    char* entities = new char[lump.length];
    bi.setPosition(lump.offset);

    bi.readBytes(entities, lump.length);
    String    entityString;
    if (entities != NULL) {

        entityString = entities;

        if (entityString == "") {
            startingPosition = Vector3::zero();
            return;            
        }

    } else {
        startingPosition = Vector3::zero();
        return;
    }
    G3D::Array<String> entityStrArray = G3D::stringSplit(entityString,'}');
    for(int    i =    0; i < entityStrArray.size(); ++i){
        String    entity = entityStrArray[i];
        G3D::Array<String>    subArray = G3D::stringSplit(entity,'\n');
        BSPEntity currEntity;
        currEntity.position    = Vector3::inf();
        currEntity.spawnflags =    0;
        currEntity.modelNum = -1;

        for(int    j =    0; j < subArray.size();    ++j){
            String    subStr = String(subArray[j]);
#ifdef G3D_WINDOWS
            // This will never be fixed; we're more likely to remove custom BSP loading in favor of Assimp than to
            // patch this otherwise-working code.
//#pragma message("Fix destructive useage of strtok in BSPMAPLoad.cpp ")
#endif
            char* pt = const_cast<char*>(strstr(subStr.c_str(), "\"classname\""));
            if (pt){
                const char* name = strtok(pt + 13, "\"    ");
                if(name){
                    currEntity.name    = name + format(" %d",i);
                }
                continue;
            }

            pt = const_cast<char*>(strstr(subStr.c_str(),    "\"origin\""));
            if(pt){
                char temp[40];
                strcpy(temp,pt);
                Vector3    pos;
                pos.x =     atoi(strtok(pt    + 10," ")) * G3D_LOAD_SCALE;
                pos.z =    -atoi(strtok(NULL," "))       * G3D_LOAD_SCALE;
                pos.y =     atoi(strtok(NULL,"\""))   * G3D_LOAD_SCALE;
                currEntity.position    = pos;
                continue;
            }
            pt = const_cast<char*>(strstr(subStr.c_str(),    "\"spawnflags\""));
            if(pt){
                currEntity.spawnflags =    atoi(strtok(pt+13,"\""));
                continue;
            }
            pt = const_cast<char*>(strstr(subStr.c_str(),    "\"targetname\""));
            if(pt){
                char* name = strtok(pt + 14,"\"    ");
                if(name){
                    currEntity.targetName = name;
                }
                continue;
            }
            pt = const_cast<char*>(strstr(subStr.c_str(),    "\"model\""));
            if(pt){
                currEntity.modelNum = atoi(strtok(pt+10,"\"")) - 1;
                continue;
            }
            pt = const_cast<char*>(strstr(subStr.c_str(),    "\"target\""));
            if(pt){
                char* name = strtok(pt + 10,"\"    ");
                if(name){
                    currEntity.target = name;
                }
                continue;
            }
            if(subStr != "{" &&    subStr != "\n{"    && subStr != "\n"){
                currEntity.otherInfo +=    subStr;
                currEntity.otherInfo +=    "\n";
            }
        }

        if (currEntity.name != "") {

            if (strstr(currEntity.name.c_str(), "info_player_deathmatch")) {
                if (currEntity.position != Vector3::inf()) {
                    startingPosition = currEntity.position;
                } else {
                    startingPosition = Vector3::zero();
                }
            } else if(strstr(currEntity.name.c_str(), "info_player_start") ) {
                if (currEntity.position != Vector3::inf()) {
                    startingPosition = currEntity.position;
                } else {
                    startingPosition = Vector3::zero();
                }
            }

            entityArray.append(currEntity);
        }
    }

    delete[] entities;
}


void Map::loadVertices(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    vertexArray.resize(lump.length / sizeof(Vertex));

    bi.setPosition(lump.offset);

    for (int ct = 0; ct < vertexArray.size(); ++ct) {
        bi.readBytes(&vertexArray[ct], sizeof(Vertex));

        swizzle(vertexArray[ct].position);
        vertexArray[ct].position *= G3D_LOAD_SCALE;

        swizzle(vertexArray[ct].normal);
    }
}


void Map::loadMeshVertices(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    meshVertexArray.resize(lump.length / sizeof(int));
    bi.setPosition(lump.offset);
    bi.readBytes(meshVertexArray.getCArray(), meshVertexArray.size() * sizeof(int));
}


void Map::loadFaces(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    alwaysAssertM(lump.length < 1e9, "Corrupt file");
    int facesCount = lump.length / sizeof(Q3BSPFace);
    faceArray.invokeDeleteOnAllElements();
    faceArray.resize(facesCount);
    alwaysAssertM(facesCount < 1e6, "Corrupt file");
    Q3BSPFace* faceData = new Q3BSPFace[facesCount];

    bi.setPosition(lump.offset);
    bi.readBytes(faceData, facesCount * sizeof(Q3BSPFace));

    for (int ct = 0; ct < facesCount; ++ct) {
        FaceSet* theFace = NULL;
        switch (faceData[ct].type) {
        case FaceSet::PATCH: {
            Patch* face  = new Patch();
            face->textureID   = faceData[ct].textureID;
            face->lightMapID  = faceData[ct].lightMapID;
            int width         = faceData[ct].patchSize[0];
            int height        = faceData[ct].patchSize[1];
            int widthCount    = (width - 1) / 2;
            int heightCount   = (height - 1) / 2;

            face->bezierArray.resize(widthCount * heightCount);

            for (int y = 0; y < heightCount; ++y) {
                for (int x = 0; x < widthCount; ++x) {
                    for (int row = 0; row < 3; ++row) {
                        for (int col = 0; col < 3; ++col) {
                    
                            face->bezierArray[y * widthCount + x].controls[row * 3 + col] =
                                vertexArray[faceData[ct].firstVertex +
                                    (y * 2 * width + x * 2)+
                                    row * width + col];
                        }
                    }

                    face->bezierArray[y * widthCount + x].tessellate(TESSELLATION_LEVEL);
                }
            }
            theFace = face;
            break;
            }

        case FaceSet::POLYGON:
        case FaceSet::MESH: {
            Mesh* face = new Mesh();

            face->textureID         = faceData[ct].textureID;
            face->lightMapID        = faceData[ct].lightMapID;
            face->firstVertex       = faceData[ct].firstVertex;
            face->vertexesCount     = faceData[ct].vertexesCount;
            face->firstMeshVertex   = faceData[ct].firstMeshVertex;
            face->meshVertexesCount = faceData[ct].meshVertexesCount;
            theFace = face;
            break;
            }

        case FaceSet::BILLBOARD: {
            Billboard* face = new Billboard();
            face->textureID  = faceData[ct].textureID;
            face->lightMapID = faceData[ct].lightMapID;
            theFace = face;
            break;
           }
        }
        faceArray[ct] = theFace;
    }
    delete[] faceData;
}



shared_ptr<Texture> Map::loadTexture(const String& resPath, const String& altPath, const String& filename, int index) {
    float brighten = 2.0f;
    const int numExt = 3;
    static const String ext[] = {".jpg", ".tga", ".png"};
    
    int numPath = 2;
    const String path[] = {resPath, altPath};
    if (altPath == "<none>") {
        numPath = 1;
    }

    try {
        for (int p = 0; p < numPath; ++p) {
            for (int i = 0; i < numExt; ++i) {
                const String& full = pathConcat(path[p], filename) + ext[i];
                
                if (FileSystem::exists(full)) {
                    shared_ptr<Texture> t = loadBrightTexture(full, brighten);

                    if (isNull(defaultTexture)) {
                        defaultTexture = t;
                    }
                    return t;
                }
            }
        }
        
        logPrintf("BSPMap reports missing texture #%d: \"%s\"\n", index, filename.c_str());
        return defaultTexture;

    } catch (const Image::Error& e) {
        logPrintf("** BSPMap reports error while loading %s: %s\n", e.filename.c_str(), e.reason.c_str());
        return defaultTexture;
    }
}


void Map::loadTextures(
    const String&     resPath,
    const String&     altPath,
    BinaryInput&           bi,
    const BSPLump&         lump) {

    int texturesCount = lump.length / sizeof(Q3BSPTexture);
    textures.resize(texturesCount);
    textureIsHollow.resize(texturesCount);
    Array<Q3BSPTexture> textureData;
    textureData.resize(texturesCount);

    bi.setPosition(lump.offset);
    bi.readBytes(textureData.getCArray(), sizeof(Q3BSPTexture) * texturesCount);

    for (int ct = 0; ct < texturesCount; ++ct)    {
        const int CONTENTS_SOLID        = 0x00000001;
        const int CONTENTS_WINDOW       = 0x00000002;
        const int CONTENTS_PLAYERCLIP   = 0x00010000;
        const int CONTENTS_MONSTER      = 0x02000000;
        const int IS_HOLLOW_MASK        = CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER;

        if (textureData[ct].contents & IS_HOLLOW_MASK) {
            textureIsHollow.set(ct);
        }

        // Locate the texture
        String filename = textureData[ct].name;
        textures[ct] = loadTexture(resPath, altPath, filename, ct);
    }
}


void Map::loadLightMaps(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int LIGHTMAP_SIZE = 128 * 128 * 3;
    uint8 lightMapData[LIGHTMAP_SIZE];

    // Some quake maps are too dark.  This code makes a lookup table
    // that can be used for brightening them.
    uint8 brighten[256];
    for (int i = 0; i < 256; ++i) {
        // Code to leave intensity unmodified:
        //brighten[i] = i;

        // Code to brighten:
        //brighten[i] = iClamp(pow(i / 200.0, 1.4) * 700, 0, 255);
        brighten[i] = iClamp(iRound(i * 1.5 + 25), 0, 255);
    }

    int lightMapsCount = lump.length / sizeof(lightMapData);

    lightMaps.resize(lightMapsCount);

    bi.setPosition(lump.offset);

    for (int ct = 0; ct < lightMapsCount; ++ct) {
        bi.readBytes(lightMapData, sizeof(lightMapData));

        for (int i = 0; i < 128 * 128 * 3; ++i) {
            lightMapData[i] = brighten[lightMapData[i]];
        }

        // Quake maps were probably originally intended as linear RBB lighting, but
        // using them as RGB instead of sRGB produces fairly low dynamic range in a 
        // modern physically-based renderer.
        
        const shared_ptr<Texture>& txt = 
            Texture::fromMemory("Light map", lightMapData, ImageFormat::SRGB8(), 128, 128, 1, 1,
            ImageFormat::SRGB8(), Texture::DIM_2D);

        lightMaps[ct] = txt;
    }
}


void Map::loadNodes(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPNODE_SIZE = 9 * 4;
    nodeArray.resize(lump.length / BSPNODE_SIZE);

    bi.setPosition(lump.offset);

    for (int ct = 0; ct < nodeArray.size(); ++ct) {
        nodeArray[ct].plane = bi.readInt32();
        nodeArray[ct].front = bi.readInt32();
        nodeArray[ct].back  = bi.readInt32();

        // Skip the BSP node's min and max bounds
        bi.skip(2 * 3 * sizeof(int32));
        
    }
}


void Map::loadHLLeaves(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPLEAF_SIZE = 4 * 12;
    leafArray.resize(lump.length / BSPLEAF_SIZE);
    bi.setPosition(lump.offset);

    for (int ct = 0; ct < leafArray.size(); ++ct) {
        int intVec[3];

        // Skip contents and visofs (visofs will be read during
        // visData loading).
        bi.skip(8);

        leafArray[ct].cluster       = ct;
        leafArray[ct].area          = 0;
        
        intVec[0]                   = bi.readInt32();
        intVec[1]                   = bi.readInt32();
        intVec[2]                   = bi.readInt32();
        Vector3 a((float)intVec[0], (float)intVec[1], (float)intVec[2]);
        swizzle(a);
        a *= G3D_LOAD_SCALE;

        intVec[0]                   = bi.readInt32();
        intVec[1]                   = bi.readInt32();
        intVec[2]                   = bi.readInt32();
        Vector3 b((float)intVec[0], (float)intVec[1], (float)intVec[2]);
        swizzle(b);
        b *= G3D_LOAD_SCALE;

        leafArray[ct].bounds        = AABox(a.min(b), a.max(b));

        leafArray[ct].firstFace     = bi.readInt32();
        leafArray[ct].facesCount    = bi.readInt32();

        bi.skip(4);

        leafArray[ct].firstBrush    = 0;
        leafArray[ct].brushesCount  = 0;
    
    }
}


void Map::loadQ3Leaves(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPLEAF_SIZE = 4 * 12;
    leafArray.resize(lump.length / BSPLEAF_SIZE);
    bi.setPosition(lump.offset);

    for (int ct = 0; ct < leafArray.size(); ++ct) {
        int intVec[3];

        leafArray[ct].cluster       = bi.readInt32();
        leafArray[ct].area          = bi.readInt32();
        
        intVec[0]                   = bi.readInt32();
        intVec[1]                   = bi.readInt32();
        intVec[2]                   = bi.readInt32();
        Vector3 a((float)intVec[0], (float)intVec[1], (float)intVec[2]);
        swizzle(a);
        a *= G3D_LOAD_SCALE;

        intVec[0]                   = bi.readInt32();
        intVec[1]                   = bi.readInt32();
        intVec[2]                   = bi.readInt32();
        Vector3 b((float)intVec[0], (float)intVec[1], (float)intVec[2]);
        swizzle(b);
        b *= G3D_LOAD_SCALE;

        leafArray[ct].bounds        = AABox(a.min(b), a.max(b));
        leafArray[ct].center        = (a + b) / 2;

        leafArray[ct].firstFace     = bi.readInt32();
        leafArray[ct].facesCount    = bi.readInt32();
        leafArray[ct].firstBrush    = bi.readInt32();
        leafArray[ct].brushesCount  = bi.readInt32();
    
    }
}


void Map::loadLeafFaceArray(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPLEAFFACES_SIZE = 4;
    leafFaceArray.resize(lump.length / BSPLEAFFACES_SIZE);
    bi.setPosition(lump.offset);

    for (int ct = 0; ct < leafFaceArray.size(); ++ct) {
        leafFaceArray[ct] = bi.readInt32();
    }
}


void Map::loadBrushes(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPBRUSH_SIZE = sizeof(Brush);
    brushArray.resize(lump.length / BSPBRUSH_SIZE);
    bi.setPosition(lump.offset);

    bi.readBytes(brushArray.getCArray(), brushArray.size() * BSPBRUSH_SIZE);
}


void Map::loadBrushSides(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPBRUSHSIDE_SIZE = sizeof(BrushSide);
    brushSideArray.resize(lump.length / BSPBRUSHSIDE_SIZE);
    bi.setPosition(lump.offset);

    bi.readBytes(brushSideArray.getCArray(), brushSideArray.size() * BSPBRUSHSIDE_SIZE);
}


void Map::loadLeafBrushes(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    static const int BSPLEAFBRUSH_SIZE = 4;
    leafBrushArray.resize(lump.length / BSPLEAFBRUSH_SIZE);
    bi.setPosition(lump.offset);

    for (int ct = 0; ct < leafBrushArray.size(); ++ct) {
        leafBrushArray[ct] = bi.readInt32();
    }
}


void Map::loadPlanes(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    planeArray.resize(lump.length / sizeof(BSPPlane));
    bi.setPosition(lump.offset);
    bi.readBytes(planeArray.getCArray(), planeArray.size() * sizeof(BSPPlane));

    // Swizzle the quake coordinates
    for (int p = 0; p < planeArray.size(); ++p) {
        swizzle(planeArray[p].normal);
        planeArray[p].distance *= G3D_LOAD_SCALE;
    }
}


void Map::loadStaticModel(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    bi.setPosition(lump.offset);
    bi.readBytes(&staticModel, sizeof(BSPModel));

    {
        // This block works with input in the quake coordinate system
        lightVolumesGrid.x      = iFloor(staticModel.max.x / 64) - iCeil(staticModel.min.x / 64) + 1;
        lightVolumesInvSizes.x  = lightVolumesGrid.x / (staticModel.max.x - staticModel.min.x);

        // Switch from Quake to G3D coordinate system here
        lightVolumesGrid.y      = iFloor(staticModel.max.z / 128) - iCeil(staticModel.min.z / 128) + 1;
        lightVolumesInvSizes.y  = lightVolumesGrid.y / (staticModel.max.z - staticModel.min.z);

        lightVolumesGrid.z      = iFloor(-staticModel.min.y / 64) - iCeil(-staticModel.max.y / 64) + 1;
        lightVolumesInvSizes.z  = lightVolumesGrid.z / (staticModel.max.y - staticModel.min.y);
    }

    Vector3 a = staticModel.min * G3D_LOAD_SCALE;
    Vector3 b = staticModel.max * G3D_LOAD_SCALE;
    swizzle(a);
    swizzle(b);

    staticModel.min = a.min(b);
    staticModel.max = a.max(b);

    debugAssert(staticModel.max.y > staticModel.min.y);
    debugAssert(staticModel.max.z > staticModel.min.z);
}


void Map::loadDynamicModels(
    BinaryInput&           bi,
    const BSPLump&         lump) {
    int totalModels = lump.length / sizeof(BSPModel);
    if(totalModels < 2){
        return;
    }
    BSPModel* modelData = new BSPModel[totalModels];
    bi.setPosition(lump.offset);
    bi.readBytes(modelData, totalModels * sizeof(BSPModel));

    for(int i = 1; i < totalModels; ++i){
        BSPModel curr;
        Vector3 a = modelData[i].min * G3D_LOAD_SCALE;
        Vector3 b = modelData[i].max * G3D_LOAD_SCALE;
        swizzle(a);
        swizzle(b);

        curr.min = a.min(b);
        curr.max = a.max(b);

        curr.brushIndex = modelData[i].brushIndex;
        curr.faceIndex = modelData[i].faceIndex;
        curr.numOfBrushes = modelData[i].numOfBrushes;
        curr.numOfFaces = modelData[i].numOfFaces;

        debugAssert(curr.max.y >= curr.min.y);
        debugAssert(curr.max.z >= curr.min.z);

        dynamicModels.append(curr);
    }
    delete[] modelData;
}


void Map::loadLightVolumes(
    BinaryInput&           bi,
    const BSPLump&         lump) {
    // size = RGB+RGB+2
    int size = 8;
    lightVolumesCount       = lump.length / size;
    if (lightVolumesCount != lightVolumesGrid.x * lightVolumesGrid.y * lightVolumesGrid.z) {
        logPrintf("WARNING: Quake map has corrupt lightVolumesCount.\n");
        lightVolumesGrid = Vector3int32(0,0,0);
        lightVolumes = NULL;
    } else {
        debugAssertM(lightVolumesCount == lightVolumesGrid.x * lightVolumesGrid.y * lightVolumesGrid.z,
            "Quake map has corrupt lightVolumesCount.");
        lightVolumes            = new LightVolume[lightVolumesCount];
        
        bi.setPosition(lump.offset);

        debugAssert(size == sizeof(LightVolume));
        // Read directly as bytes since endianness won't affect uint8s.
        bi.readBytes(lightVolumes, lightVolumesCount * size);
    }
}

/**
 Sets the specified bit of the byte array.
 */
static void setBit(uint8* byte, int bit) {
    // bit >> 3 == bit / 8
    // bit & 7  == bit % 8
    byte[bit >> 3] |= 1 << (bit & 7);
}


void Map::loadHLVisData(
    BinaryInput&           bi,
    const BSPLump&         lump,
    const BSPLump&         leafLump) {

    bi.setPosition(lump.offset);

    if (lump.length != 0) {

        // Half-Life doesn't have clusters, so assume 1 leaf per cluster.
        visData.clustersCount   = leafArray.size();
        debugAssert(visData.clustersCount > 0);
        debugAssert(visData.clustersCount < 100000);

        // Create the offset table
        uint32* visOffset = new uint32[visData.clustersCount];

        // The offset table is stored in the leafs lump
        bi.setPosition(leafLump.offset);
        for (int i = 0; i < visData.clustersCount; ++i) {
            // Skip "contents"
            bi.skip(4); 
            visOffset[i] = bi.readUInt32();
            // Skip everything else
            bi.skip(20);
        }

        visData.bytesPerCluster = iCeil(log2(visData.clustersCount));

        int size = visData.clustersCount * visData.bytesPerCluster;
        debugAssert(size > 0);
        visData.bitsets = new uint8[size];

        // Allocate a temporary buffer in which to store compressed data.
        int compressedSize = lump.length;
        uint8* pvsBuffer = new uint8[compressedSize];
        bi.setPosition(lump.offset);
        bi.readBytes(pvsBuffer, compressedSize);

        decompressQ1VisData(pvsBuffer, visOffset);


        delete[] pvsBuffer;
        delete[] visOffset;
    } else {
        visData.bitsets = NULL;
    }
}


void Map::loadQ1VisData(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    bi.setPosition(lump.offset);

    if (lump.length != 0) {

        // Quake 1 doesn't have clusters, so assume 1 leaf per cluster.
        visData.clustersCount   = leafArray.size();
        debugAssert(visData.clustersCount > 0);
        debugAssert(visData.clustersCount < 100000);

        // Load the offset table
        uint32* visOffset = new uint32[visData.clustersCount];

        for (int i = 0; i < visData.clustersCount; ++i) {
            visOffset[i] = bi.readUInt32();
            // Skip PHS data
            bi.skip(4);
        }

        visData.bytesPerCluster = iCeil(log2(visData.clustersCount));

        int size = visData.clustersCount * visData.bytesPerCluster;
        debugAssert(size > 0);
        visData.bitsets = new uint8[size];

        // Allocate a temporary buffer in which to store compressed data.
        int compressedSize = lump.length - 4 * (1 + visData.clustersCount);
        uint8* pvsBuffer = new uint8[compressedSize];
        bi.readBytes(pvsBuffer, compressedSize);
        decompressQ1VisData(pvsBuffer, visOffset);

    } else {
        visData.bitsets = NULL;
    }
}


void Map::decompressQ1VisData(
    uint8*   pvsBuffer,
    uint32*  visOffset) {

    // Load the data for each cluster
    for (int i = 0; i < visData.clustersCount; ++i) {
        // Quake stores the vis data run-length encoded.  We must
        // decompress it.
        int v = visOffset[i];
        debugAssert(v < 100000);

        // Data for cluster i
        uint8* visible = &(visData.bitsets[visData.bytesPerCluster * i]);

        // Initialize to zeros
        memset(visible, 0, visData.bytesPerCluster);

        //                                     (Intentionally increments v)
        for (int c = 0; c < visData.clustersCount; ++v) {

           if (pvsBuffer[v] == 0) {
              ++v;     
              c += 8 * pvsBuffer[v];
           } else {
              for (uint8 bit = 1; bit != 0; bit *= 2, ++c) {
                 if (pvsBuffer[v] & bit) {
                    setBit(visible, c);
                 }
              }
           }   

        }

    }
}


void Map::loadQ3VisData(
    BinaryInput&           bi,
    const BSPLump&         lump) {

    bi.setPosition(lump.offset);

    if (lump.length != 0) {

        visData.clustersCount   = bi.readInt32();
        visData.bytesPerCluster = bi.readInt32();

        int size = visData.clustersCount * visData.bytesPerCluster;

        debugAssert(size > 0);
        visData.bitsets = new uint8[size];
        bi.readBytes(visData.bitsets, size);

    } else {
        visData.bitsets = NULL;
    }
}


void Map::verifyData() {

    /*
    // Verify that faceArray are actually within the leaves that are
    // supposed to contain them.
    for (int ct = 0; ct < leafArray.size(); ++ct) {
        const BSPLeaf& leaf = leafArray[ct];
        const Box& bounds = leaf.bounds;

        for (int facesCount = leaf.facesCount - 1; facesCount >= 0; --facesCount) {

            int faceIndex = leafFaceArray[leaf.firstFace + facesCount];
            FaceSet* face = faceArray[faceIndex];

            // Connect the faceArray to the center of their leaf for debugging purposes.
            if (face->getType() == Q3BSPFace::POLYGON) {
                GLBSPPolygon* polygon = dynamic_cast<GLBSPPolygon*>(face);

                const Vector3& v = vertexArray[polygon->firstVertex].position;

                if (! bounds.contains(v)) {
                    debugPrintf("Leaf %d does not contain one of its polygons\n", ct);
                    break;
                }
            }
        }
    }
    */
}

} // _BSPMAP

} // G3D
