/**
 @file IconSet.h
 
 Viewer for .icn files

 \author Morgan McGuire
 
 \created 2010-01-04
 \edited  2014-10-04
 */
#ifndef IconSetViewer_h
#define IconSetViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class IconSetViewer : public Viewer {
private:

    shared_ptr<GFont>   m_font;
    shared_ptr<IconSet> m_iconSet;

public:
    IconSetViewer(const shared_ptr<GFont>& captionFont);
	virtual void onInit(const String& filename) override;
    virtual void onGraphics2D(RenderDevice* rd, App* app);
};

#endif 
