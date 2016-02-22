/**
 \file GLG3D/GuiButton.h

 \created 2006-05-01
 \edited  2014-07-13

 G3D Library http://g3d.cs.williams.edu
 Copyright 2001-2014, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GuiButton_h
#define G3D_GuiButton_h

#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiTheme.h"

namespace G3D {

class GuiWindow;
class GuiPane;

/** 
  Push button that can be temporarily pressed.  When the button has been pressed and released,
  a G3D::GuiEvent of type G3D::GEventType::GUI_ACTION is fired on the containing window. Alternatively,
  you can assign a GuiButton::Callback to execute when the button is pressed, <i>before</i> the event 
  is handled.

  See also GuiRadioButton and GuiCheckBox for creating buttons that stay down
  when pressed.
*/
class GuiButton : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;

protected:

    /** Is the mouse currently down over this control? */
    bool                    m_down;

    Callback                m_callback;

    GuiTheme::ButtonStyle   m_style;

public:

    /** Called by GuiContainers. See GuiPane::addButton instead. */
    GuiButton(GuiContainer*, const Callback& callback, const GuiText& text, GuiTheme::ButtonStyle style);

    /** Called by GuiContainers */
    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;

    /** Called by GuiContainers */
    virtual bool onEvent(const GEvent& event) override;

    virtual bool toolStyle() const override {
        return m_style == GuiTheme::TOOL_BUTTON_STYLE;
    }

    /** Returns whether or not button is down. */
    bool isDown();

    /** Sets the button as down. */
    void setDown();

    /** Sets the button as up. */
    void setUp();
};

} // G3D

#endif
