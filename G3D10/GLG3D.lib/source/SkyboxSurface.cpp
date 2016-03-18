/**
 \file SkyboxSurface.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu
 \created 2012-07-16
 \edited  2015-01-02
 
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/CoordinateFrame.h"
#include "GLG3D/Draw.h"
#include "G3D/Vector4.h"
#include "G3D/Array.h"
#include "G3D/AABox.h"
#include "G3D/Projection.h"
#include "G3D/fileutils.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SkyboxSurface.h"
#include "GLG3D/Light.h"

namespace G3D {

AttributeArray      SkyboxSurface::s_cubeVertices;
IndexStream         SkyboxSurface::s_cubeIndices;

SkyboxSurface::SkyboxSurface   
   (const shared_ptr<Texture>& c0,
    const shared_ptr<Texture>& c1,
    float                alpha) :

    m_alpha(alpha),
    m_texture0(c0),
    m_texture1(c1) {

    debugAssert((alpha >= 0.0f) && (alpha <= 1.0f));
}     


shared_ptr<SkyboxSurface> SkyboxSurface::create
   (const shared_ptr<Texture>&    c0,
    const shared_ptr<Texture>&    c1,
    float                         alpha) {

    debugAssert(c0);
    debugAssert(c0->dimension() == Texture::DIM_CUBE_MAP || c0->dimension() == Texture::DIM_2D);

    if (! s_cubeVertices.valid()) {
        // Size of the box (doesn't matter because we're rendering at infinity)
        const float s = 1.0f;

        Array<Vector4> vert;
        vert.append(Vector4(-s, +s, -s, 0.0f));
        vert.append(Vector4(-s, -s, -s, 0.0f));
        vert.append(Vector4(+s, -s, -s, 0.0f));
        vert.append(Vector4(+s, +s, -s, 0.0f));
    
        vert.append(Vector4(-s, +s, +s, 0.0f));
        vert.append(Vector4(-s, -s, +s, 0.0f));
        vert.append(Vector4(-s, -s, -s, 0.0f));
        vert.append(Vector4(-s, +s, -s, 0.0f));
    
        vert.append(Vector4(+s, +s, +s, 0.0f));
        vert.append(Vector4(+s, -s, +s, 0.0f));
        vert.append(Vector4(-s, -s, +s, 0.0f));
        vert.append(Vector4(-s, +s, +s, 0.0f));

        vert.append(Vector4(+s, +s, +s, 0.0f));
        vert.append(Vector4(+s, +s, -s, 0.0f));
        vert.append(Vector4(+s, -s, -s, 0.0f));
        vert.append(Vector4(+s, -s, +s, 0.0f));
    
        vert.append(Vector4(+s, +s, +s, 0.0f));
        vert.append(Vector4(-s, +s, +s, 0.0f));
        vert.append(Vector4(-s, +s, -s, 0.0f));
        vert.append(Vector4(+s, +s, -s, 0.0f));
    
        vert.append(Vector4(+s, -s, -s, 0.0f));
        vert.append(Vector4(-s, -s, -s, 0.0f));
        vert.append(Vector4(-s, -s, +s, 0.0f));
        vert.append(Vector4(+s, -s, +s, 0.0f));

	Array<int> indices;
	for (int i = 0; i < 6; ++i) {
	    int off = i*4;
	    indices.append(0 + off, 1 + off, 2 + off);
	    indices.append(0 + off, 2 + off, 3 + off);
	}

        const shared_ptr<VertexBuffer>& vb = VertexBuffer::create(vert.size() * sizeof(Vector4), VertexBuffer::WRITE_ONCE);
        s_cubeVertices = AttributeArray(vert, vb);
        const shared_ptr<VertexBuffer>& indexVB = VertexBuffer::create(indices.size() * sizeof(int), VertexBuffer::WRITE_ONCE);
        s_cubeIndices = IndexStream(indices, indexVB);
    }

    return shared_ptr<SkyboxSurface>(new SkyboxSurface(c0, c1, alpha));
}


String SkyboxSurface::name() const {
    return m_texture0->name() + " Skybox";
}


void SkyboxSurface::getCoordinateFrame(CoordinateFrame& cframe, bool previous) const {
    cframe = CFrame();
}


void SkyboxSurface::getObjectSpaceBoundingBox(AABox& box, bool previous) const {
    box = AABox::inf();
}


void SkyboxSurface::getObjectSpaceBoundingSphere(Sphere& sphere, bool previous) const {
    sphere.radius = (float)inf();
    sphere.center = Point3::zero();
}


bool SkyboxSurface::castsShadows() const {
    return false;
}


bool SkyboxSurface::canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const {
    // TODO: Check emissive
    return false;
}


void SkyboxSurface::render
(RenderDevice*                          rd, 
 const LightingEnvironment&             environment,
 RenderPassType                         passType, 
 const String&                          singlePassBlendedOutputMacro) const {
    Args args;

    // Restore the old projection matrix
    const Matrix4 oldP = rd->projectionMatrix();

    setTextureArgs(args);
    setShaderGeometryArgs(rd, args);

    LAUNCH_SHADER_WITH_HINT("SkyboxSurface_render.*", args, name());

    rd->setProjectionMatrix(oldP);
}


void SkyboxSurface::setTextureArgs(Args& args) const {
    debugAssert(m_alpha >= 0 && m_alpha <= 1);
    args.setUniform("alpha", m_alpha);

    if (m_alpha < 1) {
        if (m_texture0->dimension() == Texture::DIM_2D) {
            args.setMacro("texture0_2DSpherical", true);
            m_texture0->setShaderArgs(args, "texture0_", Sampler(WrapMode::TILE, WrapMode::CLAMP));
        } else {
            m_texture0->setShaderArgs(args, "texture0_", Sampler::defaults());
        }
    }

    if (m_alpha > 0) {
        if (m_texture1->dimension() == Texture::DIM_2D) {
            args.setMacro("texture1_2DSpherical", true);
            m_texture1->setShaderArgs(args, "texture1_", Sampler(WrapMode::TILE, WrapMode::CLAMP));
        } else {
            m_texture1->setShaderArgs(args, "texture1_", Sampler::defaults());
        }
    }
}
   

void SkyboxSurface::renderIntoGBufferHomogeneous
(RenderDevice*                          rd,
 const Array<shared_ptr<Surface> >&     surfaceArray,
 const shared_ptr<GBuffer>&             gbuffer,
 const CFrame&                          previousCameraFrame,
 const CoordinateFrame&                 expressivePreviousCameraFrame,
 const shared_ptr<Texture>&             depthPeelTexture,
 const float                            minZSeparation,
 const LightingEnvironment&             lightingEnvironment) const {

     if (surfaceArray.length() == 0) {
         return;
     }

    // There's no point in drawing more than one skybox, since they would overlap.
    // Therefore, draw the last one.
    const shared_ptr<SkyboxSurface>& skybox = dynamic_pointer_cast<SkyboxSurface>(surfaceArray.last());
    alwaysAssertM(skybox, "non-SkyboxSurface passed to SkyboxSurface::renderHomogeneous");


    rd->pushState(); {
        Args args;

        const Matrix4& P = rd->projectionMatrix();        

        // There's no reason to write depth from a skybox shader, since
        // the skybox is at infinity
        rd->setDepthWrite(false);
        const Rect2D& colorRect = gbuffer->colorRect();
        rd->setClip2D(colorRect);

        args.setUniform("lowerCoord", colorRect.x0y0());
        args.setUniform("upperCoord", colorRect.x1y1());

        const Vector4 projInfo
            (float(-2.0 / (rd->width() * P[0][0])), 
             float(-2.0 / (rd->height() * P[1][1])),
             float((1.0 - (double)P[0][2]) / P[0][0]), 
             float((1.0 + (double)P[1][2]) / P[1][1]));
        args.setUniform("projInfo", projInfo, true);

        if (gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE].format != NULL) {
            const CFrame& cameraFrame = rd->cameraToWorldMatrix();
            args.setUniform("CurrentToPreviousCameraMatrix", previousCameraFrame.inverse() * cameraFrame);
        }

        if (gbuffer->specification().encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION].format != NULL) {
            const CFrame& cameraFrame = rd->cameraToWorldMatrix();
            args.setUniform("CurrentToExpressivePreviousCameraMatrix", expressivePreviousCameraFrame.inverse() * cameraFrame);
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



        if (gbuffer->specification().encoding[GBuffer::Field::EMISSIVE].format != NULL) {
            setTextureArgs(args);
        }

        // We could just run a 2D full-screen pass here, but we want to take advantage of the depth buffer
        setShaderGeometryArgs(rd, args);
        LAUNCH_SHADER_WITH_HINT("SkyboxSurface_gbuffer.*", args, name());
    
    } rd->popState();
}


void SkyboxSurface::setShaderGeometryArgs(RenderDevice* rd, Args& args) {
    rd->setObjectToWorldMatrix(CFrame());
    // Make a camera with an infinite view frustum to avoid clipping
    Matrix4 P = rd->projectionMatrix();
    Projection projection(P);
    projection.setFarPlaneZ(-finf());
    projection.getProjectUnitMatrix(rd->viewport(), P);
    rd->setProjectionMatrix(P);

    args.setAttributeArray("g3d_Vertex", s_cubeVertices);
    args.setIndexStream(s_cubeIndices);
    args.setPrimitiveType(PrimitiveType::TRIANGLES);
 }


void SkyboxSurface::renderWireframeHomogeneous
(RenderDevice*                rd, 
 const Array<shared_ptr<Surface> >&   surfaceArray, 
 const Color4&                color,
 bool                         previous) const {
     // Intentionally empty
}


void SkyboxSurface::renderDepthOnlyHomogeneous
(RenderDevice*                      rd, 
 const Array<shared_ptr<Surface> >& surfaceArray,
 const shared_ptr<Texture>&         depthPeelTexture,
 const float                        depthPeelEpsilon,
 bool                               requireBinaryAlpha,
 const Color3&                      transmissionWeight) const {
    // Intentionally empty; nothing to do since the skybox is at infinity and
    // would just fail the depth test or set it back to infinity
}

} // namespace G3D
