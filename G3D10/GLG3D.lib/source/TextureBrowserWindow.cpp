/**
  \file TextureBrowserWindow.cpp

  \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2013-04-08
  \edited  2013-04-08
*/

#include "GLG3D/TextureBrowserWindow.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GuiTabPane.h"
#include "GLG3D/GApp.h"

namespace G3D {

const float TextureBrowserWindow::sBrowserWidth = 400;

static bool __cdecl alphabeticalG3DLast(weak_ptr<Texture> const& elem1, weak_ptr<Texture> const& elem2) {
    const shared_ptr<Texture>& elem1Locked = elem1.lock();
    const shared_ptr<Texture>& elem2Locked = elem2.lock();

    if (isNull(elem1Locked)) {
        return true;
    } else if (isNull(elem2Locked)) {
        return false;
    } else {
        return alphabeticalIgnoringCaseG3DLastLessThan(elem1Locked->name(), elem2Locked->name());
    }
}



void TextureBrowserWindow::setTextureIndex(int index) {
    alwaysAssertM(index < m_textures.size(), "Texture and name list out of sync");
    shared_ptr<Texture> selectedTexture = m_textures[index].lock();

    if (isNull(m_textureBox)) {
        m_textureBox = m_pane->addTextureBox(m_app);
    }

    float heightToWidthRatio = 0;
    if (notNull(selectedTexture)) {
        m_textureBox->setTexture(selectedTexture);
        m_textureBox->setCaption(selectedTexture->name());
        heightToWidthRatio = selectedTexture->height() / (float)selectedTexture->width();
    } else {
        m_textureBox->setTexture(shared_ptr<Texture>());
    }

    m_textureBox->setSizeFromInterior(Vector2(sBrowserWidth, sBrowserWidth * heightToWidthRatio));
    m_textureBox->zoomToFit();
 
    pack();
}


void TextureBrowserWindow::getTextureList(Array<String>& textureNames) {
    m_textures.clear();
    textureNames.clear();
    Texture::getAllTextures(m_textures);
    m_textures.sort(alphabeticalG3DLast);
    for (int i = 0; i < m_textures.size(); ++i) {
        const shared_ptr<Texture>& t = m_textures[i].lock();

        if (notNull(t) && t->appearsInTextureBrowserWindow()) {
            const String& displayedName = (t->name() != "") ? 
                                                t->name() : 
                                                format("Texture@0x%lx", (long int)(intptr_t)t.get());
            textureNames.append(displayedName);
        } else {
            m_textures.remove(i);
            --i; // Stay at current location
        }
    }
}

TextureBrowserWindow::TextureBrowserWindow
   (const shared_ptr<GuiTheme>&           skin,
    GApp* app) : 
    GuiWindow("Texture Browser", 
              skin, 
              Rect2D::xywh(5, 54, 200, 0),
              GuiTheme::FULL_DISAPPEARING_STYLE,
              GuiWindow::REMOVE_ON_CLOSE),
              m_textureBox(NULL) {

    m_pane = GuiWindow::pane();
    m_app = app;
    pack();
}


shared_ptr<TextureBrowserWindow> TextureBrowserWindow::create(const shared_ptr<GuiTheme>& skin, GApp* app) {
    return shared_ptr<TextureBrowserWindow>(new TextureBrowserWindow(skin, app));
}


void TextureBrowserWindow::setManager(WidgetManager* manager) {
    GuiWindow::setManager(manager);
    if (manager) {
        // Move to the upper right
        float osWindowWidth  = (float)manager->window()->width();
        float osWindowHeight = (float)manager->window()->height();
        setRect(Rect2D::xywh(osWindowWidth - rect().width(), osWindowHeight / 2, rect().width(), rect().height()));
    }
}

GuiTextureBox* TextureBrowserWindow::textureBox() {
    return m_textureBox;
}

}
