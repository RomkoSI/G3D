/**
 \file GLG3D.lib/source/MD3Model.cpp

 Quake III MD3 model loading and posing

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 */
#include "G3D/Sphere.h"
#include "G3D/BinaryInput.h"
#include "G3D/fileutils.h"
#include "G3D/HashTrait.h"
#include "G3D/TextInput.h"
#include "GLG3D/MD3Model.h"
#include "G3D/FileSystem.h"
#include "G3D/Any.h"
#include "GLG3D/UniversalSurface.h"
#include "G3D/Ray.h"

namespace G3D {

// 60 quake units ~= 2 meters
static const float Q3_LOAD_SCALE = 2.0f / 60.0f;
static const float BLEND_TIME = 0.125f;

const String& MD3Model::className() const {
    static String s("MD3Model");
    return s;
}

inline static Vector3 vectorToG3D(const Vector3& v) {
    return Vector3(v.y, v.z, -v.x);
}

/** Takes a point in the Q3 coordinate system to one in the G3D coordinate system */
inline static Vector3 pointToG3D(const Vector3& v) {
    return vectorToG3D(v) * Q3_LOAD_SCALE;
}


static shared_ptr<UniversalMaterial> defaultMaterial() {
    static shared_ptr<UniversalMaterial> m;
    if (isNull(m)) {
        m = UniversalMaterial::createDiffuse(Color3::white() * 0.99f);
    }
    return m;
}


MD3Model::Pose::Pose() {
    for (int i = 0; i < NUM_ANIMATED_PARTS; ++i) {
        time[i] = 0;
        prevFrame[i] = 0;
    }
    anim[PART_LOWER] = AnimType::LOWER_WALK;
    prevAnim[PART_LOWER] = AnimType::LOWER_WALK;
    anim[PART_UPPER] = AnimType::UPPER_STAND;
    prevAnim[PART_UPPER] = AnimType::UPPER_STAND;

    for (int i = 0; i < NUM_PARTS; ++i) {
        rotation[i] = Matrix3::identity();
    }
}


void MD3Model::simulatePose(Pose& pose, SimTime dt) const {
    if (isNaN(dt)) {
        // the scene is paused
        dt = 0;
    }

    for (int i = 0; i < NUM_ANIMATED_PARTS; ++i) {
        if(pose.anim[i] != pose.prevAnim[i]) {
            pose.prevFrame[i] = iCeil(findFrameNum(pose.prevAnim[i], pose.time[i]));
            pose.prevAnim[i] = pose.anim[i];
            pose.time[i] = -BLEND_TIME;
        } else {        
            pose.time[i] += dt;
        }
    }
}



//////////////////////////////////////////////////////////////////////////////////////////////

void MD3Model::Skin::loadSkinFile(const String& filename, PartSkin& partSkin) {
    // Read file as string to parse easily
    const String& skinFile = readWholeFile(filename);

    // Split the file into lines
    Array<String> lines = stringSplit(skinFile, '\n');

    // Parse each line for surface name + texture
    for (int lineIndex = 0; lineIndex < lines.length(); ++lineIndex) {
        String line = trimWhitespace(lines[lineIndex]);

        String::size_type commaPos = line.find(',');

        // Quit parsing this line if no comma is found or it is at the end of the line
        if ((commaPos == (line.length() - 1)) || (commaPos == String::npos)) {
            continue;
        }

        // Figure out actual texture filename
        const String& triListName = line.substr(0, commaPos);
        const String& textureName = filenameBaseExt(line.substr(commaPos + 1));

        if (textureName == "nodraw") {
            // Intentionally NULL material
            partSkin.set(triListName, shared_ptr<UniversalMaterial>());
        } else {
            // Assume textures are relative to the current .skin file
            const String& textureFilename = FilePath::concat(FilePath::parent(filename), textureName);

            if (FileSystem::exists(textureFilename)) {
                shared_ptr<Texture> texture = Texture::fromFile(textureFilename, ImageFormat::AUTO(), Texture::DIM_2D, true, Texture::Preprocess::quake());
                partSkin.set(triListName, UniversalMaterial::createDiffuse(texture));
            } else {
                partSkin.set(triListName, defaultMaterial());
            }
        }
    }
}


shared_ptr<MD3Model::Skin> MD3Model::Skin::create
 (const String& path,
  const String& lowerSkin, 
  const String& upperSkin, 
  const String& headSkin) {
    
    shared_ptr<Skin> s(new Skin());

    if (! headSkin.empty()) {
        s->partSkin.resize(3);
    } else if (! upperSkin.empty()) {
        s->partSkin.resize(2);
    } else if (! lowerSkin.empty()) {
        s->partSkin.resize(1);
    } else {
        alwaysAssertM(false, "No skins specified!");
    }

    // Load actual .skin files
    const Array<String> filename
        (FilePath::concat(path, lowerSkin), 
         FilePath::concat(path, upperSkin),
         FilePath::concat(path, headSkin));

    for (int i = 0; i < s->partSkin.size(); ++i) {
        loadSkinFile(filename[i], s->partSkin[i]);
    }
    
    return s;
}


shared_ptr<MD3Model::Skin> MD3Model::Skin::create
(const String& commonPath,
 const String& commonSuffix) {
    return MD3Model::Skin::create(
        commonPath,
        "lower_" + commonSuffix + ".skin",
        "upper_" + commonSuffix + ".skin",
        "head_" + commonSuffix + ".skin");
}


shared_ptr<MD3Model::Skin> MD3Model::Skin::create(const Any& any) {
    shared_ptr<Skin> s(new Skin());

    any.verifyType(Any::ARRAY);
    any.verifyName("MD3Model::Skin");
    s->partSkin.resize(any.size());

    for (int i = 0; i < s->partSkin.size(); ++i) {
        const Any& src = any[i];
        PartSkin& dst = s->partSkin[i];
        if (src.type() == Any::STRING) {
            loadSkinFile(src.resolveStringAsFilename(), dst);
        } else {
            for (Table<String, Any>::Iterator it = src.table().begin(); it.isValid(); ++it) {
                if (it->value.type() == Any::NIL) {
                    dst.set(it->key, shared_ptr<UniversalMaterial>());
                } else {
                    dst.set(it->key, UniversalMaterial::create(UniversalMaterial::Specification(it->value)));
                }
            }
        }
    }

    return s;
}

//////////////////////////////////////////////////////////////////////////////////////////////

MD3Model::PoseSequence::PoseSequence(const Any& any) {
    if (beginsWith(any.name(), "MD3::PoseSequence")) {
        AnyTableReader propertyTable(any);
        propertyTable.getIfPresent("poses", poses);
        propertyTable.getIfPresent("times", time);
        propertyTable.verifyDone();
    }
}


Any MD3Model::PoseSequence::toAny() const {
    Any a(Any::TABLE, "MD3::PoseSequence");
    a["poses"] = poses;
    a["times"] = time;
    return a;
}
       

void MD3Model::PoseSequence::getPose(float gameTime, Pose& pose) const {
    int index = time.length() - 1;
    for(int i = 0; i < time.length(); ++i) {
        if(gameTime < time[i]) {	
            index = i - 1;
            break;
        }
    } 
    if(index != -1) {
        if(index != 0) {
            if (affectsLower(poses[index - 1])) {
                pose.anim[PART_LOWER] = poses[index - 1];
            } 
            if (affectsUpper(poses[index - 1])) {
                pose.anim[PART_UPPER] = poses[index - 1];
            }
        }
        if (affectsLower(poses[index])) {
            pose.anim[PART_LOWER] = poses[index];
        } 
        if (affectsUpper(poses[index])) {
            pose.anim[PART_UPPER] = poses[index];
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
MD3Model::Specification::Specification(const Any& any) {
    if (any.type() == Any::STRING) {
        directory = any.resolveStringAsFilename();
    } else {
        any.verifyName("MD3Model::Specification");
        for (Table<String, Any>::Iterator it = any.table().begin(); it.isValid(); ++it) {
            const String& key = toLower(it->key);
            if (key == "directory") {
                directory = it->value.resolveStringAsFilename();
            } else if (key == "defaultskin") {
                defaultSkin = Skin::create(it->value);
            } else {
                any.verify(false, "Illegal key: " + it->key);
            }
        }
    }
}


shared_ptr<MD3Model> MD3Model::create(const MD3Model::Specification& spec, const String& name) {
    shared_ptr<MD3Model> model(new MD3Model());

    model->loadSpecification(spec);
    if (name == "") {
        model->m_name = FilePath::baseExt(spec.directory);
    } else {
        model->m_name = name;
    }

    return model;
}



/** Definition of MD3 file surface header structure.  These correspond to triLists */
struct MD3SurfaceHeader {
public:
    String ident;
    String name;
    int         flags;

    int         numFrames;
    int         numShaders;
    int         numVertices;
    int         numTriangles;

    int         offsetTriangles;
    int         offsetShaders;
    int         offsetUVs;
    int         offsetVertices;
    int         offsetEnd;

    MD3SurfaceHeader(BinaryInput& bi) {
        ident           = bi.readInt32();
        name            = bi.readString(64);
        flags           = bi.readInt32();

        numFrames       = bi.readInt32();
        numShaders      = bi.readInt32();
        numVertices     = bi.readInt32();
        numTriangles    = bi.readInt32();

        offsetTriangles = bi.readInt32();
        offsetShaders   = bi.readInt32();
        offsetUVs       = bi.readInt32();
        offsetVertices  = bi.readInt32();
        offsetEnd       = bi.readInt32();
    }
};

// Definition of MD3 file header structure
class MD3FileHeader {
public:
    String ident;
    int         version;
    String name;
    int         flags;

    int         numFrames;
    int         numTags;
    int         numSurfaces;
    int         numSkins;

    int         offsetFrames;
    int         offsetTags;
    int         offsetSurfaces;
    int         offsetEnd;

    /** True if this file had the right version number */
    bool        ok;

    MD3FileHeader(BinaryInput& bi) : ok(true) {
        ident   = bi.readString(4);
        version = bi.readInt32();

        // validate header before continuing
        if ((ident != "IDP3") || (version != 15)) {
            ok = false;
            return;
        }

        name            = bi.readString(64);
        flags           = bi.readInt32();

        numFrames       = bi.readInt32();
        numTags         = bi.readInt32();
        numSurfaces     = bi.readInt32();
        numSkins        = bi.readInt32();

        offsetFrames    = bi.readInt32();
        offsetTags      = bi.readInt32();
        offsetSurfaces  = bi.readInt32();
        offsetEnd       = bi.readInt32();
    }
};

/** MD3Part Model loader helper for MD3Model.  Loads an individual .md3 model. */
class MD3Part : public ReferenceCountedObject {
    // See: http://icculus.org/homepages/phaethon/q3a/formats/md3format.html

    // Terminology:
    //
    // Q3 calls an attachment point a "tag"
    //
    // Player models contain lower.md3, upper.md3, and head.md3.  The lower part is the root.
    // the upper is attached to the lower, and the weapon and head are attached to the upper.

private:
    friend class MD3Model;
    /** TriMesh */
    struct TriList {
        /** helper data copied from the surface header */
        int                                     m_numFrames;

        int                                     m_numVertices;

        /** Geometry for each frame of animation */
        Array<MeshAlg::Geometry>                m_geometry;

        /** Indexed triangle list. */
        Array<int>                              m_indexArray;
        IndexStream                             m_gpuIndex;

        /** array of texture coordinates for each vertex */
        Array<Vector2>                          m_textureCoords;

        CPUVertexArray                          m_cpuVertexArray;

        String                             m_name;
    };

    struct FrameData {
        Vector3                                 m_bounds[2];

        /** Parts are offset by a translation only. */
        Vector3                                 m_localOrigin;

        float                                   m_radius;

        String                             m_name;

        /** map of tag name to tag data for each frame*/
        Table<String, CoordinateFrame>     m_tags;
    };

    /** surface data */
    Array<TriList>              m_triListArray;

    /** per-frame bounding box and translation information */
    Array<FrameData>            m_frames;

    int                         m_numFrames;

    String                 m_modelDir;
    String                 m_modelName;

    MD3Part();

    virtual ~MD3Part() {}

    bool intersect(const Ray& r,
                   const CFrame& cframe,
                   int frame1,
                   int frame2,
                   float alpha,
                   float& maxDistance,
                   int    partType,
                   shared_ptr<MD3Model> model,
                   Model::HitInfo& info = Model::HitInfo::ignore,
                   const shared_ptr<Entity>& entity = shared_ptr<Entity>(),
                   const shared_ptr<MD3Model::Skin>& skin = shared_ptr<MD3Model::Skin>());

    CoordinateFrame tag(const int frame1, const int frame2, const float alpha, const String& name) const;

    bool loadFile(const String& filename);

    void loadSurface(BinaryInput& bi, TriList& triList);

    void loadFrame(BinaryInput& bi, FrameData& frameData);

    void loadTag(BinaryInput& bi, FrameData& frameData);
};


///////////////////////////////////////////////////////////////////////////////////////////////////////

MD3Part::MD3Part() : m_numFrames(0) {
}

bool MD3Part::intersect
   (const Ray& r, 
    const CFrame& cframe, 
    int frame1,
    int frame2,
    float alpha,
    float& maxDistance,
    int    partType,
    shared_ptr<MD3Model> model,
    Model::HitInfo& info, 
    const shared_ptr<Entity>& entity,
    const shared_ptr<MD3Model::Skin>& skin) {

    shared_ptr<UniversalMaterial> mat;
    bool hit = false;
    Point3 p[3];
    float w0 = 0, w1 = 0, w2=0;
    int frameIndex[2];
    float weightIndex[2];
    Ray intersectingRay;
    for(int i = 0; i < m_triListArray.length(); ++i) {
        const TriList& triList = m_triListArray[i];
        if (isNull(skin)) {
            mat = defaultMaterial();
        } else if (skin->partSkin.size() > partType) {
            const MD3Model::Skin::PartSkin& partSkin = skin->partSkin[partType];
            shared_ptr<UniversalMaterial>* ptr = partSkin.getPointer(triList.m_name);
            if (ptr) {
                mat = *ptr;
            } else {
                mat = defaultMaterial();
            }
        }
        frameIndex[0] = frame1;
        frameIndex[1] = frame2;
        weightIndex[1] = alpha;
        weightIndex[0] = 1.0f - alpha;
        CFrame partFrame = cframe;
        FrameData frameData1 = m_frames[frameIndex[0]];
        FrameData frameData2 = m_frames[frameIndex[1]];
        partFrame.translation += cframe.rotation * (frameData1.m_localOrigin.lerp(frameData2.m_localOrigin, weightIndex[1]));
        intersectingRay = partFrame.toObjectSpace(r);
        Sphere bounds1(frameData1.m_localOrigin, frameData1.m_radius);
        float intersectBounds = intersectingRay.intersectionTime(bounds1);
        if(intersectBounds < maxDistance) {
            const MeshAlg::Geometry& geom1 = triList.m_geometry[frameIndex[0]];
            const MeshAlg::Geometry& geom2 = triList.m_geometry[frameIndex[1]];

            const int N = geom1.vertexArray.size();
            Array<Vector3> vertexArray;
            vertexArray.resize(N);
            for (int v = 0; v < N; ++v) {
               vertexArray[v] = geom1.vertexArray[v].lerp(geom2.vertexArray[v], weightIndex[1]);
            }
            for(int k = 0; k < triList.m_indexArray.length(); k += 3) {
                Vector3 normal;
                for (int j = 0; j < 3; ++j) {  
                    p[j] = geom1.vertexArray[triList.m_indexArray[k + j]];
                }
                float testTime = intersectingRay.intersectionTime(p[0], p[1], p[2], w0, w1, w2);
                if (testTime < maxDistance) {
                    maxDistance = testTime;
                    hit = true;
                    info.set(
                        model, 
                        entity, 
                        mat,
                        partFrame.normalToWorldSpace((p[2] - p[0]).cross(p[1] - p[0]).direction()),
                        r.origin() + maxDistance * r.direction(),
                        triList.m_name,
                        0,
                        k/3,
                        w0,
                        w2);
                }
            }
        }        
    }
    return hit;
}

bool MD3Part::loadFile(const String& filename) {
    BinaryInput bi(filename, G3D_LITTLE_ENDIAN);

    // Read file header
    MD3FileHeader md3File(bi);
    if (! md3File.ok) {
        return false;
    }

    m_modelDir = filenamePath(filename);
    m_modelName = filenameBase(filename);

    m_numFrames = md3File.numFrames;

    // Allocate frames
    m_frames.resize(md3File.numFrames, false);

    // Read in frame data
    bi.setPosition(md3File.offsetFrames);
    for (int frameIndex = 0; frameIndex < m_frames.size(); ++frameIndex) {
        loadFrame(bi, m_frames[frameIndex]);
    }

    // Read in tag data
    bi.setPosition(md3File.offsetTags);
    for (int frameIndex = 0; frameIndex < md3File.numFrames; ++frameIndex) {
        for (int tagIndex = 0; tagIndex < md3File.numTags; ++tagIndex) {
            loadTag(bi, m_frames[frameIndex]);
        }
    }

    // Allocate surfaces
    m_triListArray.resize(md3File.numSurfaces, false);

    // Read in surface data
    bi.setPosition(md3File.offsetSurfaces);
    for (int surfaceIndex = 0; surfaceIndex < md3File.numSurfaces; ++surfaceIndex) {
        loadSurface(bi, m_triListArray[surfaceIndex]);
    }

    return true;
}


void MD3Part::loadSurface(BinaryInput& bi, TriList& triList) {
    // Save start of surface
    int surfaceStart = static_cast<int>(bi.getPosition());

    // Read surface header
    MD3SurfaceHeader md3Surface(bi);


    // Read surface data
    triList.m_name = md3Surface.name;

    // Store off some helper data
    triList.m_numFrames = md3Surface.numFrames;
    triList.m_numVertices = md3Surface.numVertices;

    // Read triangles
    bi.setPosition(surfaceStart + md3Surface.offsetTriangles);

    triList.m_indexArray.resize(md3Surface.numTriangles * 3);

    for (int index = 0; index < md3Surface.numTriangles; ++index) {
        triList.m_indexArray[index * 3] = bi.readInt32();
        triList.m_indexArray[index * 3 + 1] = bi.readInt32();
        triList.m_indexArray[index * 3 + 2] = bi.readInt32();
    }

    shared_ptr<VertexBuffer> vb = VertexBuffer::create(triList.m_indexArray.size() * sizeof(int), VertexBuffer::WRITE_ONCE);
    triList.m_gpuIndex = IndexStream(triList.m_indexArray, vb);

    // Read shaders
    bi.setPosition(surfaceStart + md3Surface.offsetShaders);

    for (int shaderIndex = 0; shaderIndex < md3Surface.numShaders; ++shaderIndex) {
        // Read shader name and index (need this code to update the file position correctly)
    // Currently discarding shader name and index
        bi.readString(64);
        bi.readInt32();
        
        /*
        // Find base filename for shader
        const String& shaderName = filenameBaseExt(shaderPath);

        Shader/textures loaded elsewhere in the current implementation
        // Ignore empty shader names for now (filled in with .skin file)
        if (! shaderName.empty() && FileSystem::exists(pathConcat(m_modelDir, shaderName))) {
            triList.m_texture = Texture::fromFile(m_modelDir + shaderName, ImageFormat::AUTO(), Texture::DIM_2D);
        }
        */
    }

    // Read texture coordinates
    bi.setPosition(surfaceStart + md3Surface.offsetUVs);

    triList.m_textureCoords.resize(md3Surface.numVertices);

    for (int coordIndex = 0; coordIndex < md3Surface.numVertices; ++coordIndex) {
        triList.m_textureCoords[coordIndex].x = bi.readFloat32();
        triList.m_textureCoords[coordIndex].y = bi.readFloat32();
    }

    // Read vertices
    bi.setPosition(surfaceStart + md3Surface.offsetVertices);

    triList.m_geometry.resize(md3Surface.numFrames);

    const int N = md3Surface.numVertices;
    for (int frameIndex = 0; frameIndex < md3Surface.numFrames; ++frameIndex) {
        MeshAlg::Geometry& geom = triList.m_geometry[frameIndex];
        geom.vertexArray.resize(N);
        geom.normalArray.resize(N);

        for (int vertexIndex = 0; vertexIndex < N; ++vertexIndex) {
            Vector3 vertex;
            for (int j = 0; j < 3; ++j) {
                vertex[j] = bi.readInt16();
            }

            // MD3 scales vertices by 64 when packing them into integers
            vertex *= (1.0f / 64.0f);

            geom.vertexArray[vertexIndex] = pointToG3D(vertex);

            // Decoding is at the bottom of this page:
            // http://icculus.org/homepages/phaethon/q3a/formats/md3format.html
            int16 encNormal = bi.readInt16();

            float nlat = static_cast<float>((encNormal >> 8) & 0xFF);
            float nlng = static_cast<float>(encNormal & 0xFF);

            nlat *= static_cast<float>(pi() / 128.0f);
            nlng *= static_cast<float>(pi() / 128.0f);

            Vector3 normal;
            normal.x = cosf(nlat) * sinf(nlng);
            normal.y = sinf(nlat) * sinf(nlng);
            normal.z = cosf(nlng);

            geom.normalArray[vertexIndex] = normal;
        }
    }

    // Ensure at the end of surface
    bi.setPosition(surfaceStart + md3Surface.offsetEnd);
}


void MD3Part::loadFrame(BinaryInput& bi, FrameData& frameData) {
    frameData.m_bounds[0] = pointToG3D(bi.readVector3());

    frameData.m_bounds[1] = pointToG3D(bi.readVector3());

    frameData.m_localOrigin = pointToG3D(bi.readVector3());

    frameData.m_radius = bi.readFloat32() * Q3_LOAD_SCALE;

    // TODO: why is name ignored?  Should we at least assert it or something?
    const String& name = bi.readString(16);
    (void)name;
}


void MD3Part::loadTag(BinaryInput& bi, FrameData& frameData) {
    String name = bi.readString(64);

    CoordinateFrame tag;
    tag.translation = pointToG3D(bi.readVector3());

    Matrix3 raw;
    for (int a = 0; a < 3; ++a) {
        raw.setColumn(a, vectorToG3D(bi.readVector3()));
    }

    // Perform the vectorToG3D transform on the columns
    tag.rotation.setColumn(Vector3::X_AXIS,  raw.column(Vector3::Y_AXIS));
    tag.rotation.setColumn(Vector3::Y_AXIS,  raw.column(Vector3::Z_AXIS));
    tag.rotation.setColumn(Vector3::Z_AXIS, -raw.column(Vector3::X_AXIS));

    frameData.m_tags.set(name, tag);
}


CoordinateFrame MD3Part::tag(const int frame1, const int frame2, const float interp, const String& name) const {


    CoordinateFrame blendedFrame = m_frames[frame1].m_tags[name].lerp(m_frames[frame2].m_tags[name], interp);

    blendedFrame.translation += m_frames[frame1].m_localOrigin.lerp(m_frames[frame2].m_localOrigin, interp);

    return blendedFrame;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

MD3Model::MD3Model() {
    System::memset(m_parts, 0, sizeof(m_parts));
}


MD3Model::~MD3Model() {
    for (int partIndex = 0; partIndex < NUM_PARTS; ++partIndex) {
        if (m_parts[partIndex]) {
            delete m_parts[partIndex];
            m_parts[partIndex] = NULL;
        }
    }
}


void MD3Model::loadSpecification(const Specification& spec) {
    // Load animation.cfg file
    loadAnimationCfg(pathConcat(spec.directory, "animation.cfg"));

    m_defaultSkin = spec.defaultSkin;

    // Load legs
    String filename = FilePath::concat(spec.directory, "lower.md3");

    m_parts[PART_LOWER] = new MD3Part();
    
    if (! m_parts[PART_LOWER]->loadFile(filename)) {
        debugAssertM(false, format("Unable to load %s.", filename.c_str()));
        return;
    }

    // Load torso
    {
        filename = FilePath::concat(spec.directory, "upper.md3");

        m_parts[PART_UPPER] = new MD3Part();
        
        if (! m_parts[PART_UPPER]->loadFile(filename)) {
            debugAssertM(false, format ("Unable to load %s.", filename.c_str()));
            return;
        }
    }

    // Load head
    {
        filename = FilePath::concat(spec.directory, "head.md3");

        m_parts[PART_HEAD] = new MD3Part();
        
        if (! m_parts[PART_HEAD]->loadFile(filename)) {
            debugAssertM(false, format("Unable to load %s.", filename.c_str()));
            return;
        }
    }
}


void MD3Model::loadAnimationCfg(const String& filename) {
    TextInput::Settings settings;
    settings.generateNewlineTokens = true;

    TextInput ti(filename, settings);

    for (int animIndex = 0; animIndex < AnimType::NUM_ANIMATIONS; ++animIndex) {
        // Check if animations have started in file
        while (ti.hasMore() && (ti.peek().extendedType() != Token::INTEGER_TYPE)) {
            // Eat until next line starting with an integer token
            while (ti.hasMore() && (ti.peek().type() != Token::NEWLINE)) {
                ti.read();
            }

            ti.read();
        }

        // Return early if this is an invalid file
        if (ti.peek().type() == Token::END) {
            debugAssertM(ti.peek().type() != Token::END, ("Invalid animation.cfg file!"));
            return;
        }

        m_animations[animIndex].start = static_cast<float>(ti.readNumber());
        m_animations[animIndex].num = static_cast<float>(ti.readNumber());
        m_animations[animIndex].loop = static_cast<float>(ti.readNumber());
        m_animations[animIndex].fps = static_cast<float>(ti.readNumber());

        if (ti.peek().type() == Token::NEWLINE) {
            ti.readNewlineToken();
        } else {
            debugAssert(animIndex == (AnimType::NUM_ANIMATIONS - 1));
        }
    }

    // Loop through all legs animations and adjust starting frame number to be relative to part
    float numTorsoAnimations = m_animations[AnimType::START_LOWER].start - m_animations[AnimType::START_UPPER].start;
    for (int animIndex = AnimType::START_LOWER; animIndex <= AnimType::END_LOWER; ++animIndex) {
        m_animations[animIndex].start -= numTorsoAnimations;
    }
}


void MD3Model::pose(Array<shared_ptr<Surface> >& posedModelArray, const CoordinateFrame& cframe, const CFrame& previousFrame, const Pose& pose, const shared_ptr<Entity>& entity) {

    // Coordinate frame built up from lower part
    CFrame baseFrame = cframe;
    CFrame previousBaseFrame = previousFrame;

    // Pose lower part
    if (! m_parts[PART_LOWER]) {
        return;
    }

    baseFrame.rotation *= pose.rotation[PART_LOWER];
    previousBaseFrame.rotation *= pose.rotation[PART_LOWER];

    posePart(PART_LOWER, pose, posedModelArray, baseFrame, previousBaseFrame, entity);
	
    int frame1 = 0;
	int frame2 = 0;
	float interp = 0;
	computeFrameNumbers(pose, PART_LOWER, frame1, frame2, interp);

    // Pose upper part
    if (! m_parts[PART_UPPER]) {
        return;
    }

    baseFrame = baseFrame * m_parts[PART_LOWER]->tag(frame1, frame2, interp, "tag_torso");
    baseFrame.rotation *= pose.rotation[PART_UPPER];

    previousBaseFrame = previousBaseFrame * m_parts[PART_LOWER]->tag(frame1, frame2, interp, "tag_torso");
    previousBaseFrame.rotation *= pose.rotation[PART_UPPER];

    posePart(PART_UPPER, pose, posedModelArray, baseFrame, previousBaseFrame, entity);

	computeFrameNumbers(pose, PART_UPPER, frame1, frame2, interp);

    // Pose head part
    if (! m_parts[PART_HEAD]) {
        return;
    }

    baseFrame = baseFrame * m_parts[PART_UPPER]->tag(frame1, frame2, interp, "tag_head");
    baseFrame.rotation *= pose.rotation[PART_HEAD];

    previousBaseFrame = previousBaseFrame * m_parts[PART_UPPER]->tag(frame1, frame2, interp, "tag_head");
    previousBaseFrame.rotation *= pose.rotation[PART_HEAD];

    posePart(PART_HEAD, pose, posedModelArray, baseFrame, previousBaseFrame, entity);
}


CoordinateFrame MD3Model::weaponFrame(const CoordinateFrame& cframe, const Pose& pose) const {

    // Coordinate frame built up from lower part
    CoordinateFrame baseFrame = cframe;

    // Pose lower part
    if (! m_parts[PART_LOWER] || ! m_parts[PART_UPPER]) {
        return baseFrame;
    }

	int frame1, frame2 = 0;
	float interp = 0;
	computeFrameNumbers(pose, PART_LOWER, frame1, frame2, interp);
    
    baseFrame.rotation *= pose.rotation[PART_LOWER];
    baseFrame = baseFrame * m_parts[PART_LOWER]->tag(frame1, frame2, interp, "tag_torso");

	computeFrameNumbers(pose, PART_UPPER, frame1, frame2, interp);
    return baseFrame * m_parts[PART_UPPER]->tag(frame1, frame2, interp, "tag_weapon");
}

bool MD3Model::intersect(const Ray& r, 
                         const CoordinateFrame& cframe, 
                         const Pose& pose, 
                         float& maxDistance,
                         Model::HitInfo& info,
                         const shared_ptr<Entity>& entity) {
    shared_ptr<Material> mat;
    shared_ptr<MD3Model::Skin> skin;
    if (isNull(pose.skin)) {
        skin = m_defaultSkin;
    } else {
        skin = pose.skin;
    }

    bool hit = false;
    static String tags[3] = {"tag_lower", "tag_torso", "tag_head"};
    float lastframe;
    CFrame bframe = cframe;
    int frame1, frame2 = 0;
	float alpha = 0;

    for( int partType = 0; partType < NUM_PARTS; ++partType) {
        frame1 = 0;
		frame2 = 0;
		alpha = 0;
        if (partType != PART_HEAD) {
			computeFrameNumbers(pose, (PartType)partType, frame1, frame2, alpha);
        }
        if(partType != PART_LOWER) {
            bframe = bframe * m_parts[partType - 1]->tag(frame1, frame2, alpha, tags[partType]);   
        }        
        bframe.rotation *= pose.rotation[partType];
        bool justhit = m_parts[partType]->intersect(r, bframe, frame1, frame2, alpha, maxDistance, partType, dynamic_pointer_cast<MD3Model>(shared_from_this()), info, entity, skin);
        lastframe = (float)frame2;

        hit = justhit || hit;
          
    }
    return hit;
}

struct BlendWeights {
    int frame[2];
    float weight[2];

    BlendWeights() {
        frame[0] = frame[1] = -1;
        weight[0] = weight[1] = 0;
    }
};


void MD3Model::posePart(PartType partType, const Pose& pose, Array<shared_ptr<Surface> >& posedModelArray, const CFrame& cframe, const CFrame& previousFrame, const shared_ptr<Entity>& entity) {
    const MD3Part* part = m_parts[partType];

    shared_ptr<Skin> skin;
    if (isNull(pose.skin)) {
        skin = m_defaultSkin;
    } else {
        skin = pose.skin;
    }

    for (int surfaceIndex = 0; surfaceIndex < part->m_triListArray.length(); ++surfaceIndex) {

        const MD3Part::TriList& triList = part->m_triListArray[surfaceIndex];
        shared_ptr<UniversalMaterial> material;

        // Make sure there is a skin for this part
        if (isNull(skin)) {
            material = defaultMaterial();
        } else if (skin->partSkin.size() > partType) {
            const Skin::PartSkin& partSkin = skin->partSkin[partType];

            // See if there is a skin for this trilist
            shared_ptr<UniversalMaterial>* ptr = partSkin.getPointer(triList.m_name);
            if (ptr) {
                material = *ptr;
            } else {
                // Use default material
                material = defaultMaterial();
            }
        }

        if (isNull(material)) {
            // triLists with an intentionally NULL material do not render
            continue;
        }

        /////////////////////////////////////////////////////////////////
        // TODO: abstract this blending logic into a method
        BlendWeights b;

        int frameNum1 = 0; 
        int frameNum2 = 0;
        float alpha = 0;
        if (partType != PART_HEAD) {
            computeFrameNumbers(pose, partType, frameNum1, frameNum2, alpha);
        }

        // Calculate frames for blending
        // TODO: handle blends between different animations the way that MD2Model does
        b.frame[0] = frameNum1;
        b.frame[1] = frameNum2;
        b.weight[1] = alpha;
        b.weight[0] = 1.0f - alpha;

        const MD3Part::FrameData& frame1 = part->m_frames[b.frame[0]];
        const MD3Part::FrameData& frame2 = part->m_frames[b.frame[1]];
        
        CFrame partFrame = cframe;
        partFrame.translation += cframe.rotation * (frame1.m_localOrigin.lerp(frame2.m_localOrigin, b.weight[1]));

        CFrame previousPartFrame = previousFrame;
        previousPartFrame.translation += previousFrame.rotation * (frame1.m_localOrigin.lerp(frame2.m_localOrigin, b.weight[1]));

        /////////////////////////////////////////////////////////////////

        // Copy blended vertex data for frame
        const MeshAlg::Geometry& geom1 = triList.m_geometry[b.frame[0]];
        const MeshAlg::Geometry& geom2 = triList.m_geometry[b.frame[1]];

        const int N = geom1.vertexArray.size();

        CPUVertexArray* cpuVertexArray = const_cast<CPUVertexArray*>(&triList.m_cpuVertexArray);

        cpuVertexArray->vertex.resize(N);
        for (int v = 0; v < N; ++v) {
            CPUVertexArray::Vertex& vertex = cpuVertexArray->vertex[v];
            vertex.position     = geom1.vertexArray[v].lerp(geom2.vertexArray[v], b.weight[1]);
            vertex.normal       = geom1.normalArray[v].lerp(geom2.normalArray[v], b.weight[1]);
            vertex.tangent      = Vector4::nan();
            vertex.texCoord0    = triList.m_textureCoords[v];
        }

        UniversalSurface::CPUGeom cpuGeom(&triList.m_indexArray, cpuVertexArray);

        // The final "this" argument is a back pointer so that the index array can't be 
        // garbage collected while the surface still exists.
        shared_ptr<UniversalSurface> surface =
            UniversalSurface::create
               (part->m_modelName + "::" + triList.m_name, 
                partFrame, 
                previousPartFrame,
                material,
                UniversalSurface::GPUGeom::create(), 
                cpuGeom, 
                dynamic_pointer_cast<MD3Model>(shared_from_this()), 
                pose.expressiveLightScatteringProperties,
                dynamic_pointer_cast<Model>(shared_from_this()),
                entity);

        // Upload data to the GPU
        const shared_ptr<UniversalSurface::GPUGeom>& gpuGeom = surface->gpuGeom();
        surface->cpuGeom().copyVertexDataToGPU(gpuGeom->vertex, gpuGeom->normal, gpuGeom->packedTangent, 
                                    gpuGeom->texCoord0, gpuGeom->texCoord1, gpuGeom->vertexColor, VertexBuffer::WRITE_EVERY_FRAME);

        gpuGeom->index = triList.m_gpuIndex;

        Sphere sphereBounds(frame1.m_localOrigin, frame1.m_radius);
        sphereBounds.merge(Sphere(frame2.m_localOrigin, frame2.m_radius));
        gpuGeom->sphereBounds = sphereBounds;
        
        AABox bounds((frame1.m_bounds[0].min(frame1.m_bounds[1]), frame1.m_bounds[0].max(frame1.m_bounds[1])));
        bounds.merge(AABox(frame1.m_bounds[0].min(frame1.m_bounds[1]), frame2.m_bounds[0].max(frame2.m_bounds[1])));
        
        gpuGeom->boxBounds = bounds;
        posedModelArray.append(surface);
    }
}


float MD3Model::findFrameNum(AnimType animType, SimTime animTime) const {
    debugAssert(animType < AnimType::NUM_ANIMATIONS);

    float firstFrame       = m_animations[animType].start;
    float relativeFrameNum = 0.0f;

    float initialAnimationDuration = ((m_animations[animType].num) / m_animations[animType].fps);

    // Animation goes from first frame to last frame, then jumps immediately to the first loop frame
    // Will need to be changed when blending animations
    if (animTime < initialAnimationDuration) {
        // Less than 1 loop complete, no need to account for "loop" value
        relativeFrameNum = static_cast<float>(animTime / initialAnimationDuration) * (m_animations[animType].num - 1);
    } else {
        if (m_animations[animType].loop > 0.0f) {
            // otherwise find actual frame number after number of loops
            
            float loopDuration = ((m_animations[animType].loop) / m_animations[animType].fps);
            animTime -= loopDuration;

            // How far into the last loop
            float timeIntoLastLoop = fmod(static_cast<float>(animTime), loopDuration);

            // "loop" works by specifying the number of frames to loop over minus one
            // so a loop of 1 with num frames 5 means looping starts at frame 4 with frames {1, 2, 3, 4, 5} originally

            relativeFrameNum  = (m_animations[animType].num - m_animations[animType].loop);

            // (m_animations[animType].loop-1) is the maximum frame offset
            relativeFrameNum += (timeIntoLastLoop / loopDuration) * (m_animations[animType].loop-1);
        } else { // Use the final frame
            relativeFrameNum += m_animations[animType].num - 1;
        }
    }

    return firstFrame + relativeFrameNum;
}


void MD3Model::computeFrameNumbers(const MD3Model::Pose& pose, PartType partType, int& kf0, int& kf1, float& alpha) const {
    
    AnimType anim = pose.anim[partType];
    SimTime time = pose.time[partType];

    if (pose.time[partType] >= 0) {
        float frameNum = findFrameNum(anim, time);
        kf0 = iFloor(frameNum);
        kf1 = iCeil(frameNum) % iCeil(m_animations[anim].num + m_animations[anim].start);
        alpha = frameNum - kf0;
    }
    else {
        kf0 = pose.prevFrame[partType];
        kf1 = int(m_animations[anim].start);
        alpha = float(clamp(1.0f + float(time / BLEND_TIME), 0.0f, 1.0f));
    }
}

} // namespace G3D
