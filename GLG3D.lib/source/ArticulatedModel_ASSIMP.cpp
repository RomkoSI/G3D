/**
 \file GLG3D/source/ArticulatedModel_COLLADA.cpp

 \author Michael Mara, http://www.illuminationcodified.com/
 \created 2013-01-09
 \edited  2013-01-09
 
*/
#include "GLG3D/ArticulatedModel.h"
#ifdef USE_ASSIMP
#include "G3D/FileSystem.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include "G3D/Queue.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/Shader.h"

namespace G3D {
    /*q = [sin(angle / 2) * axis, cos(angle / 2)]
    
     In Watt & Watt's notation, s = w, v = (x, y, z)
     In the Real-Time Rendering notation, u = (x, y, z), w = w */
static void getQuaternion(Quat& q, aiQuaternion& aiQuat) {
    q.x = aiQuat.x;
    q.y = aiQuat.y;
    q.z = aiQuat.z;
    q.w = aiQuat.w;
}

static void getPoint3(Point3& p, const aiVector3D& aiPoint) {
    p.x = aiPoint.x; 
    p.y = aiPoint.y; 
    p.z = aiPoint.z;
}

static void getVector3(Vector3& p, const aiVector3D& aiVector) {
    p.x = aiVector.x; 
    p.y = aiVector.y; 
    p.z = aiVector.z;
}

static void getColor4(Color4& c, const aiColor4D& aiColor) {
    c.r = aiColor.r; 
    c.g = aiColor.g;
    c.b = aiColor.b; 
    c.a = aiColor.a;
}

static void getVector2(Vector2& p, const aiVector3D& aiVector) {
    p.x = aiVector.x; 
    p.y = aiVector.y; 
}

static void getPoint2unorm16(Point2unorm16& p, const aiVector3D& aiVector) {
    p.x = unorm16(aiVector.x); 
    p.y = unorm16(aiVector.y); 
}

static void getPackedTangent(Vector4& packedTangent, const aiVector3D& aiTangent, const aiVector3D& aiNormal, const aiVector3D& aiBitangent) {
    packedTangent.x = aiTangent.x; 
    packedTangent.y = aiTangent.y; 
    packedTangent.z = aiTangent.z;
    packedTangent.w = 1; // TODO: Actually check normal and bitangent
}

static String getTextureFilename(aiTextureType type, const aiMaterial* mat, const String basePath, int id = 0) {
    int textureCount = mat->GetTextureCount(type);
    if (textureCount > 0) {
        aiString aiFilename;
        mat->GetTexture(type, id, &aiFilename);
        const String& filename = ArticulatedModel::resolveRelativeFilename(aiFilename.C_Str(), basePath);
        if ( FileSystem::exists(filename) ) {
            return filename;
        }
    } 
    return "";
}


/**
    to be used when there are multiple textures
    will combine the textures into a new image 
    save that image and return the filename
**/
static shared_ptr<Texture> getCombinedTexture(Color3 color, 
                                      const String firstTex, 
                                      aiTextureType type, 
                                      const aiMaterial* mat, 
                                      int   texCount,
                                      const String basePath) {

    RenderDevice* rd = RenderDevice::current;
    static shared_ptr<Framebuffer> fb         = Framebuffer::create("cc");
    shared_ptr<Texture> two            = Texture::fromFile(firstTex);
    static shared_ptr<Texture> one            = Texture::createEmpty("one", two->width(), two->height());
    static shared_ptr<Shader> combineShader = Shader::fromFiles(System::findDataFile("combineColorTexture.pix"));
    for(int i = 0; i < texCount; i++) {
        shared_ptr<Texture> out = Texture::createEmpty("three", two->width(), two->height());    
        fb->set(Framebuffer::COLOR0, out);    
        rd->push2D(fb); {    
            Args args;
            if(i == 0) {
                args.setMacro("COLOR", 1);
                args.setUniform("color", color);
            } else {        
                two = Texture::fromFile(getTextureFilename(type, mat, basePath, i));
                args.setMacro("COLOR", 0);
                args.setUniform("tex1", one, Sampler::video());
            }
            aiTextureOp operation = aiTextureOp_Multiply;
            mat->Get(_AI_MATKEY_TEXOP_BASE, aiTextureType_DIFFUSE, i, operation);
            args.setMacro("OPERATION", operation);
            args.setUniform("tex", two, Sampler::video());
            args.setRect(rd->viewport());
            rd->apply(combineShader, args);
        } rd->pop2D();
        one = out;
    }
    return one;
}

static void toMatrix4(const aiMatrix4x4& aiMatrix, Matrix4& m) {
    m[0][0] = aiMatrix.a1;
    m[0][1] = aiMatrix.a2;
    m[0][2] = aiMatrix.a3;
    m[0][3] = aiMatrix.a4;

    m[1][0] = aiMatrix.b1;
    m[1][1] = aiMatrix.b2;
    m[1][2] = aiMatrix.b3;
    m[1][3] = aiMatrix.b4;

    m[2][0] = aiMatrix.c1;
    m[2][1] = aiMatrix.c2;
    m[2][2] = aiMatrix.c3;
    m[2][3] = aiMatrix.c4;

    m[3][0] = aiMatrix.d1;
    m[3][1] = aiMatrix.d2;
    m[3][2] = aiMatrix.d3;
    m[3][3] = aiMatrix.d4;
}

static void toG3DCFrame(const aiMatrix4x4& aiMatrix, CFrame& cframe) {
    Matrix4 m;
    toMatrix4(aiMatrix, m);
    cframe.rotation = m.upper3x3();
    cframe.translation = m.column(3).xyz();
}

static void toMaterialSpecification(
    aiMaterial** const                  aiMaterialArray, 
    int                                 index, 
    const String&                  basePath, 
    String&                        name, 
    UniversalMaterial::Specification&   spec,
    Color3&                             transmissiveColor) {

    const aiMaterial* mat = aiMaterialArray[index];
    aiString aiName;
    if (AI_SUCCESS == mat->Get(AI_MATKEY_NAME, aiName)) {
        name = aiName.C_Str();
    } else {
        name = format("AssimpMaterial%d", index);
    }

    /** Lambertian */
    const String& diffuseFilename = getTextureFilename(aiTextureType_DIFFUSE, mat, basePath);
    aiColor3D aiDiffuse;
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse);
    Color3 diffuseColor(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b);
    if (diffuseFilename != "") {
        aiTextureOp operation;
        mat->Get(_AI_MATKEY_TEXOP_BASE, aiTextureType_DIFFUSE, 0, operation);
        // Necessary because ASSIMP doesn't necessarily return a valid enum value,
        // enums might be signed or unsigned, and we need to check for validity
        int operationInt = (int)operation; 
        int textureCount = mat->GetTextureCount(aiTextureType_DIFFUSE);
        if ((textureCount == 1) || ((operationInt > 5) || (operationInt < 0))) {
            Texture::Specification t(diffuseFilename, true);
            t.encoding.readMultiplyFirst = diffuseColor;
            spec.setLambertian(t);
        } else {
            spec.setLambertian(getCombinedTexture(diffuseColor, 
                                                  diffuseFilename, 
                                                  aiTextureType_DIFFUSE, 
                                                  mat,
                                                  textureCount,
                                                  basePath));
        }
    } else {
        spec.setLambertian(Texture::Specification(diffuseColor));
    }

