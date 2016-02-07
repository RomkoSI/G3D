/**
  \file GLG3D.lib/source/UniversalSurface.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2004-11-20
  \edited  2015-09-10

  Copyright 2001-2015, Morgan McGuire
 */
#include "G3D/Log.h"
#include "G3D/constants.h"
#include "G3D/fileutils.h"
#include "GLG3D/UniversalSurface.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/CPUVertexArray.h"
#include "GLG3D/Tri.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Light.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/AmbientOcclusion.h"
#include "GLG3D/Shader.h"
#include "GLG3D/LightingEnvironment.h"
#include "GLG3D/SVO.h"
#include "G3D/AreaMemoryManager.h"
namespace G3D {


bool UniversalSurface::canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const {
    debugAssertM(m_material->alphaHint() != AlphaHint::DETECT, "AlphaHint::DETECT must be resolved into ONE, BINARY, or BLEND when a material is created");

    const bool opaqueSamples = ((m_material->alphaHint() == AlphaHint::ONE) || 
        (m_material->alphaHint() == AlphaHint::BINARY) || 
        (m_material->alphaHint() == AlphaHint::COVERAGE_MASK) || 
        ! m_material->bsdf()->lambertian().nonUnitAlpha()) &&
        (!hasTransmission());

    const bool emissiveOk   = m_material->emissive().isBlack() || notNull(specification.encoding[GBuffer::Field::EMISSIVE].format);
    const bool lambertianOk = m_material->bsdf()->lambertian().isBlack() || notNull(specification.encoding[GBuffer::Field::LAMBERTIAN].format);
    const bool glossyOk     = m_material->bsdf()->glossy().isBlack() || notNull(specification.encoding[GBuffer::Field::GLOSSY].format);

    return opaqueSamples && emissiveOk && lambertianOk && glossyOk;
}


bool UniversalSurface::anyOpaque() const {
    // Transmissive if the largest color channel of the lowest values across the whole texture is nonzero
    const bool allTransmissive    = (m_material->bsdf()->transmissive().min().max() > 0.0f) && (! hasRefractiveTransmission());
    const bool allPartialCoverage = 
        (((m_material->alphaHint() == AlphaHint::BLEND) || (m_material->alphaHint() == AlphaHint::COVERAGE_MASK)) && (m_material->bsdf()->lambertian().max().a < 1.0f)) ||
        ((m_material->alphaHint() == AlphaHint::BINARY) && (m_material->bsdf()->lambertian().max().a < 0.5f));

    return ! (allTransmissive || allPartialCoverage);
}


void UniversalSurface::renderWireframeHomogeneous
(RenderDevice*                      rd, 
 const Array<shared_ptr<Surface> >& surfaceArray, 
 const Color4&                      color,
 bool                               previous) const {

    rd->pushState(); {

        rd->setDepthWrite(false);
        rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
        rd->setPolygonOffset(-0.5f);
        Args args;
        args.setUniform("color", color);
        args.setMacro("HAS_TEXTURE", false);
        for (int g = 0; g < surfaceArray.size(); ++g) {
            const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[g]);
            debugAssertM(surface, "Surface::renderWireframeHomogeneous passed the wrong subclass");
            const shared_ptr<UniversalSurface::GPUGeom>& geom(surface->gpuGeom());
            
            if (geom->twoSided) {
                rd->setCullFace(CullFace::NONE);
            } else {
                rd->setCullFace(CullFace::BACK);
            }

            CFrame cframe;
            surface->getCoordinateFrame(cframe, previous);
            rd->setObjectToWorldMatrix(cframe);

            args.setAttributeArray("g3d_Vertex", geom->vertex);
            args.setIndexStream(geom->index);
            args.setPrimitiveType(geom->primitive);
            LAUNCH_SHADER_WITH_HINT("unlit.*", args, m_profilerHint);
        }

    } rd->popState();
}


void UniversalSurface::bindDepthPeelArgs
(Args& args, 
 RenderDevice* rd, 
 const shared_ptr<Texture>& depthPeelTexture, 
 const float minZSeparation) {

    const bool useDepthPeel = notNull(depthPeelTexture);
    args.setMacro("USE_DEPTH_PEEL", useDepthPeel ? 1 : 0);
    if (useDepthPeel) {
        const Vector3& clipInfo = Projection(rd->projectionMatrix()).reconstructFromDepthClipInfo();
        args.setUniform("previousDepthBuffer", depthPeelTexture, Sampler::buffer());
        args.setUniform("minZSeparation",  minZSeparation);
        args.setUniform("currentToPreviousScale", Vector2(depthPeelTexture->width()  / rd->viewport().width(),
                                                          depthPeelTexture->height() / rd->viewport().height()));
        args.setUniform("clipInfo", clipInfo);
    }
}


struct DepthOnlyBatchDescriptor {
    uint32 glIndexBuffer;
    bool twoSided;
    bool hasBones;
    CFrame cframe;

    bool operator==(const DepthOnlyBatchDescriptor& other) const {
        return glIndexBuffer == other.glIndexBuffer && twoSided == other.twoSided && hasBones == other.hasBones && cframe == other.cframe;
    }

    bool operator!=(const DepthOnlyBatchDescriptor& other) const {
        return !operator==(other);
    }

    DepthOnlyBatchDescriptor() {}
    DepthOnlyBatchDescriptor(const shared_ptr<UniversalSurface>& surf) :
        glIndexBuffer(surf->gpuGeom()->index.buffer()->openGLVertexBufferObject()),
        twoSided(surf->gpuGeom()->twoSided),
        hasBones(surf->gpuGeom()->hasBones()),
        cframe(surf->frame()) {}
    
