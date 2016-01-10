/**
  \file Icon.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2010-01-04
  \edited  2011-07-02
*/
#ifndef G3D_Icon_h
#define G3D_Icon_h

#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Texture.h"

namespace G3D {

class IconSet;

class Icon {
private:
    friend class IconSet;

    /** Allows an IconSet to stay in the WeakCache as long as some Icon referencing it exists as well. */
    shared_ptr<IconSet>    m_keepAlive;
    shared_ptr<Texture>    m_texture;
    Rect2D                 m_sourceRect;

public:

    Icon() {}

    /** Create a new icon.  

        \param r In pixels
        \sa G3D::IconSet */
    Icon(const shared_ptr<Texture>& t, const Rect2D& r) : m_texture(t), m_sourceRect(r) {}

    Icon(const shared_ptr<Texture>& t) : m_texture(t), m_sourceRect(Rect2D::xywh(0.0f,0.0f,(float)t->width(), (float)t->height())) {}

    const shared_ptr<Texture>& texture() const {
        return m_texture;
    }

    /** Position within texture() of this icon, in pixels.*/
    const Rect2D& sourceRect() const {
        return m_sourceRect;
    }

    int width() const {
        return iRound(m_sourceRect.width());
    }

    int height() const {
        return iRound(m_sourceRect.height());
    }
};

}

#endif
