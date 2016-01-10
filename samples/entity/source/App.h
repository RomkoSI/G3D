#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class DemoScene;

class App : public GApp {
protected:

    shared_ptr<DemoScene>   m_scene;    
    
    shared_ptr<Sound>       m_backgroundMusic;

    /** Called from onInit */
    void makeGUI();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());
    virtual void onInit() override;
    virtual void onUserInput(UserInput* ui) override;

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
};

#endif