    // Emissive
    const String& emissiveFilename = getTextureFilename(aiTextureType_EMISSIVE, mat, basePath);
    aiColor3D aiEmissive;
    mat->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissive);
    Color3 emissiveColor(aiEmissive.r, aiEmissive.g, aiEmissive.b);
    if (! emissiveFilename.empty()) {
        aiTextureOp operation;
        mat->Get(_AI_MATKEY_TEXOP_BASE, aiTextureType_EMISSIVE, 0, operation);
        int textureCount = mat->GetTextureCount(aiTextureType_EMISSIVE);
        if ((textureCount == 1) && (operation > 5)) {
            Texture::Specification s(emissiveFilename, true);
            s.encoding.readMultiplyFirst = emissiveColor;
            spec.setEmissive(s);
        } else {
            spec.setEmissive(getCombinedTexture(emissiveColor, 
                                                emissiveFilename, 
                                                aiTextureType_EMISSIVE, 
                                                mat,
                                                textureCount,
                                                basePath));
        }
    } else {
        spec.setEmissive(Texture::Specification(emissiveColor));
    }


    //only works for models that have set the same form of specular as g3d
    aiShadingMode shademode;
    mat->Get(AI_MATKEY_SHADING_MODEL, shademode);      
    float exponentGlossy = 0;
    mat->Get(AI_MATKEY_SHININESS, exponentGlossy);    
    if ((shademode == aiShadingMode_Blinn) || (shademode == aiShadingMode_Phong) || (exponentGlossy != 0)) {
        /** glossy */        
        float scaleGlossy = 1;
        mat->Get(AI_MATKEY_SHININESS_STRENGTH, scaleGlossy);
        aiColor3D aiSpecular;
        mat->Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular);
        
        const String& glossyFilename = getTextureFilename(aiTextureType_SPECULAR, mat, basePath);
        const String& shininessFilename = getTextureFilename(aiTextureType_SHININESS, mat, basePath);

        Texture::Specification glossySpecification(glossyFilename, true);
        glossySpecification.alphaFilename = shininessFilename;
        glossySpecification.encoding.readMultiplyFirst = Color4(Color3(aiSpecular.r, aiSpecular.g, aiSpecular.b) * scaleGlossy, 
            UniversalBSDF::packGlossyExponent(exponentGlossy));
