/**
 \file GLG3D/GuiPrefixDropDownList.h

 \created 2007-06-15
 \edited  2014-08-16

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2017, Morgan McGuire morgan@casual-effects.com
 All rights reserved.
*/
#ifndef G3D_GuiPrefixDropDownList_h
#define G3D_GuiPrefixDropDownList_h

#include "G3D/Pointer.h"
#include "G3D/Array.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiMenu.h"

#include "G3D/PrefixTree.h"

namespace G3D {

class GuiPane;

/** List box for viewing strings or GuiText.  

    Fires a G3D::GuiEvent of type G3D::GEventType::GUI_ACTION on 
    the containing window when the user selects a new value, 
    GEventType::GUI_CANCEL when the user opens the dropdown and
    then clicks off or presses ESC.

    Not a subclass of GuiDropDownList because they have different backing
    data structures.
*/
class GuiPrefixDropDownList : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;

protected:
    GuiText                         m_selectedValue;    
    GuiControl::Callback            m_actionCallback;

    shared_ptr<PrefixTree>          m_prefixTree;
    shared_ptr<GuiMenu>             m_menuHead;    

    virtual bool onEvent(const GEvent& event) override;
   
    void showMenu();
    void showMenuHelper(const shared_ptr<PrefixTree>& node, const Rect2D& parentRect);
    void appendMenu(const shared_ptr<GuiMenu>& menu);

public:
    
    GuiPrefixDropDownList
       (GuiContainer*               parent, 
        const GuiText&              caption,
        const Array<String>&        items,
        const GuiControl::Callback& actionCallback);

    GuiPrefixDropDownList
       (GuiContainer*               parent, 
        const GuiText&              caption,
        const Array<GuiText>&       items,
        const GuiControl::Callback& actionCallback);

    /** Called by GuiPane **/
    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    
    void setList(const Array<GuiText>& c);
    void setList(const Array<String>& c);

    /** Remove all values from the list */
    void clear();

    /** Hide the pop-up menus */
    void close();

    void appendValue(const GuiText& c);
        
    int numElements() const { return m_prefixTree->size(); }

    virtual void setRect(const Rect2D&) override;

    /** Returns the currently selected value */
    const GuiText& selectedValue() const;

    /** True if the list contains this value */
    bool containsValue(const String& s) const;
        
    /** Selects the first value whose text() is equal to \a s. If not
        found, does nothing. */
    void setSelectedValue(const String& s);
};

} // G3D

#endif
