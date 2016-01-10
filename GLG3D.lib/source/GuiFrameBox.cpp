/**
  \file GuiFrameBox.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2013-02-23
  \edited  2013-02-25
*/
#include "GLG3D/GuiFrameBox.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GuiLabel.h"

namespace G3D {

GuiFrameBox::GuiFrameBox
 (GuiContainer*          parent,
  const Pointer<CFrame>& value,
  bool                   allowRoll,
  GuiTheme::TextBoxStyle textBoxStyle) :
    GuiContainer(parent, ""),
    m_value(value),
    m_textBox(NULL),
    m_allowRoll(allowRoll) {

    shared_ptr<GFont> greekFont = GFont::fromFile(System::findDataFile("greek.fnt"));
    GuiLabel* L1 = new GuiLabel(this, GuiText(m_allowRoll ? "q q q" : "q q", greekFont, 12), GFont::XALIGN_LEFT, GFont::YALIGN_BASELINE);
    GuiLabel* L2 = new GuiLabel(this, GuiText(m_allowRoll ? "y  x  z" : "y  x", shared_ptr<GFont>(), 9), GFont::XALIGN_LEFT, GFont::YALIGN_BASELINE);

    L1->setRect(Rect2D::xywh(19, 2, 25, 15));
    L2->setRect(Rect2D::xywh(24, 9, 25, 9));
    m_labelArray.append(L1, L2);

    static String ignore;
    m_textBox = new GuiTextBox(this, "xyz", 
        Pointer<String>(this, &GuiFrameBox::frame, 
                                   &GuiFrameBox::setFrame), 
        GuiTextBox::DELAYED_UPDATE, textBoxStyle);
    m_textBox->setCaptionWidth(38 + (m_allowRoll ? 12.0f : 0.0f));
    m_textBox->setEventSource(this);

    // Overriden by GuiPane::addControl
    // setWidth(246 + (m_allowRoll ? 20.0f : 0.0f));
}


void GuiFrameBox::setEnabled(bool e) {
    m_textBox->setEnabled(e);
}


void GuiFrameBox::setRect(const Rect2D& rect) {
    GuiContainer::setRect(rect);

    m_textBox->setRect(Rect2D::xywh(m_captionWidth, 0, rect.width(), CONTROL_HEIGHT));
}


void GuiFrameBox::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    if (! m_visible) {
        return;
    }

    theme->pushClientRect(m_clientRect); {
        for (int i = 0; i < m_labelArray.size(); ++i) {
            m_labelArray[i]->render(rd, theme, ancestorsEnabled && m_enabled);
        }

        m_textBox->render(rd, theme, ancestorsEnabled && m_enabled);
    } theme->popClientRect();
}


void GuiFrameBox::findControlUnderMouse(Vector2 mouse, GuiControl*& control) {
    if (! m_clientRect.contains(mouse) || ! m_visible || ! m_enabled) {
        return;
    }

    mouse -= m_clientRect.x0y0();
    m_textBox->findControlUnderMouse(mouse, control);
}

        
GuiFrameBox::~GuiFrameBox() {
    m_labelArray.deleteAll();
    delete m_textBox;
}

static const String DEGREE = "\xba";

String GuiFrameBox::frame() const {
    const CoordinateFrame& cframe = *m_value;
    float x, y, z, roll = 0, yaw = 0, pitch = 0;

    cframe.getXYZYPRDegrees(x, y, z, yaw, pitch, roll);
    
    if (m_allowRoll && ! fuzzyEq(roll, 0.0f)) {
        // \xba is the character 186, which is the degree symbol
        return format("(% 5.1f, % 5.1f, % 5.1f), % 5.1f\xba, % 5.1f\xba, % 5.1f\xba", 
                      x, y, z, yaw, pitch, roll);
    } else {
        return format("(% 5.1f, % 5.1f, % 5.1f), % 5.1f\xba, % 5.1f\xba", 
                      x, y, z, yaw, pitch);
    }
}


void GuiFrameBox::setFrame(const String& s) {

    TextInput t(TextInput::FROM_STRING, s);
    try {
        CFrame cframe;
        float roll = 0, yaw = 0, pitch = 0;

        Token first = t.peek();
        if (first.string() == "CFrame") {
            // Code version
            t.readSymbols("CFrame", "::", "fromXYZYPRDegrees");
            t.readSymbol("(");
            cframe.translation.x = (float)t.readNumber();
            t.readSymbol(",");
            cframe.translation.y = (float)t.readNumber();
            t.readSymbol(",");
            cframe.translation.z = (float)t.readNumber();
            t.readSymbol(",");
            yaw = (float)t.readNumber();
            t.readSymbol(",");
            pitch = toRadians((float)t.readNumber());
            if (t.hasMore() && (t.peek().type() == Token::SYMBOL) && (t.peek().string() == ",")) {
                // Optional roll
                t.readSymbol(",");
                roll = toRadians((float)t.readNumber());
            }
        } else {
            // Pretty-printed version
            cframe.translation.deserialize(t);
            t.readSymbol(",");
            yaw = (float)t.readNumber();
            if (t.peek().string() == DEGREE) {
                t.readSymbol();
            }

            if (t.hasMore()) {
                t.readSymbol(",");
                pitch = (float)t.readNumber();
                if (t.peek().string() == DEGREE) {
                    t.readSymbol();
                }
            }

            if (t.hasMore()) {
                t.readSymbol(",");
                roll = (float)t.readNumber();
                if (t.peek().string() == DEGREE) {
                    t.readSymbol();
                }
            }
        }        

        if (! m_allowRoll) {
            roll = 0;
        }
        cframe = CFrame::fromXYZYPRDegrees(cframe.translation.x, cframe.translation.y, cframe.translation.z, yaw, pitch, roll);
        *m_value = cframe;

    } catch (const TextInput::TokenException& e) {
        // Ignore the incorrectly formatted value
        (void)e;
    }
}

}