#if 0
        if (false) {
            // Broken code for importing multiple textures:
            aiTextureOp operation;
            mat->Get(_AI_MATKEY_TEXOP_BASE, aiTextureType_SPECULAR, 0, operation);
            const int textureCount = mat->GetTextureCount(aiTextureType_SPECULAR);

            if ((textureCount == 1) && (operation > 5)) {
            } else {
                spec.setGlossy(getCombinedTexture(glossyColor, 
                                                  glossyFilename, 
                                                  aiTextureType_SPECULAR, 
                                                  mat,
                                                  textureCount,
                                                  basePath));
        }
#endif


    }

    // Transmissive
    const String& transmissiveFilename = getTextureFilename(aiTextureType_OPACITY, mat, basePath);
    aiColor3D aiTransparent;
    float     opacity;
    mat->Get(AI_MATKEY_OPACITY, opacity);
    mat->Get(AI_MATKEY_COLOR_TRANSPARENT, aiTransparent);
    transmissiveColor = Color3(aiTransparent.r, aiTransparent.g, aiTransparent.b) * opacity;
    if (! transmissiveFilename.empty()) {
        aiTextureOp operation;
        mat->Get(_AI_MATKEY_TEXOP_BASE, aiTextureType_OPACITY, 0, operation);
        int textureCount = mat->GetTextureCount(aiTextureType_OPACITY);
        if ((textureCount == 1) && (operation > 5)) {
            //spec.setTransmissive(transmissiveFilename, transmissiveColor);
        } else {
            spec.setTransmissive(getCombinedTexture(transmissiveColor, 
                                                    transmissiveFilename, 
                                                    aiTextureType_OPACITY, 
                                                    mat,
                                                    textureCount,
                                                    basePath));
        }
    } else {
        //spec.setTransmissive(transmissiveColor);
    }
   

    // TODO: support bump mapping
    /** Normal mapping */
    BumpMap::Settings bumpSettings;
    const String& bumpFilename = getTextureFilename(aiTextureType_NORMALS, mat, basePath);
    if (bumpFilename != "") {
        spec.setBump(ArticulatedModel::resolveRelativeFilename(bumpFilename, basePath), bumpSettings);
    }

}


namespace _internal {
class AssimpNodesToArticulatedModelParts {

    Table<String, int> boneTable;
    Array<CFrame>           inverseBindPoseTransforms;
    int boneCount;
    static void getName( String& name, const aiString aiName, const String& defaultName ) {
        if ( aiName.length != 0 ) {
            name = aiName.C_Str();
        } else {
            name = defaultName;
        }
    }

