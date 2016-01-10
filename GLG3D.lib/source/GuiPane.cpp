/**
 \file GuiPane.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2007-06-02
 \edited  2014-07-23
 */
#include "G3D/platform.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiCheckBox.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/GuiFrameBox.h"
#include "GLG3D/GuiWidgetDestructor.h"
#include "GLG3D/RenderDevice.h"

namespace G3D {

void GuiPane::init(const Rect2D& rect) {
    setRect(rect);
    m_layoutDirection = COLUMN;
    m_layoutPreviousControl = NULL;
    m_layoutControlSize = Vector2(DEFAULT_SIZE, DEFAULT_SIZE);
    m_layoutCaptionSize = Vector2(DEFAULT_SIZE, DEFAULT_SIZE);
}


GuiPane::GuiPane(GuiWindow* gui, const GuiText& text, const Rect2D& rect, GuiTheme::PaneStyle style) 
    : GuiContainer(gui, text), m_style(style) {
    init(rect);
}


GuiPane::GuiPane(GuiContainer* parent, const GuiText& text, const Rect2D& rect, GuiTheme::PaneStyle style) 
    : GuiContainer(parent, text), m_style(style) {
    init(rect);
}


GuiWidgetDestructor* GuiPane::addWidgetDestructor(const weak_ptr<Widget>& w) {
    return addControl(new GuiWidgetDestructor(this, w), 0.0f);
}


void GuiPane::removeAllChildren() {
    m_layoutPreviousControl = NULL;
    containerArray.invokeDeleteOnAllElements();
    controlArray.invokeDeleteOnAllElements();
    labelArray.invokeDeleteOnAllElements();
}


GuiControl* GuiPane::_addControl(GuiControl* control, float height) {
    const Vector2& p = nextControlPos(control->toolStyle());
    control->m_parent = this;

    float w = CONTROL_WIDTH;
    float h = height;
    if (m_layoutControlSize.x != DEFAULT_SIZE) {
        w = m_layoutControlSize.x;            
    }
    if (m_layoutControlSize.y != DEFAULT_SIZE) {
        h = m_layoutControlSize.y;
    }
    control->setRect(Rect2D::xywh(p, Vector2(w, h)));
    if ((m_layoutCaptionSize.x != DEFAULT_SIZE) && (control->captionWidth() != 0)) {
        control->setCaptionWidth(m_layoutCaptionSize.x);
    }
    if ((m_layoutCaptionSize.y != DEFAULT_SIZE) && (control->captionHeight() != 0)) {
        control->setCaptionHeight(m_layoutCaptionSize.y);
    }

    increaseBounds(control->rect().x1y1());

    GuiContainer* container = dynamic_cast<GuiContainer*>(control);
    if (container == NULL) {
        controlArray.append(control);
    } else {
        containerArray.append(container);
    }
    m_layoutPreviousControl = control;

    return control;
}


void GuiPane::setNewChildSize
(float controlWidth, float controlHeight,
 float captionWidth, float captionHeight) {
    m_layoutCaptionSize.x = captionWidth;
    m_layoutCaptionSize.y = captionHeight;
    m_layoutControlSize.x = controlWidth;
    m_layoutControlSize.y = controlHeight;
}


void GuiPane::morphTo(const Rect2D& r) {
    m_morph.morphTo(rect(), r);
}


void GuiPane::setCaption(const GuiText& caption) {
    GuiControl::setCaption(caption);
    setRect(rect());
}


Vector2 GuiPane::contentsExtent() const {
    Vector2 p(0.0f, 0.0f);
    for (int i = 0; i < controlArray.size(); ++i) {
        p = p.max(controlArray[i]->rect().x1y1());
    }

    for (int i = 0; i < containerArray.size(); ++i) {
        p = p.max(containerArray[i]->rect().x1y1());
    }

    for (int i = 0; i < labelArray.size(); ++i) {
        p = p.max(labelArray[i]->rect().x1y1());
    }
    return p;
}


Vector2 GuiPane::nextControlPos(bool isTool) const {
    if (((m_layoutDirection == ROW) || isTool) && m_layoutPreviousControl) {
        return m_layoutPreviousControl->rect().x1y0();
    } else {
        float y = contentsExtent().y;
        return Vector2(CONTROL_PADDING, max(y, (float)CONTROL_PADDING));
    }
}


void GuiPane::pack() {
    // Resize to minimum bounds
    setSize(m_rect.wh() - m_clientRect.wh());
    for (int i = 0; i < containerArray.size(); ++i) {
        GuiPane* p = dynamic_cast<GuiPane*>(containerArray[i]);
        if (p != NULL) {
            p->pack();
        }
    }
    increaseBounds(contentsExtent());
}


void GuiPane::setRect(const Rect2D& rect) {
    m_rect = rect;       
    m_clientRect = theme()->paneToClientBounds(m_rect, m_caption, GuiTheme::PaneStyle(m_style));
    debugAssert(! m_clientRect.isEmpty());
}


GuiPane::~GuiPane() {
    removeAllChildren();
}


GuiTextureBox* GuiPane::addTextureBox
(GApp* app,
 const GuiText& caption,
 const shared_ptr<Texture>& t,
 bool embedded,
 bool drawInverted) {
    return addControl(new GuiTextureBox(this, caption, app, t, embedded, drawInverted), 240);
}


GuiFrameBox* GuiPane::addFrameBox
(const Pointer<CFrame>&         value,
 bool                           allowRoll,
 GuiTheme::TextBoxStyle         style) {
    return addControl(new GuiFrameBox(this, value, allowRoll, style));
}


GuiTextureBox* GuiPane::addTextureBox
(GApp* app,
 const shared_ptr<Texture>&     t,
 bool                           embedded,
 bool                           drawInverted) {
    return addTextureBox(app, notNull(t) ? t->name() : "", t, embedded, drawInverted);
}


GuiDropDownList* GuiPane::addDropDownList
(const GuiText&                 caption, 
 const Array<String>&           list,
 const Pointer<int>&            pointer,
 const GuiControl::Callback&    actionCallback) {

    Array<GuiText> c;
    c.resize(list.size());
    for (int i = 0; i < c.size(); ++i) {
        c[i] = list[i];
    }
    return addDropDownList(caption, c, pointer, actionCallback);
}


GuiDropDownList* GuiPane::addDropDownList
(const GuiText& caption, 
 const Array<GuiText>& list,
 const Pointer<int>& pointer,
 const GuiControl::Callback& actionCallback) {
    return addControl(new GuiDropDownList(this, caption, pointer, list, NULL, actionCallback));
}


GuiRadioButton* GuiPane::addRadioButton(const GuiText& text, int myID, void* selection, 
                                        GuiTheme::RadioButtonStyle style) {

    GuiRadioButton* c = addControl
        (new GuiRadioButton(this, text, myID, Pointer<int>(reinterpret_cast<int*>(selection)), style));

    Vector2 size(0, (float)CONTROL_HEIGHT);

    if (style == GuiTheme::TOOL_RADIO_BUTTON_STYLE) {
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::TOOL_BUTTON_STYLE);
        size.x = max(float(TOOL_BUTTON_WIDTH), bounds.x);
    } else if (style == 1) { // Doesn't compile on gcc unless we put the integer in. GuiTheme::BUTTON_RADIO_BUTTON_STYLE) {
        size.x = BUTTON_WIDTH;
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::NORMAL_BUTTON_STYLE);
        size = size.max(bounds);
    } else { // NORMAL_RADIO_BUTTON_STYLE
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::NORMAL_BUTTON_STYLE);
        size.x = bounds.x;
    }

    c->setSize(size);

    return c;
}


