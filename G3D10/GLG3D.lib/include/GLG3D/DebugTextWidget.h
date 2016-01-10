/**
\file   DebugTextWidget.h
\maintainer Morgan McGuire
\edited 2015-07-14

G3D Library http://g3d.codeplex.com
Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
All rights reserved.
Use permitted under the BSD license

*/
#ifndef GLG3D_DebugTextWidget_h
#define GLG3D_DebugTextWidget_h

#include "GLG3D/Widget.h"

namespace G3D {

/** Renders rasterizer statistics and debug text for GApp */
class DebugTextWidget : public FullScreenWidget {
    friend class GApp;
protected:

    GApp*           m_app;

    DebugTextWidget(GApp* app) : m_app(app) {}

public:

    static shared_ptr<DebugTextWidget> create(GApp* app) {
        return shared_ptr<DebugTextWidget>(new DebugTextWidget(app));
    }

    virtual void render(RenderDevice* rd) const override;
};

} // G3D

#endif
