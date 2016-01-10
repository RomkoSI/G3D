/**
 \file GLG3D/source/ArticulatedModel_BSP.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-08-23
 \edited  2014-08-23
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/


#include "GLG3D/ArticulatedModel.h"
#ifndef DISABLE_BSP
#include "G3D/FileSystem.h"
#include "GLG3D/BSPMAP.h"
#include "GLG3D/Shader.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

/** Store names for lambertian so meshes can be merges, store lightMapIndices since all lightMaps must be different anyways */
struct MaterialIdentifier {
    String lambertianName;
    int lightMapIndex;
    MaterialIdentifier(const String& lambertian, int lightMap) : lambertianName(lambertian), lightMapIndex(lightMap) {}
    MaterialIdentifier() : lambertianName(), lightMapIndex(-1) {}
    String getFullName() {
        return format("%s-LM%d", lambertianName.c_str(), lightMapIndex);
    }
    static size_t hashCode(const MaterialIdentifier& m) {
        return HashTrait<int>::hashCode(m.lightMapIndex) ^  HashTrait<String>::hashCode(m.lambertianName);
    }
    static bool equals(const MaterialIdentifier& m, const MaterialIdentifier& n) {
        return (m.lightMapIndex == n.lightMapIndex) && (m.lambertianName == n.lambertianName);
    }
};


static Point2 getCoordinateOffset
   (int         lightMapIndex,
    int         oldLightMapWidth, 
	int         oldLightMapHeight, 
    int         newWidth, 
    int         newHeight) {

	const int numAcross = newWidth / oldLightMapWidth;
	const int x = lightMapIndex % numAcross;
	const int y = lightMapIndex / numAcross;

	return Point2(float(x * oldLightMapWidth) / newWidth, float(y * oldLightMapHeight) / newHeight);
}


static void mergeLightMapCoordinates
   (Array<int>&             lightMapIndexArray,
    Array<Vector2>&         lightCoordArray, 
    const Array<int>&       indexArray,
    int                     oldLightMapWidth, 
    int                     oldLightMapHeight, 
    int                     newWidth, 
    int                     newHeight) {

	const Point2 renormalizationFactor(float(oldLightMapWidth) / newWidth, float(oldLightMapHeight) / newHeight);
	Set<int> finishedIndices;

	for (int i = 0; i < indexArray.size(); ++i) {
		const int index = indexArray[i];
		if (! finishedIndices.contains(index) ) {
			finishedIndices.insert(index);
			const int triIndex = i / 3;
			const Point2& offset = getCoordinateOffset(lightMapIndexArray[triIndex], oldLightMapWidth, oldLightMapHeight, newWidth, newHeight);
			lightCoordArray[index] = lightCoordArray[index] * renormalizationFactor + offset;
		}
	}
}


static shared_ptr<Texture> mergeLightMapTextures
   (Array< shared_ptr<Texture> >&   lightMapArray, 
    int                             oldLightMapWidth, 
    int                             oldLightMapHeight,
    int                             newWidth,
    int                             newHeight) {

    alwaysAssertM(lightMapArray.size() > 0, "No lightMaps sent to be merged");

    const static shared_ptr<Shader> blitShader = Shader::fromFiles(System::findDataFile("ArticulatedModel/ArticulatedModel_blitShader.pix"));
	// Note that we set the constant to pi because Quake3 originally rendered 
    // with the 1/pi factored out of the BSDF, so all lights were pi times darker
    // than they should have been.
    const shared_ptr<Texture>& tex = 
        Texture::createEmpty
            ("Quake LightMap", 
             newWidth,
             newHeight,
             Texture::Encoding(ImageFormat::SRGB8(), FrameName::NONE, Color3(pif())),
             Texture::DIM_2D);

    const shared_ptr<Framebuffer>& fb = Framebuffer::create(tex);
    const int numAcross = newWidth / oldLightMapWidth;
    RenderDevice* rd = RenderDevice::current;

    rd->push2D(fb); {
        rd->setSRGBConversion(true);
        for(int i = 0; i < lightMapArray.size(); ++i) {
            int xOffset = (i % numAcross) * oldLightMapWidth;
            int yOffset = (i / numAcross) * oldLightMapHeight;
            Args args;
            args.setUniform("blittedTexture", lightMapArray[i], Sampler::lightMap() );
            args.setUniform("xOffset",	xOffset);
            args.setUniform("yOffset",	yOffset);
            args.setUniform("texWidth",  oldLightMapWidth);
            args.setUniform("texHeight", oldLightMapHeight);
            args.setRect(rd->viewport());
            rd->apply(blitShader, args);
        }
    } rd->pop2D();

    tex->generateMipMaps();
    return tex;
}