GuiCheckBox* GuiPane::addCheckBox
(const GuiText& text,
 const Pointer<bool>& pointer,
 GuiTheme::CheckBoxStyle style) {
    GuiCheckBox* c = addControl(new GuiCheckBox(this, text, pointer, style));
    
    Vector2 size(0, CONTROL_HEIGHT);

    if (style == GuiTheme::TOOL_CHECK_BOX_STYLE) {
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::TOOL_BUTTON_STYLE);
        size.x = max(float(TOOL_BUTTON_WIDTH), bounds.x);
    } else {
        size.x = BUTTON_WIDTH;
        Vector2 bounds = theme()->minButtonSize(text, GuiTheme::NORMAL_BUTTON_STYLE);
        size = size.max(bounds);
    }

    c->setSize(size);

    return c;
}


GuiControl* GuiPane::addCustom(GuiControl* c) {
    c->setPosition(nextControlPos(c->toolStyle()));
    c->m_parent = this;
    GuiContainer* container = dynamic_cast<GuiContainer*>(c);
    if (container) {
        containerArray.append(container);
    } else {
        controlArray.append(c);
    }

    increaseBounds(c->rect().x1y1());
    return c;
}


GuiRadioButton* GuiPane::addRadioButton(const GuiText& text, int myID,  
    const Pointer<int>& ptr, 
    GuiTheme::RadioButtonStyle style) {
    
    GuiRadioButton* c = addControl(new GuiRadioButton(this, text, myID, ptr, style));
    
    Vector2 size((float)BUTTON_WIDTH, (float)CONTROL_HEIGHT);

    // Ensure that the button is wide enough for the caption
    const Vector2& bounds = theme()->minButtonSize(text, 
        (style == GuiTheme::TOOL_RADIO_BUTTON_STYLE) ? GuiTheme::TOOL_BUTTON_STYLE : GuiTheme::NORMAL_BUTTON_STYLE);

    if (style == GuiTheme::TOOL_RADIO_BUTTON_STYLE) {
        c->setSize(Vector2(max((float)TOOL_BUTTON_WIDTH, bounds.x), CONTROL_HEIGHT));
    } else if (style == GuiTheme::BUTTON_RADIO_BUTTON_STYLE) {
        c->setSize(Vector2(max((float)BUTTON_WIDTH, bounds.x), CONTROL_HEIGHT));
    }

    return c;
}


