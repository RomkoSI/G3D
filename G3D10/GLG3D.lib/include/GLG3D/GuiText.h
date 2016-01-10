/**
  \file GLG3D/GuiText.h
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  
  \created 2010-01-01
  \edited  2012-06-29

  Copyright 2000-2015, Morgan McGuire
 */

#ifndef G3D_GuiText_h
#define G3D_GuiText_h

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "G3D/Table.h"
#include "G3D/SmallArray.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GFont.h"

template <> struct HashTrait<shared_ptr<G3D::GFont> > {
    static size_t hashCode(const shared_ptr<G3D::GFont>& key) { return reinterpret_cast<size_t>(key.get()); }
};

namespace G3D {

class Icon;

/** 
    Text or Icon on a G3D::GuiControl. These are normally created
    implicitly by a cast from String or by G3D::IconSet, but can be
    created explicitly when more information needs to be specified.

    \sa G3D::GuiLabel, G3D::GuiTextBox, G3D::IconSet, G3D::Icon, G3D::Texture
 */
class GuiText {
public:
    struct Element {
    private:
        friend class GuiText;
        String   m_text;
        shared_ptr<GFont>    m_font;
        float         m_size;
        Color4        m_color;
        Color4        m_outlineColor;

        /** Offset from the baseline of the end of the previous element.*/
        Vector2       m_offset;

    public:

        inline Element
        (const String& text         = "", 
         const shared_ptr<GFont>&  font         = shared_ptr<GFont>(), 
         float              size         = -1, 
         const Color4&      color        = Color4(-1,-1,-1,-1), 
         const Color4&      outlineColor = Color4(-1,-1,-1,-1),
         const Vector2&     offset       = Vector2::zero()) : 
            m_text(text), m_font(font), m_size(size), m_color(color), m_outlineColor(outlineColor), m_offset(offset) {}
        
        inline const String& text() const {
            return m_text;
        }

        /** Offset from the baseline of the end of the previous element.*/
        inline const Vector2& offset() const {
            return m_offset;
        }

        /** If this element specifies a font, returns that font, otherwise returns the default font */
        inline const shared_ptr<GFont>& font(const shared_ptr<GFont>& _default) const {
            if (isNull(m_font)) {
                return _default;
            } else {
                return m_font;
            }
        }
        
        inline const Color4& color(const Color4& _default) const {
            if (m_color.a < 0) {
                return _default;
            } else {
                return m_color;
            }
        }
        
        inline const Color4& outlineColor(const Color4& _default) const {
            if (m_outlineColor.a < 0) {
                return _default;
            } else {
                return m_outlineColor;
            }
        }
        
        inline float size(float _default) const {
            if (m_size < 0) {
                return _default;
            } else {
                return m_size;
            }
        }

        /** Provides the value of default values; called by Gui to overwrite the illegal values.*/
        void setDefault(const shared_ptr<GFont>& dfont, float dsize, const Color4& dcolor, const Color4& doutline);
    };

private:

    typedef SmallArray<Element, 1> ElementArray;

    ElementArray       m_elementArray;

    /** If not NULL, this is an icon */
    shared_ptr<Texture>       m_texture;

    /** In pixels.*/
    Rect2D             m_sourceRect;
    
public:

    /**
       Negative alpha values on color, outlineColor, and size mean "use
       default".  NULL font means "use default".
       
       Defaults are set on the GuiTheme.
    */
    GuiText
    (const String& text         = "", 
     const shared_ptr<GFont>&  font         = shared_ptr<GFont>(), 
     float              size         = -1.0f, 
     const Color4&      color        = Color4(-1, -1, -1, -1), 
     const Color4&      outlineColor = Color4(-1, -1, -1, -1),
     const Vector2&     offset       = Vector2::zero());

    GuiText(const char* text);

    /** Create an icon.  
        \param texture The source texture.
        \param srcRect The source rectangle, in pixels of \a texture
        \sa G3D::IconSet. */
    GuiText(const shared_ptr<Texture>& texture, const Rect2D& srcRect);

    GuiText(const Icon& icon);

    /** True if this is an icon. */
    bool isIcon() const {
        return notNull(m_texture);
    }

    const shared_ptr<Texture>& iconTexture() const {
        return m_texture;
    }

    const Rect2D& iconSourceRect() const {
        return m_sourceRect;
    }
    
    /** True if this is an empty caption */
    bool empty() const {
        return ! isIcon() && (m_elementArray.size() == 0); 
    }

    /** Adds this text to the end of the GuiText. It is an error to append to an icon.*/
    void append
    (const String& text, 
     const shared_ptr<GFont>&  font         = shared_ptr<GFont>(), 
     float              size         = -1.0f, 
     const Color4&      color        = Color4(-1, -1, -1, -1), 
     const Color4&      outlineColor = Color4(-1, -1, -1, -1),
     const Vector2&     offset       = Vector2::zero());

    /** Provides the value of default values for current elements.*/
    void setDefault(const shared_ptr<GFont>& dfont, float dsize, const Color4& dcolor, const Color4& doutline);

    /** Returns the number of elements within this GuiText. */
    inline int numElements() const {
        return m_elementArray.size();
    }

    /** Returns element \a e within this GuiText. */
    inline const Element& element(int e) const {
        return m_elementArray[e];
    }

    /** Concatenates all of the text within the elements.*/
    String text() const;

    /** \copydoc G3D::GuiText::text */
    inline operator String() const { 
        return text(); 
    }

    /** Loads standard symbols from the greek.fnt and icon.fnt fonts.*/
    class Symbol {
    private:
        static GuiText greek(int asciiCode);
    public:

        static GuiText record();
        static GuiText play();
        static GuiText stop();
        static GuiText pause();
        static GuiText eye();
        static GuiText forward();
        static GuiText reverse();
        static GuiText previous();
        static GuiText next();

        /** >= */
        static GuiText leq();

        /** <= */
        static GuiText geq();

        /** +/- */
        static GuiText pm();

        /** Wiggly equals sign */
        static GuiText approx();

        /** Partial derivative symbol */
        static GuiText partial();

        /** Red heart */
        static GuiText heartSuit();

        static GuiText clubSuit();
        static GuiText spadeSuit();

        /** Red diamond */
        static GuiText diamondSuit();

    };

};

} // G3D
#endif
