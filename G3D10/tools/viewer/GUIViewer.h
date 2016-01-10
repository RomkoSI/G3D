/**
 @file GUIViewer.h
 
 Viewer for testing and examining .gtm files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2014-10-14
 */
#ifndef GuiViewer_h
#define GuiViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class GUIViewer : public Viewer {
private:
    shared_ptr<GuiWindow>		window;
    shared_ptr<GuiWindow>		toolWindow;
    shared_ptr<GuiWindow>		bgControl;
    shared_ptr<GuiWindow>       dropdownWindow;
    App*			            parentApp;
    shared_ptr<GuiTheme>	    skin;
    shared_ptr<Texture>		    background1;
    shared_ptr<Texture>	        background2;
    
    enum WindowBackground {WHITE = 1, BLUE = 2, BLACK = 3, 
                           BGIMAGE1 = 4, BGIMAGE2 = 5};
    
    WindowBackground	windowControl;

    /**
       A typical GUI has actual state that it is connected to.
       We're just showing off the GUI and not changing any real
       state, so we make up some variables to hold the values.  In a
       normal program, for example, you should not name your
       variables "checkbox", but instead hook a GuiCheckBox up to an
       existing bool that has a useful name.
    */
    Array<G3D::String>	dropdown;
    Array<G3D::String>	dropdownDisabled;
    int                 dropdownIndex[2];
    bool                checkbox[8];
    int                 radio[4];
    float               slider[2];
    G3D::String         text;

    void createGui(const G3D::String& filename);

public:
	GUIViewer(App* app);
	~GUIViewer();
	virtual void onInit(const String& filename) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override;
};

#endif
