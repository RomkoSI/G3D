#include "GLG3D/HeightfieldModel.h"
#include "GLG3D/Shader.h"
#include "GLG3D/Texture.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/Projection.h"
#include "GLG3D/LightingEnvironment.h"

namespace G3D {

HeightfieldModel::Tile::Tile(const HeightfieldModel* model, const Point2int32& tileIndex, const CFrame& frame, const CFrame& previousFrame, const shared_ptr<Entity>& entity, 
                             const Surface::ExpressiveLightScatteringProperties& expressiveLightScatteringProperties) :
    Surface(expressiveLightScatteringProperties),
    m_model(model),
    m_entity(entity),
    m_tileIndex(tileIndex),
    m_frame(frame),
    m_previousFrame(previousFrame) {
}


bool HeightfieldModel::Tile::hasTransmission() const {
    return ! m_model->m_material->bsdf()->transmissive().isBlack();
}


void HeightfieldModel::Tile::getCoordinateFrame(CoordinateFrame& cframe, bool previous) const {
    const float metersPerTile = m_model->m_specification.metersPerPixel * m_model->m_specification.pixelsPerTileSide;
    cframe = (previous ? m_previousFrame : m_frame) * CFrame(Point3(m_tileIndex.x * metersPerTile, 0, m_tileIndex.y * metersPerTile));
}


void HeightfieldModel::Tile::getObjectSpaceBoundingBox(AABox &box, bool previous) const {
    const float metersPerTile = m_model->m_specification.metersPerPixel * m_model->m_specification.pixelsPerTileSide;
    box = AABox(Point3(0, 0, 0), Point3(metersPerTile, m_model->m_specification.maxElevation, metersPerTile));
}


void HeightfieldModel::Tile::getObjectSpaceBoundingSphere(Sphere &sphere, bool previous) const {
    AABox box;
    getObjectSpaceBoundingBox(box, previous);
    box.getBounds(sphere);
}


String HeightfieldModel::Tile::name() const {
    return format("%s tile (%d, %d)", m_model->m_name.c_str(), m_tileIndex.x, m_tileIndex.y);
}


static void groupByModels(Array<Array<shared_ptr<Surface> > >& groupArray, const Array< shared_ptr< Surface > >& surfaceArray) {
    Array<HeightfieldModel*> models;
    for(int i = 0; i < surfaceArray.size(); ++i) {
        shared_ptr<HeightfieldModel::Tile> tile = dynamic_pointer_cast<HeightfieldModel::Tile>(surfaceArray[i]);
        alwaysAssertM(notNull(tile), "Passed non-heightfieldmodel tile to HeightfieldModel::Tile::render*");
        HeightfieldModel* m = const_cast<HeightfieldModel*>(tile->modelPtr()); 
        int index = models.findIndex(m);
        if (index < 0) {
            groupArray.next().append(surfaceArray[i]);
            models.next() = m;
        } else {
            groupArray[index].append(surfaceArray[i]);
        }
    }
}


void HeightfieldModel::Tile::renderDepthOnlyHomogeneous
    (RenderDevice*                          rd, 
     const Array< shared_ptr< Surface > >&  surfaceArray, 
     const shared_ptr<Texture>&             previousDepthBuffer,
     const float                            minDepthSeparation,
     bool                                   requireBinaryAlpha,
     const Color3&                          transmissionWeight) const {

    Args args;
    args.setMacro("NUM_LIGHTS", 0);
    args.setMacro("USE_IMAGE_STORE", 0);
    args.setMacro("HAS_VERTEX_COLOR", false);
    bool useDepthPeel = notNull(previousDepthBuffer);
    static shared_ptr<Shader> depthPeelShader = Shader::fromFiles
                (System::findDataFile("HeightfieldModel/HeightfieldModel_Tile_depthPeel.vrt"),
                 System::findDataFile("HeightfieldModel/HeightfieldModel_Tile_depthPeel.pix"));

    static shared_ptr<Shader> depthNonOpaqueShader =
        Shader::fromFiles
        (System::findDataFile("HeightfieldModel/HeightfieldModel_Tile_depthOnlyNonOpaque.vrt"),
         System::findDataFile("HeightfieldModel/HeightfieldModel_Tile_depthOnlyNonOpaque.pix"));
    
    
    Array<Array<shared_ptr<Surface> > > groupedSurfaces;
    groupByModels(groupedSurfaces, surfaceArray);
    BEGIN_PROFILER_EVENT("HeightfieldModel::Tile::renderDepthOnlyHomogeneous");
    for (int i = 0; i < groupedSurfaces.size(); ++i) {
        shared_ptr<HeightfieldModel::Tile> tile = dynamic_pointer_cast<HeightfieldModel::Tile>(groupedSurfaces[i][0]);
        const shared_ptr<Texture>& lambertian = tile->modelPtr()->m_material->bsdf()->lambertian().texture();
        const bool thisSurfaceNeedsAlphaTest = (tile->modelPtr()->m_material->alphaHint() != AlphaHint::ONE) && notNull(lambertian) && !lambertian->opaque();
        const bool thisSurfaceHasTransmissive = tile->modelPtr()->m_material->hasTransmissive();
        shared_ptr<Shader> shader;
        if (thisSurfaceHasTransmissive || (thisSurfaceNeedsAlphaTest && ((tile->modelPtr()->m_material->alphaHint() == AlphaHint::BLEND) || (tile->modelPtr()->m_material->alphaHint() == AlphaHint::BINARY)))) {
            args.setMacro("STOCHASTIC", !requireBinaryAlpha);
            shader = depthNonOpaqueShader;
        } else {
            args.setUniform("color", Color4(Color3::black(), 1.0f));
            if (useDepthPeel) {
                shader = depthPeelShader;
            } else {
                shader = m_model->m_depthAndColorShader;
            }
        }
  
        tile->modelPtr()->m_material->setShaderArgs(args, "material.");
        args.setMacro("HAS_ALPHA", tile->modelPtr()->m_material->hasAlpha());
        args.setMacro("HAS_TRANSMISSIVE", tile->modelPtr()->m_material->hasTransmissive());
        args.setMacro("HAS_EMISSIVE", tile->modelPtr()->m_material->hasEmissive());
        args.setMacro("ALPHA_HINT", tile->modelPtr()->m_material->alphaHint());

        args.setUniform("transmissionWeight", transmissionWeight);
        tile->renderAll(rd, groupedSurfaces[i], args, shader, CFrame(), CFrame(), false, false, false, false, previousDepthBuffer, minDepthSeparation);
    }
    END_PROFILER_EVENT();
}


void HeightfieldModel::Tile::render
    (RenderDevice*                           rd,
    const LightingEnvironment&              environment,
    RenderPassType                          passType, 
    const String&                           singlePassBlendedOutputMacro) const {
    
    renderHomogeneous(rd, Array<shared_ptr<Surface>>(dynamic_pointer_cast<Tile>(const_cast<Tile*>(this)->shared_from_this())), environment, passType, singlePassBlendedOutputMacro);
}


void HeightfieldModel::Tile::renderHomogeneous
    (RenderDevice*                          rd, 
    const Array< shared_ptr< Surface > >&   surfaceArray, 
    const LightingEnvironment&              environment,
    RenderPassType                          passType, 
    const String&                           singlePassBlendedOutputMacro) const {
    Args args;

    // Lighting and material
    environment.setShaderArgs(args);
    args.setMacro("HAS_VERTEX_COLOR", false);

    Array<Array<shared_ptr<Surface> > > groupedSurfaces;
    groupByModels(groupedSurfaces, surfaceArray);
    BEGIN_PROFILER_EVENT("HeightfieldModel::Tile::renderHomogeneous");
    for (int i = 0; i < groupedSurfaces.size(); ++i) {
        const shared_ptr<HeightfieldModel::Tile>& tile = dynamic_pointer_cast<HeightfieldModel::Tile>(groupedSurfaces[i][0]);
        tile->modelPtr()->m_material->setShaderArgs(args, "material.");
        args.setMacro("HAS_ALPHA", tile->modelPtr()->m_material->hasAlpha());
        args.setMacro("ALPHA_HINT", tile->modelPtr()->m_material->alphaHint());
        args.setMacro("HAS_EMISSIVE", tile->modelPtr()->m_material->hasEmissive());
        args.setMacro("HAS_TRANSMISSIVE", tile->modelPtr()->m_material->hasTransmissive());
        tile->renderAll(rd, groupedSurfaces[i], args, tile->modelPtr()->m_shader, CFrame(), CFrame(), false, false);
    }
    END_PROFILER_EVENT();
}


void HeightfieldModel::Tile::renderIntoGBufferHomogeneous
   (RenderDevice*                           rd, 
    const Array< shared_ptr< Surface > >&   surfaceArray, 
    const shared_ptr< GBuffer >&            gbuffer, 
    const CFrame&                           previousCameraFrame, 
    const CFrame&                           expressivePreviousCameraFrame,
    const shared_ptr<Texture>&              depthPeelTexture,
    const float                             minZSeparation,
    const LightingEnvironment&              lightingEnvironment) const {
    BEGIN_PROFILER_EVENT("HeightfieldModel::Tile::renderIntoGBufferHomogeneous");


    const bool bindPreviousMatrix = 
        notNull(gbuffer->specification().encoding[GBuffer::Field::CS_POSITION_CHANGE].format) || 
        notNull(gbuffer->specification().encoding[GBuffer::Field::SS_POSITION_CHANGE].format);

    const bool bindExpressivePreviousMatrix = 
        notNull(gbuffer->specification().encoding[GBuffer::Field::SS_EXPRESSIVE_MOTION].format);

    bool renderPreviousPosition = false;
    bool reverseOrder = false;
    Array<Array<shared_ptr<Surface> > > groupedSurfaces;
    groupByModels(groupedSurfaces, surfaceArray);
    Args args;
    args.setMacro("HAS_VERTEX_COLOR", false);
    args.setMacro("NUM_LIGHTS", 0);
    args.setMacro("USE_IMAGE_STORE", 0);

    const Rect2D& colorRect = gbuffer->colorRect();
    args.setUniform("lowerCoord", colorRect.x0y0());
    args.setUniform("upperCoord", colorRect.x1y1());
    for (int i = 0; i < groupedSurfaces.size(); ++i) {
        const shared_ptr<HeightfieldModel::Tile>& tile = dynamic_pointer_cast<HeightfieldModel::Tile>(groupedSurfaces[i][0]);
        tile->modelPtr()->m_material->setShaderArgs(args, "material.");
        args.setMacro("HAS_EMISSIVE", tile->modelPtr()->m_material->hasEmissive());
        args.setMacro("HAS_TRANSMISSIVE", tile->modelPtr()->m_material->hasTransmissive());
        args.setMacro("ALPHA_HINT", tile->modelPtr()->m_material->alphaHint());
        args.setMacro("HAS_ALPHA", tile->modelPtr()->m_material->hasAlpha());
        tile->renderAll(rd, groupedSurfaces[i], args, tile->modelPtr()->m_gbufferShader, previousCameraFrame, expressivePreviousCameraFrame, bindPreviousMatrix, bindExpressivePreviousMatrix,
                renderPreviousPosition, reverseOrder, depthPeelTexture, minZSeparation);
    }
    END_PROFILER_EVENT();
}


void HeightfieldModel::Tile::renderWireframeHomogeneous(RenderDevice* rd, const Array< shared_ptr< Surface > >& surfaceArray, const Color4& color, bool previous) const {
    Args args;
    const RenderDevice::RenderMode old = rd->renderMode();
    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
    args.setUniform("color", color);
    args.setMacro("HAS_VERTEX_COLOR", false);
    args.setMacro("NUM_LIGHTS", 0); 
    Array<Array<shared_ptr<Surface> > > groupedSurfaces;
    groupByModels(groupedSurfaces, surfaceArray);
    BEGIN_PROFILER_EVENT("HeightfieldModel::Tile::renderWireframeHomogeneous");
    for (int i = 0; i < groupedSurfaces.size(); ++i) {
        const shared_ptr<HeightfieldModel::Tile>& tile = dynamic_pointer_cast<HeightfieldModel::Tile>(groupedSurfaces[i][0]);
        tile->renderAll(rd, groupedSurfaces[i], args, tile->modelPtr()->m_depthAndColorShader, CFrame(), CFrame(), previous, false);
    }
    END_PROFILER_EVENT();
    
    rd->setRenderMode(old);
}


G3D_DECLARE_SYMBOL(previousDepthBuffer);
G3D_DECLARE_SYMBOL(minZSeparation);
G3D_DECLARE_SYMBOL(currentToPreviousScale);
G3D_DECLARE_SYMBOL(clipInfo);
G3D_DECLARE_SYMBOL(USE_DEPTH_PEEL);
static void bindDepthPeelArgs(Args& args, RenderDevice* rd, const shared_ptr<Texture>& depthPeelTexture, const float minZSeparation) {
    const bool useDepthPeel = notNull(depthPeelTexture);
    args.setMacro(SYMBOL_USE_DEPTH_PEEL, useDepthPeel ? 1 : 0);
    if (useDepthPeel) {
        const Vector3& clipInfo = Projection(rd->projectionMatrix(), rd->viewport().wh()).reconstructFromDepthClipInfo();
        args.setUniform(SYMBOL_previousDepthBuffer, depthPeelTexture, Sampler::buffer());
        args.setUniform(SYMBOL_minZSeparation,  minZSeparation);
        args.setUniform(SYMBOL_currentToPreviousScale, Vector2(depthPeelTexture->width()  / rd->viewport().width(),
                                                            depthPeelTexture->height() / rd->viewport().height()));
        args.setUniform(SYMBOL_clipInfo, clipInfo);
    }
}


G3D_DECLARE_SYMBOL(ExpressivePreviousObjectToCameraMatrix);
G3D_DECLARE_SYMBOL(PreviousObjectToCameraMatrix);
G3D_DECLARE_SYMBOL(ProjectToScreenMatrix);
static void maybeBindPreviousMatrices(Args& args, RenderDevice* rd, bool bindPreviousMatrix, bool bindPreviousExpressiveMatrix, const shared_ptr<HeightfieldModel::Tile>& tile, const CFrame& previousCameraFrame, const CFrame& expressivePreviousCameraFrame) {
    if (bindPreviousMatrix) {
        // Previous object-to-camera projection for velocity buffer
        CFrame previousFrame;
        tile->getCoordinateFrame(previousFrame, true);
        const CFrame& previousObjectToCameraMatrix = previousCameraFrame.inverse() * previousFrame;
        args.setUniform(SYMBOL_PreviousObjectToCameraMatrix, previousObjectToCameraMatrix);
    }

    if (bindPreviousExpressiveMatrix) {
        // Previous object-to-camera projection for velocity buffer
        CFrame expressivePreviousFrame;
        tile->getCoordinateFrame(expressivePreviousFrame, true);
        const CFrame& expressivePreviousObjectToCameraMatrix = expressivePreviousCameraFrame.inverse() * expressivePreviousFrame;
        args.setUniform(SYMBOL_ExpressivePreviousObjectToCameraMatrix, expressivePreviousObjectToCameraMatrix);
    }

    if (bindPreviousMatrix || bindPreviousExpressiveMatrix) {                
        // Map (-1, 1) normalized device coordinates to actual pixel positions
        const Matrix4& screenSize =
            Matrix4(rd->width() / 2.0f, 0.0f,                0.0f, rd->width() / 2.0f,
                    0.0f,               rd->height() / 2.0f, 0.0f, rd->height() / 2.0f,
                    0.0f,               0.0f,                1.0f, 0.0f,
                    0.0f,               0.0f,                0.0f, 1.0f);
        args.setUniform(SYMBOL_ProjectToScreenMatrix, screenSize * rd->invertYMatrix() * rd->projectionMatrix());
    }
}


void HeightfieldModel::Tile::renderAll
   (RenderDevice*                           rd, 
    const Array< shared_ptr< Surface > >&   surfaceArray, 
    Args&                                   args, 
    const shared_ptr<Shader>&               shader, 
    const CFrame&                           previousCameraFrame,
    const CFrame&                           expressivePreviousCameraFrame,
    bool                                    bindPreviousMatrix,
    bool                                    bindExpressivePreviousMatrix,
    bool                                    renderPreviousPosition, 
    bool                                    reverseOrder,
    const shared_ptr<Texture>&              previousDepthBuffer,
    const float                             minZSeparation,
    bool                                    renderTransmissiveSurfaces) const {
    m_model->setShaderArgs(args);

    // Issue all tiles
    const int start = reverseOrder ? surfaceArray.size() - 1 : 0;
    const int end   = reverseOrder ? -1 : surfaceArray.size();
    const int delta = reverseOrder ? -1 : +1;
    for (int s = start; s != end; s += delta) {
        const shared_ptr<Tile>& tile = dynamic_pointer_cast<Tile>(surfaceArray[s]);
        if (! renderTransmissiveSurfaces && tile->hasTransmission()) {
            continue;
        }
        CFrame cframe;
        tile->getCoordinateFrame(cframe, renderPreviousPosition);
        rd->setObjectToWorldMatrix(cframe); 

        // Because the current implementation of RenderDevice::apply mutates
        // the args, we must bind a clean copy for each iteration of this loop
        // body.
        Args tileArgs(args);
        tileArgs.setMacro("UNBLENDED_PASS", rd->depthWrite());
        tileArgs.setMacro("HAS_VERTEX_COLOR", false);
        tileArgs.setUniform("tilePixelOffset", tile->m_tileIndex * m_model->m_specification.pixelsPerTileSide);
        maybeBindPreviousMatrices(tileArgs, rd, bindPreviousMatrix, bindExpressivePreviousMatrix, tile, previousCameraFrame, expressivePreviousCameraFrame);
        bindDepthPeelArgs(tileArgs, rd, previousDepthBuffer, minZSeparation);
        
        LAUNCH_SHADER_PTR_WITH_HINT(shader, tileArgs, tile->name());
    }
}

} // G3D