    static size_t hashCode(const DepthOnlyBatchDescriptor& d) {
        return d.glIndexBuffer + int(d.twoSided) + int(d.hasBones) + (*(int*)&d.cframe.translation.x) + (*(int*)&d.cframe.rotation[1][1]);
    }
};

static void categorizeByBatchDescriptor(const Array<shared_ptr<Surface> >& all, Array< Array<shared_ptr<Surface> > >& derivedArray) {
    derivedArray.fastClear();

    // Allocate space for the worst case, so that we don't have to copy arrays
    // all over the place during resizing.
    derivedArray.reserve(all.size());

    Table<DepthOnlyBatchDescriptor, int, DepthOnlyBatchDescriptor> descriptorToIndex;
    // Allocate the table elements in a memory area that can be cleared all at once
    // without invoking destructors.
    descriptorToIndex.clearAndSetMemoryManager(AreaMemoryManager::create(100 * 1024));

    for (int s = 0; s < all.size(); ++s) {
      const shared_ptr<Surface>& instance = all[s];
        
        bool created = false;
        int& index = descriptorToIndex.getCreate(DepthOnlyBatchDescriptor(dynamic_pointer_cast<UniversalSurface>(instance)), created);
        if (created) {
            // This is the first time that we've encountered this subclass.
            // Allocate the next element of subclassArray to hold it.
            index = derivedArray.size();
            derivedArray.next();
        }
        derivedArray[index].append(instance);
    }
}