GuiButton* GuiPane::addButton(const GuiText& text, GuiTheme::ButtonStyle style) {
    return addButton(text, GuiButton::Callback(), style);
}


GuiButton* GuiPane::addButton(const GuiText& text, const GuiControl::Callback& callback, GuiTheme::ButtonStyle style) {
    GuiButton* b = new GuiButton(this, callback, text, style);

    addControl(b);

    Vector2 size((float)BUTTON_WIDTH, (float)CONTROL_HEIGHT);

    // Ensure that the button is wide enough for the caption
    const Vector2& bounds = theme()->minButtonSize(text, style);
    if (style == GuiTheme::NORMAL_BUTTON_STYLE) {
        size = size.max(bounds);
    } else {
        size.x = max((float) TOOL_BUTTON_WIDTH, bounds.x);
    }

    b->setSize(size);
    
    return b;
}


GuiLabel* GuiPane::addLabel(const GuiText& text, GFont::XAlign xalign, GFont::YAlign yalign) {
    GuiLabel* b = new GuiLabel(this, text, xalign, yalign);
    const Vector2& bounds = theme()->bounds(text);
    b->setRect(Rect2D::xywh(nextControlPos() + Vector2(CONTROL_PADDING, 0), 
                            bounds.max(Vector2(min(m_clientRect.width(), (float)CONTROL_WIDTH), 
                                               CONTROL_HEIGHT))));
    labelArray.append(b);

    m_layoutPreviousControl = b;
    return b;
}


GuiFunctionBox* GuiPane::addFunctionBox(const GuiText& text, Spline<float>* spline) {

    GuiFunctionBox* control = new GuiFunctionBox(this, text, spline);

    Vector2 p = nextControlPos(control->toolStyle());
    control->setRect
        (Rect2D::xywh(p, Vector2((float)CONTROL_WIDTH, control->rect().height())));
    
    increaseBounds(control->rect().x1y1());
    
    GuiContainer* container = dynamic_cast<GuiContainer*>(control);
    if (container == NULL) {
        controlArray.append(control);
    } else {
        containerArray.append(container);
    }
    m_layoutPreviousControl = control;

    return control;
}


