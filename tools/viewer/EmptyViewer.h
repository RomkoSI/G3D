/**
 @file EmptyViewer.h
 
 If the person tried to load a file that wasn't valid, this default viewer is displayed
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2014-10-08
 */
#ifndef EmptyViewer_h
#define EmptyViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class EmptyViewer : public Viewer {

public:
	EmptyViewer();
	virtual void onInit(const String& filename) override;
    virtual void onGraphics2D(RenderDevice* rd, App* app) override;
};

#endif 