void UniversalSurface::renderDepthOnlyHomogeneous
(RenderDevice*                      rd, 
 const Array<shared_ptr<Surface> >& surfaceArray,
 const shared_ptr<Texture>&         previousDepthBuffer,
 const float                        minZSeparation,
 bool                               requireBinaryAlpha,
 const Color3&                      transmissionWeight) const {
    debugAssertGLOk();

    static shared_ptr<Shader> depthNonOpaqueShader =
        Shader::fromFiles
        (System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.vrt"),
         System::findDataFile("UniversalSurface/UniversalSurface_depthOnlyNonOpaque.pix"));

    static const shared_ptr<Shader> depthShader = 
        Shader::fromFiles(System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.vrt")
#ifdef G3D_OSX // OS X crashes if there isn't a shader bound for depth only rendering
, System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.pix")
#endif
);

    static const shared_ptr<Shader> depthPeelShader =
        Shader::fromFiles(System::findDataFile("UniversalSurface/UniversalSurface_depthOnly.vrt"),
                          System::findDataFile("UniversalSurface/UniversalSurface_depthPeel.pix"));

    rd->setColorWrite(false);
    const CullFace cull = rd->cullFace();

    static Array<shared_ptr<Surface> > opaqueSurfaces;
    static Array<shared_ptr<Surface> > alphaSurfaces;
    opaqueSurfaces.fastClear();
    alphaSurfaces.fastClear();

    for (int i = 0; i < surfaceArray.size(); ++i) {
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[i]);
        debugAssertM(surface, "Surface::renderDepthOnlyHomogeneous passed the wrong subclass");
        const shared_ptr<UniversalMaterial>& material = surface->material();
        const shared_ptr<Texture>& lambertian = material->bsdf()->lambertian().texture();
        const bool thisSurfaceNeedsAlphaTest = (material->alphaHint() != AlphaHint::ONE) && notNull(lambertian) && ! lambertian->opaque();   
        if (surface->hasTransmission() || thisSurfaceNeedsAlphaTest) {
            alphaSurfaces.append(surface);
        } else {
            opaqueSurfaces.append(surface);
        }
    }

    const bool batchRender = true;
    if (batchRender) {
        static Array< Array<shared_ptr<Surface> > > batchTable;
        batchTable.fastClear();

        // Separate into batches that have the same cull face, bones, coordinate frame, and index buffer
        // We could potentially batch surfaces with different coordinate frames together by binding an array of coordinate frames instead.
        categorizeByBatchDescriptor(opaqueSurfaces, batchTable);
        // Process opaque surfaces first, front-to-back to maximize early-z test performance
        for (int b = batchTable.size() - 1; b >= 0; --b) {
            const Array<shared_ptr<Surface> >& batch = batchTable[b];
            const shared_ptr<UniversalSurface>& canonicalSurface = dynamic_pointer_cast<UniversalSurface>(batch[0]);

            const shared_ptr<UniversalSurface::GPUGeom>& geom = canonicalSurface->gpuGeom();

            if (geom->twoSided) {
                rd->setCullFace(CullFace::NONE);
            }

            // Needed for every type of pass 
            CFrame cframe;
            canonicalSurface->getCoordinateFrame(cframe, false);
            if (geom->hasBones()) {
                rd->setObjectToWorldMatrix(CFrame());
            } else {
                rd->setObjectToWorldMatrix(cframe);
            }

            Args args;
            canonicalSurface->setShaderArgs(args);

            args.setMacro("OPAQUE_PASS", 1);
            args.setMacro("HAS_ALPHA", 0);
            args.setMacro("USE_PARALLAX_MAPPING", 0);

            // Don't use lightMaps for depth only...
            args.setMacro("NUM_LIGHTMAP_DIRECTIONS", 0);
            args.setMacro("NUM_LIGHTS", 0);
            args.setMacro("USE_IMAGE_STORE", 0);
            args.setUniform("transmissionWeight", transmissionWeight, true);

            bindDepthPeelArgs(args, rd, previousDepthBuffer, minZSeparation);
            
            for (int s = batch.size() - 1; s >= 0; --s) {
                args.appendIndexStream(dynamic_pointer_cast<UniversalSurface>(batch[s])->gpuGeom()->index);
            }

            // N.B. Alpha testing is handled explicitly inside the shader.
            if (notNull(previousDepthBuffer)) {
                LAUNCH_SHADER_PTR_WITH_HINT(depthPeelShader, args, format("batch%d (%s)", b, canonicalSurface->m_profilerHint.c_str()));
            }
            else {
                LAUNCH_SHADER_PTR_WITH_HINT(depthShader, args, format("batch%d (%s)", b, canonicalSurface->m_profilerHint.c_str()));
            }
            /*
            for (int s = batch.size() - 1; s >= 0; --s) {
                args.setIndexStream(dynamic_pointer_cast<UniversalSurface>(batch[s])->gpuGeom()->index);
                // N.B. Alpha testing is handled explicitly inside the shader.
                if (notNull(previousDepthBuffer)) {
                    LAUNCH_SHADER_PTR(depthPeelShader, args);
                }
                else {
                    LAUNCH_SHADER_PTR(depthShader, args);
                }
            }            */           

            if (geom->twoSided) {
                rd->setCullFace(cull);
            }
        } // for each surface  

    } else {
        // Process opaque surfaces first, front-to-back to maximize early-z test performance
        for (int g = opaqueSurfaces.size() - 1; g >= 0; --g) {
            const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(opaqueSurfaces[g]);
            const shared_ptr<UniversalSurface::GPUGeom>& geom = surface->gpuGeom();

            if (geom->twoSided) {
                rd->setCullFace(CullFace::NONE);
            }

            // Needed for every type of pass 
            CFrame cframe;
            surface->getCoordinateFrame(cframe, false);
            if (geom->hasBones()) {
                rd->setObjectToWorldMatrix(CFrame());
            }
            else {
                rd->setObjectToWorldMatrix(cframe);
            }

            Args args;
            surface->setShaderArgs(args);

            args.setMacro("OPAQUE_PASS", 1);
            args.setMacro("HAS_ALPHA", 0);
            args.setMacro("USE_PARALLAX_MAPPING", 0);

            // Don't use lightMaps for depth only...
            args.setMacro("NUM_LIGHTMAP_DIRECTIONS", 0);
            args.setMacro("NUM_LIGHTS", 0);
            args.setMacro("USE_IMAGE_STORE", 0);

            bindDepthPeelArgs(args, rd, previousDepthBuffer, minZSeparation);

            // N.B. Alpha testing is handled explicitly inside the shader.
            if (notNull(previousDepthBuffer)) {
                LAUNCH_SHADER_PTR_WITH_HINT(depthPeelShader, args, surface->m_profilerHint);
            }
            else {
                LAUNCH_SHADER_PTR_WITH_HINT(depthShader, args, surface->m_profilerHint);
            }

            if (geom->twoSided) {
                rd->setCullFace(cull);
            }
        } // for each surface  
    }

    // Now process surfaces with alpha 
    for (int g = 0; g < alphaSurfaces.size(); ++g) {
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(alphaSurfaces[g]);
        debugAssertM(surface, "Surface::renderDepthOnlyHomogeneous passed the wrong subclass");
        const shared_ptr<Texture>& lambertian = surface->material()->bsdf()->lambertian().texture();
        const bool thisSurfaceNeedsAlphaTest = (surface->material()->alphaHint() != AlphaHint::ONE) && notNull(lambertian) && ! lambertian->opaque();
        const bool thisSurfaceHasTransmissive = surface->material()->hasTransmissive();

        const shared_ptr<UniversalSurface::GPUGeom>& geom = surface->gpuGeom();
            
        if (geom->twoSided) {
            rd->setCullFace(CullFace::NONE);
        }
            
        // Needed for every type of pass 
        CFrame cframe;
        surface->getCoordinateFrame(cframe, false);
        if (geom->hasBones()) {
            rd->setObjectToWorldMatrix(CFrame());
        } else {
            rd->setObjectToWorldMatrix(cframe);
        }
            
        Args args;
        surface->setShaderArgs(args, true);
    	bindDepthPeelArgs(args, rd, previousDepthBuffer, minZSeparation);
        args.setUniform("transmissionWeight", transmissionWeight);
        args.setMacro("OPAQUE_PASS", 1);
        
        // N.B. Alpha testing is handled explicitly inside the shader.
        if (thisSurfaceHasTransmissive || (thisSurfaceNeedsAlphaTest && ((surface->material()->alphaHint() == AlphaHint::BLEND) || (surface->material()->alphaHint() == AlphaHint::BINARY)))) {
            args.setMacro("STOCHASTIC", ! requireBinaryAlpha);
            // The depth with alpha shader handles the depth peel case internally
            LAUNCH_SHADER_PTR_WITH_HINT(depthNonOpaqueShader, args, surface->m_profilerHint);
        } else if (notNull(previousDepthBuffer)) {
            LAUNCH_SHADER_PTR_WITH_HINT(depthPeelShader, args, surface->m_profilerHint);
        } else {
            LAUNCH_SHADER_PTR_WITH_HINT(depthShader, args, surface->m_profilerHint);
        }

        if (geom->twoSided) {
            rd->setCullFace(cull);
        }
    } // for each surface   
}
    

