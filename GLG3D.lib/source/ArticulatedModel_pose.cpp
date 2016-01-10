/**
 \file GLG3D/source/ArticulatedModel_pose.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu, Michael Mara, http://www.illuminationcodified.com
 \created 2011-07-16
 \edited  2013-04-14
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "GLG3D/ArticulatedModel.h"
#include "G3D/Queue.h"
#include "GLG3D/GApp.h"
#include "G3D/CPUPixelTransferBuffer.h"
namespace G3D {

void ArticulatedModel::Pose::interpolate(const Pose& pose1, const Pose& pose2, float alpha, Pose& interpolatedPose) {
    // TODO: handle poses with different sets of keys'
    // TODO: don't clear interpolated pose every frame for every object!
    interpolatedPose.frameTable.clear();

    // TODO: don't allocate every frame for every object!
    Array<String> keys;
    pose1.frameTable.getKeys(keys);
    for (int i = 0; i < keys.size(); ++i) {
        PhysicsFrame& interpolatedFrame = interpolatedPose.frameTable.getCreate(keys[i]);
        interpolatedFrame = pose1.frame(keys[i]).lerp( pose2.frame(keys[i]), alpha);
    }
}


const ArticulatedModel::Pose& ArticulatedModel::defaultPose() {
    static const Pose p;
    return p;
}


void ArticulatedModel::computePartTransforms
   (Table<Part*, CFrame>&    partTransforms,
    Table<Part*, CFrame>&    prevPartTransforms,
    const CoordinateFrame&   cframe, 
    const Pose&              pose, 
    const CoordinateFrame&   prevCFrame,
    const Pose&              prevPose) {

    Queue<Part*> nodesToProcess;
    
    for (int i = 0; i < m_rootArray.size(); ++i) {
        nodesToProcess.enqueue(m_rootArray[i]);
    }

    // Traverse all geometry
    while (! nodesToProcess.empty() ) {
        Part* currentPart = nodesToProcess.dequeue();
        debugAssert(! currentPart->cframe.translation.isNaN());

        CFrame parentCFrame, parentPrevCFrame;
        if (currentPart->isRoot()) {
            parentCFrame     = cframe;
            parentPrevCFrame = prevCFrame;
            debugAssert(! parentCFrame.translation.isNaN());
            debugAssert(! isNaN(parentCFrame.rotation[0][0]));
            debugAssert(! parentPrevCFrame.translation.isNaN());
        } else {
            // We process parts in a queue (depth-first traversal order), so each 
            // part must be processed after its parents
            // and it is safe to fetch the parent's transform here.
            partTransforms.get(currentPart->parent(),        parentCFrame);
            prevPartTransforms.get(currentPart->parent(),    parentPrevCFrame);
            debugAssert(! parentCFrame.translation.isNaN());
            debugAssert(! isNaN(parentCFrame.rotation[0][0]));
            debugAssert(! parentPrevCFrame.translation.isNaN());
        }

        CFrame partFrame, prevPartFrame;
        if (pose.frameTable.containsKey(currentPart->name)) {
            debugAssert(! pose.frame(currentPart->name).translation.isNaN());
            debugAssert(! pose.frame(currentPart->name).rotation.isNaN());
            debugAssert(! prevPose.frame(currentPart->name).translation.isNaN());
            partFrame        = parentCFrame     * pose.frame(currentPart->name);
            prevPartFrame    = parentPrevCFrame * prevPose.frame(currentPart->name);
            debugAssert(! isNaN(partFrame.rotation[0][0]));
       } else {
            partFrame        = parentCFrame     * currentPart->cframe;
            prevPartFrame    = parentPrevCFrame * currentPart->cframe;
            debugAssert(! isNaN(partFrame.rotation[0][0]));
        }
        
        debugAssert(! partFrame.translation.isNaN());
        debugAssert(! prevPartFrame.translation.isNaN());
        partTransforms.set(currentPart,     partFrame);
        prevPartTransforms.set(currentPart, prevPartFrame);

        for (int i = 0; i < currentPart->childArray().size(); ++i) {
            nodesToProcess.enqueue(currentPart->childArray()[i]);
        }
    }
}


void ArticulatedModel::pose
(Array<shared_ptr<Surface> >&   surfaceArray,
 const CoordinateFrame&         cframe,
 const Pose&                    ppose,
 const shared_ptr<Entity>&      entity) {
    pose(surfaceArray, cframe, ppose, cframe, ppose, entity);
}


static CFrame getFinalBoneTransform(ArticulatedModel::Part* part, const Table<ArticulatedModel::Part*, CFrame>& partTransformTable) {
    CFrame frame;
    partTransformTable.get(part, frame);
    debugAssert(! frame.translation.isNaN());
    debugAssert(! part->inverseBindPoseTransform.translation.isNaN());
    return (frame * part->inverseBindPoseTransform);
}


void ArticulatedModel::getSkeletonLines(const Pose& pose, const CFrame& cframe, Array<Point3>& skeleton) {

    computePartTransforms(m_partTransformTable, m_prevPartTransformTable, cframe, pose, cframe, pose);
    
    for (int i = 0; i < m_boneArray.size(); ++i) {
        CFrame parentFrame;
        m_partTransformTable.get(m_boneArray[i], parentFrame);
        const Point3& endpoint0                 = parentFrame.translation;
        for(int j = 0; j < m_boneArray[i]->childArray().size(); ++j) {
            Part* child                             = m_boneArray[i]->childArray()[j];
            CFrame childFrame;
            m_partTransformTable.get(child, childFrame);
            skeleton.append(endpoint0, childFrame.translation);
        }
        if ( !m_boneArray.contains(m_boneArray[i]->parent()) ) { // root of the skeleton
            if ( isNull(m_boneArray[i]->parent()) ) {
                skeleton.append(cframe.translation, endpoint0);
            } else {
                CFrame nonBoneFrame;
                m_partTransformTable.get(m_boneArray[i]->parent(), nonBoneFrame);
                skeleton.append(nonBoneFrame.translation, endpoint0);
            }
        }
    }    
}


static void uploadBones
   (const shared_ptr<Texture>&                      boneTexture, 
    const Array<ArticulatedModel::Part*>&           boneArray, 
    const Table<ArticulatedModel::Part*, CFrame>&   boneTable) {

    if (notNull(boneTexture)) {
        // Copy Bones to GPU
        const shared_ptr<CPUPixelTransferBuffer>& pixelBuffer = 
                CPUPixelTransferBuffer::create( boneTexture->width(), 
                                                boneTexture->height(), 
                                                boneTexture->format());
        Vector4* row0 = (Vector4*)pixelBuffer->row(0);
        Vector4* row1 = (Vector4*)pixelBuffer->row(1);

        for (int i = 0; i < boneArray.size(); ++i) {
            const CFrame& boneFrame = getFinalBoneTransform(boneArray[i], boneTable);
            /* Unoptimized but readable version: 
                const Matrix4& boneMatrix = boneFrame.toMatrix4();
                *row0   = boneMatrix.column(0);
                *row1   = boneMatrix.column(1);
                ++row0; ++row1;
                *row0   = boneMatrix.column(2);
                *row1   = boneMatrix.column(3);
                ++row0; ++row1;
            */
            const Matrix3& R = boneFrame.rotation;
            const Vector3& T = boneFrame.translation;
            row0->x = R[0][0];
            row0->y = R[1][0];
            row0->z = R[2][0];
            row0->w = 0.0f;
            row1->x = R[0][1];
            row1->y = R[1][1];
            row1->z = R[2][1];
            row1->w = 0.0f;
            ++row0; ++row1;

            row0->x = R[0][2];
            row0->y = R[1][2];
            row0->z = R[2][2];
            row0->w = 0.0f;
            row1->x = T.x;
            row1->y = T.y;
            row1->z = T.z;
            row1->w = 1.0f;
            ++row0; ++row1;
        }

        boneTexture->update(pixelBuffer);        
    }
}


