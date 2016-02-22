/**
 \file GuiWidgetDestructor.cpp

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2014-07-23
 \edited  2014-07-23

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */

#include "GLG3D/GuiWidgetDestructor.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/Widget.h"

namespace G3D {

GuiWidgetDestructor::GuiWidgetDestructor(GuiContainer* parent, const weak_ptr<Widget>& widget) : GuiControl(parent), m_widget(widget) {
    setSize(0, 0);
    setVisible(false);
    setEnabled(false);
}


GuiWidgetDestructor::~GuiWidgetDestructor() {
    const shared_ptr<Widget>& w = m_widget.lock();
    if (notNull(w) && notNull(w->manager())) {
        w->manager()->remove(w);
    }
}

}  // G3D
