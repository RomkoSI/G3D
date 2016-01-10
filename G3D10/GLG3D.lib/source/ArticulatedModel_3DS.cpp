/**
 \file GLG3D/source/ArticulatedModel_3DS.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-08-11
 \edited  2012-07-26
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/


#include "GLG3D/ArticulatedModel.h"
#ifndef DISABLE_3DS
#include "G3D/Parse3DS.h"
#include "G3D/FileSystem.h"
#include "G3D/Stopwatch.h"
#include "G3D/Log.h"

namespace G3D {

static String find3DSTexture(String _filename, const String& path) {    
    if (! _filename.empty()) {
        String filename = _filename;
        if (endsWith(toUpper(filename), ".GIF")) {
            // Load PNG instead of GIF, since we can't load GIF
            filename = filename.substr(0, filename.length() - 3) + "png";
        }

        if (! FileSystem::exists(filename, true, false) && FileSystem::exists(FilePath::concat(path, filename), true, false)) {
            filename = FilePath::concat(path, filename);
        }

        // Load textures
        filename = System::findDataFile(filename, false, false);
        
        if (filename.empty()) {
            logPrintf("Could not locate 3DS file texture '%s'\n", _filename.c_str());
            filename = "<white>";
        }
        return filename;
    } else {
        return "<white>";
    }
}


static UniversalMaterial::Specification compute3DSMaterial
(const void*         ptr,
 const String&  path,
 const ArticulatedModel::Specification&   specification) {

    const Parse3DS::UniversalMaterial& material = *reinterpret_cast<const Parse3DS::UniversalMaterial*>(ptr);

    UniversalMaterial::Specification spec;

    if (specification.stripMaterials) {
        spec.setLambertian(Texture::Specification(Color4(Color3::one() * 0.7f, 1.0f)));
        spec.setGlossy(Texture::Specification(Color4(Color3::one() * 0.2f, UniversalBSDF::packGlossyExponent(100))));
        return spec;
    }
    
    const Parse3DS::Map& texture1 = material.texture1;

    const String& lambertianFilename = find3DSTexture(texture1.filename, path);
    
    {
        Texture::Specification s(lambertianFilename, true);
        s.encoding.readMultiplyFirst = 
            Color4((material.diffuse * material.texture1.pct) *
               (1.0f - material.transparency), 1.0f);
        spec.setLambertian(s);
    }

    // Strength of the shininess (higher is brighter)
    spec.setGlossy(
        Texture::Specification(Color4(max(material.shininessStrength * material.specular, Color3(material.reflection)) * (1.0f - material.transparency),
            (material.reflection > 0.05f) ? UniversalBSDF::packedSpecularMirror() :
                UniversalBSDF::packGlossyExponent(material.shininess * 1024))));

    spec.setTransmissive(Texture::Specification(Color3::white() * material.transparency));
    spec.setEmissive(Texture::Specification(Color3::white() * material.emissive));

    if (! material.bumpMap.filename.empty()) {
        const String& bumpFilename = find3DSTexture(material.bumpMap.filename, path);
        if (! bumpFilename.empty()) {
            // TODO: use percentage specified in material.bumpMap
            spec.setBump(bumpFilename);
        }
    }

    // TODO: load reflection, specular, etc maps.
    // triList->material.reflect.map = 

    /*
    if (preprocess.addBumpMaps) {
        // See if a bump map exists:
        String filename = 
            FilePath::concat(FilePath::concat(path, filenamePath(texture1.filename)),
                       filenameBase(texture1.filename) + "-bump");

        filename = findAnyImage(filename);
        if (filename != "") {
            BumpMap::Settings s;
            s.scale = preprocess.bumpMapScale;
            s.bias = 0;
            s.iterations = preprocess.parallaxSteps;
            spec.setBump(filename, s, preprocess.normalMapWhiteHeightInPixels);
        }
    } // if bump maps
    */

    return spec;
}


