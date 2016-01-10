#include "GLG3D/ArticulatedModel.h"

namespace G3D {
    
void ArticulatedModel::scaleWholeModel(float scaleFactor) {
    ScalePartTransformCallback transform(scaleFactor);
    forEachPart(transform);
    ScaleGeometryTransformCallback transformGeom(scaleFactor);
    forEachGeometry(transformGeom);
    scaleAnimations(scaleFactor);
}


void ArticulatedModel::preprocess(const Array<Instruction>& program) {

    for (int i = 0; i < program.size(); ++i) {
        const Instruction& instruction = program[i];

        Part* partPtr = NULL;

        switch (instruction.type) {
        case Instruction::SCALE:
            {
                // Scale every pivot translation and every vertex position by the scale factor
                float scaleFactor = instruction.arg;
                scaleWholeModel(scaleFactor);
            }
            break;

        case Instruction::MOVE_CENTER_TO_ORIGIN:
            moveToOrigin(true);
            break;

        case Instruction::MOVE_BASE_TO_ORIGIN:
            moveToOrigin(false);
            break;

        case Instruction::SET_MATERIAL:
            {
                bool keepLightMaps = true;
                if (instruction.source.size() == 3) {
                    keepLightMaps = instruction.source[2];
                }
                UniversalMaterial::Specification specification(instruction.arg);
                setMaterial(instruction.mesh, specification, keepLightMaps, instruction.source);
            }                              
            break;

        case Instruction::SET_TWO_SIDED:
            {
                SetTwoSidedCallback callback(instruction.arg);
                forEachMesh(instruction.mesh, callback, instruction.source);
            }
            break;

        case Instruction::SET_CFRAME:
            {
                const CFrame cframe = instruction.arg;
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->cframe = cframe;
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->cframe = cframe;
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.source.verify(partPtr != NULL, "Part not found.");
                    partPtr->cframe = cframe;
                }        
            }
            break;

        case Instruction::TRANSFORM_CFRAME:
            {
                const CFrame cframe = instruction.arg;
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->cframe = cframe * m_rootArray[p]->cframe;
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->cframe = cframe * m_partArray[p]->cframe;
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.source.verify(partPtr != NULL, "Part not found.");
                    partPtr->cframe = cframe * partPtr->cframe;
                }        
            }
            break;

        case Instruction::TRANSFORM_GEOMETRY:
            {
                const Matrix4 transform(instruction.arg);
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->transformGeometry(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), transform);
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->transformGeometry(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), transform);
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.source.verify(partPtr != NULL, "Part not found.");
                    partPtr->transformGeometry(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), transform);
                }        
            }
            break;


        case Instruction::RENAME_PART:
            instruction.source.verify(! instruction.part.isAll() && ! instruction.part.isRoot(), 
                "The argument to renamePart() cannot be all() or root()");
            partPtr = part(instruction.part);
            instruction.source.verify(partPtr != NULL, "Could not find part");
            partPtr->name = instruction.arg.string(); 
            break;


        case Instruction::RENAME_MESH:
            {
                // Copy the array, since it may be modified by the callback
                Array<Mesh*> meshArray;
                getIdentifiedMeshes(instruction.mesh, meshArray);
                instruction.source.verify(meshArray.size() == 1, "Must rename only one mesh");
                instruction.source.verify(meshArray[0] != NULL, "Could not find mesh");
                meshArray[0]->name = instruction.arg.string(); 
            }
            break;

        case Instruction::ADD:
            alwaysAssertM(false, "not implemented");
            /*
            partPtr = NULL;
            if (! instruction.part.isNone()) {
                partPtr = part(instruction.part);
                instruction.source.verify(partPtr != NULL, "Unrecognized parent part");
            }
            {
                // Load the child part
                shared_ptr<ArticulatedModel> m2 = ArticulatedModel::create(Specification(instruction.arg));

                // Update part table and mesh table, and overwrite IDs
                for (int p = 0; p < m2->m_partArray.size(); ++p) {

                    Part* part = m2->m_partArray[p];
                    const_cast<ID&>(part->id) = createID();
                    m_partTable.set(part->id, part);

                    for (int m = 0; m < part->m_meshArray.size(); ++m) {
                        Mesh* mesh = part->m_meshArray[m];
                        const_cast<ID&>(mesh->id) = createID();
                        m_meshTable.set(mesh->id, mesh);
                    }
                }

                // Steal all elements of the child and add them to this
                if (partPtr == NULL) {
                    // Add as roots
                    m_rootArray.append(m2->m_rootArray);
                } else {
                    // Reparent
                    partPtr->m_child.append(m2->m_rootArray);
                    for (int p = 0; p < m2->m_partArray.size(); ++p) {
                        if (m2->m_partArray[p]->isRoot()) {
                            m2->m_partArray[p]->m_parent = partPtr;
                        }
                    }
                }
                m_partArray.append(m2->m_partArray);

                // Allow m2 to be garbage collected
            }
            */
            break;

        case Instruction::REMOVE_MESH:
            {
                RemoveMeshCallback callback;
                forEachMesh(instruction.mesh, callback, instruction.source);
            }
            break;

        case Instruction::REVERSE_WINDING:
            {
                ReverseWindingCallback callback;
                forEachMesh(instruction.mesh, callback, instruction.source);
            }
            break;

        case Instruction::COPY_TEXCOORD0_TO_TEXCOORD1:
            {
                if (instruction.part.isRoot()) {
                    //TODO: implement
                    alwaysAssertM(false, "COPY_TEXCOORD0_TO_TEXCOORD1 currently implemented only for entire model");
                    /*for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->cpuVertexArray.copyTexCoord0ToTexCoord1();
                    }*/
                } else if (instruction.part.isAll()) {
                    for (int g = 0; g < m_geometryArray.size(); ++g) {
                        m_geometryArray[g]->cpuVertexArray.copyTexCoord0ToTexCoord1();
                    }
                } else {
                    //TODO: implement
                    alwaysAssertM(false, "COPY_TEXCOORD0_TO_TEXCOORD1 currently implemented only for entire model");
                    /*partPtr = part(instruction.part);
                    instruction.source.verify(partPtr != NULL, "Part not found.");
                    partPtr->cpuVertexArray.copyTexCoord0ToTexCoord1();*/
                } 
            }
            break;

        case Instruction::OFFSET_AND_SCALE_TEXCOORD1:
            {
         
                const Point2& offset = instruction.arg;
                const Point2& scale = instruction.source[2];
                if (instruction.part.isRoot()) {
                    //TODO: implement
                    alwaysAssertM(false, "OFFSET_AND_SCALE_TEXCOORD1 currently implemented only for entire model");
                    /*for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->cpuVertexArray.offsetAndScaleTexCoord1(offset, scale);
                    }*/
                } else if (instruction.part.isAll()) {
                    for (int g = 0; g < m_geometryArray.size(); ++g) {
                        m_geometryArray[g]->cpuVertexArray.offsetAndScaleTexCoord1(offset, scale);
                    }
                } else {
                    //TODO: implement
                    alwaysAssertM(false, "OFFSET_AND_SCALE_TEXCOORD1 currently implemented only for entire model");
                    /*partPtr = part(instruction.part);
                    instruction.source.verify(partPtr != NULL, "Part not found.");
                    partPtr->cpuVertexArray.offsetAndScaleTexCoord1(offset, scale);*/
                } 
            }
            break;

        case Instruction::MERGE_ALL:
             {
                 MeshMergeCallback merge(anyToMeshMergeRadius(instruction.arg), anyToMeshMergeRadius(instruction.source[1]));
                 computeBounds();
                 forEachPart(merge);
             }
             break;

        case Instruction::INTERSECT_BOX:
            {
                const Box box(instruction.arg);
                if (instruction.part.isRoot()) {
                    for (int p = 0; p < m_rootArray.size(); ++p) {
                        m_rootArray[p]->intersectBox(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), box);
                    }
                } else if (instruction.part.isAll()) {
                    for (int p = 0; p < m_partArray.size(); ++p) {
                        m_partArray[p]->intersectBox(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), box);
                    }
                } else {
                    partPtr = part(instruction.part);
                    instruction.source.verify(partPtr != NULL, "Part not found.");
                    partPtr->intersectBox(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), box);
                }        
            }
            break;

        default:
            alwaysAssertM(false, "Instruction not implemented");
        }
    }

}



