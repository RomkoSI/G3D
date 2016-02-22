/**
 @file GLG3D/GuiNumberBox.h

 @created 2008-07-09
 @edited  2013-02-23

 G3D Library http://g3d.cs.williams.edu
 Copyright 2001-2014, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiNumberBox_h
#define G3D_GuiNumberBox_h

#include "G3D/platform.h"
#include "G3D/Pointer.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/GuiTextBox.h"
#include "GLG3D/GuiSlider.h"

namespace G3D {

class GuiWindow;
class GuiPane;


/** Number editing textbox with associated slider.  
    See GuiWindow for an example of creating a number box. 

    <b>Events:</b>
    <ul>
      <li> GEventType::GUI_ACTION when the slider thumb is released or enter is pressed in the text box.
      <li> GEventType::GUI_CHANGE during slider scrolling.
      <li> GEventType::GUI_DOWN when the mouse is pressed down on the slider.
      <li> GEventType::GUI_UP when the mouse is released on the slider.
      <li> GEventType::GUI_CANCEL when ESC is pressed in the text box
    </ul>

    The min/mix/rounding values are enforced on the GUI, but not on the underlying value 
    if it is changed programmatically. 

    "nan", "inf", and "-inf" are all parsed to the appropriate floating point values.

    @sa G3D::GuiPane::addNumberBox
*/
template<class Value>
class GuiNumberBox : public GuiContainer {
    friend class GuiPane;
    friend class GuiWindow;

protected:

    // Constants
    enum {textBoxWidth = 60};

    /** Current value */
    Pointer<Value>    m_value;

    /** Value represented by m_textValue */
    Value             m_oldValue;

    /** Text version of value */
    String       m_textValue;

    String       m_formatString;

    /** Round to the nearest multiple of this value. */
    Value             m_roundIncrement;

    Value             m_minValue;
    Value             m_maxValue;

    /** NULL if there is no slider */
    GuiSlider<Value>* m_slider;

    GuiTextBox*       m_textBox;

    GuiText           m_units;
    float             m_unitsSize;

    bool             m_lowerLimitInf;
    bool             m_upperLimitInf;

    // Methods declared in the middle of member variables
    // to provide forward declarations.  Do not change 
    // declaration order.

    void roundAndClamp(Value& v) const {
        if (m_roundIncrement != 0) {
            v = (Value)floor(v / (double)m_roundIncrement + 0.5) * m_roundIncrement;
        }
        
        bool logSlider = notNull(m_slider);
        if (v <= m_minValue) {
            if (m_lowerLimitInf && logSlider) {
                v = Value(-finf());
             } else {
                v = m_minValue;
             }
        }
        if (v >= m_maxValue) {
             if (m_upperLimitInf && logSlider) {
                v = Value(finf());
             } else {
                v = m_maxValue;
             }
        }
    }

    void updateText() {
        // The text display is out of date
        m_oldValue = *m_value;
        roundAndClamp(m_oldValue);
        // Do not write the rounded value back to m_value because it
        // will trigger the setter, which might be undesirable, e.g., for
        // Scene::setTimes
        if (m_oldValue == inf()) {
            m_textValue = "inf";
        } else if (m_oldValue == -inf()) {
            m_textValue = "-inf";
        } else if (isNaN(m_oldValue)) {
            m_textValue = "nan";
        } else {
            m_textValue = format(m_formatString.c_str(), m_oldValue);
        }
    }

    /** Called when the user commits the text box. */
    void commit() {
        double f = (double)m_minValue;
        String s = m_textValue;

        // Parse the m_textValue to a float
        int numRead = sscanf(s.c_str(), "%lf", &f);
        
        if (numRead == 1) {
            // Parsed successfully
            *m_value = (Value)f;
        } else {
            s = trimWhitespace(toLower(s));

            if (m_textValue == "inf") {
                f = inf();
            } else if (m_textValue == "-inf") {
                f = -inf();
            } else if (m_textValue == "nan") {
                f = nan();
            }
        }
        updateText();
    }

    /** Notifies the containing pane when the text is commited. */
    class MyTextBox : public GuiTextBox {
    protected:
        GuiNumberBox<Value>*      m_numberBox;

        virtual void commit() {
            GuiTextBox::commit();
            m_numberBox->commit();
        }
    public:

        MyTextBox(GuiNumberBox<Value>* parent, const GuiText& caption, 
               const Pointer<String>& value, Update update, GuiTheme::TextBoxStyle style) :
               GuiTextBox(parent, caption, value, update, style),
               m_numberBox(parent) {
           // Make events appear to come from parent
           m_eventSource = parent;
       }
    };
  
    ////////////////////////////////////////////////////////
    // The following overloads allow GuiNumberBox to select the appropriate format() string
    // based on Value.

    // Smaller int types will automatically convert
    static String formatString(int v, int roundIncrement) {
        (void)roundIncrement;
        (void)v;
        return "%d";
    }

    static String formatString(int64 v, int64 roundIncrement) {
#       ifdef MSVC
            // http://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx
            return "%I64d";
#       else
            // http://www.gnu.org/software/libc/manual/html_node/Integer-Conversions.html
            return "%lld";
#       endif
    }

    static String formatString(uint64 v, uint64 roundIncrement) {
#       ifdef MSVC
            // http://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx
            return "%I64u";
#       else
            // http://www.gnu.org/software/libc/manual/html_node/Integer-Conversions.html
            return "%llu";
#       endif
    }

