/**
  \file TextureBrowserWindow.h

  \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2013-04-08
  \edited  2013-04-08

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef GLG3D_TextureBrowserWindow_h
#define GLG3D_TextureBrowserWindow_h

#include "G3D/platform.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiLabel.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/GuiDropDownList.h"
#include "GLG3D/GuiButton.h"

namespace G3D {

class GApp;

/**
 Gui used by DeveloperWindow default for displaying all currently allocated textures.
 \sa G3D::DeveloperWindow, G3D::GApp
 */
class TextureBrowserWindow : public GuiWindow {
public:

protected:

    /** Width of the texture browser */
    static const float sBrowserWidth;

    GuiTextureBox*              m_textureBox;

    GApp*                       m_app;

    GuiPane*                    m_pane;

    Array<weak_ptr<Texture> >   m_textures;

    TextureBrowserWindow(const shared_ptr<GuiTheme>& theme, GApp* app);

public:

    /** Refreshes m_textures only, and sets a list of texture names */
    void getTextureList(Array<String>& textureNames);

    void setTextureIndex(int index);

    virtual void setManager(WidgetManager* manager);

    static shared_ptr<TextureBrowserWindow> create(const shared_ptr<GuiTheme>&  theme, GApp* app);

    GuiTextureBox* textureBox();
};

}

#endif
