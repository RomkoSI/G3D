/**
 \file GLG3D/source/ArticulatedModel.h

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-07-19
 \edited  2016-02-10
 
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
 */
#include "GLG3D/ArticulatedModel.h"
#include "G3D/Ray.h"
#include "G3D/FileSystem.h"
#include "G3D/Stopwatch.h"
#include "GLG3D/GApp.h"
#include <memory>

namespace G3D {

const PhysicsFrame ArticulatedModel::Pose::identity;

const String& ArticulatedModel::className() const {
    static String s("ArticulatedModel");
    return s;
}

String ArticulatedModel::resolveRelativeFilename(const String& filename, const String& basePath) {
    if (filename.empty()) {
        return filename;
    } else {
        return FileSystem::resolve(filename, basePath);
    }
}


WeakCache<ArticulatedModel::Specification, shared_ptr<ArticulatedModel> > s_cache;

void ArticulatedModel::clearCache() {
    s_cache.clear();
}


shared_ptr<ArticulatedModel> ArticulatedModel::loadArticulatedModel(const ArticulatedModel::Specification& specification, const String& n) {
    shared_ptr<ArticulatedModel> a(new ArticulatedModel());

    if (n.empty()) {
        a->m_name = FilePath::base(specification.filename);
    }
    
    a->load(specification);

    if (! n.empty()) {
        a->m_name = n;
    }
    
    return a;
}


lazy_ptr<Model> ArticulatedModel::lazyCreate(const ArticulatedModel::Specification& specification, const String& name) {
    return lazy_ptr<Model>([specification, name]{ return ArticulatedModel::create(specification, name); });
}


shared_ptr<ArticulatedModel> ArticulatedModel::create(const ArticulatedModel::Specification& specification, const String& n) {

    if (specification.cachable) {
        shared_ptr<ArticulatedModel> a = s_cache[specification];
        if (isNull(a)) {
            a = loadArticulatedModel(specification, n);
            s_cache.set(specification, a);
        }
        return a;
    } else {
        return loadArticulatedModel(specification, n);
    }
}


shared_ptr<ArticulatedModel> ArticulatedModel::createEmpty(const String& n) {
    const shared_ptr<ArticulatedModel> a(new ArticulatedModel());
    a->m_name = n;
    return a;
}


void ArticulatedModel::forEachPart(PartCallback& callback, Part* part, const CFrame& parentFrame, const Pose& pose, const int treeDepth) {
    // Net transformation from part to world space
    const CFrame& net = parentFrame * part->cframe * pose.frame(part->name);

    // Process all children
    for (int c = 0; c < part->m_children.size(); ++c) {
        forEachPart(callback, part->m_children[c], net, pose, treeDepth + 1);
    }

    // Invoke the callback on this part
    callback(part, net, dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), treeDepth);
}


void ArticulatedModel::forEachPart(PartCallback& callback, const CFrame& cframe, const Pose& pose) {
    for (int p = 0; p < m_rootArray.size(); ++p) {
        forEachPart(callback, m_rootArray[p], cframe, pose, 0);
    }
}


ArticulatedModel::Mesh* ArticulatedModel::addMesh(const String& name, Part* part, Geometry* geom) {
    alwaysAssertM(notNull(geom), "Cannot add a mesh with null geometry");
    Mesh* m = new Mesh(name, part, geom, getID());
    m_meshArray.append(m);
    return m_meshArray.last();
}


ArticulatedModel::Part* ArticulatedModel::addPart(const String& name, Part* parent) {
    m_partArray.append(new Part(name, parent, getID()));
    if (parent == NULL) {
        m_rootArray.append(m_partArray.last());
    } else {
        parent->m_children.append(m_partArray.last());
    }

    return m_partArray.last();
}


ArticulatedModel::Geometry* ArticulatedModel::addGeometry(const String& name) {
    Geometry* g = new Geometry(name);
    m_geometryArray.append(g);
    return m_geometryArray.last();
}


ArticulatedModel::Mesh* ArticulatedModel::mesh(int ID) {
    // Exhaustively cycle through all meshes
    for (int m = 0; m < m_meshArray.size(); ++m) {
        if (ID == m_meshArray[m]->uniqueID) {
            return m_meshArray[m];
        }
    }
    
    return NULL;
}

