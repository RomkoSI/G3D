/**
  \file App.h

  The G3D 10.00 default starter app is configured for OpenGL 3.3 and
  relatively recent GPUs.
 */
#ifndef App_h
#define App_h
#include <G3D/G3DAll.h>

#include <GLG3D/SVO.h>

#define SVO_MAX_DEPTH					9
# define SVO_POOL_SIZE					(500) //MB
# define SVO_FRAGBUFFER_SIZE			(1000) //MB


/** Application framework. */
class App : public GApp {
protected:

    /** Called from onInit */
    void makeGUI();

	bool							m_enableSVO			= true;
	bool							m_debugSVONodes		= false;
	bool							m_debugSVOFragments = false;
	int								m_debugSVONodeLevel = 1;

	float							m_voxelConeAperture	= 1.0f;

	shared_ptr<SVO>					m_svo;

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;

    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface3D) override;

	virtual void onAfterLoadScene(const Any& any, const String& sceneName) override;
};

#endif