    void addMeshesToGeometry(ArticulatedModel* articulatedModel, aiNode* aiPart, ArticulatedModel::Part* part,
            ArticulatedModel::Geometry* geom, aiMesh** assimpMeshes, const Array<shared_ptr<UniversalMaterial> >& materials, const CFrame& cframe) {
        
        
        Array<CPUVertexArray::Vertex>& vertex = geom->cpuVertexArray.vertex;
        for (unsigned int i = 0; i < aiPart->mNumMeshes; ++i) {
            const aiMesh* mesh = assimpMeshes[aiPart->mMeshes[i]];
            if (mesh->HasFaces() && mesh->HasPositions()) {
                int indexOffset = vertex.size();
                vertex.resize(vertex.size() + mesh->mNumVertices);
                geom->cpuVertexArray.hasTexCoord0 = mesh->HasTextureCoords(0);
                geom->cpuVertexArray.hasTexCoord1 = mesh->HasTextureCoords(1);
                geom->cpuVertexArray.hasTangent = mesh->HasTangentsAndBitangents();
                geom->cpuVertexArray.hasVertexColors = mesh->HasVertexColors(0);

                if (geom->cpuVertexArray.hasTexCoord1) {
                    geom->cpuVertexArray.texCoord1.resize(vertex.size());
                }
                if (geom->cpuVertexArray.hasVertexColors) {
                    geom->cpuVertexArray.vertexColors.resize(vertex.size());
                }

                // Load all non-bone data
                for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                    CPUVertexArray::Vertex& vtx = vertex[v + indexOffset];
                    getPoint3(vtx.position, mesh->mVertices[v]);
                    if ( !cframe.isIdentity() ) {
                        vtx.position = cframe.pointToWorldSpace(vtx.position);
                    }
                    if (mesh->HasNormals()) {
                        getVector3(vtx.normal, mesh->mNormals[v]);
                        if ( !cframe.isIdentity() ) {
                            vtx.normal = cframe.normalToWorldSpace(vtx.normal);
                        }
                        if (mesh->HasTangentsAndBitangents()) {
                            getPackedTangent(vtx.tangent, mesh->mTangents[v], mesh->mNormals[v], mesh->mBitangents[v]);
                            if ( !cframe.isIdentity() ) {
                                vtx.tangent = Vector4(cframe.normalToWorldSpace(vtx.tangent.xyz()), vtx.tangent.w);
                            }
                        }
                    }
                    if (mesh->HasTextureCoords(0)) {
                        getVector2(vtx.texCoord0, mesh->mTextureCoords[0][v]);
                    }
                    if (mesh->HasTextureCoords(1)) {
                        getPoint2unorm16(geom->cpuVertexArray.texCoord1[v + indexOffset], mesh->mTextureCoords[1][v]);
                    }
                    if (mesh->HasVertexColors(0)) {
                        getColor4(geom->cpuVertexArray.vertexColors[v + indexOffset], mesh->mColors[0][v]);
                    }
                }

                // Bone data
                int numBones = mesh->mNumBones;
                if ( mesh->HasBones() && numBones > 0 ) {
                    geom->cpuVertexArray.hasBones = true;
                    Array<Vector4>&         boneWeights = geom->cpuVertexArray.boneWeights;
                    Array<Vector4int32>&    boneIndices = geom->cpuVertexArray.boneIndices;
                    
                    Array<int>  boneCounts;
                    int geomBoneOffset = geom->cpuVertexArray.boneWeights.size();
                    boneCounts.resize(mesh->mNumVertices);
                    boneCounts.setAll(0);
                    boneWeights.resize(vertex.size());
                    boneIndices.resize(vertex.size());
                    
                    for (int i = 0; i < numBones; ++i) {
                        aiBone* assimpBone = mesh->mBones[i];
                        const String& boneName = assimpBone->mName.C_Str();
                        bool isNewBone;
                        int& boneIndex = boneTable.getCreate(boneName, isNewBone);
                        if (isNewBone) {
                            boneIndex = boneCount;
                            CFrame cframe;
                            toG3DCFrame(assimpBone->mOffsetMatrix, cframe);
                            alwaysAssertM(fuzzyEq(cframe.rotation.determinant(), 1.0f), "Inverse bind Pose transform of bone includes scale. This is not supported in G3D");
                            inverseBindPoseTransforms.append(cframe);
                            ++boneCount;
                        }
                        for (unsigned int j = 0; j < assimpBone->mNumWeights; ++j) {
                            aiVertexWeight vertexWeight = assimpBone->mWeights[j];
                            unsigned int vertexIndex    = vertexWeight.mVertexId;
                            int boneIndexAtVertex       = boneCounts[vertexIndex];
                            debugAssertM(boneIndexAtVertex < 4, format("More than four bones affect vertex %d when loading model from ASSIMP", vertexIndex + indexOffset));
                            boneWeights[geomBoneOffset + vertexIndex][boneIndexAtVertex] = vertexWeight.mWeight;
                            boneIndices[geomBoneOffset + vertexIndex][boneIndexAtVertex] = boneIndex;
                            ++boneCounts[vertexIndex];
                        }
                    }
                    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                        while( boneCounts[i] < 4 ) {
                            boneWeights[geomBoneOffset + i][boneCounts[i]] = 0.0f;
                            boneIndices[geomBoneOffset + i][boneCounts[i]] = 0;
                            ++boneCounts[i];
                        }
                    }
                }

                String meshName;
                getName(meshName, mesh->mName, format("mesh%d", aiPart->mMeshes[i]) );
                ArticulatedModel::Mesh* g3dMesh = articulatedModel->addMesh(meshName+format("%d",i), part, geom);
                g3dMesh->cpuIndexArray.resize(mesh->mNumFaces * 3);
                // Our preprocessing guarantees that all faces are triangles.
                for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                    for (int j = 0; j < 3; ++j) {
                        g3dMesh->cpuIndexArray[i*3 + j] = mesh->mFaces[i].mIndices[j] + indexOffset;
                    }
                }
                g3dMesh->twoSided = false;
                g3dMesh->primitive = PrimitiveType::TRIANGLES;
                if ( materials.size() > 0 ) {
                    g3dMesh->material = materials[mesh->mMaterialIndex];
                } else {
                    g3dMesh->material = UniversalMaterial::createDiffuse(Color3::one() * 0.99);
                }
            
            }
        }
    }


