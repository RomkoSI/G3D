/**
 @file GuiText.cpp
 @author Morgan McGuire, http://graphics.cs.williams.edu
 */

#include "G3D/platform.h"
#include "GLG3D/GuiText.h"
#include "GLG3D/Icon.h"

namespace G3D {

void GuiText::Element::setDefault(const shared_ptr<GFont>& dfont, float dsize, const Color4& dcolor, const Color4& doutline) {
    if (isNull(m_font)) {
        m_font = dfont;
    }

    if (m_size < 0) {
        m_size = dsize;
    }

    if (m_color.a < 0) {
        m_color = dcolor;
    }
    
    if (m_outlineColor.a < 0) {
        m_outlineColor = doutline;
    }
}


GuiText::GuiText
(const String& text, 
 const shared_ptr<GFont>& font, 
 float size, 
 const Color4& color,
 const Color4& outlineColor,
 const Vector2& offset) : m_sourceRect(Rect2D::empty()) {
    append(text, font, size, color, outlineColor, offset);
}


GuiText::GuiText
(const char* text) : m_sourceRect(Rect2D::empty()) {
    append(text);
}


GuiText::GuiText(const shared_ptr<Texture>& texture, const Rect2D& srcRect) : m_texture(texture), m_sourceRect(srcRect) {
}


GuiText::GuiText(const Icon& icon) : m_texture(icon.texture()), m_sourceRect(icon.sourceRect()) {
}


void GuiText::append
(const String&              text, 
 const shared_ptr<GFont>&   font, 
 float                      size, 
 const Color4&              color,
 const Color4&              outlineColor,
 const Vector2&             offset) {

    if (text == "") { 
        // Nothing to append
        return;
    }

    Element& e = m_elementArray.next();
    e.m_text = text;
    e.m_font = font;
    e.m_size = size;
    e.m_color = color;
    e.m_outlineColor = outlineColor;
    e.m_offset = offset;    
}


String GuiText::text() const {
    if (m_elementArray.size() == 1) {
        return m_elementArray[0].text();
    } else {
        String s;
        for (int e = 0; e < m_elementArray.size(); ++e) {
            s += m_elementArray[e].text();
        }
        return s;
    }
}


void GuiText::setDefault(const shared_ptr<GFont>& dfont, float dsize, const Color4& dcolor, const Color4& doutline) {
    for (int e = 0; e < m_elementArray.size(); ++e) {
        m_elementArray[e].setDefault(dfont, dsize, dcolor, doutline);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

GuiText GuiText::Symbol::record() {
    return GuiText("=", GFont::fromFile(System::findDataFile("icon.fnt")), 16, Color3::red() * 0.5f);
}


GuiText GuiText::Symbol::play() {
    return GuiText("4", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::eye() {
    return GuiText("N", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::pause() {
    return GuiText(";", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::stop() {
    return GuiText("<", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::forward() {
    return GuiText(String() + (char)0x38, GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::reverse() {
    return GuiText(String() + (char)0x37, GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::previous() {
    return GuiText("1", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::next() {
    return GuiText("2", GFont::fromFile(System::findDataFile("icon.fnt")), 16);
}


GuiText GuiText::Symbol::greek(int code) {
    return GuiText(String() + (char)code, GFont::fromFile(System::findDataFile("symbol.fnt")));
}


GuiText GuiText::Symbol::leq() {
    return greek(0xA3);
}


GuiText GuiText::Symbol::geq() {
    return greek(0xB3);
}


GuiText GuiText::Symbol::pm() {
    return greek(0xB1);
}


GuiText GuiText::Symbol::heartSuit() {
    return GuiText(String() + char(0xA9), GFont::fromFile(System::findDataFile("symbol.fnt")), -1, Color3::red() * 0.6f);
}


GuiText GuiText::Symbol::diamondSuit() {
    return GuiText(String() + char(0xA8), GFont::fromFile(System::findDataFile("symbol.fnt")), -1, Color3::red() * 0.6f);
}


GuiText GuiText::Symbol::clubSuit() {
    return greek(0xA7);
}


GuiText GuiText::Symbol::spadeSuit() {
    return greek(0xAA);
}


GuiText GuiText::Symbol::approx() {
    return greek(0xBB);
}


GuiText GuiText::Symbol::partial() {
    return greek(0xB6);
}


}