void ArticulatedModel::setMaterial
   (Instruction::Identifier                     meshId,
    const UniversalMaterial::Specification&     spec,
    const bool                                  keepLightMaps,
    const Any&                                  source) {

    class SetMaterialCallback : public MeshCallback {
    public:
        const bool                          keepLightMaps;
        UniversalMaterial::Specification    spec;
        shared_ptr<UniversalMaterial>       material;

        SetMaterialCallback(const UniversalMaterial::Specification& s, bool k) : keepLightMaps(k), spec(s) {
            if (! keepLightMaps) {
                material = UniversalMaterial::create(spec);
            }
        }

        virtual void operator()
           (shared_ptr<ArticulatedModel> model,
            ArticulatedModel::Mesh* mesh) override {

            if (keepLightMaps) {
                debugAssert(notNull(mesh));
                spec.setLightMaps(mesh->material);
                material = UniversalMaterial::create(spec);
            }
            mesh->material = material;
         }

    } callback(spec, keepLightMaps);

    forEachMesh(meshId, callback, source);       
}


void ArticulatedModel::forEachMesh(Instruction::Identifier meshId, MeshCallback& callback, const Any& source) {
    shared_ptr<ArticulatedModel> me(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()));

    // Copy the array, since it may be modified by the callback
    Array<Mesh*> meshArray;
    getIdentifiedMeshes(meshId, meshArray);
    for (int m = 0; m < meshArray.size(); ++m) {
        callback(me, meshArray[m]);
    }
}