ArticulatedModel::Mesh* ArticulatedModel::mesh(const String& meshName) {

    // Exhaustively cycle through all meshes
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const String& currentName = m_meshArray[m]->name;
        if (currentName == meshName) {
            return m_meshArray[m];
        }
    }
    
    return NULL;
}

ArticulatedModel::Geometry* ArticulatedModel::geometry(const String& geomName) {

    // Exhaustively cycle through all meshes
    for (int g = 0; g < m_geometryArray.size(); ++g) {
        if (m_geometryArray[g]->name == geomName) {
            return m_geometryArray[g];
        }
    }
    
    return NULL;
}


ArticulatedModel::Part* ArticulatedModel::part(const Instruction::Identifier& partIdent) {
    return part(partIdent.name);
}


ArticulatedModel::Mesh* ArticulatedModel::mesh(const Instruction::Identifier& meshIdent) {
    return mesh(meshIdent.name);
}


ArticulatedModel::Geometry* ArticulatedModel::geometry(const Instruction::Identifier& geomIdent) {
    return geometry(geomIdent.name);
}


void ArticulatedModel::getIdentifiedMeshes(const Instruction::Identifier& meshIdent, Array<Mesh*>& identifiedMeshes) {
    if (meshIdent.isAll()) {
        identifiedMeshes.appendPOD(m_meshArray);
    } else if (! meshIdent.name.empty()) {
        Mesh* identifiedMesh = mesh(meshIdent);
        alwaysAssertM(notNull(identifiedMesh), format("Tried to access nonexistent mesh %s", meshIdent.name.c_str()));
        identifiedMeshes.append(identifiedMesh);
    } else {
        alwaysAssertM(false, "Only named meshes or <all> currently can be specified for getIdentifiedMeshes");
    }
}


void ArticulatedModel::getIdentifiedGeometry(const Instruction::Identifier& geomIdent, Array<Geometry*>& identifiedGeometry) {
    if (geomIdent.isAll()) {
        identifiedGeometry.appendPOD(m_geometryArray);
    } else if (! geomIdent.name.empty()){
        identifiedGeometry.append(geometry(geomIdent));
    } else {
        alwaysAssertM(false, "Only named geometry or <all> currently can be specified for identifiedGeometry");
    }
}


ArticulatedModel::Part* ArticulatedModel::part(const String& partName) {
    // Exhaustively cycle through all parts
    for (int p = 0; p < m_partArray.size(); ++p) {
        const String& n = m_partArray[p]->name;
        if (n == partName) {
            return m_partArray[p];
        }
    }
    return NULL;
}


void ArticulatedModel::load(const Specification& specification) {
    Stopwatch timer;
    timer.setEnabled(false);
    
    const String& ext = toLower(FilePath::ext(specification.filename));

    if (ext == "obj") {
        loadOBJ(specification);
    } else if (ext == "ifs") {
        loadIFS(specification);
    } else if (ext == "ply2") {
        loadPLY2(specification);
    } else if (ext == "ply") {
        loadPLY(specification);
    } else if (ext == "off") {
        loadOFF(specification);
    } else if (ext == "3ds") {
        load3DS(specification);
    } else if (ext == "bsp") {
        loadBSP(specification);
    } else if ((ext == "stl") || (ext == "stla")) {
        loadSTL(specification);
    } else if ((ext == "dae") || (ext == "fbx") || (ext == "lwo") || (ext == "ase")) {
        loadASSIMP(specification);
    } else if (ext == "hair") {
        loadHAIR(specification);
    } else {
        loadHeightfield(specification);
    }
    timer.after("parse file");
    
    if (((specification.meshMergeOpaqueClusterRadius != 0.0f) ||
        (specification.meshMergeTransmissiveClusterRadius != 0.0f)) &&
        (m_meshArray.length() > 1) &&
        (ext != "hair")) {
        MeshMergeCallback merge(specification.meshMergeOpaqueClusterRadius, specification.meshMergeTransmissiveClusterRadius);
        forEachPart(merge);
    }
    
    // If this model is very large, compact the vertex arrays to save RAM
    // during the post-processing step
    maybeCompactArrays();
    
    // Perform operations as demanded by the specification
    if (specification.scale != 1.0f) {
        scaleWholeModel(specification.scale);
    }
    preprocess(specification.preprocess);
    timer.after("preprocess");
    
    // Compute missing elements (normals, tangents) of the part geometry, 
    // perform vertex welding, and recompute bounds.
    if (ext != "hair") {
        cleanGeometry(specification.cleanGeometrySettings);
    } else {
        computeBounds();
    }

    maybeCompactArrays();
    timer.after("cleanGeometry");
}


