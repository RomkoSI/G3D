/**
 G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/

#include "GLG3D/SceneEditorWindow.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GApp.h"
#include "GLG3D/Entity.h"
#include "GLG3D/Scene.h"
#include "GLG3D/IconSet.h"
#include "GLG3D/PhysicsFrameSplineEditor.h"
#include "GLG3D/VisualizeLightSurface.h"
#include "GLG3D/VisualizeCameraSurface.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/Texture.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/VisibleEntity.h"
#include "GLG3D/GuiFrameBox.h"
#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/Light.h"
#include "GLG3D/Draw.h"

namespace G3D {

const Vector2 SceneEditorWindow::s_defaultWindowSize(286 + 16, 97);
const Vector2 SceneEditorWindow::s_expandedWindowSize(s_defaultWindowSize + Vector2(0, 390));

namespace _internal {

class SplineSurface : public Surface {
public:
    shared_ptr<PhysicsFrameSplineEditor>   m_editor;
        
    SplineSurface(const shared_ptr<PhysicsFrameSplineEditor>& e) : m_editor(e) {}
        
    virtual void render
    (RenderDevice*                          rd,
     const LightingEnvironment&             environment,
     RenderPassType                         passType, 
     const String&                          singlePassBlendedOutputMacro) const override {
        Draw::physicsFrameSpline(m_editor->spline(), rd);
    }

    virtual bool anyOpaque() const override {
        return true;
    }

    virtual bool requiresBlending() const override {
        return false;
    }

    virtual void renderWireframeHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray, 
     const Color4&                          color,
     bool                                   previous) const override {
        // Intentionally empty
    }

    virtual String name() const override {
        return "PhysicsFrameSplineEditor";
    }
        
    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override {
        // Doesn't implement the renderIntoGBufferHomogeoneous method
        return false;
    }

    virtual void getCoordinateFrame(CoordinateFrame& c, bool previous = false) const override {
        c = CFrame();
    }

    virtual void getObjectSpaceBoundingBox(G3D::AABox& b, bool previous = false) const override {
        const PhysicsFrameSpline& spline = m_editor->spline();
        switch (spline.size()) {
        case 0:
            b = AABox::empty();
            return;

        case 1:
            // Bound the axes display on the first control point
            b = AABox(Vector3::one() * -1.5f, Vector3::one() * 1.5f) + spline.control[0].translation;
            return;

        default:
            b = AABox::inf();
            return;
        }
    }

    virtual void getObjectSpaceBoundingSphere(G3D::Sphere& s, bool previous = false) const override {
        const PhysicsFrameSpline& spline = m_editor->spline();
        switch (spline.size()) {
        case 0:
            s = Sphere(Point3::zero(), 0.0f);
            return;

        case 1:
            // Bound the axes display on the first control point
            s = Sphere(spline.control[0].translation, 1.5f);
            return;

        default:
            s = Sphere(Point3::zero(), finf());
            return;
        }
 
    }

protected:

    virtual void defaultRender(RenderDevice* rd) const override {
        alwaysAssertM(false, "Not implemented");
    }
};

/**
 Widget created by SceneEditorWindow that sits at the bottom of the event
 order so that any click that passes through the rest of the Widget queue
 can be interpreted as entity selection.
 */
class EntitySelectWidget : public Widget {
protected:
    SceneEditorWindow*              m_sceneEditor;
    mutable EventCoordinateMapper   m_mapper;

public:

    EntitySelectWidget(SceneEditorWindow* s) : m_sceneEditor(s) {}

    virtual void render(RenderDevice* rd) const override { 
        m_mapper.update(rd); 
    }


    virtual float positionalEventZ(const Point2& pixel) const override {
        if (m_sceneEditor->m_preventEntitySelect) {
            return fnan();
        }

        const Rect2D& view   = m_sceneEditor->m_app->renderDevice->viewport();

        float distance;

		const Ray& ray = m_sceneEditor->m_scene->eyeRay(m_sceneEditor->m_app->activeCamera(), 
                pixel + Vector2::one() * 0.5f, 
                view, 
                m_sceneEditor->m_app->settings().depthGuardBandThickness);

        m_sceneEditor->m_scene->intersect(ray, distance, m_sceneEditor->m_visualizationSettings.showMarkers);

        return distance / ray.direction().z;
    }


    bool onEvent(const GEvent& event) override {
        if (m_sceneEditor->m_preventEntitySelect) {
            return false;
        }

        // Keep the selector at the back of the z-order so that it doesn't steal events from other objects
        manager()->moveWidgetToBack(dynamic_pointer_cast<Widget>(shared_from_this()));

 
        if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && (event.button.button == 0) && ! event.button.controlKeyIsDown) {
            
            const GApp* app = m_sceneEditor->m_app;
            const shared_ptr<Scene>& scene = m_sceneEditor->m_scene;
            const Rect2D& view = app->renderDevice->viewport();

            const Ray& ray = scene->eyeRay(m_sceneEditor->m_app->activeCamera(), event.mousePosition() + Vector2(0.5f, 0.5f), view, m_sceneEditor->m_app->settings().depthGuardBandThickness);

            // Left click: select by casting a ray through the center of the pixel
            float distance = finf();
            const shared_ptr<Entity>& newSelection = scene->intersect(ray, distance, m_sceneEditor->m_visualizationSettings.showMarkers, Array<shared_ptr<Entity> >(),  m_sceneEditor->m_selectionInfo);                        

            if (newSelection != m_sceneEditor->m_selectedEntity) {
                m_sceneEditor->selectEntity(newSelection);
                return true;
            }
        } 