void ArticulatedModel::pose
(Array<shared_ptr<Surface> >& surfaceArray,
 const CoordinateFrame&       cframe,
 const Pose&                  pose,
 const CoordinateFrame&       prevCFrame,
 const Pose&                  prevPose,
 const shared_ptr<Entity>&    entity) {

    computePartTransforms(m_partTransformTable, m_prevPartTransformTable, cframe, pose, prevCFrame, prevPose);
    uploadBones(m_gpuBoneTransformations, m_boneArray, m_partTransformTable);
    uploadBones(m_gpuBonePrevTransformations, m_boneArray, m_prevPartTransformTable);
    
    for (int g = 0; g < m_geometryArray.size(); ++g) {
        Geometry* geometry = m_geometryArray[g];
        if ((geometry->cpuVertexArray.size() > 0) && ! geometry->gpuPositionArray.valid()) {
            geometry->copyToGPU(this);
        }
    }

    bool anyMeshIndexArrayOutOfDate = false;
    for (int m = 0; (m < m_meshArray.size()) && ! anyMeshIndexArrayOutOfDate; ++m) {
        const Mesh* mesh = m_meshArray[m];
        anyMeshIndexArrayOutOfDate = (mesh->cpuIndexArray.size() > 0) && ! mesh->gpuIndexArray.valid();
    }

    // Only allocated if needed
    shared_ptr<VertexBuffer> indexBuffer;
    if (anyMeshIndexArrayOutOfDate) {
        size_t totalIndexSize = 0;
        for (int m = 0; m < m_meshArray.size(); ++m) {
            const Mesh* mesh = m_meshArray[m];
            // We don't need padding on this because currently all indices are 32-bits, and must
            // be 4-byte aligned.
            totalIndexSize += mesh->cpuIndexArray.size();
        }

        if (totalIndexSize > 0) {
            indexBuffer = VertexBuffer::create(totalIndexSize*sizeof(int), VertexBuffer::WRITE_ONCE);
        } 
    }

    for (int m = 0; m < m_meshArray.size(); ++m) {
        Mesh* mesh         = m_meshArray[m];
        Geometry* geometry = mesh->geometry;
        alwaysAssertM(geometry != NULL, format("Null geometry in mesh %s", mesh->name.c_str()));
        
        // Extract the part's material table (don't bother looking if the table is empty)
        shared_ptr<UniversalMaterial>* const materialTable = 
            (pose.materialTable.size() > 0) ? 
                pose.materialTable.getPointer(mesh->name) : 
                NULL;
        
        if (anyMeshIndexArrayOutOfDate && (geometry->cpuVertexArray.size() > 0)) {
            mesh->copyToGPU(indexBuffer);
        }

        CFrame frame, prevFrame;
        if (! mesh->gpuGeom->hasBones()) { // TODO Mike: Check contributingJoints size instead?
            frame     = m_partTransformTable.get(mesh->logicalPart);
            prevFrame = m_prevPartTransformTable.get(mesh->logicalPart);
        } 
        debugAssert(! isNaN(frame.translation.x));
        debugAssert(! isNaN(frame.rotation[0][0]));

        shared_ptr<UniversalMaterial> material = mesh->material;
        if (notNull(materialTable)) {
            // See if there is a material for this mesh
            shared_ptr<UniversalMaterial>* ptr = materialTable;
            if (notNull(ptr)) {
                // Override the material from the mesh
                material = *ptr;
            }
        }

        shared_ptr<UniversalSurface::GPUGeom> gpuGeom;
        if (mesh->gpuGeom->hasBones()) { 
            // Transform bounds
            AABox fullBounds;
            AABox aaBoneTransformedBounds;
            Box boneTransformedBounds;

            gpuGeom = UniversalSurface::GPUGeom::create(mesh->gpuGeom);

            for (int i = 0; i < mesh->contributingJoints.size(); ++i) {
                const CFrame& frame = getFinalBoneTransform(mesh->contributingJoints[i], m_partTransformTable);
                debugAssert(! frame.translation.isNaN());
                boneTransformedBounds = frame.toWorldSpace(mesh->boxBounds);
                boneTransformedBounds.getBounds(aaBoneTransformedBounds);
                fullBounds.merge(aaBoneTransformedBounds);
            }
            gpuGeom->boxBounds = fullBounds;
            gpuGeom->boxBounds.getBounds(gpuGeom->sphereBounds);
        } else {
            gpuGeom = mesh->gpuGeom;
        }

        const UniversalSurface::CPUGeom cpuGeom(&mesh->cpuIndexArray, &mesh->geometry->cpuVertexArray);

        const shared_ptr<UniversalSurface>& surface = 
            UniversalSurface::create
            (mesh->name, frame, 
             prevFrame, material, gpuGeom, cpuGeom, dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), 
             pose.expressiveLightScatteringProperties,
             dynamic_pointer_cast<Model>(shared_from_this()),
             entity, 
             pose.uniformTable,
             pose.numInstances);

        surfaceArray.append(surface);
        
     }
}

