/**
 \file GuiTheme.cpp
 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2008-01-01
 \edited  2016-05-31

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.

*/

#include "G3D/platform.h"
#include "G3D/Any.h"
#include "G3D/fileutils.h"
#include "G3D/Image.h"
#include "G3D/Image3.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "G3D/TextInput.h"
#include "G3D/WeakCache.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/GuiTheme.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Shader.h"

namespace G3D {
    
namespace _internal {    

Morph::Morph() : active(false), start(Rect2D::empty()), end(Rect2D::empty()) {}

void Morph::morphTo(const Rect2D& startPos, const Rect2D& endPos) {
    active = true;
    start  = startPos;
    end    = endPos;

    // Make the morph approximately constant velocity
    const float pixelsPerSecond = 1500;

    duration = max((double)0.12, (double)(start.center() - end.center()).length() / pixelsPerSecond);

    startTime = System::time();
}

}

weak_ptr<GuiTheme> GuiTheme::lastThemeLoaded;

GuiTheme::GuiTheme(const String& filename,
        const shared_ptr<GFont>&   fallbackFont,
        float               fallbackSize, 
        const Color4&       fallbackColor, 
        const Color4&       fallbackOutlineColor) : m_delayedStringsCount(0), m_delayedImagesCount(0), m_inRendering(false){

    alwaysAssertM(FileSystem::exists(filename), "Cannot find " + filename);
    //makeThemeFromSourceFiles("c:\\Users\\graphics\\g3d\\data-source\\guithemes\\osx-10.7", "osx-10.7_white.png", "osx-10.7_black.png", "osx-10.7.gtm.any", "osx-10.7.gtm");
    BinaryInput b(filename, G3D_LITTLE_ENDIAN, true);
    m_textStyle.font = fallbackFont;
    m_textStyle.size = fallbackSize;
    m_textStyle.color = fallbackColor;
    m_textStyle.outlineColor = fallbackOutlineColor;
    loadTheme(b);
}

namespace _internal {
     WeakCache< String, shared_ptr<GuiTheme> >& themeCache() {
        static WeakCache< String, shared_ptr<GuiTheme> > c;
        return c;
     }
}

shared_ptr<GuiTheme> GuiTheme::fromFile
(const String&  filename,
 shared_ptr<GFont>   fallbackFont,
 float               fallbackSize, 
 const Color4&       fallbackColor, 
 const Color4&       fallbackOutlineColor) {


    WeakCache<String, shared_ptr<GuiTheme> >& cache = _internal::themeCache();
    
    shared_ptr<GuiTheme> instance = cache[filename];
    if (isNull(instance)) {

        if (isNull(fallbackFont)) {
            fallbackFont = GFont::fromFile(System::findDataFile("arial.fnt"));
        }

        instance.reset(new GuiTheme(filename, fallbackFont, fallbackSize, fallbackColor, fallbackOutlineColor));
        cache.set(filename, instance);
    }

    lastThemeLoaded = instance;

    return instance;
}


void GuiTheme::loadTheme(BinaryInput& b) {
    String f = b.readString32();
    (void)f;
    // G3D 8.x and earlier had two nulls at the end of a string written to a file,
    // so we can't use String comparison on them.
    debugAssert(strcmp(f.c_str(), "G3D Skin File") == 0);

    float version = b.readFloat32();
    (void)version;
    debugAssert(fuzzyEq(version, 1.0f));

    // Read specification file
    String coords = b.readString32();
    TextInput::Settings settings;
    settings.sourceFileName = b.getFilename();    
    TextInput t(TextInput::FROM_STRING, coords, settings);

    // Load theme specification
    Any any;
    any.deserialize(t);
    loadCoords(any);

    // Read theme texture
    shared_ptr<G3D::Image> image = G3D::Image::fromBinaryInput(b);

    Texture::Preprocess p;
    p.computeMinMaxMean = false;
    
    // Load theme texture
    bool generateMipMaps = false;
    m_texture = Texture::fromPixelTransferBuffer(String("G3D::GuiTheme ") + b.getFilename(), image->toPixelTransferBuffer(), ImageFormat::RGBA8(), Texture::DIM_2D, generateMipMaps, p);
    glBindTexture(m_texture->openGLTextureTarget(), m_texture->openGLID()); {
        glTexParameteri(m_texture->openGLTextureTarget(), GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
        glTexParameteri(m_texture->openGLTextureTarget(), GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    } glBindTexture(m_texture->openGLTextureTarget(), GL_NONE);
 
}

void GuiTheme::loadCoords(const Any& any) {
    float version = any["format"];
    (void)version;
    debugAssertM(fuzzyEq(version, 1.0f), format("Only version 1.0 is supported (version = %f)", version));

    m_textStyle.load(any["font"]);
    m_disabledTextStyle = m_textStyle;
    m_disabledTextStyle.load(any["disabledFont"]);
    
    // Controls (all inherit the default text style and may override)
    m_checkBox.textStyle = m_textStyle;
    m_checkBox.disabledTextStyle = m_disabledTextStyle;
    m_checkBox.load(any["checkBox"]);
    
    m_radioButton.textStyle = m_textStyle;
    m_radioButton.disabledTextStyle = m_disabledTextStyle;
    m_radioButton.load(any["radioButton"]);

    m_button[NORMAL_BUTTON_STYLE].textStyle = m_textStyle;
    m_button[NORMAL_BUTTON_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_button[NORMAL_BUTTON_STYLE].load(any["button"]);

    m_button[TOOL_BUTTON_STYLE].textStyle = m_textStyle;
    m_button[TOOL_BUTTON_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_button[TOOL_BUTTON_STYLE].load(any["toolButton"]);

    m_closeButton.load(any["closeButton"]);

    const String& windowButtonStyle = any["windowButtonStyle"];
    m_osxWindowButtons = (windowButtonStyle == "osx");

    static String windowStyleName[WINDOW_STYLE_COUNT] = {"window", "toolWindow", "dialogWindow", "drawer", "menu", "panel", "partial_disappearing", "full_disappearing", "no"};
    debugAssert(windowStyleName[WINDOW_STYLE_COUNT - 1] == "no");
    // Skip the no-style window
    for (int i = 0; i < WINDOW_STYLE_COUNT - 3; ++i) {
        if (i != PANEL_WINDOW_STYLE) {
            m_window[i].textStyle = m_textStyle;
            m_window[i].defocusedTextStyle = m_textStyle;
            m_window[i].load(any[windowStyleName[i]]);
        }
    }
    m_window[PARTIAL_DISAPPEARING_STYLE] = m_window[NORMAL_WINDOW_STYLE];
    m_window[FULL_DISAPPEARING_STYLE] = m_window[NORMAL_WINDOW_STYLE];
    m_window[PANEL_WINDOW_STYLE] = m_window[MENU_WINDOW_STYLE];
    m_hSlider.textStyle = m_textStyle;
    m_hSlider.disabledTextStyle = m_disabledTextStyle;
    m_hSlider.load(any["horizontalSlider"]);

    m_pane[SIMPLE_PANE_STYLE].textStyle = m_textStyle;
    m_pane[SIMPLE_PANE_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_pane[SIMPLE_PANE_STYLE].load(any["simplePane"]);

    m_pane[ORNATE_PANE_STYLE].textStyle = m_textStyle;
    m_pane[ORNATE_PANE_STYLE].disabledTextStyle = m_disabledTextStyle;
    m_pane[ORNATE_PANE_STYLE].load(any["ornatePane"]);

    m_textBox.textStyle = m_textStyle;
    m_textBox.disabledTextStyle = m_disabledTextStyle;
    m_textBox.load(any["textBox"]);

    m_dropDownList.textStyle = m_textStyle;
    m_dropDownList.disabledTextStyle = m_disabledTextStyle;
    m_dropDownList.load(any["dropDownList"]);

    m_canvas.textStyle = m_textStyle;
    m_canvas.disabledTextStyle = m_disabledTextStyle;
    m_canvas.load(any["canvas"]);

    m_selection.load(any["selection"]); 

    m_vScrollBar.textStyle         = m_textStyle;
    m_vScrollBar.disabledTextStyle = m_disabledTextStyle;
    m_vScrollBar.load(any["verticalScrollBar"]);

    m_hScrollBar.textStyle         = m_textStyle;
    m_hScrollBar.disabledTextStyle = m_disabledTextStyle;
    m_hScrollBar.load(any["horizontalScrollBar"]);
}


void GuiTheme::beginRendering(RenderDevice* rd) {
    m_rd = rd;

    debugAssert(! m_inRendering);
    m_inRendering = true;

    
    rd->push2D();
    
    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    
    rd->setCullFace(CullFace::NONE);
}


void GuiTheme::pauseRendering() {
    drawDelayedParts();
    debugAssert(m_inRendering);

    m_rd->pushState();
}


void GuiTheme::resumeRendering() {
    m_rd->popState();
}


void GuiTheme::endRendering() {
    // Draw any remaining text
    drawDelayedParts();

    debugAssert(m_inRendering);

    debugAssertM( m_coordinateFrameStack.size() == 0, 
        "pushClientRect without matching popClientRect");

    m_rd->pop2D();
    m_inRendering = false;
    m_rd = NULL;
}


void GuiTheme::drawCheckable
    (const Checkable& control, const Rect2D& bounds, 
     bool enabled, bool focused, bool selected, const GuiText& text) {

    debugAssert(m_inRendering);
    control.render(m_rd, bounds, enabled, focused, selected, m_delayedRectangles);

    if (text.numElements() > 0) {
        const TextStyle& style = enabled ? control.textStyle : control.disabledTextStyle;

        for (int e = 0; e < text.numElements(); ++e) {
            const GuiText::Element& element = text.element(e);
            addDelayedText
            (element.font(style.font), 
             element.text(), 
             Vector2(control.width() + bounds.x0(), 
                     (bounds.y0() + bounds.y1()) / 2) + control.textOffset,
             element.size(style.size), 
             element.color(style.color), 
             element.outlineColor(style.outlineColor), 
             GFont::XALIGN_LEFT);
        }
    }
}


void GuiTheme::addDelayedText
(const GuiText&             text, 
 const GuiTheme::TextStyle& defaults,
 const Vector2&             position, 
 GFont::XAlign              xalign,
 GFont::YAlign              yalign,
 float                      wordWrapWidth) const {
    if (text.isIcon()) {
        addDelayedText(text.iconTexture(), text.iconSourceRect(), position, xalign, yalign);
    } else {
        if (text.numElements() == 0) {
            return;
        }
    
        const GuiText::Element& element = text.element(0);

        float size                  = element.size(defaults.size);
        const shared_ptr<GFont>& font      = element.font(defaults.font);
        const Color4& color         = element.color(defaults.color);
        const Color4& outlineColor  = element.outlineColor(defaults.outlineColor);
 
        addDelayedText(font, element.text(), position, size, color, outlineColor, xalign, yalign, wordWrapWidth);
    }
}


void GuiTheme::renderDropDownList
(const Rect2D&        initialBounds,
 bool                 enabled,
 bool                 focused,
 bool                 down,
 const GuiText&       contentText,
 const GuiText&       text,
 float                captionWidth) {

    // Dropdown list has a fixed height
    // Offset by left_caption_width
    const Rect2D& bounds = dropDownListToClickBounds(initialBounds, captionWidth);

    m_dropDownList.render(m_rd, bounds, enabled, focused, down, m_delayedRectangles);

    // Area in which text appears
    Rect2D clientArea = Rect2D::xywh(bounds.x0y0() + m_dropDownList.textPad.topLeft, 
                                     bounds.wh() - (m_dropDownList.textPad.bottomRight + m_dropDownList.textPad.topLeft));

    // Display cropped text
    // Draw inside the client area
    const_cast<GuiTheme*>(this)->pushClientRect(clientArea);
    addDelayedText(contentText, m_dropDownList.textStyle, Vector2(0, clientArea.height() / 2), GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);
    const_cast<GuiTheme*>(this)->popClientRect();

    addDelayedText(text, m_dropDownList.textStyle,
                   Vector2(initialBounds.x0(), (initialBounds.y0() + initialBounds.y1()) * 0.5f), 
                   GFont::XALIGN_LEFT);
}


void GuiTheme::renderSelection(const Rect2D& bounds)  {
    m_selection.render(m_rd, bounds, Vector2(0, 0), m_delayedRectangles);
}

void GuiTheme::renderTextBoxBorder(const Rect2D&      fullBounds, 
                                        bool         enabled, 
                                        bool         focused) {
    const Rect2D& bounds = textBoxToClickBounds(fullBounds, 0);
    m_textBox.render(m_rd, bounds, enabled, focused, m_delayedRectangles);
}

void GuiTheme::renderTextBox
    (const Rect2D&      fullBounds, 
     bool               enabled, 
     bool               focused, 
     const GuiText&     caption,
     float              captionWidth,
     const GuiText&     text, 
     const GuiText&     cursor, 
     int                cursorPosition,
     TextBoxStyle       style)  {

    const Rect2D& bounds = textBoxToClickBounds(fullBounds, captionWidth);

    // Render the background
    if (focused || (style != NO_BACKGROUND_UNLESS_FOCUSED_TEXT_BOX_STYLE)) {
        m_textBox.render(m_rd, bounds, enabled, focused, m_delayedRectangles);
    }

    alwaysAssertM(text.numElements() < 2, "Text box cannot contain GuiText with more than 1 element");

    
    // Area in which text appears
    Rect2D clientArea = Rect2D::xywh(bounds.x0y0() + m_textBox.textPad.topLeft, 
                                     bounds.wh() - (m_textBox.textPad.bottomRight + m_textBox.textPad.topLeft));


    // Draw inside the client area
    const_cast<GuiTheme*>(this)->pushClientRect(clientArea);
    String beforeCursor;
    Vector2 beforeBounds;
    float size          = m_textBox.contentStyle.size;
    shared_ptr<GFont> font     = m_textBox.contentStyle.font;
    Color4 color        = m_textBox.contentStyle.color;
    Color4 outlineColor = m_textBox.contentStyle.outlineColor;
    String all;
    if (text.numElements() == 1) {
        const GuiText::Element& element = text.element(0);

        // Compute pixel distance from left edge to cursor position
        all = element.text();
        beforeCursor   = all.substr(0, cursorPosition);
        size           = element.size(m_textBox.contentStyle.size);
        font           = element.font(m_textBox.contentStyle.font);
        color          = element.color(m_textBox.contentStyle.color);
        outlineColor   = element.outlineColor(m_textBox.contentStyle.outlineColor);
        beforeBounds   = font->bounds(beforeCursor, size);
    }

    // Slide the text backwards so that the cursor is visible
    float textOffset = -max(0.0f, beforeBounds.x - clientArea.width());

    if (! enabled) {
        // Dim disabled text color
        color.a *= 0.8f;
    }

    // Draw main text
    addDelayedText(font, all, Vector2(textOffset, clientArea.height() / 2), size, color, 
                   outlineColor, GFont::XALIGN_LEFT, GFont::YALIGN_CENTER);

    // Draw cursor
    if (focused) {
        addDelayedText(cursor,
                       m_textBox.contentStyle,
                       Vector2(textOffset + beforeBounds.x, clientArea.height() / 2), 
                       GFont::XALIGN_CENTER, GFont::YALIGN_CENTER);
    }
    
    const_cast<GuiTheme*>(this)->popClientRect();
    
    addDelayedText(caption, m_textBox.textStyle,
                   Vector2(fullBounds.x0(), (fullBounds.y0() + fullBounds.y1()) * 0.5f), 
                       GFont::XALIGN_LEFT);
}


Vector2 GuiTheme::bounds(const GuiText& text) const {
    if (text.isIcon()) {
        return text.iconSourceRect().wh();
    }

    if (text.numElements() == 0) {
        return Vector2::zero();
    }

    Vector2 b(0, 0);

    // TODO Elements: need to look at each element
    const GuiText::Element& element = text.element(0);
    const String& str  = element.text();

    // Loop twice, one for normal style and once for disabled style
    for (int i = 0; i < 2; ++i) {
        const TextStyle& style = (i == 0) ? m_textStyle : m_disabledTextStyle;

        const shared_ptr<GFont>&  font = element.font(style.font);
        int                size = (int)element.size(style.size);
        bool               outline = element.outlineColor(style.outlineColor).a > 0;

        Vector2 t = font->bounds(str, (float)size);
        if (outline) {
            t += Vector2(2, 2);
        }
        b = b.max(t);
    }
    return b;
}


void GuiTheme::renderCanvas
    (const Rect2D&      fullBounds, 
     bool               enabled, 
     bool               focused, 
     const GuiText&     caption,
     float              captionHeight) {

    const Rect2D& bounds = canvasToClickBounds(fullBounds, captionHeight);

    m_canvas.render(m_rd, bounds, enabled, focused, m_delayedRectangles);

    addDelayedText(caption, m_canvas.textStyle, Vector2(fullBounds.x0(), bounds.y0()), 
                   GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM);
}


void GuiTheme::renderCheckBox(const Rect2D& bounds, bool enabled, bool focused, bool selected, 
                             const GuiText& text) {
    drawCheckable(m_checkBox, bounds, enabled, focused, selected, text);
}


void GuiTheme::renderPane(const Rect2D& fullBounds, const GuiText& caption, PaneStyle paneStyle) {

    Rect2D paneRenderBounds = fullBounds;

    if (! caption.empty()) {
        const float pad = paneTopPadding(caption, paneStyle);
        paneRenderBounds = Rect2D::xyxy(fullBounds.x0(), fullBounds.y0() + pad, fullBounds.x1(), fullBounds.y1());
        addDelayedText(caption, m_pane[paneStyle].textStyle, Vector2(fullBounds.x0(), paneRenderBounds.y0()), 
                       GFont::XALIGN_LEFT, GFont::YALIGN_BOTTOM);
    }

    if (paneStyle != NO_PANE_STYLE) {
        m_pane[paneStyle].frame.render(m_rd, paneRenderBounds, Vector2::zero(), m_delayedRectangles);
    }
}


void GuiTheme::renderWindow(const Rect2D& bounds, bool focused, bool hasClose, 
                           bool closeIsDown, bool closeIsFocused, const GuiText& text, 
                           WindowStyle windowStyle) {
    drawWindow(m_window[windowStyle], bounds, focused, hasClose, closeIsDown, closeIsFocused, text);
}


Rect2D GuiTheme::windowToCloseButtonBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return closeButtonBounds(m_window[windowStyle], bounds);
}


Rect2D GuiTheme::closeButtonBounds(const Window& window, const Rect2D& bounds) const {
    // If the close button is larger
    // than the title area, draw it half size (e.g., for tool
    // windows)
    float titleHeight = window.borderThickness.topLeft.y;
    float scale = 1.0f;
    if (titleHeight < m_closeButton.base.height()) {
        scale = 0.5f;
    }
    
    // Position button
    Vector2 center; 
    if (m_osxWindowButtons) {
        center.x = bounds.x0() + max(window.borderThickness.topLeft.x, window.borderThickness.topLeft.y * 0.25f) * scale + scale * m_closeButton.base.width() / 2;
    } else {
        center.x = bounds.x1() - max(window.borderThickness.bottomRight.x, window.borderThickness.topLeft.y * 0.25f) * scale - scale * m_closeButton.base.width() / 2;
    }
    center.y = bounds.y0() + window.borderThickness.topLeft.y / 2;
    
    // Draw button
    Vector2 wh = m_closeButton.base.wh() * scale;
    Rect2D vertex = Rect2D::xywh(center - wh / 2, wh);
    
    return vertex;
}


void GuiTheme::drawWindow(const Window& window, const Rect2D& bounds, 
                         bool focused, bool hasClose, bool closeIsDown, bool closeIsFocused,                          
                         const GuiText& text) {
    // Update any pending text since the window may overlap another window
    drawDelayedParts();

    window.render(m_rd, bounds, focused, m_delayedRectangles);

    if (hasClose) {

        const Rect2D& vertex = closeButtonBounds(window, bounds);

        Vector2 offset;
        if (focused) {
            if (closeIsFocused) {
                if (closeIsDown) {
                    offset = m_closeButton.focusedDown;
                } else {
                    offset = m_closeButton.focusedUp;
                }
            } else {
                offset = m_closeButton.defocused;
            }
        } else {
            offset = m_closeButton.windowDefocused;
        }

        drawRect(vertex, m_closeButton.base + offset, m_rd, m_delayedRectangles);
    }
    
    if (window.borderThickness.topLeft.y > 4) {
        const TextStyle& style = focused ? window.textStyle : window.defocusedTextStyle;

        addDelayedText(text, style,
                       Vector2(bounds.center().x, bounds.y0() + window.borderThickness.topLeft.y * 0.5f), 
                       GFont::XALIGN_CENTER, 
                       GFont::YALIGN_CENTER);
    }
}


Rect2D GuiTheme::horizontalSliderToSliderBounds(const Rect2D& bounds, float captionWidth) const {
    return Rect2D::xywh(bounds.x0() + captionWidth, bounds.y0(), bounds.width() - captionWidth, bounds.height());
}


Rect2D GuiTheme::horizontalSliderToThumbBounds(const Rect2D& bounds, float pos, float captionWidth) const {
    return m_hSlider.thumbBounds(horizontalSliderToSliderBounds(bounds, captionWidth), pos);
}


Rect2D GuiTheme::horizontalSliderToTrackBounds(const Rect2D& bounds, float captionWidth) const {
    return m_hSlider.trackBounds(horizontalSliderToSliderBounds(bounds, captionWidth));
}


Rect2D GuiTheme::verticalScrollBarToThumbBounds(const Rect2D& bounds, float pos, float scale) const {
    return m_vScrollBar.thumbBounds(bounds, pos, scale);
}


Rect2D GuiTheme::verticalScrollBarToBarBounds(const Rect2D& bounds) const {
    return m_vScrollBar.barBounds(bounds);
}

Rect2D GuiTheme::horizontalScrollBarToThumbBounds(const Rect2D& bounds, float pos, float scale) const {
    return m_hScrollBar.thumbBounds(bounds, pos, scale);
}

Rect2D GuiTheme::horizontalScrollBarToBarBounds(const Rect2D& bounds) const {
    return m_hScrollBar.barBounds(bounds);
}


Rect2D GuiTheme::windowToTitleBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return Rect2D::xywh(bounds.x0y0(), Vector2(bounds.width(), m_window[windowStyle].borderThickness.topLeft.y));
}


Rect2D GuiTheme::windowToClientBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return Rect2D::xywh(bounds.x0y0() + m_window[windowStyle].netClientPad.topLeft, 
                        bounds.wh() - m_window[windowStyle].netClientPad.wh());
}


Rect2D GuiTheme::clientToWindowBounds(const Rect2D& bounds, WindowStyle windowStyle) const {
    return Rect2D::xywh(bounds.x0y0() - m_window[windowStyle].netClientPad.topLeft, 
                        bounds.wh() + m_window[windowStyle].netClientPad.wh());
}


Rect2D GuiTheme::textBoxToClickBounds(const Rect2D& bounds, float captionWidth) const {
    return Rect2D::xyxy(bounds.x0() + captionWidth, bounds.y0(), bounds.x1(), bounds.y1());
}


Rect2D GuiTheme::canvasToClickBounds(const Rect2D& bounds, float captionHeight) const {
    // Canvas does not receive indent; its caption goes on top
    return Rect2D::xyxy(bounds.x0(), bounds.y0() + captionHeight, bounds.x1(), bounds.y1());
}


Rect2D GuiTheme::canvasToClientBounds(const Rect2D& bounds, float captionHeight) const {
    Rect2D r = canvasToClickBounds(bounds, captionHeight);

    return Rect2D::xyxy(r.x0y0() + m_canvas.pad.topLeft, r.x1y1() - m_canvas.pad.bottomRight);
}


Rect2D GuiTheme::dropDownListToClickBounds(const Rect2D& bounds, float captionWidth) const {
    // Note: if you change these bounds to not be the same as the
    // rendering bounds for the control itself then update
    // renderDropDownList to not call dropDownListToClickBounds.
    float h = m_dropDownList.base.left.height();
    return Rect2D::xywh(bounds.x0() + captionWidth,
                        bounds.center().y - h / 2,
                        bounds.width() - captionWidth,
                        h);
}


void GuiTheme::renderRadioButton(const Rect2D& bounds, bool enabled, bool focused, bool selected, const GuiText& text) {
    drawCheckable(m_radioButton, bounds, enabled, focused, selected, text);
}


Vector2 GuiTheme::minButtonSize(const GuiText& text, ButtonStyle buttonStyle) const {
    Vector2 textBounds = bounds(text);

    Vector2 borderPadding = 
        m_button[buttonStyle].base.centerLeft.source.wh() + 
        m_button[buttonStyle].base.centerRight.source.wh();

    return textBounds + borderPadding;
}


void GuiTheme::renderButton(const Rect2D& bounds, bool enabled, bool focused, 
                           bool pushed, const GuiText& text, ButtonStyle buttonStyle) {

    debugAssert(m_inRendering);
    if (buttonStyle != NO_BUTTON_STYLE) {
        m_button[buttonStyle].render(m_rd, bounds, enabled, focused, pushed, m_delayedRectangles);
    }

    const TextStyle& style = 
        enabled ? 
        m_button[buttonStyle].textStyle : 
        m_button[buttonStyle].disabledTextStyle;
    
    addDelayedText(text, style,
                   bounds.center() + m_button[buttonStyle].textOffset,
                   GFont::XALIGN_CENTER);
}

void GuiTheme::renderButtonBorder(const Rect2D& bounds, bool focused, ButtonStyle buttonStyle) {

    debugAssert(m_inRendering);
    if (buttonStyle != NO_BUTTON_STYLE) {
        m_button[buttonStyle].render(m_rd, bounds, true, focused, false, m_delayedRectangles);
    }
}

void GuiTheme::renderVerticalScrollBar
(const Rect2D& bounds, 
 float pos, 
 float scale, bool focused) {
    
    debugAssert(m_inRendering);
    m_vScrollBar.render
        (m_rd, 
         bounds,
         pos, scale, focused, m_delayedRectangles);
}

void GuiTheme::renderHorizontalScrollBar
(const Rect2D& bounds, 
 float pos, 
 float scale, bool focused) {
    
    debugAssert(m_inRendering);
    m_hScrollBar.render
        (m_rd, 
         bounds,
         pos, scale, focused, m_delayedRectangles);
}

void GuiTheme::renderHorizontalSlider
(const Rect2D& bounds, 
 float pos, 
 bool enabled, 
 bool focused, 
 const GuiText& text,
 float captionWidth) {
    
    debugAssert(m_inRendering);
    m_hSlider.render
        (m_rd, 
         horizontalSliderToSliderBounds(bounds, captionWidth),
         pos, enabled, focused, m_delayedRectangles);

    const TextStyle& style = enabled ? m_hSlider.textStyle : m_hSlider.disabledTextStyle;

    addDelayedText(text, style,
                   Vector2(bounds.x0(), (bounds.y0() + bounds.y1()) * 0.5f), 
                   GFont::XALIGN_LEFT);
}


void GuiTheme::renderLabel(const Rect2D& bounds, const GuiText& text, GFont::XAlign xalign, GFont::YAlign yalign, bool enabled, bool wordWrap) const {
    debugAssert(m_inRendering);

    if (text.isIcon() || (text.numElements() > 0)) {
        Vector2 pos;

        switch (xalign) {
        case GFont::XALIGN_LEFT:
            pos.x = bounds.x0();
            break;
        case GFont::XALIGN_CENTER:
            pos.x = bounds.center().x;
            break;
        case GFont::XALIGN_RIGHT:
            pos.x = bounds.x1();
            break;
        }

        switch (yalign) {
        case GFont::YALIGN_TOP:
            pos.y = bounds.y0();
            break;
        case GFont::YALIGN_CENTER:
            pos.y = bounds.center().y;
            break;
        case GFont::YALIGN_BOTTOM:
        case GFont::YALIGN_BASELINE:
            pos.y = bounds.y1();
            break;
        }

        const TextStyle& style = enabled ? m_textStyle : m_disabledTextStyle;

        addDelayedText(text, style, pos, xalign, yalign, wordWrap ? bounds.width() : finf());
    }
}


void GuiTheme::drawDelayedRectangles() const {
    if (m_delayedRectangles.size() > 0) {
        
            
        const int N = m_delayedRectangles.size();
        const Vector2 scale(1.0f / float(m_texture->width()), 1.0f / float(m_texture->height()));
        
        const shared_ptr<VertexBuffer>& vb = 
            VertexBuffer::create(N * 8 * sizeof(Vector2) + 16, VertexBuffer::WRITE_EVERY_FRAME);

        IndexStream     indexStream(int(0), N * 6, VertexBuffer::create(N * 6 * sizeof(int), VertexBuffer::WRITE_EVERY_FRAME));

        AttributeArray  interleavedArray(N * 8 * sizeof(Vector2), vb);
        AttributeArray  texCoordArray(Vector2(), N * 4, interleavedArray, 0,                  sizeof(Vector2) * 2);
        AttributeArray  positionArray(Vector2(), N * 4, interleavedArray, sizeof(Vector2),    sizeof(Vector2) * 2);

        

        Vector2* dst = (Vector2*)interleavedArray.mapBuffer(GL_WRITE_ONLY);
        debugAssert(notNull(dst));
        

        for(int i = 0; i < N; ++i) {
            const Rect2D& tcRect    = m_delayedRectangles[i].texCoordRect;
            const Rect2D& vRect     = m_delayedRectangles[i].positionRect;
            const int offset = i * 8;
            dst[offset + 0] = tcRect.x0y0() * scale;
            dst[offset + 1] = vRect.x0y0();

            dst[offset + 2] = tcRect.x0y1() * scale;
            dst[offset + 3] = vRect.x0y1();

            dst[offset + 4] = tcRect.x1y1() * scale;
            dst[offset + 5] = vRect.x1y1();

            dst[offset + 6] = tcRect.x1y0() * scale;
            dst[offset + 7] = vRect.x1y0();
        }
        interleavedArray.unmapBuffer();
        dst = NULL;

        int* index = (int*)indexStream.mapBuffer(GL_WRITE_ONLY);
        debugAssert(notNull(index));
        for(int i = 0; i < N; ++i) {
            const int offset = i * 4;
            const int j = i * 6;
            index[j + 0] = offset;
            index[j + 1] = offset + 1;
            index[j + 2] = offset + 2;

            index[j + 3] = offset;
            index[j + 4] = offset + 2;
            index[j + 5] = offset + 3;
        }
        indexStream.unmapBuffer();
        index = NULL;

        

        Args args; 
        args.enableG3DArgs(false); // Reduce CPU overhead for uniform bind

        args.setUniform("textureMap", m_texture, Sampler::video());
        args.setUniform("g3d_ObjectToScreenMatrixTranspose", RenderDevice::current->objectToScreenMatrix().transpose()); //and explicitly pass what we need

        args.setPrimitiveType(PrimitiveType::TRIANGLES);
        args.setAttributeArray("g3d_Vertex",    positionArray);
        args.setAttributeArray("g3d_TexCoord0", texCoordArray);
        args.setIndexStream(indexStream);

        // Avoid the overhead of Profiler events by explicitly launching the shader
        static const shared_ptr<Shader> shader = Shader::getShaderFromPattern("GuiTheme_render.*");
        RenderDevice::current->apply(shader, args);

        const_cast<Array<Rectangle>&>(m_delayedRectangles).fastClear();
    }
}

void GuiTheme::drawDelayedParts() const {
    drawDelayedRectangles();
    drawDelayedStrings();
    drawDelayedImages();
}


void GuiTheme::drawDelayedImages() const {

    if (m_delayedImagesCount > 0) {

        for (Table<shared_ptr<Texture>, Array<Image> >::Iterator it = m_delayedImages.begin(); it.isValid(); ++it) {
            const shared_ptr<Texture>& texture    = it->key;
            const Array<Image>& imageArray = it->value;
            
            if (imageArray.size() > 0) {
                const Vector2 scale(1.0f / texture->width(), 1.0f / texture->height());
				const int N = imageArray.size();
                shared_ptr<VertexBuffer> vb = VertexBuffer::create(N * 8 * sizeof(Vector2) + 8, VertexBuffer::WRITE_EVERY_FRAME);
                AttributeArray interleavedArray = AttributeArray(N * 8 * sizeof(Vector2), vb);
                Vector2 ignore;
                AttributeArray  texCoordArray   = AttributeArray(ignore, N * 4, interleavedArray, 0,                  sizeof(Vector2) * 2);
                AttributeArray  positionArray   = AttributeArray(ignore, N * 4, interleavedArray, sizeof(Vector2),    sizeof(Vector2) * 2);

				Vector2* dst = (Vector2*)interleavedArray.mapBuffer(GL_WRITE_ONLY);
                // Render the images that use this texture font
                for (int t = 0; t < imageArray.size(); ++t) {
                    const Image& im = imageArray[t];
                    Point2 pos0 = im.position;

                    switch (im.xAlign) {
                    case GFont::XALIGN_LEFT:
                        break;

                    case GFont::XALIGN_CENTER:
                        pos0.x -= im.srcRect.width() / 2;
                        break;

                    case GFont::XALIGN_RIGHT:
                        pos0.x -= im.srcRect.width();
                        break;
                    }

                    switch (im.yAlign) {
                    case GFont::YALIGN_TOP:
                        break;

                    case GFont::YALIGN_CENTER:
                        pos0.y -= im.srcRect.height() / 2;
                        break;

                    case GFont::YALIGN_BOTTOM:
                    case GFont::YALIGN_BASELINE:
                        pos0.y -= im.srcRect.height();
                        break;
                    }
                    const Rect2D& tcRect    = im.srcRect * scale;
                    const Rect2D& vRect     = Rect2D::xywh(pos0.x, pos0.y, im.srcRect.width(), im.srcRect.height());
                    int offset = t * 8;
                    dst[offset + 0] = tcRect.x0y0();
                    dst[offset + 1] = vRect.x0y0();
                    dst[offset + 2] = tcRect.x0y1();
                    dst[offset + 3] = vRect.x0y1();
                    dst[offset + 4] = tcRect.x1y1();
                    dst[offset + 5] = vRect.x1y1();
                    dst[offset + 6] = tcRect.x1y0();
                    dst[offset + 7] = vRect.x1y0();     
                }
				interleavedArray.unmapBuffer();
				dst = NULL;

				IndexStream     indexStream(int(0), N * 6, VertexBuffer::create(N * 6 * sizeof(int), VertexBuffer::WRITE_EVERY_FRAME));
				int* index = (int*)indexStream.mapBuffer(GL_WRITE_ONLY);
				debugAssert(notNull(index));
				for(int i = 0; i < N; ++i) {
					const int offset = i * 4;
					const int j = i * 6;
					index[j + 0] = offset;
					index[j + 1] = offset + 1;
					index[j + 2] = offset + 2;

					index[j + 3] = offset;
					index[j + 4] = offset + 2;
					index[j + 5] = offset + 3;
				}
				indexStream.unmapBuffer();
				index = NULL;
				
                Args args;
                args.setPrimitiveType(PrimitiveType::TRIANGLES);
                args.setAttributeArray("g3d_Vertex", positionArray);
                args.setAttributeArray("g3d_TexCoord0", texCoordArray);
				args.setIndexStream(indexStream);

                args.enableG3DArgs(false); // Lower CPU overhead for uniform bind
                args.setUniform("textureMap", texture, Sampler::video());
                // Avoid the overhead of Profiler events...
                args.setUniform("g3d_ObjectToScreenMatrixTranspose", RenderDevice::current->objectToScreenMatrix().transpose()); //and explicitly pass what we need
                static const shared_ptr<Shader> shader = Shader::getShaderFromPattern("GuiTheme_render.*");
	            RenderDevice::current->apply(shader, args);

                // Fast clear to avoid memory allocation and deallocation
                const_cast<Array<Image>&>(imageArray).fastClear();
            }
        }

        // Reset the count
        const_cast<GuiTheme*>(this)->m_delayedImagesCount = 0;
    }
}


void GuiTheme::drawDelayedStrings() const {
    if (m_delayedStringsCount > 0) {
        for (Table<shared_ptr<GFont>, Array<Text> >::Iterator it = m_delayedStrings.begin(); it.isValid(); ++it) {
            const shared_ptr<GFont>& thisFont = it->key;
            const Array<Text>& label = it->value;
            
            if (label.size() > 0) {
                // Load this font

                static Array<GFont::CPUCharVertex> charVertexArray;
                static Array<int> indexArray;
                charVertexArray.fastClear();
                indexArray.fastClear();

                // Render the text in this font
                for (int t = 0; t < label.size(); ++t) {
                    const Text& text = label[t];
                    thisFont->appendToCharVertexArrayWordWrap(charVertexArray, indexArray, m_rd, text.wrapWidth, text.text, text.position, text.size, text.color, 
                                            text.outlineColor, text.xAlign, text.yAlign);
                }
                thisFont->renderCharVertexArray(m_rd, charVertexArray, indexArray);

                // Fast clear to avoid memory allocation and deallocation
                const_cast<Array<Text>&>(label).fastClear();                
            }
        }

        // Reset the count
        const_cast<GuiTheme*>(this)->m_delayedStringsCount = 0;
    }
}


void GuiTheme::addDelayedText(const shared_ptr<Texture>& t, const Rect2D& srcRectPixels, const Point2& position, 
                    GFont::XAlign xalign, GFont::YAlign yalign) const {
    GuiTheme* me = const_cast<GuiTheme*>(this);

    bool created = false;    
    Image& im = me->m_delayedImages.getCreate(t, created).next();
    im.position = position;
    im.srcRect  = srcRectPixels;
    im.xAlign   = xalign;
    im.yAlign   = yalign;
    ++me->m_delayedImagesCount;
}
    

void GuiTheme::addDelayedText
(shared_ptr<GFont>  font,
 const String&      label, 
 const Vector2&     position,
 float              size, 
 const Color4&      color, 
 const Color4&      outlineColor,
 GFont::XAlign      xalign,
 GFont::YAlign      yalign,
 float              wordWrapWidth) const {

    if (isNull(font)) {
        font = m_textStyle.font;
        debugAssertM(font, "Must set default font first.");
    }
    
    if (size < 0) {
        size = m_textStyle.size;
    }

    GuiTheme* me = const_cast<GuiTheme*>(this);
    
    ++(me->m_delayedStringsCount);

    bool created = false;    
    Text& text = me->m_delayedStrings.getCreate(font, created).next();
    text.text       = label;
    text.position   = position;
    text.xAlign     = xalign;
    text.yAlign     = yalign;
    text.size       = size;
    text.wrapWidth  = wordWrapWidth;

    if (color.a < 0) {
        text.color = m_textStyle.color;
    } else {
        text.color = color;
    }

    if (outlineColor.a < 0) {
        text.outlineColor = m_textStyle.outlineColor;
    } else {
        text.outlineColor = outlineColor;
    }
}


void GuiTheme::drawRect(const Rect2D& vertex, const Rect2D& texCoord, RenderDevice* rd, Array<Rectangle>& delayedRectangles) {
    delayedRectangles.append(Rectangle(vertex,texCoord));
}


float GuiTheme::paneTopPadding(const GuiText& caption, PaneStyle paneStyle) const {
    if (caption.empty()) {
        return 0.0f;
    } else if (caption.isIcon()) {
        return caption.iconSourceRect().height();
    } else {
        // Space for text
        if (m_pane[paneStyle].textStyle.size >= 0) {
            return m_pane[paneStyle].textStyle.size;
        } else {
            return m_textStyle.size;
        }
    }
}


Rect2D GuiTheme::paneToClientBounds(const Rect2D& bounds, const GuiText& caption, PaneStyle paneStyle) const {
    const Vector2 captionSpace(0, paneTopPadding(caption, paneStyle));

    debugAssert(! bounds.isEmpty());
    debugAssert(captionSpace.isFinite() && m_pane[paneStyle].clientPad.topLeft.isFinite() && m_pane[paneStyle].clientPad.wh().isFinite());

    return Rect2D::xywh(bounds.x0y0() + m_pane[paneStyle].clientPad.topLeft + captionSpace,
                        bounds.wh() - m_pane[paneStyle].clientPad.wh() - captionSpace);
}

Rect2D GuiTheme::clientToPaneBounds(const Rect2D& bounds, const GuiText& caption, PaneStyle paneStyle) const {
    const Vector2 captionSpace(0, paneTopPadding(caption, paneStyle));
    return Rect2D::xywh(bounds.x0y0() - m_pane[paneStyle].clientPad.topLeft - captionSpace,
                        bounds.wh() + m_pane[paneStyle].clientPad.wh() + captionSpace);
}


void GuiTheme::makeThemeFromSourceFiles(
  const String& sourceDir,
  const String& whiteName,
  const String& blackName,
  const String& coordsFile,
  const String& destFile) {

    Image3Ref white = Image3::fromFile(FilePath::concat(sourceDir, whiteName));
    Image3Ref black = Image3::fromFile(FilePath::concat(sourceDir, blackName));

    shared_ptr<G3D::Image> outputImage = G3D::Image::create(white->width(), white->height(), ImageFormat::RGBA8());

    for (int y = 0; y < outputImage->height(); ++y) {
        for (int x = 0; x < outputImage->width(); ++x) {
            const Color3& U = white->get(x, y);
            const Color3& V = black->get(x, y);

            // U = F * a + (1-a) * 1
            // V = F * a + (1-a) * 0
            //
            // F * a = V
            // a = 1 - (U - V)
            
            Color3 diff = U - V;
            float a = clamp(1.0f - diff.average(), 0.0f, 1.0f);

            Color3 base = V;
            if (a > 0) {
                base = base / a;
            }
            if (a != 0.0f)
            {
                printf("hi");
            }
            outputImage->set(Point2int32(x, y), Color4(base, a));
        }
    }
    
    String coords = readWholeFile(FilePath::concat(sourceDir, coordsFile));

    BinaryOutput b(destFile, G3D_LITTLE_ENDIAN);
    
    b.writeString32("G3D Skin File");
    b.writeFloat32(1.0f);
    b.writeString32(coords);
    outputImage->serialize(b, G3D::Image::TARGA);

    b.compress();
    b.commit();
}


void GuiTheme::pushClientRect(const Rect2D& r) {
    debugAssert(! r.isEmpty());
    debugAssert(m_inRendering);

    // Must draw old text since we don't keep track of which text 
    // goes with which client rect.
    drawDelayedParts();

    const CoordinateFrame& oldMatrix = m_rd->objectToWorldMatrix();
    m_coordinateFrameStack.append(oldMatrix);
    const Rect2D& oldRect = m_rd->clip2D();
    m_scissorStack.append(oldRect);

    Rect2D newRect = r + oldMatrix.translation.xy();
    const Rect2D& afterIntersectRect = oldRect.intersect(newRect);
    m_rd->setClip2D(afterIntersectRect);

    const CoordinateFrame& newMatrix = oldMatrix * CoordinateFrame(Vector3(r.x0y0(), 0));
    m_rd->setObjectToWorldMatrix(newMatrix);
}


void GuiTheme::popClientRect() {
    debugAssertM( m_coordinateFrameStack.size() > 0, 
        "popClientRect without matching pushClientRect");

    drawDelayedParts();

    m_rd->setObjectToWorldMatrix(m_coordinateFrameStack.pop());
    m_rd->setClip2D(m_scissorStack.pop());
}

/////////////////////////////////////////////

void GuiTheme::Pane::load(const Any& any) {
    any.verifyName("Pane");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    frame.load(any["frame"]);
    clientPad.load(any["clientPad"]);
}

///////////////////////////////////////////////
void GuiTheme::VScrollBar::load(const Any& any) {
    any.verifyName("VScrollBar");
    topArrow    = any["topButton"];
    bottomArrow = any["bottomButton"];
    if(any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }
    bar.load(any["bar"]);
    thumb.load(any["thumb"]);
    enabled.load(any["enabled"]);

    width = topArrow.height(); 
}


void GuiTheme::VScrollBar::Bar::load(const Any& any) {
    any.verifyName("VScrollBar::Bar");
    base.load(any["base"]);
}


void GuiTheme::VScrollBar::Thumb::load(const Any& any) {
    any.verifyName("VScrollBar::Thumb");
    base.load(any["base"]);
    thumbEnabled.load(any["thumbEnabled"]);
}


void GuiTheme::VScrollBar::Focus::load(const Any& any) {
    any.verifyName("VScrollBar::Focus");
    focused = any["focused"];
    defocused = any["defocused"];
}


Rect2D GuiTheme::VScrollBar::barBounds(const Rect2D scrollBounds) const {
     return Rect2D::xywh(scrollBounds.x0y0() + Vector2(0, topArrow.height()),
         scrollBounds.wh()- Vector2(0, topArrow.height() + bottomArrow.height()));
}


Rect2D GuiTheme::VScrollBar::thumbBounds(const Rect2D barbounds, float pos, float scale) const {
     const float thumbW = max<float>(barbounds.height() / (scale + 1), MIN_THUMB_HEIGHT);
     if (thumbW == MIN_THUMB_HEIGHT) {
        return Rect2D::xywh(barbounds.x0y0() + Vector2(0, pos * (barbounds.height() - MIN_THUMB_HEIGHT) / (scale)), 
                Vector2(barbounds.width(), thumbW));
     }
     return Rect2D::xywh(barbounds.x0y0() + Vector2(0, pos * thumbW), 
                    Vector2(barbounds.width(), thumbW));
}


void GuiTheme::VScrollBar::render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, float scale, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    Vector2 focus(focused ? enabled.focused : enabled.defocused);
    drawRect(Rect2D::xywh(bounds.x0y0(), topArrow.wh()), topArrow + focus, rd, delayedRectangles);
    drawRect(Rect2D::xywh(bounds.x0y1() - Vector2(0, bottomArrow.height()), bottomArrow.wh()), bottomArrow + focus, rd, delayedRectangles);
    
    const Rect2D& barbounds = barBounds(bounds);

    // Draw the bar:
    bar.base.render(rd, barbounds, focus, delayedRectangles);

    // Draw the thumb:
    Vector2 thumbFocus(focused ? thumb.thumbEnabled.focused : thumb.thumbEnabled.defocused);
    thumb.base.render(rd, thumbBounds(barbounds, thumbPos, scale), thumbFocus, delayedRectangles);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GuiTheme::HScrollBar::load(const Any& any) {
    any.verifyName("HScrollBar");
    leftArrow    = any["leftButton"];
    rightArrow = any["rightButton"];
    if(any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }
    bar.load(any["bar"]);
    thumb.load(any["thumb"]);
    enabled.load(any["enabled"]);
}


void GuiTheme::HScrollBar::Bar::load(const Any& any) {
    any.verifyName("HScrollBar::Bar");
    base.load(any["base"]);
}


void GuiTheme::HScrollBar::Thumb::load(const Any& any) {
    any.verifyName("HScrollBar::Thumb");
    base.load(any["base"]);
    thumbEnabled.load(any["thumbEnabled"]);
}


void GuiTheme::HScrollBar::Focus::load(const Any& any) {
    any.verifyName("HScrollBar::Focus");
    focused = any["focused"];
    defocused = any["defocused"];
}


Rect2D GuiTheme::HScrollBar::barBounds(const Rect2D scrollBounds) const {
     return Rect2D::xywh(scrollBounds.x0y0() + Vector2(leftArrow.width(), 0.0f),
         scrollBounds.wh() - Vector2(leftArrow.width() + rightArrow.width(), 0.0f));
}


Rect2D GuiTheme::HScrollBar::thumbBounds(const Rect2D barbounds, float pos, float scale) const {
     const float thumbW = max<float>(barbounds.width() / (scale + 1), MIN_THUMB_WIDTH);
     if (thumbW == MIN_THUMB_WIDTH) {
        return Rect2D::xywh(barbounds.x0y0() + Vector2(pos * (barbounds.width() - MIN_THUMB_WIDTH) / (scale), 0), 
                Vector2(thumbW, barbounds.height()));
     }
     return Rect2D::xywh(barbounds.x0y0() + Vector2(pos * thumbW, 0), 
                    Vector2(thumbW, barbounds.height()));
}


void GuiTheme::HScrollBar::render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, float scale, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    Vector2 focus(focused ? enabled.focused : enabled.defocused);
    drawRect(Rect2D::xywh(bounds.x0y0(), leftArrow.wh()), leftArrow + focus, rd, delayedRectangles);
    drawRect(Rect2D::xywh(bounds.x1y0() - Vector2(rightArrow.width(), 0), rightArrow.wh()), rightArrow + focus, rd, delayedRectangles);
    
    const Rect2D& barbounds = barBounds(bounds);

    // Draw the bar:
    bar.base.render(rd, barbounds, focus, delayedRectangles);

    // Draw the thumb:
    Vector2 thumbFocus(focused ? thumb.thumbEnabled.focused : thumb.thumbEnabled.defocused);
    thumb.base.render(rd, thumbBounds(barbounds, thumbPos, scale), thumbFocus, delayedRectangles);
}

///////////////////////////////////////////////
void GuiTheme::HSlider::load(const Any& any) {
    any.verifyName("HSlider");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    bar.load(any["bar"]);
    thumb.load(any["thumb"]);
}


void GuiTheme::HSlider::Bar::load(const Any& any) {
    any.verifyName("HSlider::Bar");
    base.load(any["base"]);
    enabled = any["enabled"];
    disabled = any["disabled"];
}


void GuiTheme::HSlider::Thumb::load(const Any& any) {
    any.verifyName("HSlider::Thumb");
    base = any["base"];
    enabled.load(any["enabled"]);
    disabled = any["disabled"];
}


void GuiTheme::HSlider::Thumb::Focus::load(const Any& any) {
    any.verifyName("HSlider::Thumb::Focus");
    focused = any["focused"];
    defocused = any["defocused"];
}


void GuiTheme::HSlider::render(RenderDevice* rd, const Rect2D& bounds, float thumbPos, 
                              bool _enabled, bool _focused, Array<GuiTheme::Rectangle>& delayedRectangles) const {

    Rect2D barBounds = trackBounds(bounds);

    // Draw the bar:
    bar.base.render(rd, barBounds, _enabled ? bar.enabled : bar.disabled, delayedRectangles);

    // Draw the thumb:
    Vector2 offset = _enabled ? 
        (_focused ? thumb.enabled.focused : thumb.enabled.defocused) : 
        thumb.disabled;

    drawRect(thumbBounds(bounds, thumbPos), thumb.base + offset, rd, delayedRectangles);
}


Rect2D GuiTheme::HSlider::trackBounds(const Rect2D& sliderBounds) const {
    return
        Rect2D::xywh(sliderBounds.x0(), sliderBounds.center().y - bar.base.height() * 0.5f, 
                 sliderBounds.width(), bar.base.height());
}


Rect2D GuiTheme::HSlider::thumbBounds(const Rect2D& sliderBounds, float pos) const {

    float halfWidth = thumb.base.width() * 0.5f;

    Vector2 thumbCenter(sliderBounds.x0() + halfWidth + (sliderBounds.width() - thumb.base.width()) * clamp(pos, 0.0f, 1.0f), 
                        sliderBounds.center().y);

    return Rect2D::xywh(thumbCenter - Vector2(halfWidth, thumb.base.height() * 0.5f), thumb.base.wh());
}
    
//////////////////////////////////////////////////////////////////////////////

void GuiTheme::WindowButton::load(const Any& any) {
    any.verifyName("WindowButton");
    base = any["base"];
    
    focusedDown = any["focusedDown"];
    focusedUp = any["focusedUp"];

    defocused = any["defocused"];
    windowDefocused = any["windowDefocused"];
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Window::load(const Any& any) {
    any.verifyName("Window");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }

    defocusedTextStyle = textStyle;
    if (any.containsKey("defocusedFont")) {
        defocusedTextStyle.load(any["defocusedFont"]);
    }

    base.load(any["base"]);
    borderPad.load(any["borderPad"]);
    borderThickness.load(any["borderThickness"]);
    Pad clientPad;
    clientPad.load(any["clientPad"]);
    netClientPad.topLeft = borderThickness.topLeft + clientPad.topLeft; 
    netClientPad.bottomRight = borderThickness.bottomRight + clientPad.bottomRight; 

    focused = any["focused"];
    defocused = any["defocused"];
    resizeFrameThickness = any["WindowFrameThickness"];
}


void GuiTheme::Pad::load(const Any& any) {
    any.verifyName("Pad");
    topLeft = any["topLeft"];
    bottomRight = any["bottomRight"];
}


void GuiTheme::Window::render(RenderDevice* rd, const Rect2D& bounds, bool _focused, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    Vector2 offset = _focused ? focused : defocused;

    // Grow bounds to accomodate the true extent of the window
    base.render(rd,
                Rect2D::xywh(bounds.x0y0() - borderPad.topLeft, bounds.wh() + borderPad.wh()),
                offset, delayedRectangles);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Checkable::load(const Any& any) {
    any.verifyName("Checkable");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    enabled.load(any["enabled"]);
    disabled.load(any["disabled"]);
    textOffset = any["textOffset"];
}


void GuiTheme::Checkable::Focus::load(const Any& any) {
    any.verifyName("Checkable::Focus");
    focused.load(any["focused"]);
    defocused.load(any["defocused"]);
}


void GuiTheme::Checkable::Pair::load(const Any& any) {
    any.verifyName("Checkable::Pair");
    checked   = any["checked"];
    unchecked = any["unchecked"];
}


void GuiTheme::Checkable::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _checked, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    const Rect2D* r = NULL;

    if (_enabled) {
        if (_focused) {
            if (_checked) {
                r = &enabled.focused.checked;
            } else {
                r = &enabled.focused.unchecked;
            }
        } else {
            if (_checked) {
                r = &enabled.defocused.checked;
            } else {
                r = &enabled.defocused.unchecked;
            }
        }
    } else {
        if (_checked) {
            r = &disabled.checked;
        } else {
            r = &disabled.unchecked;
        }
    }

    Vector2 extent(r->width(), r->height());
    drawRect(Rect2D::xywh(bounds.x0y0() + Vector2(0, (bounds.height() - extent.y) / 2), extent), *r, rd, delayedRectangles);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::StretchRectHV::load(const Any& any) {
    any.verifyName("StretchRectHV");

    top.load(any["top"]);

    centerLeft.load(any["centerLeft"]);
    centerCenter.load(any["centerCenter"]);
    centerRight.load(any["centerRight"]);

    bottom.load(any["bottom"]);
}


void GuiTheme::StretchRectHV::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    float topHeight    = top.left.height();
    float bottomHeight = bottom.left.height();
    float centerHeight = max(0.0f, bounds.height() - topHeight - bottomHeight);

    top.render(rd, Rect2D::xywh(bounds.x0y0(), 
                                Vector2(bounds.width(), topHeight)),
               texOffset, delayedRectangles);

    centerLeft.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(0, topHeight),
                                       Vector2(top.left.width(), centerHeight)),
                      texOffset, delayedRectangles);

