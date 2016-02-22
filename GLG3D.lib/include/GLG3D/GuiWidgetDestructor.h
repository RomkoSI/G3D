/**
 \file GLG3D/GuiWidgetDestructor.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2014-07-23
 \edited  2014-07-23

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license

 */
#ifndef GLG3D_GuiWidgetDestructor_h
#define GLG3D_GuiWidgetDestructor_h

#include "G3D/platform.h"
#include "GLG3D/GuiControl.h"

namespace G3D {

class Widget;
class GuiContainer;

/** Detects when this object is removed from the GUI and removes the corresponding widget from its manager. */
class GuiWidgetDestructor : public GuiControl {
protected:

    weak_ptr<Widget> m_widget;

public:

    GuiWidgetDestructor(GuiContainer* parent, const weak_ptr<Widget>& widget);

    virtual void render(RenderDevice* rd, const shared_ptr< GuiTheme >& theme, bool ancestorsEnabled) const override {}

    ~GuiWidgetDestructor();
};

} // namespace

#endif