/*
void ArticulatedModel::Part::pose
(const shared_ptr<ArticulatedModel>& model,
 Array<shared_ptr<Surface> >&        surfaceArray,
 const CoordinateFrame&              parentFrame,
 const Pose&                         posex,
 const CoordinateFrame&              prevParentFrame,
 const Pose&                         prevPose) {
    
    const CFrame& frame     = parentFrame     * cframe * posex.cframe(name);
    const CFrame& prevFrame = prevParentFrame * cframe * prevPose.cframe(name);
    
    // Extract the part's material table (don't bother looking if the table is empty)
    const Table<String, shared_ptr<UniversalMaterial> >* materialTable = (posex.materialTable.size() > 0) ? posex.materialTable.getPointer(name) : NULL;

    debugAssert(! isNaN(frame.translation.x));
    debugAssert(! isNaN(frame.rotation[0][0]));

    if ((cpuVertexArray.size() > 0) && ! gpuPositionArray.valid()) {
        copyToGPU();
    }

    // Pose the meshes
    for (int m = 0; m < m_meshArray.size(); ++m) {
        const Mesh* mesh = m_meshArray[m];

        if (mesh->m_fullName.size() == 0) {
            // Cache the full name.  Note that if the mesh is renamed after posing
            // (which is not a normal situation), then the surface will have the wrong
            // name (which is not generally a concern, since it is only for debugging)
            const_cast<Mesh*>(mesh)->m_fullName = name + "/" + mesh->name;
        }

        shared_ptr<UniversalMaterial> material = mesh->material;
        if (notNull(materialTable)) {
            // See if there is a material for this mesh
            shared_ptr<UniversalMaterial>* ptr = materialTable->getPointer(mesh->name);
            if (notNull(ptr)) {
                // Override the material from the mesh
                material = *ptr;
            }
        }

        const UniversalSurface::CPUGeom cpuGeom(&mesh->cpuIndexArray, &cpuVertexArray);
        const shared_ptr<UniversalSurface>& surface = 
            UniversalSurface::create(mesh->m_fullName, frame, 
                                 prevFrame, material, mesh->gpuGeom, cpuGeom, model, posex.castsShadows);

        surfaceArray.append(surface);
    }

    // Pose the children
    for (int c = 0; c < m_child.size(); ++c) {
        m_child[c]->pose(model, surfaceArray, frame, posex, prevFrame, prevPose);
    }
}
*/