public:
    void convert(ArticulatedModel* articulatedModel, aiNode* aiRootPart, 
        aiMesh** assimpMeshes, aiAnimation** assimpAnimations, int animationCount, const Array<shared_ptr<UniversalMaterial> >& materials) {
        Table<aiNode*, ArticulatedModel::Part*> partTable;
        Queue<aiNode*> nodesToProcess;
        nodesToProcess.enqueue(aiRootPart);
        ArticulatedModel::Part* currentPart;
        ArticulatedModel::Geometry* currentGeom;

        boneCount = 0;

        // Mapping from nodes we collapsed to the transform we applied to them
        int partNumber = 0; // Keep track of how many parts we have

        /** Traverse all geometry */
        while ( !nodesToProcess.empty() ) {
            aiNode* currentNode = nodesToProcess.dequeue();
            if ( currentNode->mNumChildren == 0 && currentNode->mNumMeshes == 0 && String(currentNode->mName.C_Str()) == "") { // Skip childless geometryless nameless parts
                continue;
            }
            String partName;
            if ( currentNode->mParent == NULL ) { // root
                getName(partName, currentNode->mName, "root");
                currentPart = articulatedModel->addPart(partName);
            } else { // generic part
                getName(partName, currentNode->mName, format("part%d", partNumber));
                currentPart = articulatedModel->addPart(partName, partTable.get(currentNode->mParent));
            }
            partTable.set(currentNode, currentPart);
            // Part transform
            toG3DCFrame(currentNode->mTransformation, currentPart->cframe);

            CFrame transformFrame = CFrame();

            // Fill queue with children
            for (unsigned int i = 0; i < currentNode->mNumChildren; ++i) {
                nodesToProcess.enqueue(currentNode->mChildren[i]);
            }
            
            if ( currentNode->mNumMeshes > 0 ) { // Early-out
                currentGeom = articulatedModel->addGeometry(partName + "_geom");
                addMeshesToGeometry(articulatedModel, currentNode, currentPart, currentGeom, assimpMeshes, materials, transformFrame);
            }
            
        }

        
        if ( boneTable.size() > 0 ) {
            articulatedModel->m_boneArray.resize(boneTable.size());
            for ( Table<String, int>::Iterator it = boneTable.begin();
                        it != boneTable.end(); ++it) {
                ArticulatedModel::Part* bone = articulatedModel->part(it->key);
                alwaysAssertM(notNull(bone), format("Bone %s doesn't exist", it->key.c_str()));
                articulatedModel->m_boneArray[it->value] = bone;
            }

            articulatedModel->m_gpuBoneTransformations = Texture::createEmpty(articulatedModel->name() + "_boneTexture", 
                                                                                articulatedModel->m_boneArray.size() * 2, 
                                                                                2, 
                                                                                ImageFormat::RGBA32F(),
                                                                                Texture::DIM_2D);
            articulatedModel->m_gpuBonePrevTransformations = Texture::createEmpty(articulatedModel->name() + "_prevBoneTexture", 
                                                                                articulatedModel->m_boneArray.size() * 2, 
                                                                                2, 
                                                                                ImageFormat::RGBA32F(),
                                                                                Texture::DIM_2D);
            for (int i = 0; i < articulatedModel->m_meshArray.size(); ++i) {
                ArticulatedModel::Mesh* mesh = articulatedModel->m_meshArray[i];
                mesh->boneTexture = articulatedModel->m_gpuBoneTransformations;
                mesh->prevBoneTexture = articulatedModel->m_gpuBonePrevTransformations;
                mesh->contributingJoints.fastClear();
                if ( mesh->geometry->cpuVertexArray.hasBones ) {
                    // Find all contributing bones
                    Set<int> contributingBoneIndices;
                    const CPUVertexArray& cpuVertexArray = mesh->geometry->cpuVertexArray;
                    for (int j = 0; j < mesh->cpuIndexArray.size(); ++j) {
                        const int vertexIndex = mesh->cpuIndexArray[j];
                        Vector4int32    boneIndices = cpuVertexArray.boneIndices[vertexIndex];
                        Vector4         boneWeights = cpuVertexArray.boneWeights[vertexIndex];
                        for (int k = 0; k < 4; ++k) {
                            if (boneWeights[k] > 0) {
                                contributingBoneIndices.insert(boneIndices[k]);
                            }
                        }
                       
                    }
                    for (   Set<int>::Iterator it = contributingBoneIndices.begin(); 
                            it != contributingBoneIndices.end();
                            ++it) {
                        mesh->contributingJoints.append(articulatedModel->m_boneArray[(*it)]);
                    }
                    
                } else {
                    mesh->contributingJoints.append(mesh->logicalPart);
                }

            }
            for (int i = 0; i < articulatedModel->m_boneArray.size(); ++i) {
                articulatedModel->m_boneArray[i]->inverseBindPoseTransform = inverseBindPoseTransforms[i];
            }
        }
        
        for (int i = 0; i < animationCount; ++i) {
            const aiAnimation* aiAnim = assimpAnimations[i];
            ArticulatedModel::Animation& animation = articulatedModel->m_animationTable.getCreate(aiAnim->mName.C_Str());
            // TODO: per frame animation time
            animation.duration = float(aiAnim->mDuration / aiAnim->mTicksPerSecond);
            
            for (unsigned int j = 0; j < aiAnim->mNumChannels; ++j) {
                aiNodeAnim* aiChannel = aiAnim->mChannels[j];
                PhysicsFrameSpline& physicsFrameSpline = animation.poseSpline.partSpline.getCreate(aiChannel->mNodeName.C_Str());

                SplineExtrapolationMode extrapolationMode;
                switch (aiChannel->mPreState) {
                case aiAnimBehaviour_DEFAULT: // Intentionally fall-through
                case aiAnimBehaviour_CONSTANT:
                    extrapolationMode = SplineExtrapolationMode::CLAMP;
                    break;

                case aiAnimBehaviour_REPEAT:
                    extrapolationMode = SplineExtrapolationMode::CYCLIC;
                    break;

                case aiAnimBehaviour_LINEAR:
                    extrapolationMode = SplineExtrapolationMode::LINEAR;
                    break;

                default:
                    alwaysAssertM(false, "Invalid aiAnimBehavior");
                    break;
                }

                PhysicsFrameSpline positionSpline;
                PhysicsFrameSpline rotationSpline;
                PhysicsFrame currentPhysicsFrame;

                // Get position spline
                for (unsigned int i = 0; i < aiChannel->mNumPositionKeys; ++i) {
                    getPoint3(currentPhysicsFrame.translation, aiChannel->mPositionKeys[i].mValue);
                    positionSpline.append(float(aiChannel->mPositionKeys[i].mTime), currentPhysicsFrame);
                }
                positionSpline.finalInterval = float(positionSpline.time.first() + (animation.duration - positionSpline.time.last()));
                positionSpline.extrapolationMode = extrapolationMode;

                // Get rotation spline
                currentPhysicsFrame.translation = Point3();
                for (unsigned int i = 0; i < aiChannel->mNumRotationKeys; ++i) {
                    getQuaternion(currentPhysicsFrame.rotation, aiChannel->mRotationKeys[i].mValue);
                    debugAssert(! currentPhysicsFrame.rotation.isNaN());
                    if (! currentPhysicsFrame.rotation.isUnit()) {
                        debugPrintf("Warning: converted non-unit quaternion to unit quaternion during ArticulatedModel load\n");
                        if (currentPhysicsFrame.rotation.magnitude() < 0.1f) {
                            currentPhysicsFrame.rotation = Quat(1,0,0,0);
                        } else {
                            currentPhysicsFrame.rotation /= currentPhysicsFrame.rotation.magnitude();
                        }
                    }
                    rotationSpline.append(float(aiChannel->mRotationKeys[i].mTime), currentPhysicsFrame);
                }
                rotationSpline.finalInterval = float(rotationSpline.time.first() + (animation.duration - rotationSpline.time.last()));
                rotationSpline.extrapolationMode = extrapolationMode;

                // Merge frame times into single array
                Array<float> times;
                unsigned int positionIndex = 0;
                unsigned int rotationIndex = 0;

                // Prime the loop
                float firstPositionTime = (float)aiChannel->mPositionKeys[positionIndex].mTime;
                float firstRotationTime = (float)aiChannel->mRotationKeys[rotationIndex].mTime;

                if (firstPositionTime < firstRotationTime) {
                    times.append(firstPositionTime);
                    ++positionIndex;
                } else {
                    times.append(firstRotationTime);
                    ++rotationIndex;
                }

                while (true) {
                    if (positionIndex == aiChannel->mNumPositionKeys) {
                        // Add rest of rotation array times
                        while (rotationIndex < aiChannel->mNumRotationKeys) {
                            float rotationTime = (float)aiChannel->mRotationKeys[rotationIndex].mTime;
                            if ( !fuzzyEq(rotationTime, times.last()) ) {
                                times.append(rotationTime);
                            }
                            ++rotationIndex;
                        }
                        break;
                    } else if (rotationIndex == aiChannel->mNumRotationKeys) { 
                        // Add rest of position array times
                        while (positionIndex < aiChannel->mNumPositionKeys) {
                            float positionTime = (float)aiChannel->mPositionKeys[positionIndex].mTime;
                            if ( !fuzzyEq(positionTime, times.last()) ) {
                                times.append(positionTime);
                            }
                            ++positionIndex;
                        }
                        break;
                    } else {
                        float positionTime = (float)aiChannel->mPositionKeys[positionIndex].mTime;
                        float rotationTime = (float)aiChannel->mRotationKeys[rotationIndex].mTime;
                        if (fuzzyEq(positionTime, times.last())) {
                            ++positionIndex;
                        } else if (fuzzyEq(rotationTime, times.last())) {
                            ++rotationIndex;
                        } else {
                            if ( positionTime < rotationTime ) {
                                times.append(positionTime);
                                ++positionIndex;
                            } else {
                                times.append(rotationTime);
                                ++rotationIndex;
                            }
                        }
                    }
                }

                // Merge
                for (int i = 0; i < times.size(); ++i) {
                    const PhysicsFrame& rotationFrame = rotationSpline.evaluate(times[i]);
                    const PhysicsFrame& positionFrame = positionSpline.evaluate(times[i]);
                    debugAssert(! rotationFrame.rotation.isNaN());
                    physicsFrameSpline.append(times[i], PhysicsFrame(rotationFrame.rotation, positionFrame.translation));
                }
                physicsFrameSpline.finalInterval = float(physicsFrameSpline.time.first() + (animation.duration - physicsFrameSpline.time.last()));
                physicsFrameSpline.extrapolationMode = extrapolationMode;
            }
        }

    }
};
}