    centerCenter.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(top.left.width(), topHeight),
                                       Vector2(max(0.0f, bounds.width() - (top.left.width() + top.right.width())), centerHeight)),
                        texOffset, delayedRectangles);

    centerRight.render(rd, Rect2D::xywh(bounds.x1y0() + Vector2(-top.right.width(), topHeight),
                                       Vector2(top.right.width(), centerHeight)),
                      texOffset, delayedRectangles);

    bottom.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(0, topHeight + centerHeight), 
                                   Vector2(bounds.width(), bottomHeight)),
                  texOffset, delayedRectangles);
}

//////////////////////////////////////////////////////////////////////////////
GuiTheme::StretchMode GuiTheme::stringToStretchMode(const String& name) {
    if (name == "STRETCH") {
        return GuiTheme::STRETCH;
    } else if (name == "TILE") {
        return GuiTheme::TILE;
    } else {
        debugAssertM(false, "Invalid stretch mode name loading GuiTheme.");
        return GuiTheme::STRETCH;
    }
}

void GuiTheme::Fill::load(const Any& any) {
    any.verifyName("Fill");

    source = any["source"];

    horizontalMode = GuiTheme::stringToStretchMode(any["hmode"]);
    verticalMode = GuiTheme::stringToStretchMode(any["vmode"]);
}