void ArticulatedModel::clearGPUArrays() {
    for (int g = 0; g < m_geometryArray.size(); ++g) {
        m_geometryArray[g]->clearAttributeArrays();
    }

    for (int m = 0; m < m_meshArray.size(); ++m) {
        m_meshArray[m]->clearIndexStream();
    }
}


float ArticulatedModel::anyToMeshMergeRadius(const Any& a) {
    if (a.type() == Any::NUMBER) {
        return float(a.number());
    } else if (a.type() == Any::STRING) {
        if (a.string() == "AUTO") {
            return -finf();
        } else if (a.string() == "NONE") {
            return 0.0f;
        } else if (a.string() == "ALL") {
            return finf();
        } else {
            a.verify(false, "Unrecognized mesh merge radius named constant");
            return finf();
        }
    } else {
        a.verify(false, "Unrecognized mesh merge radius value");
        return finf();
    }
}


Any ArticulatedModel::meshMergeRadiusToAny(const float r) {
    if (r == 0.0f) {
        return Any("NONE");
    } else if (r < -1.0f) {
        return Any("AUTO");
    } else if (r == finf()) {
        return Any("ALL");
    } else {
        return Any(r);
    }
}


void ArticulatedModel::maybeCompactArrays() {
    int numVertices     = 0;
    int numIndices      = 0;
    int numTexCoord1    = 0;
    int numVertexColors = 0;
    // TODO: Take into account bones
    for (int g = 0; g < m_geometryArray.size(); ++g) {
        Geometry* geom = m_geometryArray[g];
        numVertices += geom->cpuVertexArray.size();
        if(geom->cpuVertexArray.hasTexCoord1) {
            numTexCoord1 += geom->cpuVertexArray.texCoord1.size();
        }
        if(geom->cpuVertexArray.hasVertexColors) {
            numVertexColors += geom->cpuVertexArray.vertexColors.size();
        }
    }
    for (int m = 0; m < m_meshArray.size(); ++m) {
        numIndices += m_meshArray[m]->cpuIndexArray.size();
    }
    
    size_t vertexBytes      = sizeof(CPUVertexArray::Vertex) * numVertices;
    size_t indexBytes       = sizeof(int) * numIndices;
    size_t texCoord1Bytes   = sizeof(Point2unorm16) * numTexCoord1;
    size_t vertexColorBytes = sizeof(Color4) * numVertexColors;

    if (vertexBytes + indexBytes + texCoord1Bytes + vertexColorBytes > 5000000) {
        // There's a lot of data in this model: compact it

        for (int g = 0; g < m_geometryArray.size(); ++g) {
            Geometry* geom = m_geometryArray[g];
            geom->cpuVertexArray.vertex.trimToSize();
            if(geom->cpuVertexArray.hasTexCoord1) {
                geom->cpuVertexArray.texCoord1.trimToSize();
            }
            if(geom->cpuVertexArray.hasVertexColors) {
                geom->cpuVertexArray.vertexColors.trimToSize();
            }
        }
        for (int m = 0; m < m_meshArray.size(); ++m) {
            m_meshArray[m]->cpuIndexArray.trimToSize();
        }

    }
}


/** Used by ArticulatedModel::intersect */
class AMIntersector : public ArticulatedModel::MeshCallback {
public:
    bool                        hit;

private:
    const Ray&                  m_wsR;
    float&                      m_maxDistance;
    Model::HitInfo&             m_info;
    Table<ArticulatedModel::Part*, CFrame> m_cframeTable;
    const shared_ptr<Entity>& m_entity; 

public:

    AMIntersector(const Ray& wsR, float& maxDistance, Model::HitInfo& information, Table<ArticulatedModel::Part*, CFrame>& cframeTable, const shared_ptr<Entity>& entityset) :
        hit(false), 
        m_wsR(wsR), 
        m_maxDistance(maxDistance), 
        m_info(information), 
        m_cframeTable(cframeTable),
        m_entity(entityset){
    }

    virtual void operator()(shared_ptr<ArticulatedModel> model, ArticulatedModel::Mesh* mesh) override {

        SmallArray<CFrame, 8> jointCFrameArray;
        Vector3 normal;

        AABox boxBounds;
        for (int i = 0; i < mesh->contributingJoints.size(); ++i) {
            const CFrame& jointCFrame = m_cframeTable.get(mesh->contributingJoints[i]) * mesh->contributingJoints[i]->inverseBindPoseTransform;
            jointCFrameArray.append(jointCFrame);
            AABox jointBounds;
            jointCFrame.toWorldSpace(mesh->boxBounds).getBounds(jointBounds);
            boxBounds.merge(jointBounds);
        }
        // Bounding sphere test
        const float testTime = m_wsR.intersectionTime(boxBounds);

        if (testTime >= m_maxDistance) {
            // Could not possibly hit this part's geometry since it doesn't
            // hit the bounds
            return;
        }
        
        const int numIndices = mesh->cpuIndexArray.size();
        const int* index = mesh->cpuIndexArray.getCArray();

        alwaysAssertM(mesh->primitive == PrimitiveType::TRIANGLES, 
                        "Only implemented for PrimitiveType::TRIANGLES meshes.");

        const Array<CPUVertexArray::Vertex>& vertex = mesh->geometry->cpuVertexArray.vertex;
        const Array<Vector4>& boneWeightArray       = mesh->geometry->cpuVertexArray.boneWeights;
        const Array<Vector4int32>& boneIndexArray   = mesh->geometry->cpuVertexArray.boneIndices;

        Ray intersectingRay;

        // Needed if doing bone animation
        SmallArray<int, 8> contributingIndexArray; 

        if (mesh->contributingJoints.size() == 1) {
            intersectingRay = jointCFrameArray[0].toObjectSpace(m_wsR);
        } else {
            // Transform vertices instead
            intersectingRay = m_wsR; 
            contributingIndexArray.resize(jointCFrameArray.size());
            for (int i = 0; i < contributingIndexArray.size(); ++i) {
                contributingIndexArray[i] = model->m_boneArray.findIndex(mesh->contributingJoints[i]);
            }
        }

        Array<Point3> triMesh;
        for (int i = 0; i < numIndices; i += 3) {    
            
            Point3 p[3];
            if (mesh->contributingJoints.size() == 1) {
                p[0] = vertex[index[i]].position;
                p[1] = vertex[index[i + 1]].position;
                p[2] = vertex[index[i + 2]].position;
            } else {
                for (int j = 0; j < 3; ++j) {
                    const int currentIndex          = index[i + j];
                    const Vector4&  boneWeights     = boneWeightArray[currentIndex];
                    const Vector4int32& boneIndices = boneIndexArray[currentIndex];
                    CFrame boneTransform;
                    boneTransform.rotation = Matrix3::diagonal(0.0f,0.0f,0.0f);
                    for (int k = 0; k < 4; ++k) {
                        const int boneIndex = boneIndices[k];
                        
                        for (int b = 0; b < contributingIndexArray.size(); ++b) {
                            if ( contributingIndexArray[b] == boneIndex ) {
                                boneTransform.rotation      = boneTransform.rotation + (jointCFrameArray[b].rotation * boneWeights[k]);
                                boneTransform.translation   = boneTransform.translation + (jointCFrameArray[b].translation * boneWeights[k]);
                            }
                        }
                    }
                    p[j] = boneTransform.pointToWorldSpace(vertex[currentIndex].position);
      
                }
                triMesh.append(p[0], p[1], p[2]);
            }

            // Barycentric weights
            float w0 = 0, w1 = 0, w2 = 0;
            float testTime = intersectingRay.intersectionTime(p[0], p[1], p[2], w0, w1, w2);
            bool justHit = false;

            if (testTime < m_maxDistance) {
                hit         = true;
                justHit     = true;
                m_maxDistance = testTime;
                normal      = (p[1] - p[0]).cross(p[2] - p[0]).direction();
            } else if (mesh->twoSided) {
                // Check the backface. We can't possibly hit it unless
                // the test failed for the front face of this
                // triangle.
                testTime = intersectingRay.intersectionTime(p[0], p[2], p[1], w0, w1, w2);
                    
                if (testTime < m_maxDistance) {
                    hit         = true;
                    justHit     = true;
                    m_maxDistance = testTime;
                    normal      = (p[2] - p[0]).cross(p[1] - p[0]).direction();
                }
            }

            if (justHit) {
                m_info.set(
                    model, 
                    m_entity,
                    mesh->material,
                    mesh->contributingJoints.size() == 1 ? jointCFrameArray[0].normalToWorldSpace(m_info.normal) : normal,
                    m_wsR.origin() + m_maxDistance * m_wsR.direction(),
                    mesh->name,
                    mesh->uniqueID,
                    i/3,
                    w0,
                    w2);
            }
        } // for each triangle

#   if 0
        if (triMesh.size() > 0) {
            Array<int> indices;
            indices.resize(triMesh.size());
            for (int i = 0; i < triMesh.size(); ++i) {
                indices[i] = i;
            }
            debugDraw(shared_ptr<MeshShape>(new MeshShape(triMesh, indices)), 0.5f);
        }
#   endif
        
    } // operator ()
}; // AMIntersector