        return false;
    }
};
}

using namespace _internal;

shared_ptr<SceneEditorWindow> SceneEditorWindow::create
   (GApp*                      app,
    shared_ptr<Scene>          scene,
    shared_ptr<GuiTheme>       theme) {

    shared_ptr<SceneEditorWindow> ptr(new SceneEditorWindow(app, scene, theme));
    shared_ptr<EntitySelectWidget> selector(new EntitySelectWidget(ptr.get()));
    app->addWidget(selector);
    app->addWidget(ptr);

    return ptr;
}


SceneEditorWindow::SceneEditorWindow(GApp* app, shared_ptr<Scene> scene, shared_ptr<GuiTheme> theme) : 
    GuiWindow("Scene Editor", theme, Rect2D::xywh(0,0,100,100), GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
    m_scene(scene),
    m_app(app),
    m_selectionInfo(),
    m_sceneDropDownList(NULL),
    m_cameraDropDownList(NULL),
    m_showAxes(false),
    m_showCameras(false),
    m_showLightSources(false),
    m_showLightBounds(false),
    m_frameEditor(NULL),
    m_trackEditor(NULL),
    m_splineEditor(NULL),
    m_entityList(NULL),
    m_preventEntitySelect(false),
    m_expanded(false),
    m_drawerButton(NULL),
    m_drawerButtonPane(NULL),
    m_lastStructuralChangeTime(0),
    m_saveButton(NULL),
    m_advanceSimulation(true),
    m_simTimeScale(1.0f) {
    makeGUI(app);
}


String SceneEditorWindow::selectedSceneName() const {
    return m_sceneDropDownList->selectedValue().text();
}


void SceneEditorWindow::checkForChanges() {
    const RealTime lastChange = m_scene->lastStructuralChangeTime();
    if (lastChange > m_lastStructuralChangeTime) {
        if (! m_sceneDropDownList->containsValue(m_scene->name())) {
            m_sceneDropDownList->appendValue(m_scene->name());
        }
        m_sceneDropDownList->setSelectedValue(m_scene->name());
        m_lastStructuralChangeTime = lastChange;

        // Update the Entity list in the GUI
        Array<String> nameList;
        m_scene->getEntityNames(nameList);
        nameList.sort();

        if (notNull(m_entityList)) {
            m_entityList->clear();
            m_entityList->append("<none>");
            for (int i = 0; i < nameList.size(); ++i) {
                m_entityList->append(nameList[i]);
            }
        }

        // Preserve entity selection if the list didn't change, select none if the list did change
        if (notNull(m_selectedEntity)) {
            selectEntity(m_scene->entity(m_selectedEntity->name()));
        }

        // Update the Camera list in the GUI
        m_cameraDropDownList->clear();
        m_cameraDropDownList->append(m_app->debugCamera()->name());
        for (int i = 0; i < nameList.size(); ++i) {
            // Is this a camera?
            shared_ptr<Camera> camera = m_scene->typedEntity<Camera>(nameList[i]);
            if (notNull(camera)) {
                m_cameraDropDownList->append(nameList[i]);
            }
        } // For each camera

        // Preserve selection
        if (notNull(m_app->activeCamera()) && m_cameraDropDownList->containsValue(m_app->activeCamera()->name())) {
            m_cameraDropDownList->setSelectedValue(m_app->activeCamera()->name());
        }
    }

    if (notNull(m_selectedEntity)) {

        const shared_ptr<Entity::Track>& track = m_selectedEntity->track();

        if (isNull(track)) {
            m_frameEditor->setVisible(true);
            m_trackEditor->setVisible(false);
            m_splineEditor->setVisible(false);
        } else{
            m_frameEditor->setVisible(false);
            const shared_ptr<Entity::SplineTrack>& splineTrack = dynamic_pointer_cast<Entity::SplineTrack>(track);
            if (notNull(splineTrack)) {
                m_splineEditor->setVisible(true);
                m_trackEditor->setVisible(false);
            } else {
                m_splineEditor->setVisible(false);
                m_trackEditor->setVisible(true);
            }
        }
    }
}


void SceneEditorWindow::setManager(WidgetManager* m) {
    if ((m == NULL) && (manager() != NULL)) {
        // Remove controls from old manager
        manager()->remove(m_manipulator);
        if (notNull(m_popupSplineEditor)) {
            manager()->remove(m_popupSplineEditor);
        }
    }

    GuiWindow::setManager(m);
    
    if (m != NULL) {
        m->add(m_manipulator);
        if (notNull(m_popupSplineEditor)) {
            m->add(m_popupSplineEditor);
        }
    }
}


CFrame SceneEditorWindow::selectedEntityFrame() const {
    return notNull(m_selectedEntity) ? m_selectedEntity->frame() : CFrame();
}


void SceneEditorWindow::setSelectedEntityFrame(const CFrame& f) {
    if (notNull(m_selectedEntity)) {
        m_selectedEntity->setFrame(f);
        m_manipulator->setFrame(f);
    }
}


void SceneEditorWindow::makeGUI(GApp* app) {

    m_manipulator = ThirdPersonManipulator::create();
    m_manipulator->setEnabled(false);

    GuiPane* editorPane = pane();
    shared_ptr<IconSet> iconSet = IconSet::fromFile(System::findDataFile("tango.icn"));
    shared_ptr<GFont> iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));
    
    m_sceneDropDownList = editorPane->addDropDownList("Scene", Scene::sceneNames(), NULL, GuiControl::Callback(this, &SceneEditorWindow::loadSceneCallback));
    m_sceneDropDownList->moveBy(-3, -3);
    m_sceneDropDownList->setWidth(s_defaultWindowSize.x - 57);
    m_sceneDropDownList->setCaptionWidth(44);
    
    GuiButton* b =
        editorPane->addButton
        (GuiText("\x71", iconFont, 16),
         //iconSet->get("16x16/actions/view-refresh.png"),
         this, &SceneEditorWindow::loadSceneCallback, GuiTheme::TOOL_BUTTON_STYLE);

    b->setSize(27, 21);
    b->moveRightOf(m_sceneDropDownList, Vector2(-2, 2));
    m_sceneLockBox = editorPane->addCheckBox
        (iconSet->get("16x16/uwe/Lock.png"), NotAdapter::wrap(Pointer<bool>(m_scene, &Scene::editing, &Scene::setEditing)),
         GuiTheme::TOOL_CHECK_BOX_STYLE);
    m_sceneLockBox->setSize(27, 21);

    editorPane->beginRow(); {
        m_cameraDropDownList = editorPane->addDropDownList("Camera", Array<String>(app->debugCamera()->name()), NULL, GuiControl::Callback(this, &SceneEditorWindow::changeCameraCallback));
        m_cameraDropDownList->moveBy(-3, -1);
        m_cameraDropDownList->setWidth(s_defaultWindowSize.x - 57);
        m_cameraDropDownList->setCaptionWidth(44);
    } editorPane->endRow();


    // Time controls
    editorPane->beginRow(); {
        GuiLabel* label = editorPane->addLabel("Time");
        label->setWidth(44);

        GuiButton* resetButton = editorPane->addButton(iconSet->get("16x16/actions/media-skip-backward.png"), this, &SceneEditorWindow::resetTime, GuiTheme::TOOL_BUTTON_STYLE);
        resetButton->moveRightOf(label, Vector2(0, 2));
        resetButton->setSize(27, 21);

        GuiNumberBox<SimTime>* tbox = editorPane->addNumberBox("", Pointer<SimTime>(m_scene, &Scene::time, &Scene::setTime), "s", GuiTheme::NO_SLIDER, -finf(), finf(), 0.01f);
        tbox->moveBy(0, -2);
        tbox->setWidth(100);

        Array<String> rates("1/20x", "1/10x", "1/4x", "1/2x", "1x");
        rates.append("2x", "3x", "4x", "8x", "10x", "20x");
        m_rateDropDownList = editorPane->addDropDownList("Rate", rates, NULL, GuiControl::Callback(this, &SceneEditorWindow::onRateChange));
        m_rateDropDownList->setSelectedValue("1x");
        m_rateDropDownList->setWidth(95);
        m_rateDropDownList->setCaptionWidth(30);
        m_rateDropDownList->moveBy(3, 0);
        GuiCheckBox* playButton = editorPane->addCheckBox(iconSet->get("16x16/actions/media-playback-start.png"),
            &m_advanceSimulation, GuiTheme::TOOL_CHECK_BOX_STYLE);
        playButton->setSize(27, 21);
        playButton->moveBy(-2, 2);
    } editorPane->endRow();


    GuiTabPane* tabPane = editorPane->addTabPane();
    m_tabPane = tabPane;
    // Move down so that the tabs are below the drawer button
    tabPane->moveBy(-5, 6);

    // Debug visualizations
    {
        GuiPane* viewPane = tabPane->addTab("View");
    
        const float w = 120.0f;
        viewPane->beginRow(); {
            GuiControl* c = viewPane->addCheckBox("Axes", &m_showAxes);
            c->setWidth(w);
            c->moveBy(0, 4);
            viewPane->addCheckBox("Cameras", &m_showCameras)->setWidth(w);
        } viewPane->endRow();
        viewPane->beginRow(); {
            viewPane->addCheckBox("Sphere bounds", &m_visualizationSettings.showEntitySphereBounds)->setWidth(w);
            viewPane->addCheckBox("Light sources", &m_showLightSources)->setWidth(w);
        } viewPane->endRow();
        viewPane->beginRow(); {
            viewPane->addCheckBox("Box bounds", &m_visualizationSettings.showEntityBoxBounds)->setWidth(w);
            viewPane->addCheckBox("Light bounds", &m_showLightBounds)->setWidth(w);
        } viewPane->endRow();
        viewPane->beginRow(); {
            viewPane->addCheckBox("Box bound array", &m_visualizationSettings.showEntityBoxBoundArray)->setWidth(w);
            viewPane->addCheckBox("Markers", &m_visualizationSettings.showMarkers)->setWidth(w);
        } viewPane->endRow();
        viewPane->beginRow(); {
            viewPane->addCheckBox("Wireframe", &m_visualizationSettings.showWireframe)->setWidth(w);
            viewPane->addCheckBox("Entity Names", &m_visualizationSettings.showEntityNames)->setWidth(w);
        } viewPane->endRow();
        viewPane->pack();
    }

    const float tabPaneHeight = 330;
    // The entity editor
    {
        m_entityPane = tabPane->addTab("Entity");

        GuiScrollPane* scrollPane = m_entityPane->addScrollPane(true, false, GuiTheme::BORDERLESS_SCROLL_PANE_STYLE);
        scrollPane->moveBy(-7, -6);
        scrollPane->setSize(s_defaultWindowSize.x - 1, tabPaneHeight);

        GuiPane* p = scrollPane->viewPane();
        p->setSize(s_defaultWindowSize.x - 12, 400);

		m_entityList = p->addDropDownList("Name", Array<String>(), NULL, GuiControl::Callback(this, &SceneEditorWindow::onEntityDropDownAction));
        m_entityList->setCaptionWidth(41);
        m_entityList->setWidth(265);
        m_entityList->moveBy(0, 5);
        p->addButton("Duplicate", this, &SceneEditorWindow::duplicateSelectedEntity)->moveBy(-2, -3);

        m_frameEditor = p->addPane("CoordinateFrame");
        GuiFrameBox* b = m_frameEditor->addFrameBox(Pointer<CFrame>(this, &SceneEditorWindow::selectedEntityFrame, &SceneEditorWindow::setSelectedEntityFrame));
        b->setWidth(s_defaultWindowSize.x - 30);
        b->moveBy(-4, 0);
        m_frameEditor->addButton("Convert to SplineTrack", this, &SceneEditorWindow::onConvertToSplineTrack)->moveBy(55, 10);
        m_frameEditor->pack();

        // Put the track editor in the same place as the frame editor.  Only one is visible at a time.
        m_trackEditor = p->addPane("Track");
        m_trackEditor->setPosition(m_frameEditor->rect().x0y0());
        m_trackEditor->addLabel("This Entity has a Track program that cannot be edited visually. Remove it to create a SplineTrack or directly manipulate the coordinate frame.")->setSize(250, 70);
        m_trackEditor->addButton("Remove Track Program", this, &SceneEditorWindow::onRemoveTrack)->moveBy(50, 20);
        m_trackEditor->pack();
        m_trackEditor->setVisible(false);

        m_splineEditor = p->addPane("SplineTrack");
        m_splineEditor->setPosition(m_frameEditor->rect().x0y0());
        m_splineEditor->addButton("Remove SplineTrack", this, &SceneEditorWindow::onRemoveTrack)->moveBy(60, 20);
        m_splineEditor->setVisible(false);
        m_popupSplineEditor = PhysicsFrameSplineEditor::create("Spline Editor", NULL);
        m_splineSurface = shared_ptr<Surface>(new SplineSurface(m_popupSplineEditor));
        m_popupSplineEditor->setEnabled(false);
        m_splineEditor->pack();
        m_popupSplineEditor->setVisible(false);

        m_perEntityPane = p->addPane();
        m_perEntityPane->setSize(s_defaultWindowSize.x - 12, 200);
        m_perEntityPane->moveBy(-6, 0);
        
        m_entityPane->pack();
    }

    // AO editor
    {
        const float indent = 10.0f;
        GuiPane* aoTabPane = tabPane->addTab("AO");
        GuiScrollPane* aoScrollPane = aoTabPane->addScrollPane(true, false, GuiTheme::BORDERLESS_SCROLL_PANE_STYLE);
        const float w = 260.0f;
        aoScrollPane->moveBy(-7, -6);
        aoScrollPane->setSize(s_defaultWindowSize.x - 1, tabPaneHeight);
        m_aoPane = aoScrollPane->viewPane();

        m_aoPane->addCheckBox("Enabled",              Pointer<bool>(this, &SceneEditorWindow::aoEnabled, &SceneEditorWindow::aoSetEnabled));
        
        m_aoPane->setNewChildSize(w, -1.0f);
        m_aoPane->addNumberBox("Samples",             Pointer<int>(this, &SceneEditorWindow::aoNumSamples, &SceneEditorWindow::aoSetNumSamples), "", GuiTheme::LINEAR_SLIDER, 3, 99);
        m_aoPane->addNumberBox("Radius",              Pointer<float>(this, &SceneEditorWindow::aoRadius, &SceneEditorWindow::aoSetRadius), "m", GuiTheme::LOG_SLIDER, 0.05f, 30.0f);
        m_aoPane->addNumberBox("Bias",                Pointer<float>(this, &SceneEditorWindow::aoBias, &SceneEditorWindow::aoSetBias), "m", GuiTheme::LINEAR_SLIDER, 0.0f, 0.1f);
        m_aoPane->addNumberBox("Intensity",           Pointer<float>(this, &SceneEditorWindow::aoIntensity, &SceneEditorWindow::aoSetIntensity), "", GuiTheme::LINEAR_SLIDER, 0.0f, 10.0f);
        m_aoPane->addNumberBox("Crispness",           Pointer<float>(this, &SceneEditorWindow::aoEdgeSharpness, &SceneEditorWindow::aoSetEdgeSharpness), "", GuiTheme::LINEAR_SLIDER, 0.0f, 3.0f);
        m_aoPane->addNumberBox("Blur Radius",         Pointer<int>(this, &SceneEditorWindow::aoBlurRadius, &SceneEditorWindow::aoSetBlurRadius), "", GuiTheme::LINEAR_SLIDER, 0, 6);
        m_aoPane->addNumberBox("Blur Stride",         Pointer<int>(this, &SceneEditorWindow::aoBlurStepSize, &SceneEditorWindow::aoSetBlurStepSize), "", GuiTheme::LINEAR_SLIDER, 1, 4);
        m_aoPane->addCheckBox("Use Depth Peel Buffer", Pointer<bool>(this, &SceneEditorWindow::aoUseDepthPeelBuffer, &SceneEditorWindow::aoSetUseDepthPeelBuffer));

        GuiNumberBox<float>* separationBox = m_aoPane->addNumberBox("Separation", Pointer<float>(this, &SceneEditorWindow::aoDepthPeelSeparationHint, &SceneEditorWindow::aoSetDepthPeelSeparationHint), "m", GuiTheme::LINEAR_SLIDER, 0.0f, 2.0f);
        separationBox->moveBy(indent, 0.0f);
        separationBox->setWidth(w - indent);
        separationBox->setCaptionWidth(70.0f);
        m_aoPane->addCheckBox("Use Normal Buffer", Pointer<bool>(this, &SceneEditorWindow::aoUseNormalBuffer, &SceneEditorWindow::aoSetUseNormalBuffer));
        m_aoPane->addCheckBox("In Blur",  Pointer<bool>(this, &SceneEditorWindow::aoUseNormalsInBlur, &SceneEditorWindow::aoSetUseNormalsInBlur))->moveBy(indent,0.0f);
        m_aoPane->addCheckBox("Monotonically Decreasing Bilateral Weights",  Pointer<bool>(this, &SceneEditorWindow::aoMonotonicallyDecreasingBilateralWeights, &SceneEditorWindow::aoSetMonotonicallyDecreasingBilateralWeights));
        m_aoPane->addCheckBox("High Quality Blur", Pointer<bool>(
            [this]() { return m_scene->lightingEnvironment().ambientOcclusionSettings.highQualityBlur;  },
            [this](bool b) {  m_scene->lightingEnvironment().ambientOcclusionSettings.highQualityBlur = b; }));
        m_aoPane->addCheckBox("Pack Blur Keys", Pointer<bool>(
            [this]() { return m_scene->lightingEnvironment().ambientOcclusionSettings.packBlurKeys;  },
            [this](bool b) {  m_scene->lightingEnvironment().ambientOcclusionSettings.packBlurKeys = b; }));
        m_aoPane->beginRow(); {
            const Pointer<int> zStoragePointer(
                [this]() { return m_scene->lightingEnvironment().ambientOcclusionSettings.zStorage;  },
                [this](int i) {  m_scene->lightingEnvironment().ambientOcclusionSettings.zStorage = (AmbientOcclusionSettings::ZStorage)i; });
            m_aoPane->addLabel("Z Precision")->setWidth(120.0f);
            m_aoPane->setNewChildSize(100.0f, -1);
            m_aoPane->addRadioButton("Half", (int)AmbientOcclusionSettings::ZStorage::HALF, zStoragePointer, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);
            m_aoPane->addRadioButton("Float", (int)AmbientOcclusionSettings::ZStorage::FLOAT, zStoragePointer, GuiTheme::NORMAL_RADIO_BUTTON_STYLE);
        } m_aoPane->endRow();


        m_aoPane->addCheckBox("Temporally Vary Samples", Pointer<bool>(this, &SceneEditorWindow::aoTemporallyVarySamples, &SceneEditorWindow::aoSetTemporallyVarySamples));
        m_aoPane->addLabel("Temporal Filter Settings");
        m_aoPane->setNewChildSize(w, -1);
        m_aoPane->addNumberBox("Hysteresis",      Pointer<float>(this, &SceneEditorWindow::aoTemporalFilterHysteresis, &SceneEditorWindow::aoSetTemporalFilterHysteresis), "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f)->moveBy(indent, 0);
        m_aoPane->addNumberBox("Falloff Start",   Pointer<float>(this, &SceneEditorWindow::aoTemporalFilterFalloffStart, &SceneEditorWindow::aoSetTemporalFilterFalloffStart), "m", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f)->moveBy(indent, 0);
        m_aoPane->addNumberBox("Falloff End",     Pointer<float>(this, &SceneEditorWindow::aoTemporalFilterFalloffEnd, &SceneEditorWindow::aoSetTemporalFilterFalloffEnd), "m", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f)->moveBy(indent, 0);
        m_aoPane->pack();
    }

    // Entity Seletion Info
    {
        GuiPane* selectedEntityPane = tabPane->addTab("Info");
        const float w = 275.0;

        GuiControl* b = NULL;
        
        b = selectedEntityPane->addTextBox("Entity", Pointer<String>(this, &SceneEditorWindow::selEntityName, &SceneEditorWindow::selEntitySetName)); b->setCaptionWidth(65); b->setWidth(w); b->setEnabled(false);
        b->moveBy(0, 5);
        b = selectedEntityPane->addTextBox("Model", Pointer<String>(this, &SceneEditorWindow::selEntityModelName, &SceneEditorWindow::selEntitySetModelName)); b->setCaptionWidth(65);b->setWidth(w); b->setEnabled(false);
        b = selectedEntityPane->addTextBox("Mesh", Pointer<String>(this, &SceneEditorWindow::selEntityMeshName, &SceneEditorWindow::selEntitySetMeshName)); b->setCaptionWidth(65);b->setWidth(w); b->setEnabled(true);
        b = selectedEntityPane->addTextBox("Material", Pointer<String>(this, &SceneEditorWindow::selEntityMaterialName, &SceneEditorWindow::selEntitySetMaterialName)); b->setCaptionWidth(65);b->setWidth(w); b->setEnabled(false);        
        b = selectedEntityPane->addNumberBox("Primitive", Pointer<int>(this, &SceneEditorWindow::selPrimIndex, &SceneEditorWindow::selSetPrimIndex)); b->setCaptionWidth(65); b->setWidth(w + 20); b->setEnabled(false);
        selectedEntityPane->beginRow(); {
          b = selectedEntityPane->addNumberBox("Barycentric", Pointer<float>(this, &SceneEditorWindow::selU, &SceneEditorWindow::selSetU)); b->setCaptionWidth(65); b->setWidth(w - 90); b->setEnabled(false);
          b = selectedEntityPane->addNumberBox("", Pointer<float>(this, &SceneEditorWindow::selV, &SceneEditorWindow::selSetV)); b->setWidth(130); b->setEnabled(false);
          b->moveBy(-20, 0);
        } selectedEntityPane->endRow();
    }
    tabPane->pack();
    tabPane->setWidth(s_defaultWindowSize.x);

    m_saveButton = editorPane->addButton
    (iconSet->get("16x16/devices/media-floppy.png"),
        this, &SceneEditorWindow::saveSceneCallback, GuiTheme::TOOL_BUTTON_STYLE);
    m_saveButton->setPosition(10, tabPane->rect().y1() + 10);

    // Have to create the m_drawerButton last, otherwise the setRect
    // code for moving it to the bottom of the window will cause
    // layout to become broken.
    m_drawerCollapseCaption = GuiText("5", iconFont);
    m_drawerExpandCaption = GuiText("6", iconFont);
    m_drawerButtonPane = editorPane->addPane("", GuiTheme::NO_PANE_STYLE);
    m_drawerButton = 
        m_drawerButtonPane->addButton
        (m_drawerExpandCaption, 
         GuiControl::Callback(this, &SceneEditorWindow::onDrawerButton), 
         GuiTheme::TOOL_BUTTON_STYLE);
    m_drawerButton->setRect(Rect2D::xywh(0, 0, 12, 10));
    m_drawerButtonPane->setSize(12, 10);

    // Select the Entity pane by default
    tabPane->setSelectedIndex(1);

    setRect(Rect2D::xywh(rect().x0y0(), m_expanded ? s_expandedWindowSize : s_defaultWindowSize));
    pane()->setSize(s_expandedWindowSize);
}


