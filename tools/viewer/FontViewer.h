/**
 @file FontViewer.h
 
 Viewer for .fnt files, with a default font displayed for comparison
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-06-08
 */
#ifndef FontViewer_h
#define FontViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class FontViewer : public Viewer {
private:

	String		            m_fontfilename;
	shared_ptr<GFont>		m_font;
	shared_ptr<GFont>		m_basefont;

public:
	FontViewer(shared_ptr<GFont> base);
	virtual void onInit(const String& filename) override;
    virtual void onGraphics2D(RenderDevice* rd, App* app);
};

#endif 
