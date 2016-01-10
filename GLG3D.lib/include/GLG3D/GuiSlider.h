/**
 \file GLG3D/GuiSlider.h

 \created 2006-05-01
 \edited  2014-07-18

 G3D Library http://g3d.codeplex.com
 Copyright 2001-2014, Morgan McGuire morgan@users.sf.net
 All rights reserved.
*/
#ifndef G3D_GuiSlider_h
#define G3D_GuiSlider_h

#include "GLG3D/GuiControl.h"
#include "G3D/Pointer.h"

namespace G3D {

class GuiWindow;
class GuiPane;

class _GuiSliderBase : public GuiControl {
    friend class GuiPane;

protected:

    bool              m_horizontal;
    bool              m_inDrag;
    float             m_dragStartValue;

    _GuiSliderBase(GuiContainer* parent, const GuiText& text, bool horizontal);

    /** Position from which the mouse drag started, relative to
        m_gui.m_clientRect.  When dragging the thumb, the cursor may not be
        centered on the thumb the way it is when the mouse clicks
        on the track.
    */
    Vector2           m_dragStart;
    
    /** Get value on the range 0 - 1 */
    virtual float floatValue() const = 0;

    /** Set value on the range 0 - 1 */
    virtual void setFloatValue(float f) = 0;

    virtual bool onEvent(const GEvent& event) override;

public:

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;

};

/** Used by GuiSlider */
template<class T>
class LogScaleAdapter : public ReferenceCountedObject {
private:

// The maximum Log Base
#define MAX_LOG_BASE 20.0
// The scale factor of the range
#define LOG_BASE_SCALE 100.0

    friend class Pointer<T>;

    Pointer<T>      m_source;
    double          m_low;
    double          m_high;

    /** true if m_low < 0 and m_high > 0 */
    bool            m_containsZero;

    /** m_high - m_low */
    double          m_range;

    double          m_base;
    double          m_logBase;

    bool            m_lowerLimitInf;
    bool            m_upperLimitInf;

    typedef shared_ptr<LogScaleAdapter> Ptr;

    LogScaleAdapter(const Pointer<T>& ptr, const T& low, const T& high, bool lowerLimitInf = false, bool upperLimitInf = false) : 
          m_source(ptr), 
          m_low((double)low), 
          m_high((double)high), 
          m_lowerLimitInf(lowerLimitInf), 
          m_upperLimitInf(upperLimitInf) {

        // If the slider bounds include zero then set the log base on the absalout value of the largest value
        if ((m_low < 0) && (m_high > 0)) {
            m_containsZero = true;
            m_base = max(max(abs(m_low), m_high) / LOG_BASE_SCALE, MAX_LOG_BASE);
        } else {
            m_containsZero = false;
            m_base = max(MAX_LOG_BASE, m_range / LOG_BASE_SCALE);
        }

        m_range = m_high - m_low;
        m_logBase = ::log(m_base);
    }

    /** For use by Pointer<T> */
    T get() const {
        double normalizer = m_range;
        double lowerBound = m_low;

        double v = double(m_source.getValue());
        
        //If the range contains zero then respond to the cases when it is posotive or negative
        if (m_containsZero) {
            lowerBound  = 0;
            if (v > 0) {
                normalizer = m_high;
            } else {
                normalizer = m_low;
            }
        } 
            
        if (m_range == 0) {
            // No scaling necessary
            return m_source.getValue();
        }

        // Normalize the value
        double y = (double(v) - lowerBound) / normalizer;

        // Scale logarithmically
        double x = ::log(y * (m_base - 1.0) + 1.0) / m_logBase;

        // Expand range
        return T(x * normalizer + lowerBound);
    }