void SceneEditorWindow::onRemoveTrack() {
    if (notNull(m_selectedEntity)) {
        m_selectedEntity->setTrack(shared_ptr<Entity::Track>());
        m_manipulator->setFrame(m_selectedEntity->frame());
    }
}


void SceneEditorWindow::onEntityDropDownAction() {
	selectEntity(m_scene->entity(m_entityList->selectedValue().text()));
}


void SceneEditorWindow::onConvertToSplineTrack() {
    if (notNull(m_selectedEntity)) {
        PhysicsFrameSpline spline;
        spline.append(m_selectedEntity->frame());
        m_selectedEntity->setFrameSpline(spline);
        m_popupSplineEditor->setSpline(spline);
    }
}


void SceneEditorWindow::onRateChange() {
    const String& s = m_rateDropDownList->selectedValue().text();

    TextInput ti(TextInput::FROM_STRING, s);

    m_simTimeScale = (float)ti.readNumber();
    if (ti.readSymbol() == "/") {
        // There is a denominator as well
        m_simTimeScale /= (float)ti.readNumber();
    }

    debugAssert(m_simTimeScale > 0);
    if (m_advanceSimulation) {
        m_app->setSimulationTimeScale(m_simTimeScale);
    }
}


void SceneEditorWindow::resetTime() {
    m_scene->setTime(0);
}