void ArticulatedModel::loadASSIMP(const Specification& specification) {
    
    Assimp::Importer importer;
    
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    const aiScene* scene = importer.ReadFile(specification.filename.c_str(), 
        aiProcessPreset_TargetRealtime_MaxQuality
        | aiProcess_FlipUVs);
    const String& basePath = FilePath::parent(FileSystem::resolve(specification.filename));

    if (isNull(scene)) {
        String err = importer.GetErrorString();
        if (specification.filename.find(".zip", 0) != String::npos) {
            err += " The Open Asset Import Library (which we use for this model type) has trouble loading files in zip files. Try moving it out of its zip file.";
        }  
        
        alwaysAssertM(false, err);
    }
    

    alwaysAssertM(scene->HasMeshes(), format("ASSIMP found no meshes in %s", specification.filename.c_str() ));

    Array<String>                      materialNames;
    Array<UniversalMaterial::Specification> materialSpecs;
    Array<Color3>                           transmissives;
    Array<shared_ptr<UniversalMaterial> >   materials;
    // TODO: Handle embedded textures
    int transmissiveReverse = 0;
    if (scene->HasMaterials()) {
        materials.resize(scene->mNumMaterials);
        materialNames.resize(scene->mNumMaterials);
        materialSpecs.resize(scene->mNumMaterials);
        transmissives.resize(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            String materialName;
            UniversalMaterial::Specification materialSpec;
            Color3  transmissive;
            toMaterialSpecification(scene->mMaterials, i, basePath, materialName, materialSpec, transmissive);
            materialSpecs[i] = materialSpec;
            materialNames[i] = materialName;
            transmissives[i] = transmissive;

            if (transmissive == Color3::one()) {
                ++transmissiveReverse; 
            } else if (transmissive == Color3()) {
                --transmissiveReverse;
            }
        }
        Specification::ColladaOptions::TransmissiveOption transOption = specification.colladaOptions.transmissiveChoice;
        bool inverted = transOption == Specification::ColladaOptions::TransmissiveOption::INVERTED ||
            (transOption == Specification::ColladaOptions::TransmissiveOption::MINIMIZE_TRANSMISSIVES && transmissiveReverse > 0) ||
            (transOption == Specification::ColladaOptions::TransmissiveOption::MAXIMIZE_TRANSMISSIVES && transmissiveReverse <= 0);
        for(int i = 0; i < materials.size(); ++i) {         
            if (inverted) {
                materialSpecs[i].setTransmissive(Texture::Specification(Color3(1.0, 1.0, 1.0) - transmissives[i]));
            } 
            materials[i] = UniversalMaterial::create(materialNames[i], materialSpecs[i]);
        }
    }
    _internal::AssimpNodesToArticulatedModelParts converter;
    converter.convert(this, scene->mRootNode, scene->mMeshes, scene->mAnimations, scene->mNumAnimations, materials);
   
}

} // namespace G3D
#endif // USE_ASSIMP