void ArticulatedModel::forEachGeometry(Instruction::Identifier geomId, GeometryCallback& callback, const Any& source) {
    shared_ptr<ArticulatedModel> me(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()));

    // Copy the array, since it may be modified by the callback
    Array<Geometry*> geomArray;
    getIdentifiedGeometry(geomId, geomArray);
    for (int g = 0; g < geomArray.size(); ++g) {
        callback(me, geomArray[g]);
    }
}


void ArticulatedModel::Part::transformGeometry(shared_ptr<ArticulatedModel> am, const Matrix4& xform) {
    // TODO: this is a linear search through the mesh array for every part, could be sped up
    // TODO: this transforms any geometry that is touched by a mesh in this part. This will have
    // unintended side effects when multiple parts have meshes that share geometry
    Set<ArticulatedModel::Geometry*> touchedGeometry;
    for (int i = 0; i < am->m_meshArray.size(); ++i) {
        Mesh* mesh = am->m_meshArray[i];
        alwaysAssertM(notNull(mesh->geometry), "Found a NULL mesh in transformGeometry");
        if ((mesh->logicalPart == this) && ! touchedGeometry.contains(mesh->geometry)) {
            touchedGeometry.insert(mesh->geometry);
            CPUVertexArray::Vertex* vertex = mesh->geometry->cpuVertexArray.vertex.getCArray();
            const int N = mesh->geometry->cpuVertexArray.size();
            for (int i = 0; i < N; ++i) {
                vertex->position = xform.homoMul(vertex->position, 1.0f);
                vertex->tangent  = Vector4::nan();
                vertex->normal   = Vector3::nan();
                ++vertex;
            }
        }  
    }

    for (int c = 0; c < m_children.size(); ++c) {
        m_children[c]->cframe.translation = 
            xform.homoMul(m_children[c]->cframe.translation, 1.0f);
    }
}




void ArticulatedModel::Part::intersectBox(shared_ptr<ArticulatedModel> am, const Box& box) {
	alwaysAssertM(false, "ArticulatedModel::Part::intersectBox unimplemented");
    /*
    // TODO: this is a linear search through the mesh array for every part, could be sped up
    // TODO: this transforms any geometry that is touched by a mesh in this part. This will have
    // unintended side effects when multiple parts have meshes that share geometry
    Set<ArticulatedModel::Geometry*> touchedGeometry;
    for (int i = 0; i < am->m_meshArray.size(); ++i) {
        Mesh* mesh = am->m_meshArray[i];
        alwaysAssertM(notNull(mesh->geometry), "Found a NULL mesh in transformGeometry");
        if ((mesh->logicalPart == this) && ! touchedGeometry.contains(mesh->geometry)) {
            touchedGeometry.insert(mesh->geometry);
            CPUVertexArray::Vertex* vertex = mesh->geometry->cpuVertexArray.vertex.getCArray();
            const int N = mesh->geometry->cpuVertexArray.size();
            for (int i = 0; i < N; ++i) {
                vertex->position = xform.homoMul(vertex->position, 1.0f);
                vertex->tangent  = Vector4::nan();
                vertex->normal   = Vector3::nan();
                ++vertex;
            }
        }  
    }

    for (int c = 0; c < m_children.size(); ++c) {
        m_children[c]->cframe.translation = 
            xform.homoMul(m_children[c]->cframe.translation, 1.0f);
    }
    */
}


