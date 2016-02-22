/**
 \file GLG3D/GuiTextureBox.h

 \created 2009-09-11
 \edited  2014-06-28

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire http://graphics.cs.williams.edu
 All rights reserved.
*/
#ifndef G3D_GuiTextureBox_h
#define G3D_GuiTextureBox_h

#include "G3D/platform.h"
#include "G3D/Vector2.h"
#include "GLG3D/GuiContainer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GuiText.h"

namespace G3D {

class GuiPane;
class GApp;
class GuiButton;
class GuiTextureBoxInspector;

class GuiTextureBox : public GuiContainer {
public:
    friend class GuiTextureBoxInspector;

protected:

    /** Padding around the image */
    enum {BORDER = 1};

    shared_ptr<Texture>             m_texture;

    weak_ptr<GuiTextureBoxInspector>m_inspector;

    Texture::Visualization          m_settings;

    /** Bounds for mouse clicks and scissor region, updated by every render. */
    Rect2D                          m_clipBounds;
    
    bool                            m_showInfo;


    bool                            m_showCubemapEdges;

    /** If true, textures are drawn with the Y coordinate inverted.
        Ignored if drawing a cube map. */
    bool                            m_drawInverted;

    /** Cached formatting of m_lastSize */
    mutable GuiText                 m_lastSizeCaption;
    mutable Vector2int16            m_lastSize;
    mutable const ImageFormat*      m_lastFormat;

    float                           m_zoom;
    Vector2                         m_offset;

    /** True when dragging the image */
    bool                            m_dragging;
    Vector2                         m_dragStart;
    Vector2                         m_offsetAtDragStart;

    /** Readback texel */
    mutable Color4                  m_texel;

    /** Readback position */
    mutable Vector2int16            m_readbackXY;

    /** If true, this is the texture box inside of the inspector and should not be a button */
    bool                            m_embeddedMode;


    /** Returns the bounds of the canvas (display) region for this GuiTextBox */
    Rect2D canvasRect() const;

    /** Returns the bounds of the canvas (display) region for a GuiTextBox of size \a rect*/
    Rect2D canvasRect(const Rect2D& rect) const;

    void drawTexture(RenderDevice* rd, const Rect2D& r) const;

    void computeSizeString() const;

public:

    /** In most cases, you'll want to call GuiPane::addTextureBox instead.

      \param embeddedMode When set to true, hides the controls that duplicate inspector functionality.
     */
    GuiTextureBox
    (GuiContainer*       parent,
     const GuiText&      caption,
     GApp*               app,
     const shared_ptr<Texture>& t = shared_ptr<Texture>(),
     bool                embeddedMode = false,
     bool                drawInverted = false);

    virtual ~GuiTextureBox();

    GApp*                           m_app;

    /** Starts the inspector window.  Invoked by the inspector button. */
    void showInspector();

    /** Zoom factor for the texture display.  Greater than 1 = zoomed in. */
    inline float viewZoom() const {
        return m_zoom;
    }
    
    void setViewZoom(float z);

    /** Offset of the texture from the centered position. Positive = right and down. */
    inline const Vector2& viewOffset() const {
        return m_offset;
    }

    void zoomIn();
    void zoomOut();

    /** Brings up the modal save dialog */
    void save();

    void setViewOffset(const Vector2& x);

    /** Change the scale to 1:1 pixel */
    void zoomTo1();

    /** Center the image and scale it to fill the viewport */
    void zoomToFit();

    /** If the texture was previously NULL, also invokes zoomToFit() */
    void setTexture(const shared_ptr<Texture>& t);
    void setSettings(const Texture::Visualization& s);

    /** Set m_drawInverted */
    void setInverted(const bool inverted) {
        m_drawInverted = inverted;
    }

    inline const shared_ptr<Texture>& texture() const {
        return m_texture;
    }

    inline const Texture::Visualization& settings() const {
        return m_settings;
    }

    /** Controls the display of (x,y)=rgba when the mouse is over the box.
        Defaults to true.  Note that displaying these values can significantly
        impact performance because it must read back from the GPU to the CPU.*/
    inline void setShowInfo(bool b) {
        m_showInfo = b;
    }

    inline bool showInfo() const {
        return m_showInfo;
    }

    /** Sizes the control so that exactly \a dims of viewing space is available. 
        Useful for ensuring that textures are viewed at 1:1.*/
    void setSizeFromInterior(const Vector2& dims);

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    virtual void setRect(const Rect2D& rect) override;
    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override;

    virtual bool onEvent(const GEvent& event) override;

    /** Invoked by the drawer button. Do not call directly. */
    void toggleDrawer();

    /* Bind arguments to the specified shader */
    void setShaderArgs(UniformTable& args, bool isCubemap);

    virtual void setCaption(const GuiText& text) override;
};

} // namespace

#endif