void GuiTheme::Fill::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    if (horizontalMode == STRETCH) {
        if (verticalMode == STRETCH) {
            // Stretch, Stretch
            drawRect(bounds, source + texOffset, rd, delayedRectangles);
        } else {
            // Stretch, Tile

            // Draw horizontal strips
            float height = source.height();
            float x0 = bounds.x0();
            float y1 = bounds.y1();
            float y = bounds.y0();
            Rect2D strip = Rect2D::xywh(0, 0, bounds.width(), source.height());
            while (y <= y1 - height) {
                drawRect(strip + Vector2(x0, y), source + texOffset, rd, delayedRectangles);
                y += height;
            }

            if (y < y1) {
                // Draw the remaining fraction of a strip
                Rect2D src = Rect2D::xywh(source.x0y0() + texOffset, Vector2(source.width(), y1 - y));
                Rect2D dst = Rect2D::xywh(Vector2(x0, y), Vector2(bounds.width(), src.height()));
                drawRect(dst, src, rd, delayedRectangles);
            }
        }
    } else {
        if (verticalMode == STRETCH) {
            // Tile, Stretch
            // Draw vertical strips
            float width = source.width();
            float y0 = bounds.y0();
            float x1 = bounds.x1();
            float x = bounds.x0();
            Rect2D strip = Rect2D::xywh(0, 0, source.width(), bounds.height());
            while (x <= x1 - width) {
                drawRect(strip + Vector2(x, y0), source + texOffset, rd, delayedRectangles);
                x += width;
            }

            if (x < x1) {
                // Draw the remaining fraction of a strip
                Rect2D src = Rect2D::xywh(source.x0y0() + texOffset, Vector2(x1 - x, source.height()));
                Rect2D dst = Rect2D::xywh(Vector2(x, y0), Vector2(src.width(), bounds.height()));
                drawRect(dst, src, rd, delayedRectangles);
            }

        } else {
            // Tile, Tile

            // Work in horizontal strips first

            float width = source.width();
            float height = source.height();
            float x0 = bounds.x0();
            float x1 = bounds.x1();
            float y1 = bounds.y1();
            float y = bounds.y0();

            Rect2D tile = Rect2D::xywh(Vector2(0, 0), source.wh());

            while (y <= y1 - height) {
                float x = x0;
                while (x <= x1 - width) {
                    drawRect(tile + Vector2(x, y), source + texOffset, rd, delayedRectangles);
                    x += width;
                }
                
                // Draw the remaining fraction of a tile
                if (x < x1) {
                    Rect2D src = Rect2D::xywh(source.x0y0() + texOffset, Vector2(x1 - x, height));
                    Rect2D dst = Rect2D::xywh(Vector2(x, y), src.wh());
                    drawRect(dst, src, rd, delayedRectangles);
                }
                
                y += height;
            }
            
            if (y < y1) {
                float x = x0;
                
                float height = y1 - y;
                tile = Rect2D::xywh(0, 0, width, height);
                while (x <= x1 - width) {
                    drawRect(tile + Vector2(x, y), tile + (source.x0y0() + texOffset), rd, delayedRectangles);
                    x += width;
                }
                
                // Draw the remaining fraction of a tile
                if (x < x1) {
                    Rect2D src = Rect2D::xywh(source.x0y0() + texOffset, Vector2(x1 - x, height));
                    Rect2D dst = Rect2D::xywh(Vector2(x, y), src.wh());
                    drawRect(dst, src, rd, delayedRectangles);
                }
            }
        }
    }        
}
//////////////////////////////////////////////////////////////////////////////