void ArticulatedModel::load3DS(const Specification& specification) {
    // During loading, we make no attempt to optimize the mesh.  We leave that until the
    // Parts have been created.  The vertex arrays are therefore much larger than they
    // need to be.
    Stopwatch timer;
    timer.setEnabled(false);

    Parse3DS parseData;
    { 
        BinaryInput bi(specification.filename, G3D_LITTLE_ENDIAN);
        timer.after(" open file");
        parseData.parse(bi);
        timer.after(" parse");
    }

    const String& path = FilePath::parent(specification.filename);

    /*
    if (specification.stripMaterials) {
        stripMaterials(parseData);
    }

    if (specification.mergeMeshesByMaterial) {
        mergeGroupsAndMeshesByMaterial(parseData);
        }*/

    for (int p = 0; p < parseData.objectArray.size(); ++p) {
        Parse3DS::Object& object = parseData.objectArray[p];

        // Create a unique name for this part
        String name = object.name;
        int count = 0;
        while (this->part(name) != NULL) {
            ++count;
            name = object.name + format("_#%d", count);
        }

        // Create the new part
        // All 3DS parts are promoted to the root in the current implementation.
        Part* part      = addPart(name);
        Geometry* geom  = addGeometry(name + "_geom");

        // Process geometry
        geom->cpuVertexArray.vertex.resize(object.vertexArray.size());
        part->cframe = object.keyframe.approxCoordinateFrame();
        debugAssert(isFinite(part->cframe.rotation.determinant()));
        debugAssert(part->cframe.rotation.isOrthonormal());

        if (! part->cframe.rotation.isRightHanded()) {
            // TODO: how will this impact other code?  I think we can't just force it like this -- Morgan
            part->cframe.rotation.setColumn(0, -part->cframe.rotation.column(0));
        }

        debugAssert(part->cframe.rotation.isRightHanded());

        //debugPrintf("%s %d %d\n", object.name.c_str(), object.hierarchyIndex, object.nodeID);

        if (geom->cpuVertexArray.vertex.size() > 0) {
            // Convert vertices to object space (there is no surface normal data at this point)
            Matrix4 netXForm = part->cframe.inverse().toMatrix4();
            
            debugAssertM(netXForm.row(3) == Vector4(0,0,0,1), 
                        "3DS file loading requires that the last row of the xform matrix be 0, 0, 0, 1");

            
            geom->cpuVertexArray.hasTexCoord0 = (object.texCoordArray.size() > 0);
            
            
            const Matrix3& S = netXForm.upper3x3();
            const Vector3& T = netXForm.column(3).xyz();
            for (int v = 0; v < geom->cpuVertexArray.vertex.size(); ++v) {
#               ifdef G3D_DEBUG
                {
                    const Vector3& vec = object.vertexArray[v];
                    debugAssert(vec.isFinite());
                }
#               endif

                CPUVertexArray::Vertex& vertex = geom->cpuVertexArray.vertex[v];
                vertex.position = S * object.vertexArray[v] + T;
                vertex.tangent = Vector4::nan();
                vertex.normal  = Vector3::nan();

                if (geom->cpuVertexArray.hasTexCoord0) {
                    vertex.texCoord0 = object.texCoordArray[v];
                }

#               ifdef G3D_DEBUG
                {
                    const Vector3& vec = vertex.position;
                    debugAssert(vec.isFinite());
                }
#               endif
            }


            if (object.faceMatArray.size() == 0) {

                // Merge all geometry into one mesh since there are no materials
                Mesh* mesh = addMesh("mesh", part, geom);
                mesh->cpuIndexArray = object.indexArray;
                debugAssert(mesh->cpuIndexArray.size() % 3 == 0);

            } else {
                for (int m = 0; m < object.faceMatArray.size(); ++m) {
                    const Parse3DS::FaceMat& faceMat = object.faceMatArray[m];

                    if (faceMat.faceIndexArray.size() > 0) {

                        shared_ptr<UniversalMaterial> mat;
                        bool twoSided = false;

                        const String& materialName = faceMat.materialName;
                        if (parseData.materialNameToIndex.containsKey(materialName)) {
                            int i = parseData.materialNameToIndex[materialName];
                            const Parse3DS::UniversalMaterial& material = parseData.materialArray[i];
                            
                            //if (! materialSubstitution.get(material.texture1.filename, mat)) {
                            const UniversalMaterial::Specification& spec = compute3DSMaterial(&material, path, specification);
                            mat = UniversalMaterial::create(spec);
                            //}
                            twoSided = material.twoSided || mat->hasAlpha();
                        } else {
                            mat = UniversalMaterial::create();
                            logPrintf("Referenced unknown material '%s'\n", materialName.c_str());
                        }                        

                        Mesh* mesh = addMesh(materialName, part, geom);
                        debugAssert(isValidHeapPointer(mesh));
                        mesh->material = mat;
                        mesh->twoSided = twoSided;

                        // Construct an index array for this part
                        for (int i = 0; i < faceMat.faceIndexArray.size(); ++i) {
                            // 3*f is an index into object.indexArray
                            int f = faceMat.faceIndexArray[i];
                            debugAssert(f >= 0);
                            for (int v = 0; v < 3; ++v) {
                                mesh->cpuIndexArray.append(object.indexArray[3 * f + v]);
                            }
                        }
                        debugAssert(mesh->cpuIndexArray.size() > 0);
                        debugAssert(mesh->cpuIndexArray.size() % 3 == 0);

                    }
                } // for m
            } // if has materials 
        }
    }

    timer.after(" convert");
}

} // namespace G3D
#endif // DISABLE_3DS