GuiTabPane* GuiPane::addTabPane(const Pointer<int>& index) {
    GuiTabPane* p = new GuiTabPane(this, index);
    
    Vector2 pos = nextControlPos();
    p->moveBy(pos);

    containerArray.append(p);
    increaseBounds(p->rect().x1y1());

    m_layoutPreviousControl = p;

    return p;


}


GuiScrollPane* GuiPane::addScrollPane(bool enabledVerticalScrolling, bool enabledHorizontalScrolling, GuiTheme::ScrollPaneStyle style) {
    GuiScrollPane* p = new GuiScrollPane(this, enabledVerticalScrolling, enabledHorizontalScrolling, style);
    Vector2 pos = nextControlPos();
    p->moveBy(pos);

    containerArray.append(p);
    increaseBounds(p->rect().x1y1());
    return p;
}


GuiPane* GuiPane::addPane(const GuiText& text, GuiTheme::PaneStyle style) {
    Rect2D minRect = theme()->clientToPaneBounds(Rect2D::xywh(0,0,0,0), text, style);

    Vector2 pos = nextControlPos();

    // Back up by the border size
    pos -= minRect.x0y0();

    // Ensure the width isn't negative due to a very small m_clientRect
    // which would push the position off the parent panel
    float newRectWidth = max(m_clientRect.width() - pos.x * 2, 0.0f);

    Rect2D newRect = Rect2D::xywh(pos, Vector2(newRectWidth, minRect.height()));

    GuiPane* p = new GuiPane(this, text, newRect, style);

    containerArray.append(p);
    increaseBounds(p->rect().x1y1());

    return p;
}


void GuiPane::findControlUnderMouse(Vector2 mouse, GuiControl*& control) {
    if (! m_clientRect.contains(mouse) || ! m_visible || ! m_enabled) {
        return;
    }

    mouse -= m_clientRect.x0y0();

    for (int i = 0; i < containerArray.size(); ++i) {
        containerArray[i]->findControlUnderMouse(mouse, control);
    }

    // Test in the opposite order of rendering so that the top-most control receives the event
    for (int i = 0; i < controlArray.size(); ++i) {
        controlArray[i]->findControlUnderMouse(mouse, control);
    }
}


void GuiPane::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    if (m_morph.active) {
        GuiPane* me = const_cast<GuiPane*>(this);
        me->m_morph.update(me);
    }

    if (! m_visible) {
        return;
    }

    theme->renderPane(m_rect, m_caption, m_style);

    renderChildren(rd, theme, ancestorsEnabled);
}


void GuiPane::renderChildren(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    theme->pushClientRect(m_clientRect);

    const Rect2D& clipRect = rd->clip2D();
    if (! clipRect.isEmpty()) {

        for (int L = 0; L < labelArray.size(); ++L) {
            labelArray[L]->render(rd, theme, ancestorsEnabled && m_enabled);
        }

        for (int c = 0; c < controlArray.size(); ++c) {
            controlArray[c]->render(rd, theme, ancestorsEnabled && m_enabled);
        }

        for (int p = 0; p < containerArray.size(); ++p) {
            containerArray[p]->render(rd, theme, ancestorsEnabled && m_enabled);
        }
    }

    theme->popClientRect();
}


void GuiPane::remove(GuiControl* control) {
    
    int i = labelArray.findIndex(reinterpret_cast<GuiLabel* const>(control));
    int j = controlArray.findIndex(control);
    int k = containerArray.findIndex(reinterpret_cast<GuiPane* const>(control));

    if (i != -1) {

        labelArray.fastRemove(i);
        delete control;

    } else if (j != -1) {

        controlArray.fastRemove(j);

        if (m_gui->keyFocusGuiControl == control) {
            m_gui->keyFocusGuiControl = NULL;
        }

        if (m_gui->mouseOverGuiControl == control) {
            m_gui->mouseOverGuiControl = NULL;
        }

        delete control;

    } else if (k != -1) {

        containerArray.fastRemove(k);
        delete control;

    }
}

}