void GuiTheme::StretchRectV::load(const Any& any) {
    any.verifyName("StretchRectV"); 
    top = any["top"];
    center.load(any["center"]);
    bottom = any["bottom"];
}


void GuiTheme::StretchRectV::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    drawRect(Rect2D::xywh(bounds.x0y0(), top.wh()), top + texOffset, rd, delayedRectangles);
    center.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(0, top.height()), 
                  Vector2(bounds.width(), bounds.height() - top.height() - bottom.height())), texOffset, delayedRectangles);
    drawRect(Rect2D::xywh(bounds.x0y1() - Vector2(0,bottom.height()), bottom.wh()), bottom + texOffset, rd, delayedRectangles);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::StretchRectH::load(const Any& any) {
    any.verifyName("StretchRectH");
    left = any["left"];
    center.load(any["center"]);
    right = any["right"];
}


void GuiTheme::StretchRectH::render(class RenderDevice* rd, const Rect2D& bounds, const Vector2& texOffset, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    drawRect(Rect2D::xywh(bounds.x0y0(), left.wh()), left + texOffset, rd, delayedRectangles);
    center.render(rd, Rect2D::xywh(bounds.x0y0() + Vector2(left.width(), 0), 
                                   Vector2(bounds.width() - left.width() - right.width(), bounds.height())), texOffset, delayedRectangles);
    drawRect(Rect2D::xywh(bounds.x1y0() - Vector2(right.width(), 0), right.wh()), right + texOffset, rd, delayedRectangles);
}

