/**
 @file GLG3D/GuiRadioButton.h

 @created 2006-05-01
 @edited  2014-07-01

 G3D Library http://g3d.cs.williams.edu
 Copyright 2001-2014, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GuiRadioButton_h
#define G3D_GuiRadioButton_h

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;

/**
   Radio button or exclusive set of toggle butons.
 */
class GuiRadioButton : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;

protected:
    
    Pointer<int>                m_value;
    int                         m_myID;
    GuiTheme::RadioButtonStyle  m_style;

    /** 
        @param myID The ID of this button 
        
        @param groupSelection Pointer to the current selection.  This button is selected
        when myID == *groupSelection
     */
    GuiRadioButton(GuiContainer* parent, const GuiText& text, int myID,
                   const Pointer<int>& groupSelection,
                   GuiTheme::RadioButtonStyle style = GuiTheme::NORMAL_RADIO_BUTTON_STYLE);

    bool selected() const;
    
    void setSelected();

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;

    virtual bool onEvent(const GEvent& event) override;

public:

    virtual void setRect(const Rect2D& rect) override;

    virtual bool toolStyle() const override { 
        return m_style == GuiTheme::TOOL_RADIO_BUTTON_STYLE;
    }
};

} // G3D

#endif