void UniversalSurface::renderIntoGBufferHomogeneous
(RenderDevice*                      rd, 
 const Array<shared_ptr<Surface> >& surfaceArray,
 const shared_ptr<GBuffer>&         gbuffer,
 const CFrame&                      previousCameraFrame,
 const CFrame&                      expressivePreviousCameraFrame,
 const shared_ptr<Texture>&         depthPeelTexture,
 const float                        minZSeparation,
 const LightingEnvironment&         lightingEnvironment) const {

    rd->pushState(); {
        const CullFace oldCullFace = rd->cullFace();

        // Render front-to-back for early-out Z
        for (int s = surfaceArray.size() - 1; s >= 0; --s) {
            const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[s]);

            debugAssertM(notNull(surface), 
                         "Non UniversalSurface element of surfaceArray "
                         "in UniversalSurface::renderIntoGBufferHomogeneous");

            if (surface->hasRefractiveTransmission()) {
                // These surfaces can't appear in the G-buffer because they aren't shaded
                // until after the screen-space refraction texture has been captured.
                continue;
            }

            if (! surface->anyOpaque()) {
                continue;
            }

            const shared_ptr<GPUGeom>&           gpuGeom = surface->gpuGeom();
            const shared_ptr<UniversalMaterial>& material = surface->material();
            debugAssertM(notNull(material), "NULL material on surface");
            (void)material;

            Args args;
            CFrame cframe;
            surface->getCoordinateFrame(cframe, false);
            rd->setObjectToWorldMatrix(cframe);

            if ((gbuffer->specification().encoding[GBuffer::Field::CS_POSITION_CHANGE].format != NULL) || 
                (gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE].format != NULL)) {
                // Previous object-to-camera projection for velocity buffer
                CFrame previousFrame;
                surface->getCoordinateFrame(previousFrame, true);
                const CFrame& previousObjectToCameraMatrix = previousCameraFrame.inverse() * previousFrame;
                args.setUniform("PreviousObjectToCameraMatrix", previousObjectToCameraMatrix);
            }
            
            if (gbuffer->specification().encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION].format != NULL) {
                // Previous object-to-camera projection for velocity buffer
                CFrame previousFrame;
                surface->getCoordinateFrame(previousFrame, true);
                const CFrame& expressivePreviousObjectToCameraMatrix = expressivePreviousCameraFrame.inverse() * previousFrame;
                args.setUniform("ExpressivePreviousObjectToCameraMatrix", expressivePreviousObjectToCameraMatrix);
            }

            if ((gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE].format != NULL) ||
                (gbuffer->specification().encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION].format != NULL)) {
                // Map (-1, 1) normalized device coordinates to actual pixel positions
                const Matrix4& screenSize =
                    Matrix4(rd->width() / 2.0f, 0.0f,                0.0f, rd->width() / 2.0f,
                            0.0f,               rd->height() / 2.0f, 0.0f, rd->height() / 2.0f,
                            0.0f,               0.0f,                1.0f, 0.0f,
                            0.0f,               0.0f,                0.0f, 1.0f);
                args.setUniform("ProjectToScreenMatrix", screenSize * rd->invertYMatrix() * rd->projectionMatrix());
            }

            if (gpuGeom->twoSided) {
                rd->setCullFace(CullFace::NONE);
            }

            surface->setShaderArgs(args, true);

            args.setMacro("NUM_LIGHTS", 0);
            args.setMacro("USE_IMAGE_STORE", 0);

            const Rect2D& colorRect = gbuffer->colorRect();
            args.setUniform("lowerCoord", colorRect.x0y0());
            args.setUniform("upperCoord", colorRect.x1y1());
            
    	    bindDepthPeelArgs(args, rd, depthPeelTexture, minZSeparation);

            // N.B. Alpha testing is handled explicitly inside the shader.
            LAUNCH_SHADER_WITH_HINT("UniversalSurface/UniversalSurface_gbuffer.*", args, surface->m_profilerHint);

            if (gpuGeom->twoSided) {
                rd->setCullFace(oldCullFace);
            }
        }
    } rd->popState();
}



void UniversalSurface::renderIntoSVOHomogeneous
(RenderDevice*                  rd,
Array<shared_ptr<Surface> >&   surfaceArray,
const shared_ptr<SVO>&         svo,
const CFrame&                  previousCameraFrame) const {
	/*
	static shared_ptr<Shader> s_svoGbufferShader;
	if (isNull(s_svoGbufferShader)) {
		s_svoGbufferShader = Shader::fromFiles
			(System::findDataFile("UniversalSurface/UniversalSurface_SVO.vrt"),
			System::findDataFile("UniversalSurface/UniversalSurface_SVO.geo"),
			System::findDataFile("UniversalSurface/UniversalSurface_SVO.pix"));
	}
	shared_ptr<Shader> shader = s_svoGbufferShader;
	*/
	rd->pushState(); {

		rd->setColorWrite(false);
		rd->setAlphaWrite(false);
		rd->setDepthWrite(false);
		rd->setCullFace(CullFace::NONE);
		rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
		svo->setOrthogonalProjection(rd);

		glEnable(0x9346); //CONSERVATIVE_RASTERIZATION_NV

		for (int s = 0; s < surfaceArray.size(); ++s) {
			const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[s]);
			debugAssertM(notNull(surface),
				"Non UniversalSurface element of surfaceArray "
				"in UniversalSurface::renderIntoSVOHomogeneous");

			const shared_ptr<GPUGeom>&           gpuGeom = surface->gpuGeom();
			const shared_ptr<UniversalMaterial>& material = surface->material();
			debugAssert(material);

			Args args;

			CFrame cframe;
			surface->getCoordinateFrame(cframe, false);
			rd->setObjectToWorldMatrix(cframe);

			args.setMacro("NUM_LIGHTS", 0);
			args.setMacro("HAS_ALPHA", 0);

			// Bind material arguments
			material->setShaderArgs(args, "material_");

			// Define FragmentBuffer GBuffer outputs
			//args.appendToPreamble(svo->writeDeclarationsFragmentBuffer());	//Done in bindWriteUniforms

			// Bind image, bias, scale arguments
			svo->bindWriteUniformsFragmentBuffer(args);


			// Bind geometry
			gpuGeom->setShaderArgs(args);

			for (int s = 0; s< svo->getNumSurfaceLayers(); s++){
				args.setUniform("curSurfaceOffset", -float(s)*1.0f / float(svo->fineVoxelResolution()));

				// N.B. Alpha testing is handled explicitly inside the shader.
				LAUNCH_SHADER_WITH_HINT("UniversalSurface/UniversalSurface_SVO.*", args, m_profilerHint);
				//rd->apply(shader, args);
			}

		} // for each surface

		glDisable(0x9346);

	} rd->popState();

}