//////////////////////////////////////////////////////////////////////////////

void GuiTheme::Button::load(const Any& any) {
    any.verifyName("Button");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    base.load(any["base"]);
    textOffset = any["textOffset"];
    enabled.load(any["enabled"]);
    disabled.load(any["disabled"]);
}


void GuiTheme::Button::Focus::load(const Any& any) {
    any.verifyName("Button::Focus");
    focused.load(any["focused"]);
    defocused.load(any["defocused"]);
}


void GuiTheme::Button::Pair::load(const Any& any) {
    any.verifyName("Button::Pair");
    down = any["down"];
    up = any["up"];
}


void GuiTheme::Button::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _checked, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (_focused) {
            if (_checked) {
                r = &enabled.focused.down;
            } else {
                r = &enabled.focused.up;
            }
        } else {
            if (_checked) {
                r = &enabled.defocused.down;
            } else {
                r = &enabled.defocused.up;
            }
        }
    } else {
        if (_checked) {
            r = &disabled.down;
        } else {
            r = &disabled.up;
        }
    }

    base.render(rd, bounds, *r, delayedRectangles);
}

////////////////////////////////////////////////////////////////////////////////////////

void GuiTheme::TextBox::load(const Any& any) {
    any.verifyName("TextBox");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    contentStyle = textStyle;
    if (any.containsKey("contentFont")) {
        contentStyle.load(any["contentFont"]);
    }

    base.load(any["base"]);
    textPad.load(any["textPad"]);
    enabled.load(any["enabled"]);
    disabled = any["disabled"];

    borderWidth = base.top.left.height()/2;
}