void ArticulatedModel::Geometry::copyToGPU(ArticulatedModel* model) {
    cpuVertexArray.copyToGPU(gpuPositionArray, gpuNormalArray, gpuTangentArray, gpuTexCoord0Array, gpuTexCoord1Array, gpuVertexColorArray, gpuBoneIndicesArray, gpuBoneWeightsArray);

    // Go to every Mesh referencing this and mutate its GPUGeom to reference my new vertex arrays
    for (int m = 0; m < model->m_meshArray.size(); ++m) {
        if (model->m_meshArray[m]->geometry == this) {
            model->m_meshArray[m]->updateGPUGeom();
        }
    } // for each mesh
}


void ArticulatedModel::Mesh::updateGPUGeom() {
    if (isNull(gpuGeom) || ! gpuGeom.unique()) {
        // Need to allocate a new GPU geom because the other one is in use or does not exist
        gpuGeom = UniversalSurface::GPUGeom::create(primitive);
    } else {
        gpuGeom->primitive      = primitive;
    }

    // TODO: bounding 
    gpuGeom->boxBounds      = boxBounds;
    gpuGeom->sphereBounds   = sphereBounds;
    gpuGeom->index          = gpuIndexArray;
    gpuGeom->vertex         = geometry->gpuPositionArray;
    gpuGeom->normal         = geometry->gpuNormalArray;
    gpuGeom->packedTangent  = geometry->gpuTangentArray;
    gpuGeom->texCoord0      = geometry->gpuTexCoord0Array;
    gpuGeom->texCoord1      = geometry->gpuTexCoord1Array;
    gpuGeom->vertexColor    = geometry->gpuVertexColorArray;
    gpuGeom->boneIndices    = geometry->gpuBoneIndicesArray;
    gpuGeom->boneWeights    = geometry->gpuBoneWeightsArray;
    gpuGeom->twoSided       = twoSided;

    // TODO: get directly from the model?
    gpuGeom->boneTexture        = boneTexture;    
    gpuGeom->prevBoneTexture    = prevBoneTexture; 
}


void ArticulatedModel::Mesh::copyToGPU(const shared_ptr<VertexBuffer>& indexBuffer) {
    
    typedef uint16 smallIndexType;
    // If fewer than 2^16 vertices, switch to uint16 indices
    // TODO: re-enable and debug; the 2nd index array uploaded becomes corrupt for some reason
    //const size_t indexBytes = (geometry->cpuVertexArray.size() < (1<<16)) && false ? sizeof(smallIndexType) : sizeof(int);

    shared_ptr<VertexBuffer> all = indexBuffer;
    
    if (isNull(all)) {
        const size_t indexBytes = 4;
        all = VertexBuffer::create(triangleCount() * 3 * indexBytes, VertexBuffer::WRITE_ONCE);
    }

    if (false) { //indexBytes == 2) {
        // Explicitly map and convert to 16-bit indices
        const int N = cpuIndexArray.size();            
        gpuIndexArray = IndexStream((smallIndexType*)(NULL), N, all);

        const int32* src = cpuIndexArray.getCArray();
        smallIndexType* dst = (smallIndexType*)gpuIndexArray.mapBuffer(GL_WRITE_ONLY);
        for (int i = 0; i < N; ++i) {
            dst[i] = (smallIndexType)src[i];
        }
        gpuIndexArray.unmapBuffer();
    } else {
        // Directly copy the 32-bit indices
        gpuIndexArray = IndexStream(cpuIndexArray, all);
    }

    updateGPUGeom();
}

} // namespace G3D

