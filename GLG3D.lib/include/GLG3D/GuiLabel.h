/**
 @file GLG3D/GuiLabel.h

 @created 2006-05-01
 @edited  2014-06-01

 G3D Library http://g3d.codeplex.com
 Copyright 2001-2014, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GUILABEL_H
#define G3D_GUILABEL_H

#include "GLG3D/GuiControl.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** A text label.  Will never be the focused control and will never
    have the mouse over it. */
class GuiLabel : public GuiControl {
private:
    friend class GuiWindow;
    friend class GuiPane;

    GFont::XAlign m_xalign;
    GFont::YAlign m_yalign;
    
    void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const;

public:

    GuiLabel(GuiContainer*, const GuiText&, GFont::XAlign, GFont::YAlign);

    void setXAlign(GFont::XAlign a) {
        m_xalign = a;
    }

    GFont::XAlign xAlign() const {
        return m_xalign;
    }

    void setYAlign(GFont::YAlign a) {
        m_yalign = a;
    }

    GFont::YAlign yAlign() const {
        return m_yalign;
    }
};

} // G3D

#endif