static void mergeLightMaps
   (Array< shared_ptr<Texture> >&   lightMapArray, 
    Array<int>&                     lightMapIndexArray,
	Array< Vector2 >&               lightCoordArray, 
    const Array<int>&               indexArray) {

	const int oldLightMapWidth  = 128;
	const int oldLightMapHeight = 128;

	// Calculate new dimensions
	int oldPixelCount = oldLightMapWidth * oldLightMapHeight * lightMapArray.size();
	int logNewWidth = 1;

	while (square(1 << logNewWidth) < oldPixelCount) {
		++logNewWidth;
	}

	int newWidth  = 1 << logNewWidth;
	int newHeight = newWidth;

	shared_ptr<Texture> newLightMap = mergeLightMapTextures(lightMapArray, oldLightMapWidth, oldLightMapHeight, newWidth, newHeight);
	mergeLightMapCoordinates(lightMapIndexArray, lightCoordArray, indexArray, oldLightMapWidth, oldLightMapHeight, newWidth, newHeight);
	lightMapArray.clear();
	lightMapArray.append(newLightMap);

	for (int i = 0; i < lightMapIndexArray.size(); ++i) {
		lightMapIndexArray[i] = 0;
	}

}


void ArticulatedModel::loadBSP(const Specification& specification) {
    String defaultTexture = "<white>";

    Sphere keepOnly(Vector3::zero(), finf());

    // Parse filename to find enclosing directory
    const String& pk3File = FilePath::parent(FilePath::parent(FileSystem::resolve(specification.filename)));
    const String& bspFile = FilePath::baseExt(specification.filename);

    // Load the Q3-format map    
    const shared_ptr<BSPMap>& src = BSPMap::fromFile(pk3File, bspFile, 1.0, "", defaultTexture);
    debugAssertM(src, "Could not find " + pk3File);

    Array< Vector3 >      vertexArray;
    Array< Vector3 >      normalArray;
    Array< int >          indexArray;
    Array< Vector2 >      texCoordArray;
    Array< int >          textureMapIndexArray;
    Array< Vector2 >      lightCoordArray;
    Array< int >          lightMapIndexArray;
    Array< shared_ptr<Texture> > textureMapArray;
    Array< shared_ptr<Texture> > lightMapArray;
    
    src->getTriangles(vertexArray, normalArray, indexArray, texCoordArray,
                      textureMapIndexArray, lightCoordArray, lightMapIndexArray,
                      textureMapArray, lightMapArray);

    if (lightMapArray.size() > 0) {
	    mergeLightMaps(lightMapArray, lightMapIndexArray, lightCoordArray, indexArray);
    }

    // Convert it to an ArticulatedModel
    m_name = bspFile;
    Part* part = addPart("root");
    Geometry* geom = addGeometry("root_geom");
    
    Component3          ignoreEmssive;
    shared_ptr<BumpMap> ignoreBumpMap;

    // Maps texture names to meshes
    Table<MaterialIdentifier, Mesh*, MaterialIdentifier, MaterialIdentifier> triListTable;
    

    Texture::Specification blackSpecification;
    blackSpecification.encoding.readMultiplyFirst = Color4::clear();
    blackSpecification.filename = "<white>";
    shared_ptr<Texture> black = Texture::create(blackSpecification);

    // There will be one part with many tri lists, one for each
    // texture/lightMap pair.  Create those tri lists here.  Note that many textures
    // are simply "white".
    MaterialIdentifier currentMaterialIdentifier;
    for (int i = 0; i < textureMapArray.size(); ++i) {

        const shared_ptr<Texture>& lambertianTexture = textureMapArray[i];
        currentMaterialIdentifier.lambertianName = lambertianTexture->name();
            
        const shared_ptr<UniversalBSDF>& bsdf =
            UniversalBSDF::create(Component4( lambertianTexture), 
                                Component4(black), 
                                black, 1.0, Color3::black());

        for (int j = 0; j < lightMapArray.size(); ++j) {
            currentMaterialIdentifier.lightMapIndex = j;
            // Only add textures not already present
            if (! triListTable.containsKey(currentMaterialIdentifier)) {
                Mesh* mesh = addMesh(currentMaterialIdentifier.getFullName(), part, geom);
                triListTable.set(currentMaterialIdentifier, mesh);

                mesh->twoSided = ! lambertianTexture->opaque();

                // Create the material for this part.
                if (specification.stripLightMaps) {
                    mesh->material = UniversalMaterial::create(bsdf);
                } else {
                    
					
                    mesh->material = UniversalMaterial::create(bsdf, ignoreEmssive, ignoreBumpMap, 
                        Array<Component3>(lightMapArray[j]));
                }
            }
        } // j
    } // i

    Array<CPUVertexArray::Vertex>& vertex = geom->cpuVertexArray.vertex;
    Array<Point2unorm16>& texCoord1 = geom->cpuVertexArray.texCoord1;
    vertex.resize(vertexArray.size());
    if (! specification.stripLightMapCoords) {
        texCoord1.resize(vertexArray.size());
    }
    for (int v = 0; v < vertex.size(); ++v) {
        CPUVertexArray::Vertex& vtx = vertex[v];
        vtx.position  = vertexArray[v];
        vtx.normal    = normalArray[v].direction();
        vtx.texCoord0 = texCoordArray[v];
        vtx.tangent   = Vector4::nan();
        if (! specification.stripLightMapCoords) {
            texCoord1[v]  = Point2unorm16(lightCoordArray[v]);
        }
    }
    geom->cpuVertexArray.hasTangent   = false;
    geom->cpuVertexArray.hasTexCoord0 = true;

    if (! specification.stripLightMapCoords) {
        geom->cpuVertexArray.hasTexCoord1 = true;
    }

    // Iterate over triangles, putting into the appropriate trilist based on their texture map index.
    const int numTris = textureMapIndexArray.size();
    for (int t = 0; t < numTris; ++t) {
        
        const int tlIndex = textureMapIndexArray[t];
        const String& name = textureMapArray[tlIndex]->name();
        const int lmIndex = lightMapIndexArray[t];
        const MaterialIdentifier& matID = MaterialIdentifier(name, lmIndex);
        Mesh* mesh = triListTable[matID];

        const int i = t * 3;

        bool keep = true;

        if (keepOnly.radius < inf()) {
            // Keep only faces that have at least one vertex within the sphere
            keep = false;
            for (int j = 0; j < 3; ++j) {
                if (keepOnly.contains(vertexArray[indexArray[i + j]])) {
                    keep = true;
                    break;
                }
            }
        }

        if (keep) {
            // Copy the indices of the triangle's vertices
            int i = t * 3;
            for (int j = 0; j < 3; ++j) {
                mesh->cpuIndexArray.append(indexArray[i + j]);
            }
        }
    }
    triListTable.clear();

    // Remove any meshes that were empty
    for (int t = 0; t < m_meshArray.size(); ++t) {
        if ((m_meshArray[t]->cpuIndexArray.size() == 0) ||
            (m_meshArray[t]->material->bsdf()->lambertian().max().a < 0.4f)) {
            m_meshArray.fastRemove(t);
            --t;
        /*} else {
            debugPrintf("Q3 parts kept: %s, %f\n", 
                part.triList[t]->material->bsdf()->lambertian().texture()->name().c_str(),
                part.triList[t]->material->bsdf()->lambertian().max().a);
                */
        }
    }
}

} // namespace G3D
#endif // DISABLE_BSP