String SceneEditorWindow::selEntityName() const {
    if (notNull(m_selectionInfo.entity)) {
        return m_selectionInfo.entity->name();
    } else {
        return "N/A";
    }
}


String SceneEditorWindow::selEntityModelName() const {
    if (notNull(m_selectionInfo.model)) {
        return m_selectionInfo.model->name();
    } 
    return "N/A";
}


String SceneEditorWindow::selEntityMeshName() const {
    return m_selectionInfo.meshName;
}


String SceneEditorWindow::selEntityMaterialName() const {
    if(notNull(m_selectionInfo.material)) {
        return m_selectionInfo.material->name();
    }
    return "N/A";
}


int SceneEditorWindow::selPrimIndex() const {
    return m_selectionInfo.primitiveIndex;
}


float SceneEditorWindow::selU() const {
    return m_selectionInfo.u;
}


float SceneEditorWindow::selV() const {
    return m_selectionInfo.v;
}


bool SceneEditorWindow::aoEnabled() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.enabled;
}


void SceneEditorWindow::aoSetEnabled(bool b) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.enabled = b;
}


int SceneEditorWindow::aoNumSamples() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.numSamples;
}


void SceneEditorWindow::aoSetNumSamples(int i) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.numSamples = i;
}


float SceneEditorWindow::aoRadius() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.radius;
}