    /** Computes the format string for the precision specification needed to see the most
        significant digit of roundIncrement. */
    static String precision(double roundIncrement) {
        if (roundIncrement == 0) {
            return "";
        } else if (roundIncrement > 1) {
            // Show only the integer part
            return ".0";
        } else {
            // Compute the number of decimal places needed for the roundIncrement
            int n = iCeil(-log10(roundIncrement));
            return format(".%d", n);
        }
    }

    static String formatString(float v, float roundIncrement) {
        (void)v;
        return "%" + precision(roundIncrement) + "f";
    }

    String formatString(double v, double roundIncrement) {
        (void)v;
        return "%" + precision(roundIncrement) + "lf";
    }

    /////////////////////////////////////////////////////////////////

public:

    /** For use when building larger controls out of GuiNumberBox.  For making a regular GUI, use GuiPane::addNumberBox. */
    GuiNumberBox
       (GuiContainer*           parent, 
        const GuiText&          caption, 
        const Pointer<Value>&   value, 
        const GuiText&          units,
        GuiTheme::SliderScale   scale,
        Value                   minValue, 
        Value                   maxValue,
        Value                   roundIncrement,
        GuiTheme::TextBoxStyle	textBoxStyle,
        bool                   useLowerInf = false,
        bool                   useUpperInf = false) :
        GuiContainer(parent, caption),
        m_value(value),
        m_roundIncrement(roundIncrement),
        m_minValue(minValue),
        m_maxValue(maxValue),
        m_slider(NULL),
        m_textBox(NULL),
        m_units(units),
        m_unitsSize(22),
        m_lowerLimitInf(useLowerInf),
        m_upperLimitInf(useUpperInf) {

        debugAssert(m_roundIncrement >= 0);

        if (scale != GuiTheme::NO_SLIDER) {
            debugAssertM(m_minValue > -inf() && m_maxValue < inf(), 
                "Cannot have min and max values be infinite instead set useLowerInf and useUpperInf to set the top value to infnity");

            m_slider = new GuiSlider<Value>(this, "", value, m_minValue, m_maxValue, true, scale, this, useLowerInf, useUpperInf);
        }

        m_textBox = new MyTextBox(this, "", &m_textValue, GuiTextBox::DELAYED_UPDATE, textBoxStyle);

        m_formatString = formatString(*value, roundIncrement);

        m_oldValue = *m_value;
        roundAndClamp(m_oldValue);
        *m_value = m_oldValue;
        updateText();
    }
        
    ~GuiNumberBox() {
        delete m_textBox;
        delete m_slider;
    }

    // The return value is not a const reference, since value is usually int or float
    Value minValue() const {
        return m_minValue;
    }

    Value maxValue() const {
        return m_maxValue;
    }

    virtual void setCaption(const GuiText& c)  override {
        GuiContainer::setCaption(c);

        // Resize other parts in response to caption size changing
        setRect(m_rect);
    } 

    void setRange(Value lo, Value hi) {
        if (m_slider != NULL) {
            m_slider->setRange(lo, hi);
        }

        m_minValue = min(lo, hi);
        m_maxValue = max(lo, hi);
    }

    virtual void setRect(const Rect2D& rect) override {
        GuiContainer::setRect(rect);

        // Total size of the GUI, after the caption
        float controlSpace = m_rect.width() - m_captionWidth;

        if (m_slider == NULL) {
            // No slider: the text box will fill the rest of the size
            m_textBox->setRect(Rect2D::xywh(m_captionWidth, 0, controlSpace - m_unitsSize, CONTROL_HEIGHT));
        } else {
            m_textBox->setRect(Rect2D::xywh(m_captionWidth, 0, textBoxWidth, CONTROL_HEIGHT));

            float x = m_textBox->rect().x1() + m_unitsSize;
            m_slider->setRect(Rect2D::xywh(x, 0.0f, 
                max(controlSpace - (x - m_captionWidth) - 2, 5.0f), (float)CONTROL_HEIGHT));
        }
    }

    void setUnitsSize(float s) {
        m_unitsSize = s;
        setRect(rect());
    }

    /** The number of pixels between the text box and the slider.*/
    float unitsSize() const {
        return m_unitsSize;
    }

    virtual void setEnabled(bool e) override {
        m_textBox->setEnabled(e);
        if (m_slider != NULL) {
            m_slider->setEnabled(e);
        }
    }

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override {
        if (! m_clientRect.contains(mouse) || ! m_visible || ! m_enabled) {
            return;
        }

        mouse -= m_clientRect.x0y0();
        m_textBox->findControlUnderMouse(mouse, control);
        if (m_slider != NULL) {
            m_slider->findControlUnderMouse(mouse, control);
        }
    }

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override {
        if (! m_visible) {
            return;
        }

        if (m_oldValue != *m_value) {
            const_cast<GuiNumberBox<Value>*>(this)->updateText();
        }

        theme->pushClientRect(m_clientRect); {
            m_textBox->render(rd, theme, ancestorsEnabled && enabled());

            // Don't render the slider if there isn't enough space for it 
            if ((m_slider != NULL) && (m_slider->rect().width() > 10)) {
                m_slider->render(rd, theme, ancestorsEnabled && enabled());
            }

            // Render caption and units
            theme->renderLabel(m_rect - m_clientRect.x0y0(), m_caption, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER, m_enabled);

            const Rect2D& textBounds = m_textBox->rect();
            theme->renderLabel(Rect2D::xywh(textBounds.x1y0(), Vector2(m_unitsSize, textBounds.height())), 
                m_units, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER, m_enabled);
        } theme->popClientRect();
    }

};

} // G3D

#endif