void UniversalSurface::sortFrontToBack(Array<shared_ptr<UniversalSurface> >& a, const Vector3& v) {
    Array<shared_ptr<Surface> > s;
    s.resize(a.size());
    for (int i = 0; i < s.size(); ++i) {
        s[i] = a[i];
    }
    Surface::sortFrontToBack(s, v);
    for (int i = 0; i < s.size(); ++i) {
        a[i] = dynamic_pointer_cast<UniversalSurface>(s[i]);
    }
}
UniversalSurface::UniversalSurface
    (const String&                          name,
    const CoordinateFrame&                 frame,
    const CoordinateFrame&                 previousFrame,
    const shared_ptr<UniversalMaterial>&   material,
    const shared_ptr<GPUGeom>&             gpuGeom,
    const CPUGeom&                         cpuGeom,
    const shared_ptr<ReferenceCountedObject>& source,
    const ExpressiveLightScatteringProperties& expressive,
    const shared_ptr<Model>&         model,
    const shared_ptr<Entity>&        entity,
    const shared_ptr<UniformTable>&  uniformTable,
    int                                    numInstances) :
    Surface(expressive),
    m_name(name),
    m_frame(frame),
    m_previousFrame(previousFrame),
    m_material(material),
    m_gpuGeom(gpuGeom),
    m_cpuGeom(cpuGeom),
    m_numInstances(numInstances),
    m_uniformTable(uniformTable),
    m_source(source) {
    m_model = model;
    m_entity = entity;
    m_profilerHint = (notNull(entity) ? entity->name() + "/" : "") + (notNull(model) ? model->name() + "/" : "") + m_name;
}

shared_ptr<UniversalSurface> UniversalSurface::create
(const String&                              name,
 const CFrame&                              frame, 
 const CFrame&                              previousFrame,
 const shared_ptr<UniversalMaterial>&       material,
 const shared_ptr<GPUGeom>&                 gpuGeom,
 const CPUGeom&                             cpuGeom,
 const shared_ptr<ReferenceCountedObject>&  source,
 const ExpressiveLightScatteringProperties& expressive,
 const shared_ptr<Model>&                   model,
 const shared_ptr<Entity>&                  entity,
 const shared_ptr<UniformTable>&            uniformTable,
 int                                        numInstances) {
    debugAssertM(gpuGeom, "Passed a void GPUGeom pointer to UniversalSurface::create.");
    // Cannot check if the gpuGeom is valid because it might not be filled out yet
    return shared_ptr<UniversalSurface>(new UniversalSurface(name, frame, previousFrame, material, gpuGeom, cpuGeom, source, expressive, model, entity, uniformTable, numInstances));
}   


bool UniversalSurface::requiresBlending() const {
    // Note that non-refractive transmission is processed as opaque
    return hasNonRefractiveTransmission() ||
        (hasTransmission() && (m_material->refractionHint() == RefractionHint::DYNAMIC_FLAT_OIT)) ||
        (m_material->alphaHint() == AlphaHint::BLEND);
}


bool UniversalSurface::hasRefractiveTransmission() const {
    return hasTransmission() && (m_material->bsdf()->etaReflect() != m_material->bsdf()->etaTransmit());
}


bool UniversalSurface::hasNonRefractiveTransmission() const {
    return hasTransmission() && (m_material->bsdf()->etaReflect() == m_material->bsdf()->etaTransmit());
}


void UniversalSurface::setShaderArgs(Args& args, bool useStructFormat) const {
    m_gpuGeom->setShaderArgs(args);

    if (useStructFormat) {
        m_material->setShaderArgs(args, "material.");
        args.setMacro("INFER_AMBIENT_OCCLUSION_AT_TRANSPARENT_PIXELS", m_material->inferAmbientOcclusionAtTransparentPixels());
        args.setMacro("HAS_ALPHA",        m_material->hasAlpha());
        args.setMacro("HAS_TRANSMISSIVE", m_material->hasTransmissive());
        args.setMacro("HAS_EMISSIVE",     m_material->hasEmissive());
        args.setMacro("ALPHA_HINT",       m_material->alphaHint());
    } else {
        m_material->setShaderArgs(args, "material_");
    }
    args.append(m_uniformTable);
    args.setNumInstances(m_numInstances);
}


void UniversalSurface::launchForwardShader(Args& args) const {
    if (false && m_gpuGeom->hasBones()) {
        LAUNCH_SHADER_WITH_HINT("UniversalSurface/UniversalSurface_boneWeights.*", args, m_profilerHint);
    } else {
        LAUNCH_SHADER_WITH_HINT("UniversalSurface/UniversalSurface_render.*", args, m_profilerHint);
    }
}


void UniversalSurface::modulateBackgroundByTransmission(RenderDevice* rd) const {
    if (! hasTransmission()) { return; }

    rd->pushState(); {
        // Modulate background by the transmission color
        Args args;

        args.setMacro("HAS_ALPHA",  m_material->hasAlpha());
        args.setMacro("ALPHA_HINT", m_material->alphaHint());
        args.setMacro("HAS_TRANSMISSIVE", m_material->hasTransmissive());
        args.setMacro("HAS_EMISSIVE", false);

        // Don't use lightMaps
        args.setMacro("NUM_LIGHTMAP_DIRECTIONS", 0);

        m_gpuGeom->setShaderArgs(args);
        m_material->setShaderArgs(args, "material.");
        rd->setObjectToWorldMatrix(m_frame);
        rd->setBlendFunc(RenderDevice::BLEND_ZERO, RenderDevice::BLEND_SRC_COLOR);
        LAUNCH_SHADER_WITH_HINT("UniversalSurface/UniversalSurface_modulateBackground.*", args, m_profilerHint);

    } rd->popState();
}