void SceneEditorWindow::aoSetRadius(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.radius = f;
}


float SceneEditorWindow::aoBias() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.bias;
}


void SceneEditorWindow::aoSetBias(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.bias = f;
}


float SceneEditorWindow::aoIntensity() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.intensity;
}


void SceneEditorWindow::aoSetIntensity(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.intensity = f;
}


float SceneEditorWindow::aoEdgeSharpness() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.edgeSharpness;
}


void SceneEditorWindow::aoSetEdgeSharpness(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.edgeSharpness = f;
}


int SceneEditorWindow::aoBlurStepSize() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.blurStepSize;
}


void SceneEditorWindow::aoSetBlurStepSize(int i) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.blurStepSize = i;
}


int SceneEditorWindow::aoBlurRadius() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.blurRadius;
}


void SceneEditorWindow::aoSetBlurRadius(int i) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.blurRadius = i;
}


bool SceneEditorWindow::aoUseNormalsInBlur() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.useNormalsInBlur;
}


void SceneEditorWindow::aoSetUseNormalsInBlur(bool b) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.useNormalsInBlur = b;
}


bool SceneEditorWindow::aoMonotonicallyDecreasingBilateralWeights() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.monotonicallyDecreasingBilateralWeights;
}


void SceneEditorWindow::aoSetMonotonicallyDecreasingBilateralWeights(bool b) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.monotonicallyDecreasingBilateralWeights = b;
}