void ArticulatedModel::moveToOrigin(bool centerY) {
    BoundsCallback boundsCallback;
    computeBounds();
    forEachPart(boundsCallback);

    Vector3 translate = -boundsCallback.bounds.center();
    if (! centerY) {
        translate.y += boundsCallback.bounds.extent().y * 0.5f;
    }
    
    alwaysAssertM(translate.isFinite(), "Cannot translate by non-finite amount or NaN");
    const Matrix4& xform = Matrix4::translation(translate);

    // Center
    for (int p = 0; p < m_rootArray.size(); ++p) {
        Part* part = m_rootArray[p];
        // Done so that moveToOrigin() and transformGeometry are commutative in the preprocessor
        part->transformGeometry(dynamic_pointer_cast<ArticulatedModel>(shared_from_this()), xform);
        
        //part->cframe.translation += translate;
    }
    
}


void ArticulatedModel::BoundsCallback::operator()
    (ArticulatedModel::Part* part, const CFrame& worldToPartFrame, 
     shared_ptr<ArticulatedModel> m, const int treeDepth) {
    for (int i = 0; i < m->meshArray().size(); ++i) {
        Mesh* mesh = m->meshArray()[i];
        if (mesh->logicalPart == part) {
            const Box& b = worldToPartFrame.toWorldSpace(mesh->boxBounds);
            AABox partBounds;
            b.getBounds(partBounds);
            bounds.merge(partBounds);
        }
    }
}


void ArticulatedModel::ScalePartTransformCallback::operator()
    (ArticulatedModel::Part* part, const CFrame& worldToPartFrame,
     shared_ptr<ArticulatedModel> m, const int treeDepth) {
    part->cframe.translation *= scaleFactor;
    part->inverseBindPoseTransform.translation *= scaleFactor;
}


void ArticulatedModel::ScaleGeometryTransformCallback::operator()
    (shared_ptr<ArticulatedModel> am, Geometry* geom) {

    const int N = geom->cpuVertexArray.size();
    CPUVertexArray::Vertex* ptr = geom->cpuVertexArray.vertex.getCArray();
    for (int v = 0; v < N; ++v) {
        ptr[v].position *= scaleFactor;
    }
}