void GuiTheme::TextBox::Focus::load(const Any& any) {
    any.verifyName("TextBox::Focus");
    focused = any["focused"];
    defocused = any["defocused"];
}


void GuiTheme::TextBox::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (focused) {
            r = &enabled.focused;
        } else {
            r = &enabled.defocused;
        }
    } else {
        r = &disabled;
    }

    base.render(rd, bounds, *r, delayedRectangles);
}
///////////////////////////////////////////////////////

void GuiTheme::Canvas::load(const Any& any) {
    any.verifyName("Canvas");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    base.load(any["base"]);
    pad.load(any["pad"]);
    enabled.load(any["enabled"]);
    disabled = any["disabled"];
}

void GuiTheme::Canvas::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool focused, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (focused) {
            r = &enabled.focused;
        } else {
            r = &enabled.defocused;
        }
    } else {
        r = &disabled;
    }

    base.render(rd, bounds, *r, delayedRectangles);
}

///////////////////////////////////////////////////////

void GuiTheme::DropDownList::render(RenderDevice* rd, const Rect2D& bounds, bool _enabled, bool _focused, bool _down, Array<GuiTheme::Rectangle>& delayedRectangles) const {
    const Vector2* r = NULL;

    if (_enabled) {
        if (_focused) {
            if (_down) {
                r = &enabled.focused.down;
            } else {
                r = &enabled.focused.up;
            }
        } else {
            r = &enabled.defocused;
        }
    } else {
        r = &disabled;
    }
    
    base.render(rd, bounds, *r, delayedRectangles);
}