bool SceneEditorWindow::aoUseNormalBuffer() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.useNormalBuffer;
}


void SceneEditorWindow::aoSetUseNormalBuffer(bool b) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.useNormalBuffer = b;
}


void SceneEditorWindow::aoSetUseDepthPeelBuffer(bool b) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.useDepthPeelBuffer = b;
}


bool SceneEditorWindow::aoUseDepthPeelBuffer() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.useDepthPeelBuffer;
}


void SceneEditorWindow::aoSetDepthPeelSeparationHint(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.depthPeelSeparationHint = f;
}


float SceneEditorWindow::aoDepthPeelSeparationHint() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.depthPeelSeparationHint;
}


bool SceneEditorWindow::aoTemporallyVarySamples() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.temporallyVarySamples;
}


void SceneEditorWindow::aoSetTemporallyVarySamples(bool b) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.temporallyVarySamples = b;
}


float SceneEditorWindow::aoTemporalFilterHysteresis() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.temporalFilterSettings.hysteresis;
}


void SceneEditorWindow::aoSetTemporalFilterHysteresis(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.temporalFilterSettings.hysteresis = f;
}


float SceneEditorWindow::aoTemporalFilterFalloffStart() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.temporalFilterSettings.falloffStartDistance;
}


void SceneEditorWindow::aoSetTemporalFilterFalloffStart(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.temporalFilterSettings.falloffStartDistance = f;
}


