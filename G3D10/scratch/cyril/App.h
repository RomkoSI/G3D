/**
  \file App.h

  The G3D 9.00 default starter app is configured for OpenGL 3.0 and
  relatively recent GPUs.
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>

/** Application framework. */
class App : public GApp {
protected:

    bool                m_visualizeVoxelFragments;
    bool                m_visualizeVoxelTree;
	bool                m_visualizeVoxelRaycasting;
    int                 m_visualizeTreeLevel;

    // Note that you can subclass Scene
    shared_ptr<Scene>   m_scene;
    
    shared_ptr<SVO>     m_svo;
    
	shared_ptr<ProfilerResultWindow>     m_profilerResultWindow;

    bool                m_showWireframe;

    /** Called from onInit */
    void makeGUI();

    /** Called from onInit */
    void makeGBuffer();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    /** Invoked by SceneEditorWindow */
    virtual void loadScene(const std::string& sceneName) override;

    /** Save the current scene over the one on disk. */
    virtual void saveScene() override;

    virtual void onInit() override;
    virtual void onAI() override;
    virtual void onNetwork() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;

    // You can override onGraphics if you want more control over the rendering loop.
    // virtual void onGraphics(RenderDevice* rd, Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;
    virtual void onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual bool onEvent(const GEvent& e) override;
    virtual void onUserInput(UserInput* ui) override;
    virtual void onCleanup() override;
    
    /** Sets m_endProgram to true. */
    virtual void endProgram();
};

#endif