void GuiTheme::DropDownList::load(const Any& any) {
    any.verifyName("DropDownList");

    // Custom text styles are optional
    if (any.containsKey("font")) {
        textStyle.load(any["font"]);
    }
    if (any.containsKey("disabledFont")) {
        disabledTextStyle.load(any["disabledFont"]);
    }

    base.load(any["base"]);
    textPad.load(any["textPad"]);

    enabled.load(any["enabled"]);
    disabled = any["disabled"];
}

void GuiTheme::DropDownList::Focus::load(const Any& any) {
    any.verifyName("DropDownList::Focus");
    focused.load(any["focused"]);
    defocused = any["defocused"];
}

void GuiTheme::DropDownList::Pair::load(const Any& any) {
    any.verifyName("DropDownList::Pair");
    down = any["down"];
    up = any["up"];
}


///////////////////////////////////////////////////////

void GuiTheme::TextStyle::load(const Any& any) {

    any.verifyName("TextStyle");

    // All of these are optional
    if (any.containsKey("face")) {
        // Try to load the font
        String filename = any["face"].resolveStringAsFilename();
        if (! filename.empty()) {
            font = GFont::fromFile(filename);
        } else {
            logPrintf("GuiTheme Warning: could not find font %s\n", any["face"].string().c_str());
        }
    }

    if (any.containsKey("size")) {
        size = any["size"];
    }

    if (any.containsKey("color")) {
        color = any["color"];
    }

    if (any.containsKey("outlineColor")) {
        outlineColor = any["outlineColor"];
    }
}


} // namespace G3D