float SceneEditorWindow::aoTemporalFilterFalloffEnd() const {
    return m_scene->lightingEnvironment().ambientOcclusionSettings.temporalFilterSettings.falloffEndDistance;
}

void SceneEditorWindow::aoSetTemporalFilterFalloffEnd(float f) {
    m_scene->lightingEnvironment().ambientOcclusionSettings.temporalFilterSettings.falloffEndDistance = f;
}


void SceneEditorWindow::duplicateSelectedEntity() {
    if (isNull(m_selectedEntity)) {
        return;
    }

    // Clone the Entity by converting to Any and back.  This avoids
    // design challenges with cloning arbitrary objects.  It is slow
    // to do it this way, but this only occurs when the GUI is
    // activated, so "slow" doesn't really matter.
    
    const Any& any = Any::parse(m_selectedEntity->toAny().unparse());
    String newName = m_selectedEntity->name() + "_0";
    
    Array<String> entityNames;
    m_scene->getEntityNames(entityNames);
    // creates a unique name in order to avoid conflicts from multiple models being dragged in
    int nameModifier = 0;
    if (entityNames.contains(newName)) { 
        while (entityNames.contains(format("%s%d", newName.c_str(), ++nameModifier)));
        newName = newName.append(format("%d", nameModifier));
    }

    m_scene->createEntity(any.name(), newName, any);
    selectEntity(m_scene->entity(newName));
}


void SceneEditorWindow::onPose(Array< shared_ptr< Surface > > &surfaceArray, Array< shared_ptr< Surface2D > > &surface2DArray) {
    GuiWindow::onPose(surfaceArray, surface2DArray);

    if (m_showCameras) {
        Array<String> name;
        m_scene->getCameraNames(name);
        for (int i = 0; i < name.length(); ++i) {
            surfaceArray.append(VisualizeCameraSurface::create(m_scene->typedEntity<Camera>(name[i])));
        }
    }

    if (m_showLightSources || m_showLightBounds) {
        // Extract lights
        Array< shared_ptr<Light> > lightArray;
        m_scene->getTypedEntityArray(lightArray);

        if (m_showLightSources) {
            for (int i = 0; i < lightArray.size(); ++i) {
                surfaceArray.append(VisualizeLightSurface::create(lightArray[i], false));
            }
        }

        if (m_showLightBounds) {
            for (int i = 0; i < lightArray.size(); ++i) {
                surfaceArray.append(VisualizeLightSurface::create(lightArray[i], true));
            }
        }
    }

    // Show the selected entity
    if (notNull(m_selectedEntity)) {
        Box b;
        m_selectedEntity->getLastBounds(b);
        debugDraw(shared_ptr<Shape>(new BoxShape(b)), 0, Color4::clear(), Color3::yellow());
    }

    if (m_showAxes) {
        debugDraw(shared_ptr<Shape>(new AxesShape(CFrame())));
    }

    // Show the spline editor if appropriate
    if (editingSpline()) {
        surfaceArray.append(m_splineSurface);
    }
}


void SceneEditorWindow::changeCameraCallback() {
    const String& cameraName = m_cameraDropDownList->selectedValue().text();

    if (cameraName == m_app->debugCamera()->name()) {
        m_app->setActiveCamera(m_app->debugCamera());
    } else {
        shared_ptr<Camera> camera = m_scene->typedEntity<Camera>(cameraName);
        if (notNull(camera)) {
            m_app->setActiveCamera(camera);
        } else {
            // This camera does not exist!
        }
    }
}


void SceneEditorWindow::loadSceneCallback() {
	// Refresh the scene list
	m_sceneDropDownList->setList(Scene::sceneNames());

	// Load the selected scene
    m_app->loadScene(selectedSceneName());
}


void SceneEditorWindow::saveSceneCallback() {
    // We can't directly bind the GUI to GApp::saveScene because that would be a pointer to the base class (GApp) method
    // and not the subclass (App) method.
    m_app->saveScene();
}


void SceneEditorWindow::selectEntity(const shared_ptr<Entity>& e) {
    if (m_selectedEntity != e) {
        m_selectedEntity = e;
        onSelectEntity();
    }
}