void UniversalSurface::render
   (RenderDevice*                       rd,
    const LightingEnvironment&          environment,
    RenderPassType                      passType, 
    const String&                       declareWritePixel) const {

    const bool anyOpaquePass = (passType == RenderPassType::OPAQUE_SAMPLES) || (passType == RenderPassType::OPAQUE_SAMPLES_WITH_SCREEN_SPACE_REFRACTION);

    if ((anyOpaquePass && ! anyOpaque()) ||
        ((passType == RenderPassType::OPAQUE_SAMPLES) && hasRefractiveTransmission()) ||
        ((passType == RenderPassType::OPAQUE_SAMPLES_WITH_SCREEN_SPACE_REFRACTION) && ! hasRefractiveTransmission()) ||
        (! anyOpaquePass && ! requiresBlending())) {
        // Nothing to do in these cases
        return;
    }

    Sphere myBounds;
    CFrame cframe;
    getObjectSpaceBoundingSphere(myBounds, false);
    getCoordinateFrame(cframe, false);
    myBounds = cframe.toWorldSpace(myBounds);
    
    RenderDevice::BlendFunc srcBlend;
    RenderDevice::BlendFunc dstBlend;
    RenderDevice::BlendEq   blendEq;
    RenderDevice::BlendEq   ignoreEq;
    RenderDevice::BlendFunc ignoreFunc;
    rd->getBlendFunc(Framebuffer::COLOR0, srcBlend, dstBlend, blendEq, ignoreFunc, ignoreFunc, ignoreEq);

    const bool twoSided = m_gpuGeom->twoSided;
    const CullFace oldCullFace = rd->cullFace();

    LightingEnvironment reducedLighting(environment);

    // Remove lights that cannot affect this object
    for (int L = 0; L < reducedLighting.lightArray.size(); ++L) {
        Sphere s = reducedLighting.lightArray[L]->effectSphere();
        if (! s.intersects(myBounds)) {
            // This light does not affect this object
            reducedLighting.lightArray.fastRemove(L);
            --L;
        }
    }

    Args args;
    reducedLighting.setShaderArgs(args, "");
    args.setMacro("OPAQUE_PASS", anyOpaquePass);

    if (passType == RenderPassType::SINGLE_PASS_UNORDERED_BLENDED_SAMPLES) {
        args.setMacro("DECLARE_writePixel", declareWritePixel);
    } else {
        debugAssertM(declareWritePixel.empty() || (declareWritePixel == Surface::defaultWritePixelDeclaration()),
            "Passed a custom declareWritePixel value with a render pass type that does not support it. G3D will ignore declareWritePixel!");
    }
    rd->setObjectToWorldMatrix(m_frame);
    setShaderArgs(args, true);

    rd->setDepthWrite(anyOpaquePass);
    rd->setDepthTest(RenderDevice::DEPTH_LESS);

    bindScreenSpaceTexture(args, reducedLighting, rd, environment.screenColorGuardBand());

    if (hasRefractiveTransmission()) {
        args.setMacro("REFRACTION", 1);
    }

    switch (passType) {
    case RenderPassType::OPAQUE_SAMPLES:
    case RenderPassType::OPAQUE_SAMPLES_WITH_SCREEN_SPACE_REFRACTION:
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
        if (twoSided) { rd->setCullFace(CullFace::NONE); }
        launchForwardShader(args);
        break;

    case RenderPassType::MULTIPASS_BLENDED_SAMPLES:
        // The shader is configured for premultipled alpha
        if (hasTransmission()) {
            // The modulate background pass will darken the background appropriately
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        } else {
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        }
        if (twoSided) {
            rd->setCullFace(CullFace::FRONT);
            modulateBackgroundByTransmission(rd);
            launchForwardShader(args);

            rd->setCullFace(CullFace::BACK);
            modulateBackgroundByTransmission(rd);
            launchForwardShader(args);
        } else {
            rd->setCullFace(CullFace::BACK);
            modulateBackgroundByTransmission(rd);
            launchForwardShader(args);
        }
        break;

    case RenderPassType::SINGLE_PASS_UNORDERED_BLENDED_SAMPLES:
        if (m_gpuGeom->twoSided) { rd->setCullFace(CullFace::NONE); }
        launchForwardShader(args);
        break;
       
    }
    
    // Restore old blend state
    rd->setBlendFunc(srcBlend, dstBlend, blendEq);
    rd->setCullFace(oldCullFace);
}


void UniversalSurface::bindScreenSpaceTexture
   (Args&                       args, 
    const LightingEnvironment&  lightingEnvironment, 
    RenderDevice*               rd, 
    const Vector2int16          guardBandSize) const {

    const CFrame& cameraFrame = rd->cameraToWorldMatrix();

    Sphere sphere;
    getObjectSpaceBoundingSphere(sphere);
    const Sphere& bounds3D = m_frame.toWorldSpace(sphere);
    // const float distanceToCamera = (sphere.center + m_frame.translation - cameraFrame.translation).dot(cameraFrame.lookVector());
                    
    // Estimate of distance from object to background to
    // be constant (we could read back depth buffer, but
    // that won't produce frame coherence)

    const float z0 = max(8.0f - (m_material->bsdf()->etaTransmit() - 1.0f) * 5.0f, bounds3D.radius);
    const float backZ = cameraFrame.pointToObjectSpace(bounds3D.center).z - z0;
    args.setUniform("backgroundZ", backZ);
                    
    args.setUniform("etaRatio", m_material->bsdf()->etaReflect() / m_material->bsdf()->etaTransmit());
                    
    // Find out how big the back plane is in meters
    float backPlaneZ = min(-0.5f, backZ);
    Projection P(rd->projectionMatrix());
    P.setFarPlaneZ(backPlaneZ);
    Vector3 ur, ul, ll, lr;
    P.getFarViewportCorners(rd->viewport(), ur, ul, ll, lr);

    // Since we use the lengths only, do not bother taking to world space
    Vector2 backSizeMeters((ur - ul).length(), (ur - lr).length());

    args.setUniform("backSizeMeters", backSizeMeters);
    args.setUniform("background", lightingEnvironment.screenColorTexture(), Sampler::video());
    args.setUniform("backgroundMinCoord", Vector2(guardBandSize + Vector2int16(1, 1)) / lightingEnvironment.screenColorTexture()->vector2Bounds());
    args.setUniform("backgroundMaxCoord", Vector2(1,1) - Vector2(guardBandSize + Vector2int16(1, 1)) / lightingEnvironment.screenColorTexture()->vector2Bounds());
}