    /** For use by Pointer<T> */
    void set(const T& v) {
        if (m_range == 0) {
            // No scaling necessary
            m_source.setValue(v);
            return;
        }
        //if clamping to inf at the borders is enabled then check to make sure the value should not be inf
        if (m_lowerLimitInf && (v <= m_low)) {
            m_source.setValue(T(-inf()));
        } else if (m_upperLimitInf && (v >= m_high)) {
            m_source.setValue(T(inf()));
        } else {

            double normalizer = m_range;
            double lowerBound = m_low;

            if (m_containsZero) {
                if (v > 0) {
                    normalizer = m_high;
                    lowerBound = 0;
                } else {
                    normalizer = m_low;
                    lowerBound = 0;
                }
            } 
            
            // Normalize the value to the range (0, 1)
            double x = (v - lowerBound) / normalizer;

            // Keep [0, 1] range but scale exponentially
            const double y = (::pow(m_base, x) - 1.0) / (m_base - 1.0);

            m_source.setValue(T(y * normalizer + lowerBound));

           
        }
    }

public:

    /** \brief Converts a pointer to a linear scale value on the range 
        [@a low, @a high] to a logarithic scale value on the same 
        range. 
        
        Note that the scale is spaced logarithmically between
        low and high. However, the transformed value is not the logarithm 
        of the value, so low = 0 is supported, but negative low values 
        will not yield a negative logarithmic scale. */
    static Pointer<T> wrap(const Pointer<T>& ptr, const T& low, const T& high, bool lowerLimitInf = false, bool upperLimitInf = false) {
        debugAssert(high >= low);
        Ptr p(new LogScaleAdapter(ptr, low, high, lowerLimitInf, upperLimitInf));
        return Pointer<T>(p, &LogScaleAdapter<T>::get, &LogScaleAdapter<T>::set);
    }

#undef MAX_LOG_BASE 
#undef LOG_BASE_SCALE
};


/** Slider.  See GuiWindow for an example of creating a slider. 

    <ul>
     <li> GEventType::GUI_ACTION when the thumb is released.
     <li> GEventType::GUI_CHANGE during scrolling.
     <li> GEventType::GUI_DOWN when the mouse is pressed down.
     <li> GEventType::GUI_UP when the mouse is released.
    </ul>

    The min/mix values are enforced on the GUI, but not on the value 
    if it is changed programmatically.
*/
template<class Value>
class GuiSlider : public _GuiSliderBase {
    friend class GuiPane;
    friend class GuiWindow;
protected:

    Pointer<Value>    m_value;
    Value             m_minValue;
    Value             m_maxValue;

    float floatValue() const {
        return (float)(*m_value - m_minValue) / (float)(m_maxValue - m_minValue);
    }

    void setFloatValue(float f) {
        *m_value = (Value)(f * (m_maxValue - m_minValue) + m_minValue);
    }

public:

    /** Public for GuiNumberBox.  Do not call 
    @param eventSource if null, set to this.
     */
    GuiSlider(GuiContainer* parent, 
              const GuiText& text, 
              const Pointer<Value>& value, 
              Value minValue, 
              Value maxValue, 
              bool horizontal, 
              GuiTheme::SliderScale scale,
              GuiControl*       eventSource           = NULL,
              bool              lowerInf              = false,
              bool              upperInf              = false) :
        _GuiSliderBase(parent, text, horizontal), 
        m_minValue(minValue), m_maxValue(maxValue) {

        debugAssertM(scale != GuiTheme::NO_SLIDER, "Cannot construct a slider with GuiTheme::NO_SLIDER");
        if (scale == GuiTheme::LOG_SLIDER) {
            m_value = LogScaleAdapter<Value>::wrap(value, minValue, maxValue, lowerInf, upperInf);
        } else {
            m_value = value;
        }

        if (eventSource != NULL) {
            m_eventSource = eventSource;
        }
    }

    Value minValue() const {
        return m_minValue;
    }

    Value maxValue() const {
        return m_maxValue;
    }

    void setRange(Value lo, Value hi) {
        m_minValue = min(lo, hi);
        m_maxValue = max(lo, hi);
    }

};

} // G3D

#endif