void SceneEditorWindow::onSelectEntity() {
    m_perEntityPane->removeAllChildren();
    if (notNull(m_selectedEntity) && ! m_preventEntitySelect) {
        // Populate the GUI
        m_selectedEntity->makeGUI(m_perEntityPane, m_app);

        m_entityList->setSelectedValue(m_selectedEntity->name());
        // Copy the current track or frame
        const shared_ptr<Entity::SplineTrack>& splineTrack = dynamic_pointer_cast<Entity::SplineTrack>(m_selectedEntity->track());
        if (notNull(splineTrack)) {
            // Track
            m_popupSplineEditor->setSpline(splineTrack->spline());
        } else {
            // Frame
            m_manipulator->setFrame(m_selectedEntity->frame());
        }
    }
}


void SceneEditorWindow::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    // These values used to be separate, but are today tied together to simplify
    // the user interface
    m_preventEntitySelect = ! m_scene->editing();
    m_aoPane->setEnabled(m_scene->editing());
    m_entityPane->setEnabled(m_scene->editing());

    checkForChanges();
    
    if (notNull(m_selectedEntity) && m_scene->editing()) {
        if (isNull(dynamic_pointer_cast<Entity::SplineTrack>(m_selectedEntity->track()))) {
            m_manipulator->setEnabled(true);
            const CFrame& newFrame = m_manipulator->frame();
            if (newFrame != m_selectedEntity->frame()) {
                // Move the Entity to the new frame position
                m_selectedEntity->setFrame(newFrame);

                // Wipe out its velocity to avoid motion blur if the scene is paused
                m_selectedEntity->onSimulation(m_scene->time(), fnan());
                m_selectedEntity->onSimulation(m_scene->time(), fnan());
            }
        } else {
            m_manipulator->setEnabled(false);
        }
    } else {
        m_manipulator->setEnabled(false);
    }

    m_saveButton->setEnabled(m_scene->editing());

    if (m_advanceSimulation) {
        m_app->setSimulationTimeScale(m_simTimeScale);
    } else {
        m_app->setSimulationTimeScale(0.0f);
    }

    GuiWindow::onSimulation(rdt, sdt, idt);

    if (notNull(m_popupSplineEditor)) {
        
        m_popupSplineEditor->setEnabled(editingSpline());
        m_popupSplineEditor->setVisible(m_popupSplineEditor->enabled());
        
        // Add physical simulation here.  You can make your time
        // advancement based on any of the three arguments.
        if (m_popupSplineEditor->enabled()) {
            // Apply the edited spline.  Do this before object simulation, so that the object
            // is in sync with the widget for manipulating it.
            m_selectedEntity->setFrameSpline(m_popupSplineEditor->spline());
            // Wipe out its velocity to avoid motion blur if the scene is paused
            m_selectedEntity->onSimulation(m_scene->time(), fnan());
            m_selectedEntity->onSimulation(m_scene->time(), fnan());
        }
    }
}


bool SceneEditorWindow::editingSpline() const {
    return 
        notNull(m_selectedEntity) && 
        m_scene->editing() && 
        notNull(dynamic_pointer_cast<Entity::SplineTrack>(m_selectedEntity->track()));
}


bool SceneEditorWindow::onEvent(const GEvent& event) {
    if (GuiWindow::onEvent(event)) {
        return true;
    }

    if (m_scene->editing()) {
        if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_entityList)) {
            // User clicked on dropdown list
            selectEntity(m_scene->entity(m_entityList->selectedValue().text()));
            return true;
        }
    }

    if ((event.type == GEventType::GUI_ACTION) && (event.gui.control == m_sceneLockBox)) {
        // Just enabled editing and the window is not expanded...expand it
        if (m_scene->editing() && ! m_expanded) {
            // Select the Entity tab
            m_tabPane->setSelectedIndex(1);
            setExpanded(true);
        }
    }

    // Accelerator key for toggling camera control.  Active even when the SceneEditorWindow is hidden.
    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F2)) {
        static const String DEBUG_CAMERA_NAME = "(Debug Camera)";

        static String lastCameraName = DEBUG_CAMERA_NAME;
        if (m_cameraDropDownList->selectedValue().text() == DEBUG_CAMERA_NAME) {
            // Select some other camera
            m_cameraDropDownList->setSelectedValue(lastCameraName);
            changeCameraCallback();
        } else {
            // Remember the old camera
            lastCameraName = m_cameraDropDownList->selectedValue().text();

            // Select the debug camera
            // Move the debug camera
            const CFrame& frame = m_scene->entity(lastCameraName)->frame();
            m_app->m_debugCamera->setFrame(frame);
            m_app->m_cameraManipulator->setFrame(frame);

            m_cameraDropDownList->setSelectedValue(DEBUG_CAMERA_NAME);
            changeCameraCallback();
        }
        return true;
    }

    if ((event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == GKey::F9)) { // Toggle scene play/pause
        m_advanceSimulation = !m_advanceSimulation;
        return true;
    }

    return false;
}


void SceneEditorWindow::setExpanded(bool e) {
    if (m_expanded != e) {
        m_expanded = e;
        morphTo(Rect2D::xywh(rect().x0y0(), m_expanded ? s_expandedWindowSize : s_defaultWindowSize));
        m_drawerButton->setCaption(m_expanded ? m_drawerCollapseCaption : m_drawerExpandCaption);
    }
}


void SceneEditorWindow::onDrawerButton() {
    setExpanded(! m_expanded);
}


void SceneEditorWindow::setRect(const Rect2D& r) {
    GuiWindow::setRect(r);
    if (m_drawerButtonPane) {
        const Rect2D& r = clientRect();
        m_drawerButtonPane->setPosition((r.width() - m_drawerButtonPane->rect().width()) / 2.0f, 
                                        r.height() - m_drawerButtonPane->rect().height());
    }
}

} // namespace G3D