bool ArticulatedModel::intersect
    (const Ray&     R, 
    const CFrame&   cframe, 
    const Pose&     pose, 
    float&          maxDistance, 
    Model::HitInfo& info,
    const shared_ptr<Entity>& entity) {
    // TODO: change structure to no longer keep cache of part transforms
    computePartTransforms(m_partTransformTable, m_partTransformTable, cframe, pose, cframe, pose);
    AMIntersector intersectOperation(R, maxDistance, info, m_partTransformTable, entity);
    forEachMesh(intersectOperation);

    return intersectOperation.hit;
}


void ArticulatedModel::countTrianglesAndVertices(int& tri, int& vert) const {
    tri = 0;
    vert = 0;
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];
        tri += mesh->triangleCount();
    }

    for (int g = 0; g < m_geometryArray.size(); ++g) {
        const Geometry* geom = m_geometryArray[g];
        vert += geom->cpuVertexArray.size();
    }
}

ArticulatedModel::~ArticulatedModel() {
    m_partArray.invokeDeleteOnAllElements();
    m_meshArray.invokeDeleteOnAllElements();
    m_geometryArray.invokeDeleteOnAllElements();
}

void ArticulatedModel::getBoundingBox(AABox& box) {
    Array<shared_ptr<Surface> > arrayModel;
    Animation animation;
    Pose pos;
    if (usesSkeletalAnimation()) {
        Array<String> animationNames;
        getAnimationNames(animationNames);
        // TODO: Add support for selecting animations.
        getAnimation(animationNames[0], animation); 
        animation.getCurrentPose(0.0f, pos);
    } 
    
    pose(arrayModel, CFrame(), pos);

    int numFaces = 0;
    int numVertices = 0;
    countTrianglesAndVertices(numFaces, numVertices);
    
    bool overwrite = true;
    
    if (arrayModel.size() > 0) {
        
        for (int x = 0; x < arrayModel.size(); ++x) {		
            
            //merges the bounding boxes of all the seperate parts into the bounding box of the entire object
            AABox temp;
            CFrame cframe;
            arrayModel[x]->getCoordinateFrame(cframe);
            arrayModel[x]->getObjectSpaceBoundingBox(temp);
            Box partBounds = cframe.toWorldSpace(temp);
            
            // Some models have screwed up bounding boxes
            if (partBounds.extent().isFinite()) {
                if (overwrite) {
                    partBounds.getBounds(box);
                    overwrite = false;
                } else {
                    partBounds.getBounds(temp);
                    box.merge(temp);
                }
            }
        }
        
        if (overwrite) {
            // We never found a part with a finite bounding box
            box = AABox(Vector3::zero());
        }
    }
}

ArticulatedModel::Part::~Part() {
}

} // namespace G3D
