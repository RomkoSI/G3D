/**
 @file TextureViewer.h
 
 Viewer for image files
 
 @maintainer Eric Muller 09edm@williams.edu
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2014-10-08
 */
#ifndef TextureViewer_h
#define TextureViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class TextureViewer : public Viewer {
private:

	shared_ptr<Texture> m_texture;
	int				    m_width;
	int				    m_height;
    bool                m_isSky;

public:
	TextureViewer();
	virtual void onInit(const String& filename) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override;
    virtual void onGraphics2D(RenderDevice* rd, App* app) override;

};

#endif 
