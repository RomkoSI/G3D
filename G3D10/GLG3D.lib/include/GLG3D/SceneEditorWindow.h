#ifndef GLG3D_SceneEditorWindow_h
#define GLG3D_SceneEditorWindow_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/PhysicsFrameSpline.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/SceneVisualizationSettings.h"
#include "GLG3D/PhysicsFrameSplineEditor.h"

namespace G3D {

class ThirdPersonManipulator;
class Entity;
class GApp;
class GuiDropDownList;
class GuiPane;
class GuiButton;
class GuiTabPane;
class GuiCheckBox;
class Scene;

namespace _internal {
    class EntitySelectWidget;
} // _internal

class SceneEditorWindow : public GuiWindow {

protected:
    friend class _internal::EntitySelectWidget;

    shared_ptr<Scene>               m_scene;

    GApp*                           m_app;

    Model::HitInfo                  m_selectionInfo;

    GuiDropDownList*                m_sceneDropDownList;
    GuiDropDownList*                m_cameraDropDownList;

    bool                            m_showAxes;
    bool                            m_showCameras;
    bool                            m_showLightSources;
    bool                            m_showLightBounds;

    SceneVisualizationSettings      m_visualizationSettings;
    
    shared_ptr<Entity>              m_selectedEntity;

    /** For manipulating control points and frames */
    shared_ptr<ThirdPersonManipulator> m_manipulator;

    /** Only one of frameEditor, trackEditor, and splineEditor is visible at a time */
    GuiPane*                        m_frameEditor;
    GuiPane*                        m_trackEditor;
    GuiPane*                        m_splineEditor;
    shared_ptr<PhysicsFrameSplineEditor> m_popupSplineEditor;
    
    /** When editing a spline, this is the surface used for rendering */
    shared_ptr<Surface>             m_splineSurface;

    GuiDropDownList*                m_entityList;

    GuiCheckBox*                    m_sceneLockBox;
    GuiTabPane*                     m_tabPane;

    bool                            m_preventEntitySelect;

    /** If true, the window is big enough to show all controls */
    bool                            m_expanded;
    /** Button to expand and contract additional manual controls. */
    GuiButton*                      m_drawerButton;

    /** The button must be in its own pane so that it can float over
        the expanded pane. */
    GuiPane*                        m_drawerButtonPane;
    GuiText                         m_drawerExpandCaption;
    GuiText                         m_drawerCollapseCaption;

    static const Vector2            s_defaultWindowSize;
    static const Vector2            s_expandedWindowSize;

    /** Last time that the GUI was updated from the Scene */
    RealTime                        m_lastStructuralChangeTime;

    GuiButton*                      m_saveButton;

    bool                            m_advanceSimulation;
    float                           m_simTimeScale;
    GuiDropDownList*                m_rateDropDownList;

    /** Custom controls added by each entity */
    GuiPane*                        m_perEntityPane;
    GuiPane*                        m_aoPane;
    /** Selection information for an entity */
    GuiPane*                        m_entityPane;

    SceneEditorWindow(GApp* app, shared_ptr<Scene> scene, shared_ptr<GuiTheme> theme);

    /** Checks to see if the scene changed. If the scene did change, 
      then this method updates the SceneEditorWindow GUI. */
    void checkForChanges();

    void makeGUI(GApp* app);

    /** Invoked by the GUI */
    void loadSceneCallback();

    /** Invoked by the GUI */
    void saveSceneCallback();

    /** Invoked by the GUI */
    void changeCameraCallback();

    /** Invoked by the GUI */
    void duplicateSelectedEntity();

    void onDrawerButton();

    // AO GUI callbacks
    bool aoEnabled() const;
    void aoSetEnabled(bool b);
    int aoNumSamples() const;
    void aoSetNumSamples(int i);
    float aoRadius() const;
    void aoSetRadius(float f);
    
    float           aoBias() const;
    float           aoIntensity() const;
    float           aoEdgeSharpness() const;
    int             aoBlurStepSize() const;
    int             aoBlurRadius() const;
    bool            aoUseNormalsInBlur() const;
    bool            aoMonotonicallyDecreasingBilateralWeights() const;
    bool            aoUseDepthPeelBuffer() const;
    bool            aoUseNormalBuffer() const;
    float           aoDepthPeelSeparationHint() const;
    bool            aoTemporallyVarySamples() const;
    float           aoTemporalFilterHysteresis() const;
    float           aoTemporalFilterFalloffStart() const;
    float           aoTemporalFilterFalloffEnd() const;

    void            aoSetBias(float f);
    void            aoSetIntensity(float f);
    void            aoSetEdgeSharpness(float f);
    void            aoSetBlurStepSize(int i);
    void            aoSetBlurRadius(int i);
    void            aoSetUseNormalsInBlur(bool b);
    void            aoSetMonotonicallyDecreasingBilateralWeights(bool b);
    void            aoSetUseDepthPeelBuffer(bool b);
    void            aoSetUseNormalBuffer(bool b);
    void            aoSetDepthPeelSeparationHint(float f);
    void            aoSetTemporallyVarySamples(bool b);
    void            aoSetTemporalFilterHysteresis(float f);
    void            aoSetTemporalFilterFalloffStart(float f);
    void            aoSetTemporalFilterFalloffEnd(float f);

    String          selEntityName() const;
    String          selEntityModelName() const;
    String          selEntityMeshName() const;
    String          selEntityMaterialName() const;
    int             selPrimIndex() const;
    float           selU() const;
    float           selV() const;

    //Dont allow explicit setting
    void            selEntitySetName(const String &k) {};
    void            selEntitySetModelName(const String &k) {};
    void            selEntitySetMeshName(const String &k) {};
    void            selEntitySetMaterialName(const String &k) {};
    void            selSetPrimIndex(const int k) {};
    void            selSetU(const float k) {};
    void            selSetV(const float k) {};

    /** Callback */
    void resetTime();

    void onRateChange();

    /** Callback for the GuiFrameBox */
    CFrame selectedEntityFrame() const;

    /** Callback for the GuiFrameBox */
    void setSelectedEntityFrame(const CFrame& f);

    void onRemoveTrack();

    void onConvertToSplineTrack();

    bool editingSpline() const;

	void onEntityDropDownAction();

    /** Invoked when m_selectedEntity changes. */
    void onSelectEntity();
	
public:

    bool preventEntitySelect() const {
        return m_preventEntitySelect;
    }

    void setPreventEntitySelect(bool b) {
        m_preventEntitySelect = b;
    }

    virtual void setRect(const Rect2D& r) override;

    void setManager(WidgetManager* m) override;

    void selectEntity(const shared_ptr<Entity>& e);

    static shared_ptr<SceneEditorWindow> create
        (GApp*                      app,
         shared_ptr<Scene>          scene,
         shared_ptr<GuiTheme>       theme);

    String selectedSceneName() const;

    const SceneVisualizationSettings& sceneVisualizationSettings() const {
        return m_visualizationSettings;
    }

    void setExpanded(bool e);

    virtual void onPose(Array< shared_ptr< Surface > > &surfaceArray, Array< shared_ptr< Surface2D > > &surface2DArray) override;

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;

    virtual bool onEvent(const GEvent &event) override;

    shared_ptr<Entity> selectedEntity() const {
        return m_selectedEntity;
    }

    void setShowLightSources(bool showLightSources) {
        m_showLightSources = showLightSources;
    }
};

} // namespace G3D

#endif
