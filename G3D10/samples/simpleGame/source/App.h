#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

class PhysicsScene;

class App : public GApp {
protected:

    bool                        m_firstPersonMode;

    String                      m_playerName;

    shared_ptr<PhysicsScene>    m_scene;    
    
    /** Called from onInit */
    void makeGUI();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
    virtual void onAI() override;
    virtual void onNetwork() override;
    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;

    virtual bool onEvent(const GEvent& e) override;
    virtual void onUserInput(UserInput* ui) override;
};

#endif
