/**
 @file GLG3D.lib/source/GuiFrameBox.h

 @created 2013-02-23
 @edited  2014-07-23

 G3D Library http://g3d.codeplex.com
 Copyright 2001-2014, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiFrameBox_h
#define G3D_GuiFrameBox_h

#include "G3D/platform.h"
#include "G3D/Pointer.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiContainer.h"

namespace G3D {

class GuiWindow;
class GuiPane;
class GuiTextBox;
class GuiTheme;
class RenderDevice;

/**
 For editing coordinate frames
 */
class GuiFrameBox : public GuiContainer {
    friend class GuiPane;
    friend class GuiWindow;

protected:

    Pointer<CFrame>             m_value;
    Array<GuiControl*>          m_labelArray;
    GuiTextBox*                 m_textBox;
    bool                        m_allowRoll;

    String frame() const;
    void setFrame(const String& s);

public:

    /** For use when building larger controls out of GuiFrameBox.  
        For making a regular GUI, use GuiPane::addFrameBox. */
    GuiFrameBox
       (GuiContainer*          parent,
        const Pointer<CFrame>& value,
        bool                   allowRoll,
        GuiTheme::TextBoxStyle textBoxStyle);

    virtual void setRect(const Rect2D& rect) override;

    virtual void setEnabled(bool e) override;

    void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override;

    ~GuiFrameBox();
};

} // namespace G3D
#endif