void ArticulatedModel::MeshMergeCallback::operator() 
    (ArticulatedModel::Part* part, const CFrame& worldToPartFrame,
     shared_ptr<ArticulatedModel> am, const int treeDepth) {

    if ((opaqueRadius == 0.0f) && (transmissiveRadius == 0.0f)) { return; }

    alwaysAssertM(opaqueRadius >= 0.0f, "AUTO merge radius not implemented yet");
    alwaysAssertM(transmissiveRadius >= 0.0f, "AUTO merge radius not implemented yet");

    // Maps UniversalMaterial instances to lists of Meshes that will be preserved that
    // use those materials.  Lists are needed because there are other properties,
    // such as the two-sided flag, that must also match to merge two meshes.
    Table<shared_ptr<UniversalMaterial>, Array<Mesh*> > table;

	// For each source mesh, try to find a destination mesh to merge into
    for (int m = 0; m < am->m_meshArray.size(); ++m) {
        Mesh* src = am->m_meshArray[m];

        bool created;
        Array<Mesh*>& list = table.getCreate(src->material, created);

        bool merged = false;

        if (! created) {
            // There is at least one other mesh with the same material.  See if we can merge into it.
            for (int i = 0; (i < list.size()) && ! merged; ++i) {
                Mesh* dst = list[i];

                AlphaHint h = dst->material->alphaHint();
                debugAssertM(h != AlphaHint::DETECT, "AlphaHint::DETECT should have been resolved by this point");
    
                const Color3& maxTransmission = dst->material->bsdf()->transmissive().texture()->max().rgb();
                const bool opaque = 
                    maxTransmission.isZero() &&
                    ((h == AlphaHint::ONE) || (h == AlphaHint::BINARY) || (dst->material->bsdf()->lambertian().min().a == 1.0f));  
                const bool transmissive = ! opaque;

                // Ensure that we obey the required radii
                const float srcRadius = src->boxBounds.extent().length() / 2.0f;
                const float dstRadius = dst->boxBounds.extent().length() / 2.0f;
                AABox combinedBounds = src->boxBounds;
                combinedBounds.merge(dst->boxBounds);
                const float combinedRadius = combinedBounds.extent().length() / 2.0f;

                const bool didNotGrow = (combinedRadius == max(srcRadius, dstRadius));

                if ((dst->primitive == src->primitive) &&
                    (dst->twoSided == src->twoSided) && 
                    (dst->logicalPart == src->logicalPart) &&
                    (dst->geometry == src->geometry) &&
                    ((opaque && ((opaqueRadius > 0.0f) && (didNotGrow || (combinedRadius <= opaqueRadius)))) ||
                     ((transmissive && ((transmissiveRadius > 0.0f) && (didNotGrow || (combinedRadius <= transmissiveRadius))))))) {

                    for (int i = 0; i < src->contributingJoints.size(); ++i) {
                        if (! dst->contributingJoints.contains(src->contributingJoints[i]) ) {
                            dst->contributingJoints.append(src->contributingJoints[i]);
                        }
                    }

                    // Merge the parts
                    dst->cpuIndexArray.append(src->cpuIndexArray);

                    // Update the bounding box
                    dst->boxBounds = combinedBounds;

                    // Rename using the alphabetically lower part name.  This ensures
                    // consistent naming on each loading of the model in the face of
                    // non-deterministic iteration or parsing.
                    if (src->name < dst->name) {
                        dst->name = src->name;
                    }

                    // Remove the src mesh from model
                    am->m_meshArray.remove(m);
                    --m;

                    delete src;
                    src = NULL;

                    merged = true;
                } // If mergable
            } // For each mesh in the list
        } 

        if (! merged) {
            // Record this mesh
            list.append(src);
        }
    }
} // MeshMergeCallback::operator()


void ArticulatedModel::RemoveMeshCallback::operator() 
   (shared_ptr<ArticulatedModel> model,
    ArticulatedModel::Mesh*      meshPtr) {


    const int index = model->m_meshArray.findIndex(meshPtr);
    debugAssert(index >= 0);
    model->m_meshArray.remove(index);

    // Delete
    delete meshPtr;
    meshPtr = NULL;
} // ArticulatedModel::RemoveMeshCallback::operator()


void ArticulatedModel::ReverseWindingCallback::operator() 
   (shared_ptr<ArticulatedModel> model,
    ArticulatedModel::Mesh*      meshPtr) {

    debugAssert(meshPtr->primitive == PrimitiveType::TRIANGLES);

    // Preserve the order of triangles, but change the orientation of each
    for (int i = 0; i < meshPtr->cpuIndexArray.size(); i += 3) {
        std::swap(meshPtr->cpuIndexArray[i + 2], meshPtr->cpuIndexArray[i]);
    }
} // ArticulatedModel::ReverseWindingCallback::operator()



void ArticulatedModel::SetTwoSidedCallback::operator() 
   (shared_ptr<ArticulatedModel> model,
    ArticulatedModel::Mesh*      meshPtr) {
        
    meshPtr->twoSided = twoSided;
}


void ArticulatedModel::scaleAnimations(float scaleFactor) {
    for (Table<String, Animation>::Iterator it = m_animationTable.begin(); it.isValid(); ++it) {
        const Animation& anim = it->value;
        const PoseSpline::SplineTable& splineTable = anim.poseSpline.partSpline;
        for (PoseSpline::SplineTable::Iterator spIt = splineTable.begin(); spIt.isValid(); ++spIt) {
            spIt->value.scaleControlPoints(scaleFactor);
        }
    }
}


} // namespace