String UniversalSurface::name() const {
    return m_name;
}


bool UniversalSurface::hasTransmission() const {
    return m_material->bsdf()->transmissive().notBlack();
}


void UniversalSurface::getCoordinateFrame(CoordinateFrame& c, bool previous) const {
    if (previous) {
        c = m_previousFrame;
    } else {
        c = m_frame;
    }
}


void UniversalSurface::getObjectSpaceBoundingSphere(Sphere& s, bool previous) const {
    (void) previous;
    s = m_gpuGeom->sphereBounds;
}


void UniversalSurface::getObjectSpaceBoundingBox(AABox& b, bool previous) const {
    (void) previous;
    b = m_gpuGeom->boxBounds;
}


void UniversalSurface::getObjectSpaceGeometry
   (Array<int>&                  index, 
    Array<Point3>&               vertex, 
    Array<Vector3>&              normal, 
    Array<Vector4>&              packedTangent, 
    Array<Point2>&               texCoord, 
    bool                         previous) const {

    index.copyPOD(*(m_cpuGeom.index));
    //  If the CPUVertexArray is not null then it superceeds the other data
    if (notNull(m_cpuGeom.vertexArray)) {
        const Array<CPUVertexArray::Vertex>& vertexArray = (m_cpuGeom.vertexArray)->vertex;
        for (int i = 0; i < vertexArray.size(); ++i) {
            const CPUVertexArray::Vertex& vert = vertexArray[i];
            vertex.append(vert.position);
            normal.append(vert.normal);
            packedTangent.append(vert.tangent);
            texCoord.append(vert.texCoord0);
        }
    } else {
        vertex.copyPOD((m_cpuGeom.geometry)->vertexArray);
        normal.copyPOD((m_cpuGeom.geometry)->normalArray);
        packedTangent.copyPOD(*(m_cpuGeom.packedTangent));
        texCoord.copyPOD(*(m_cpuGeom.texCoord0));
    }
}


void UniversalSurface::CPUGeom::copyVertexDataToGPU
(AttributeArray&               vertex, 
 AttributeArray&               normal, 
 AttributeArray&               packedTangentVAR, 
 AttributeArray&               texCoord0VAR, 
 AttributeArray&               texCoord1VAR, 
 AttributeArray&               vertexColorVAR,
 VertexBuffer::UsageHint       hint) {

    if (notNull(vertexArray)) {
        vertexArray->copyToGPU(vertex, normal, packedTangentVAR, texCoord0VAR, texCoord1VAR, vertexColorVAR, hint);
    } else {
        // Non-interleaved support
        texCoord1VAR = AttributeArray();
        vertexColorVAR = AttributeArray();

        size_t vtxSize = sizeof(Vector3) * geometry->vertexArray.size();
        size_t texSize = sizeof(Vector2) * texCoord0->size();
        size_t tanSize = sizeof(Vector4) * packedTangent->size();

        if ((vertex.maxSize() >= vtxSize) &&
            (normal.maxSize() >= vtxSize) &&
            ((tanSize == 0) || (packedTangentVAR.maxSize() >= tanSize)) &&
            ((texSize == 0) || (texCoord0VAR.maxSize() >= texSize))) {
            AttributeArray::updateInterleaved
               (geometry->vertexArray,  vertex,
                geometry->normalArray,  normal,
                *packedTangent,         packedTangentVAR,
                *texCoord0,             texCoord0VAR);

        } else {

            // Maximum round-up size of varArea.
            int roundOff = 16;

            // Allocate new VARs
            shared_ptr<VertexBuffer> varArea = VertexBuffer::create(vtxSize * 2 + texSize + tanSize + roundOff, hint);
            AttributeArray::createInterleaved
                (geometry->vertexArray, vertex,
                 geometry->normalArray, normal,
                 *packedTangent,        packedTangentVAR,
                 *texCoord0,            texCoord0VAR,
                 varArea);       
        }
    }
}



namespace _internal {

class IndexOffsetTableKey {
public:
    const CPUVertexArray* vertexArray;
    CFrame cFrame;
    IndexOffsetTableKey(const CPUVertexArray* vArray) : vertexArray(vArray){}
    static size_t hashCode(const IndexOffsetTableKey& key) {
        const size_t cframeHash = key.cFrame.rotation.row(0).hashCode() + key.cFrame.rotation.row(1).hashCode() + 
                                    key.cFrame.rotation.row(2).hashCode() + key.cFrame.translation.hashCode();
        return HashTrait<void*>::hashCode(key.vertexArray) + cframeHash;
    }
    static bool equals(const _internal::IndexOffsetTableKey& a, const _internal::IndexOffsetTableKey& b) {
        return a.vertexArray == b.vertexArray && a.cFrame == b.cFrame;
    }
};

}


