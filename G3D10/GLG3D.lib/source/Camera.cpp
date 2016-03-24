/**
  \file G3D/source/Camera.cpp

  \author Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2005-07-20
  \edited  2015-06-04

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "GLG3D/Camera.h"
#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Ray.h"
#include "G3D/Matrix4.h"
#include "G3D/Any.h"
#include "G3D/stringutils.h"
#include "G3D/Frustum.h"
#include "GLG3D/Args.h"
#include "GLG3D/GApp.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTabPane.h"

namespace G3D {

shared_ptr<Camera> Camera::create(const String& name) {    
    Any a(Any::TABLE);
    AnyTableReader reader(a);
    return dynamic_pointer_cast<Camera>(create(name, NULL, reader, ModelTable()));
}

    
shared_ptr<Entity> Camera::create
(const String&      name,
 Scene*             scene,
 AnyTableReader&    reader,
 const ModelTable&  modelTable,
 const Scene::LoadOptions& options) {

     (void)modelTable;

     const shared_ptr<Camera> c(new Camera());

     c->Entity::init(name, scene, reader);
     c->Camera::init(reader);
     reader.verifyDone();

     return c;
}


void Camera::init
   (AnyTableReader&    reader) {
    
    reader.getIfPresent("projection",           m_projection);
    reader.getIfPresent("depthOfFieldSettings", m_depthOfFieldSettings);
    reader.getIfPresent("motionBlurSettings",   m_motionBlurSettings);
    reader.getIfPresent("filmSettings",         m_filmSettings);
    reader.getIfPresent("visualizationScale",   m_visualizationScale);
}


Any Camera::toAny(const bool forceAll) const {
    Any any = Entity::toAny(forceAll);
    
    any.setName("Camera");

    any["projection"]           = m_projection;
    any["depthOfFieldSettings"] = m_depthOfFieldSettings;
    any["motionBlurSettings"]   = m_motionBlurSettings;
    any["filmSettings"]         = m_filmSettings;
    any["visualizationScale"]   = m_visualizationScale;
    return any;
}

    
float Camera::circleOfConfusionRadiusPixels(float z, const class Rect2D& viewport) const {
    return m_depthOfFieldSettings.circleOfConfusionRadiusPixels(z, imagePlanePixelsPerMeter(viewport), 
            (m_projection.fieldOfViewDirection() == FOVDirection::HORIZONTAL) ?
                viewport.width() : viewport.height());
}


Camera::Camera() : 
    m_exposureTime(0.0f), m_visualizationScale(1.0f) {

    setNearPlaneZ(-0.15f);
    setFarPlaneZ(-150.0f);
    setFieldOfView((float)toRadians(90.0f), FOVDirection::HORIZONTAL);
    m_lastBoxBounds = Box(Point3(), Point3());
    m_lastSphereBounds = Sphere(Point3::zero(), 0);
}


Camera::Camera(const Matrix4& proj, const CFrame& frame) {
    *this = Camera();
    m_projection = proj;
    setFrame(frame);
    m_exposureTime =  0.0f;
    m_lastBoxBounds = AABox();
    m_lastSphereBounds = Sphere(Point3::zero(), 0);
}


Camera::~Camera() {
}


void Camera::setShaderArgs(UniformTable& args, const Vector2& screenSize, const String& prefix) const {
    args.setUniform(prefix + "invFrame", m_frame.inverse());
    args.setUniform(prefix + "frame", m_frame);
    args.setUniform(prefix + "previousFrame", m_previousFrame);

    Matrix4 P;
    projection().getProjectPixelMatrix(screenSize, P);
    // Invert Y
    args.setUniform(prefix + "projectToPixelMatrix", P * Matrix4::scale(1, -1, 1));

    args.setUniform(prefix + "clipInfo", m_projection.reconstructFromDepthClipInfo());
    args.setUniform(prefix + "projInfo", m_projection.reconstructFromDepthProjInfo(int(screenSize.x), int(screenSize.y)));
    args.setUniform(prefix + "pixelOffset", m_projection.pixelOffset());
}


float Camera::nearPlaneViewportWidth(const Rect2D& viewport) const {
    return m_projection.nearPlaneViewportHeight(viewport);
}


float Camera::nearPlaneViewportHeight(const Rect2D& viewport) const {
    return m_projection.nearPlaneViewportHeight(viewport);
}


float Camera::imagePlanePixelsPerMeter(const class Rect2D& viewport) const {
    return m_projection.imagePlanePixelsPerMeter(viewport);
}


Ray Camera::worldRay(float x, float y, float u, float v, const class Rect2D &viewport) const {
    alwaysAssertM(m_depthOfFieldSettings.model() != DepthOfFieldModel::ARTIST, 
                  "Cannot cast rays under the ARTIST depth of field model.");
    
    if (m_depthOfFieldSettings.model() == DepthOfFieldModel::NONE) {
        // Ignore the lense
        u = 0.0f; 
        v = 0.0f;
    }

    // Pinhole ray
    const Ray& ray = Camera::worldRay(x, y, viewport);

    // Find the point where all rays through this pixel will converge
    // in the scene.
    const Vector3& focusPoint = ray.origin() +
        ray.direction() * (-m_depthOfFieldSettings.focusPlaneZ() / ray.direction().dot(m_frame.lookVector()));

    // Shift the ray origin
    const Vector3& origin = 
        m_frame.rightVector() * (u * m_depthOfFieldSettings.lensRadius()) +
        m_frame.upVector()    * (v * m_depthOfFieldSettings.lensRadius()) +
        ray.origin();

    // Find the new direction to the focus point
    const Vector3& direction = (focusPoint - origin).direction();

    return Ray(origin, direction);
}


Ray Camera::worldRay(float x, float y, const Rect2D& viewport) const {
    return m_frame.toWorldSpace(m_projection.ray(x, y, viewport));
}


void Camera::getProjectPixelMatrix(const Rect2D& viewport, Matrix4& P) const {
    m_projection.getProjectPixelMatrix(viewport, P);
}


void Camera::getProjectUnitMatrix(const Rect2D& viewport, Matrix4& P) const {
    m_projection.getProjectUnitMatrix(viewport, P);
}


Vector3 Camera::projectUnit(const Vector3& point, const Rect2D& viewport) const {
    return m_projection.projectUnit(m_frame.pointToObjectSpace(point), viewport);
}


Vector3 Camera::project(const Vector3& point, const Rect2D&  viewport) const {

    // Find the point in the homogeneous cube
    const Point3& cube = projectUnit(point, viewport);

    return convertFromUnitToNormal(cube, viewport);
}

Vector3 Camera::unprojectUnit(const Vector3& v, const Rect2D& viewport) const {

    const Vector3& projectedPoint = convertFromUnitToNormal(v, viewport);

    return unproject(projectedPoint, viewport);
}


Vector3 Camera::unproject(const Vector3& v, const Rect2D& viewport) const {
    return m_frame.pointToWorldSpace(m_projection.unproject(v, viewport));
}


float Camera::worldToScreenSpaceArea(float area, float z, const Rect2D& viewport) const {
    (void)viewport;
    if (z >= 0) {
        return finf();
    }
    return area * (float)square(-nearPlaneZ() / z);
}


void Camera::getClipPlanes
   (const Rect2D&       viewport,
    Array<Plane>&       clip) const {

    Frustum fr;
    frustum(viewport, fr);
    clip.resize(fr.faceArray.size(), DONT_SHRINK_UNDERLYING_ARRAY);
    for (int f = 0; f < clip.size(); ++f) {
        clip[f] = fr.faceArray[f].plane;
    }
}
 

Frustum Camera::frustum(const Rect2D& viewport) const {
    Frustum f;
    frustum(viewport, f);
    return f;
}


void Camera::frustum(const Rect2D& viewport, Frustum& fr) const {
    m_projection.frustum(viewport, fr);
    fr = m_frame.toWorldSpace(fr);
}

void Camera::getNearViewportCorners
(const Rect2D& viewport,
 Vector3&      outUR,
 Vector3&      outUL,
 Vector3&      outLL,
 Vector3&      outLR) const {
    
    m_projection.getNearViewportCorners(viewport, outUR, outUL, outLL, outLR);

    // Take to world space
    outUR = m_frame.pointToWorldSpace(outUR);
    outUL = m_frame.pointToWorldSpace(outUL);
    outLR = m_frame.pointToWorldSpace(outLR);
    outLL = m_frame.pointToWorldSpace(outLL);
}


void Camera::getFarViewportCorners
   (const Rect2D& viewport,
    Vector3& outUR,
    Vector3& outUL,
    Vector3& outLL,
    Vector3& outLR) const {

    m_projection.getFarViewportCorners(viewport, outUR, outUL, outLL, outLR);

    // Take to world space
    outUR = m_frame.pointToWorldSpace(outUR);
    outUL = m_frame.pointToWorldSpace(outUL);
    outLR = m_frame.pointToWorldSpace(outLR);
    outLL = m_frame.pointToWorldSpace(outLL);
}


void Camera::setPosition(const Vector3& t) { 
    m_frame.translation = t;
}


void Camera::lookAt(const Vector3& position, const Vector3& up) { 
    m_frame.lookAt(position, up);
}


Vector3 Camera::convertFromUnitToNormal(const Vector3& in, const Rect2D& viewport) const{
    return (in + Vector3(1.0f, 1.0f, 1.0f)) * 0.5f * Vector3(viewport.width(), viewport.height(), 1.0f) + 
            Vector3(viewport.x0(), viewport.y0(), 0.0f);
}


void Camera::copyParametersFrom(const shared_ptr<Camera>& camera) {
    const String myName = m_name;
    *this = *camera;

    // Restore my name
    m_name = myName;
    m_lastChangeTime = System::time();
}


float Camera::maxCircleOfConfusionRadiusPixels(const Rect2D& viewport, float viewportFractionMax) {
    switch (depthOfFieldSettings().modelValue()) {
    case DepthOfFieldModel::NONE:
        return 0.0f;
    
    case DepthOfFieldModel::ARTIST:
        {
            const float dimension = float((fieldOfViewDirection() == FOVDirection::HORIZONTAL) ? viewport.width() : viewport.height());
            return max(depthOfFieldSettings().nearBlurRadiusFraction(), depthOfFieldSettings().farBlurRadiusFraction()) * dimension;
        }

    case DepthOfFieldModel::PHYSICAL:
        return min(max(circleOfConfusionRadiusPixels(nearPlaneZ(), viewport),
                   circleOfConfusionRadiusPixels(farPlaneZ(), viewport)), 
                   viewport.width() * viewportFractionMax);
    default:
        alwaysAssertM(false, "Illegal case");
        return 0.0f;
    }
}


void Camera::makeGUI(GuiPane* p, GApp* app) {
    const shared_ptr<GFont>& greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));
    shared_ptr<GFont> defaultFont;

    const int tabCaptionSize = 11;
    GuiTabPane* tabPane = p->addTabPane();
    tabPane->moveBy(-9, 5);

    GuiPane* entityPane = tabPane->addTab(GuiText("Entity", defaultFont, (float)tabCaptionSize));
    Entity::makeGUI(entityPane, app);

    if (isNull(app)) {
        return;
    }

    m_app = app;
    entityPane->beginRow(); {
        entityPane->addButton("this = debugCamera", this, &Camera::onOverwriteCameraFromDebug)->moveBy(-5, 0);
        entityPane->addButton("debugCamera = this", this, &Camera::onOverwriteDebugFromCamera);
    } entityPane->endRow();

    GuiPane* filmPane = tabPane->addTab(GuiText("Film", defaultFont, (float)tabCaptionSize));
    {
        const float sliderWidth = 260, indent = 2.0f;
        filmPane->moveBy(0, 5);
        filmSettings().makeGui(filmPane, 10.0f, sliderWidth, indent);
        filmPane->pack();
        filmPane->setWidth(286 + 10);
    }

#   define INDENT_SLIDER(n) n->setWidth(275); n->moveBy(15, 0); n->setCaptionWidth(100);

    GuiPane* focusPane = tabPane->addTab(GuiText("Depth of Field", defaultFont, (float)tabCaptionSize));
    {
        focusPane->moveBy(0, 5);

        focusPane->addCheckBox
            ("Enabled",
            Pointer<bool>(&depthOfFieldSettings(),
            &DepthOfFieldSettings::enabled,
            &DepthOfFieldSettings::setEnabled));

        focusPane->addRadioButton("None (Pinhole)", DepthOfFieldModel::NONE, &depthOfFieldSettings(), &DepthOfFieldSettings::modelValue, &DepthOfFieldSettings::setModel, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);
        focusPane->addRadioButton("Physical Lens", DepthOfFieldModel::PHYSICAL, &depthOfFieldSettings(), &DepthOfFieldSettings::modelValue, &DepthOfFieldSettings::setModel, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);
        {
            GuiControl* n = focusPane->addNumberBox
                ("Focus Dist.",
                NegativeAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(), &DepthOfFieldSettings::focusPlaneZ, &DepthOfFieldSettings::setFocusPlaneZ)),
                "m", GuiTheme::LOG_SLIDER, 0.01f, 200.0f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox
                ("Lens Radius",
                Pointer<float>(&depthOfFieldSettings(), &DepthOfFieldSettings::lensRadius, &DepthOfFieldSettings::setLensRadius),
                "m", GuiTheme::LOG_SLIDER, 0.0f, 0.5f);
            INDENT_SLIDER(n);
        }

        focusPane->addRadioButton("Artist Custom", DepthOfFieldModel::ARTIST, &depthOfFieldSettings(), &DepthOfFieldSettings::modelValue, &DepthOfFieldSettings::setModel, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);

        {
            GuiControl* n;

            n = focusPane->addNumberBox
                ("Nearfield Blur",
                PercentageAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(),
                &DepthOfFieldSettings::nearBlurRadiusFraction,
                &DepthOfFieldSettings::setNearBlurRadiusFraction)),
                "%", GuiTheme::LINEAR_SLIDER, 0.0f, 4.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox
                ("Near Blur Dist.",
                NegativeAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(),
                &DepthOfFieldSettings::nearBlurryPlaneZ,
                &DepthOfFieldSettings::setNearBlurryPlaneZ)),
                "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox("Near Sharp Dist.",
                NegativeAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(),
                &DepthOfFieldSettings::nearSharpPlaneZ,
                &DepthOfFieldSettings::setNearSharpPlaneZ)),
                "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox("Far Sharp Dist.",
                NegativeAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(),
                &DepthOfFieldSettings::farSharpPlaneZ,
                &DepthOfFieldSettings::setFarSharpPlaneZ)),
                "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox("Far Blur Dist.",
                NegativeAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(),
                &DepthOfFieldSettings::farBlurryPlaneZ,
                &DepthOfFieldSettings::setFarBlurryPlaneZ)),
                "m", GuiTheme::LOG_SLIDER, 0.01f, 400.0f, 0.01f);
            INDENT_SLIDER(n);

            n = focusPane->addNumberBox
                ("Farfield Blur",
                PercentageAdapter<float>::create(Pointer<float>(&depthOfFieldSettings(),
                &DepthOfFieldSettings::farBlurRadiusFraction,
                &DepthOfFieldSettings::setFarBlurRadiusFraction)),
                "%", GuiTheme::LINEAR_SLIDER, 0.0f, 4.0f, 0.01f);
            INDENT_SLIDER(n);
        }
        focusPane->pack();

    }

#   undef INDENT_SLIDER
#   undef INDENT_SLIDER
#   define INDENT_SLIDER(n) n->setWidth(275);  n->setCaptionWidth(100);

    GuiPane* motionPane = tabPane->addTab(GuiText("Motion Blur", defaultFont, (float)tabCaptionSize));
    {
        motionPane->moveBy(0, 5);
        motionPane->addCheckBox
            ("Enabled",
            Pointer<bool>(&motionBlurSettings(),
            &MotionBlurSettings::enabled,
            &MotionBlurSettings::setEnabled));

        GuiControl* n;

        n = motionPane->addNumberBox
            ("Camera Motion",
            PercentageAdapter<float>::create(Pointer<float>(&motionBlurSettings(),
            &MotionBlurSettings::cameraMotionInfluence,
            &MotionBlurSettings::setCameraMotionInfluence)),
            "%", GuiTheme::LOG_SLIDER, 0.0f, 200.0f, 1.0f);
        INDENT_SLIDER(n);

        n = motionPane->addNumberBox
            ("Exposure",
            PercentageAdapter<float>::create(Pointer<float>(&motionBlurSettings(),
            &MotionBlurSettings::exposureFraction,
            &MotionBlurSettings::setExposureFraction)),
            "%", GuiTheme::LOG_SLIDER, 0.0f, 200.0f, 1.0f);
        INDENT_SLIDER(n);

        n = motionPane->addNumberBox
            ("Max Diameter",
            PercentageAdapter<float>::create(Pointer<float>(&motionBlurSettings(),
            &MotionBlurSettings::maxBlurDiameterFraction,
            &MotionBlurSettings::setMaxBlurDiameterFraction)),
            "%", GuiTheme::LOG_SLIDER, 0.0f, 20.0f, 0.01f);
        INDENT_SLIDER(n);

        n = motionPane->addNumberBox
            ("Samples/Pixel",
            Pointer<int>(&motionBlurSettings(),
            &MotionBlurSettings::numSamples,
            &MotionBlurSettings::setNumSamples),
            "", GuiTheme::LOG_SLIDER, 1, 63, 1);
        INDENT_SLIDER(n);
    }
    motionPane->pack(); 
#   undef INDENT_SLIDER

    /////////////////////////////////////////////////////////
    GuiPane* projectionPane = tabPane->addTab(GuiText("Projection", defaultFont, (float)tabCaptionSize));
    projectionPane->moveBy(-3, 2);
    {
        // Near and far planes
        GuiNumberBox<float>* b = projectionPane->addNumberBox("Near Plane Z",
            Pointer<float>(this, &Camera::nearPlaneZ, &Camera::setNearPlaneZ),
            "m", GuiTheme::LOG_SLIDER, -20.0f, -0.001f);
        b->setWidth(290);  b->setCaptionWidth(105);

        GuiRadioButton* radioButton;
        b = projectionPane->addNumberBox("Far Plane Z", Pointer<float>(this, &Camera::farPlaneZ, &Camera::setFarPlaneZ), "m", GuiTheme::LOG_SLIDER, -1000.0f, -0.10f, 0.0f, GuiTheme::NORMAL_TEXT_BOX_STYLE, true, false);
        b->setWidth(290);  b->setCaptionWidth(105);

        // Field of view
        b = projectionPane->addNumberBox("Field of View", Pointer<float>(this, &Camera::fieldOfViewAngleDegrees, &Camera::setFieldOfViewAngleDegrees), GuiText("\xB0", greekFont, 15), GuiTheme::LINEAR_SLIDER, 10.0f, 120.0f, 0.5f);
        b->setWidth(290);  b->setCaptionWidth(105);

        projectionPane->beginRow();
        Pointer<int> directionPtr(this, &Camera::_fieldOfViewDirectionInt, &Camera::_setFieldOfViewDirectionInt);
        GuiRadioButton* radioButton2 = projectionPane->addRadioButton("Horizontal", FOVDirection::HORIZONTAL, directionPtr, GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        radioButton2->moveBy(106, 0);
        radioButton2->setWidth(91);
        radioButton = projectionPane->addRadioButton("Vertical", FOVDirection::VERTICAL, directionPtr, GuiTheme::TOOL_RADIO_BUTTON_STYLE);
        radioButton->setWidth(radioButton2->rect().width());
        projectionPane->endRow();

        projectionPane->pack();
    }

    tabPane->pack();
}


void Camera::onOverwriteCameraFromDebug() {
    copyParametersFrom(m_app->debugCamera());
}


void Camera::onOverwriteDebugFromCamera() {
    m_app->debugCamera()->copyParametersFrom(dynamic_pointer_cast<Camera>(shared_from_this()));
}


} // namespace G3D