void UniversalSurface::getTrisHomogeneous
(const Array<shared_ptr<Surface> >& surfaceArray, 
 CPUVertexArray&                    cpuVertexArray, 
 Array<Tri>&                        triArray,
 bool                               computePrevPosition) const{

    // Maps already seen surface-owned vertexArrays to the vertex index offset in the CPUVertexArray


    Table<const _internal::IndexOffsetTableKey, uint32, _internal::IndexOffsetTableKey, _internal::IndexOffsetTableKey> indexOffsetTable;

    const bool PREVIOUS = true;
    const bool CURRENT = false;

    for (int i = 0; i < surfaceArray.size(); ++i) {
            
        const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(surfaceArray[i]);
        alwaysAssertM(surface, "Non-UniversalSurface passed to UniversalSurface::getTrisHomogenous.");
 
        const UniversalSurface::CPUGeom&             cpuGeom   = surface->cpuGeom();
        const shared_ptr<UniversalSurface::GPUGeom>& gpuGeom   = surface->gpuGeom();
    
        bool twoSided = gpuGeom->twoSided;
        
        debugAssert(gpuGeom->primitive == PrimitiveType::TRIANGLES);
        debugAssert(cpuGeom.index != NULL);
    
        const Array<int>& index(*cpuGeom.index);
    
        _internal::IndexOffsetTableKey key(cpuGeom.vertexArray);
        // Object to world matrix.  Guaranteed to be an RT transformation,
        // so we can directly transform normals as if they were vectors.
        surface->getCoordinateFrame(key.cFrame, CURRENT);

        CFrame prevFrame;
        surface->getCoordinateFrame(prevFrame, PREVIOUS);

        bool created = false;
        uint32& indexOffset = indexOffsetTable.getCreate(key, created);
        if (created) {
            indexOffset = cpuVertexArray.size();

            if (computePrevPosition) {
                cpuVertexArray.transformAndAppend(*(key.vertexArray), key.cFrame, prevFrame);
            } else {
                cpuVertexArray.transformAndAppend(*(key.vertexArray), key.cFrame);
            }
        } 

        alwaysAssertM(cpuGeom.vertexArray != NULL, "No support for non-interlaced vertex formats");

        // G3D 9.00 format with interlaced vertices
        // All data are in object space
        for (int i = 0; i < index.size(); i += 3) {
            triArray.append
                (Tri(index[i + 0] + indexOffset,
                     index[i + 1] + indexOffset,
                     index[i + 2] + indexOffset,

                     cpuVertexArray,
                     surface,
                     twoSided));

        } // for index
    } // for surface
}


G3D_DECLARE_SYMBOL(g3d_Vertex);
G3D_DECLARE_SYMBOL(g3d_Normal);
G3D_DECLARE_SYMBOL(g3d_TexCoord0);
G3D_DECLARE_SYMBOL(g3d_TexCoord1);
G3D_DECLARE_SYMBOL(g3d_PackedTangent);
G3D_DECLARE_SYMBOL(g3d_BoneIndices);
G3D_DECLARE_SYMBOL(g3d_BoneWeights);
G3D_DECLARE_SYMBOL(boneMatrixTexture);
G3D_DECLARE_SYMBOL(prevBoneMatrixTexture);
G3D_DECLARE_SYMBOL(HAS_BONES);
void UniversalSurface::GPUGeom::setShaderArgs(Args& args) const {
    debugAssert(normal.valid());
    debugAssert(index.valid());

    args.setAttributeArray(SYMBOL_g3d_Vertex, vertex);
    args.setAttributeArray(SYMBOL_g3d_Normal, normal);
            
    if (texCoord0.valid() && (texCoord0.size() > 0)){
        args.setAttributeArray(SYMBOL_g3d_TexCoord0, texCoord0);
    }

    if (texCoord1.valid() && (texCoord1.size() > 0)){
        args.setAttributeArray(SYMBOL_g3d_TexCoord1, texCoord1);
    }

    if (vertexColor.valid() && (vertexColor.size() > 0)){
        args.setMacro("HAS_VERTEX_COLOR", true);
        args.setAttributeArray("g3d_VertexColor", vertexColor);
    } else {
        args.setMacro("HAS_VERTEX_COLOR", false);
    }

    if (packedTangent.valid() && (packedTangent.size() > 0)) {
        args.setAttributeArray(SYMBOL_g3d_PackedTangent, packedTangent);
    }

    if (hasBones()) {
        args.setAttributeArray(SYMBOL_g3d_BoneIndices, boneIndices);
        args.setAttributeArray(SYMBOL_g3d_BoneWeights, boneWeights);
        args.setUniform(SYMBOL_boneMatrixTexture, boneTexture, Sampler::buffer());
        args.setUniform(SYMBOL_prevBoneMatrixTexture, prevBoneTexture, Sampler::buffer(), false);
        args.setMacro("HAS_BONES", 1);
    } else {
        args.setMacro("HAS_BONES", 0); 
    }
    args.setIndexStream(index);
    args.setPrimitiveType(primitive);
}


UniversalSurface::GPUGeom::GPUGeom(const shared_ptr<GPUGeom>& other) {
    alwaysAssertM(notNull(other), "Tried to contruct GPUGeom from NULL pointer");
    primitive       = other->primitive;
    index           = other->index;
    vertex          = other->vertex;
    normal          = other->normal;
    packedTangent   = other->packedTangent;
    texCoord0       = other->texCoord0;
    texCoord1       = other->texCoord1;
    vertexColor     = other->vertexColor;
    boneIndices     = other->boneIndices;
    boneWeights     = other->boneWeights;
    boneTexture     = other->boneTexture;
    prevBoneTexture = other->prevBoneTexture;
    twoSided        = other->twoSided;
    boxBounds       = other->boxBounds;
    sphereBounds    = other->sphereBounds;
}

} // G3D

