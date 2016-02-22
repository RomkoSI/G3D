/**
 \file GLG3D/GuiTextureBox.cpp

 \created 2009-09-11
 \edited  2013-06-28

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
*/
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"
#include "GLG3D/GuiTextureBox.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/Draw.h"
#include "GLG3D/FileDialog.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Shader.h"
#include "GLG3D/ScreenshotDialog.h"
#include "GLG3D/GApp.h"

namespace G3D {


GuiTextureBox::GuiTextureBox
(GuiContainer*       parent,
 const GuiText&      caption,
 GApp*               app,
 const shared_ptr<Texture>& t,
 bool                embeddedMode,
 bool                drawInverted) : 
    GuiContainer(parent, caption),
    m_showInfo(embeddedMode), 
    m_showCubemapEdges(false),
    m_drawInverted(drawInverted),
    m_lastFormat(NULL),
    m_dragging(false), 
    m_readbackXY(-1, -1),
    m_embeddedMode(embeddedMode),
    m_app(app) {

    setTexture(t);
    setCaptionHeight(0.0f);
    float aspect = 1440.0f / 900.0f;
    setSize(Vector2(240 * aspect, 240));

    zoomToFit();
}


GuiTextureBox::~GuiTextureBox() {
}

class GuiTextureBoxInspector : public GuiWindow {
protected:

    /** Settings of the original GuiTextureBox */
    Texture::Visualization&     m_settings;

    GuiTextureBox*              m_textureBox;

    GApp*                       m_app;

    shared_ptr<GuiWindow>       m_parentWindow;

    GuiDropDownList*            m_modeDropDownList;
    
    GuiDropDownList*            m_layerDropDownList;
    GuiDropDownList*            m_mipLevelDropDownList;

    GuiPane*                    m_drawerPane;

    mutable GuiLabel*           m_xyLabel;
    mutable GuiLabel*           m_uvLabel;
    mutable GuiLabel*           m_xyzLabel;
    mutable GuiLabel*           m_rgbaLabel;
    mutable GuiLabel*           m_ARGBLabel;
 
    /** Adds two labels to create a two-column display and returns a pointer to the second label. */
    static GuiLabel* addPair(GuiPane* p, const GuiText& key, const GuiText& val, int captionWidth = 130, GuiLabel* nextTo = NULL, int moveDown = 0) {
        GuiLabel* keyLabel = p->addLabel(key);
        if (nextTo) {
            keyLabel->moveRightOf(nextTo);
        }
        if (moveDown != 0) {
            keyLabel->moveBy(0, (float)moveDown);
        }
        keyLabel->setWidth((float)captionWidth);
        GuiLabel* valLabel = p->addLabel(val);
        valLabel->moveRightOf(keyLabel);
        valLabel->setWidth(200);
        valLabel->setXAlign(GFont::XALIGN_LEFT);
        return valLabel;
    }


    static String valToText(const Color4& val) {
        if (val.isFinite()) {
            return format("(%6.3f, %6.3f, %6.3f, %6.3f)", val.r, val.g, val.b, val.a);
        } else {
            return "Unknown";
        }
    }

public:

    /** \param parentWindow Hold a pointer to the window containing the original 
        GuiTextureBox so that it is not collected while we have its Settings&. */
    GuiTextureBoxInspector(const String& windowCaption, const shared_ptr<Texture>& texture, Texture::Visualization& settings, const shared_ptr<GuiWindow>& parentWindow, GApp* app) :
        GuiWindow(windowCaption,
            parentWindow->theme(), 
            Rect2D::xywh(0,0, 100, 100),
            GuiTheme::NORMAL_WINDOW_STYLE, 
            REMOVE_ON_CLOSE),
        m_settings(settings),
        m_app(app),
        m_parentWindow(parentWindow) {

        Vector2 screenBounds((float)parentWindow->window()->width(), (float)parentWindow->window()->height());


        GuiPane* leftPane = pane();

        m_textureBox = leftPane->addTextureBox(m_app,"", texture, true);
        m_textureBox->setSize(screenBounds - Vector2(375, 200));
        m_textureBox->zoomToFit();

        leftPane->pack();

        //////////////////////////////////////////////////////////////////////
        // Place the preset list in the empty space next to the drawer, over the TextureBox control
        Array<String> presetList;

        // This list must be kept in sync with onEvent
        presetList.append("<Click to load>", "sRGB Image", "Radiance", "Reflectivity");
        presetList.append( "8-bit Normal/Dir", "Float Normal/Dir");
        presetList.append("Depth Buffer", "Bump Map (in Alpha)");
        m_modeDropDownList = leftPane->addDropDownList("Vis. Preset", presetList);
        m_modeDropDownList->moveBy(5,0);
        GuiPane* visPane = leftPane->addPane("", GuiTheme::NO_PANE_STYLE);
        
        Array<String> channelList;
        // this order must match the order of the Channels enum
        channelList.append("RGB", "R", "G", "B");
        channelList.append("R as Luminance", "G as Luminance", "B as Luminance", "A as Luminance");
        channelList.append("RGB/3 as Luminance", "True Luminance");
        visPane->addDropDownList("Channels", channelList, (int*)&m_settings.channels);

        GuiLabel* documentCaption = visPane->addLabel("Document");
        documentCaption->setWidth(65.0f);
        GuiNumberBox<float>* gammaBox = 
            visPane->addNumberBox(GuiText("g", GFont::fromFile(System::findDataFile("greek.fnt"))), &m_settings.documentGamma, "", GuiTheme::LINEAR_SLIDER, 0.1f, 15.0f);
        gammaBox->setCaptionWidth(15.0f);
        gammaBox->setUnitsSize(5.0f);
        gammaBox->setWidth(150.0f);
        gammaBox->moveRightOf(documentCaption);

        GuiNumberBox<float>* minBox;
        GuiNumberBox<float>* maxBox;
        minBox = visPane->addNumberBox("Range", &m_settings.min);
        minBox->setUnitsSize(0.0f);
        minBox->setWidth(140.0f);
        
        maxBox = visPane->addNumberBox("-", &m_settings.max);
        maxBox->setCaptionWidth(10.0f);
        maxBox->setWidth(95.0f);
        maxBox->moveRightOf(minBox);
        visPane->pack();
        visPane->setWidth(230);

        //////////////////////////////////////////////////////////////////////
        // Height of caption and button bar
        const float cs = 20;

        // Height of the drawer
        const float h = cs - 1;
        const shared_ptr<GFont> iconFont = GFont::fromFile(System::findDataFile("icon.fnt"));


        m_drawerPane = pane();

        // Contents of the tools drawer:
        {
            // static const char* infoIcon = "i";
            static const char* zoomIcon = "L";
            static const char* diskIcon = "\xcd";
            //static const char* inspectorIcon = "\xa0";

            debugAssert(! m_clientRect.isEmpty());
            GuiButton* saveButton = m_drawerPane->addButton(GuiText(diskIcon, iconFont, h), 
                                                            GuiControl::Callback(m_textureBox, &GuiTextureBox::save),
                                                            GuiTheme::TOOL_BUTTON_STYLE);
            debugAssert(! m_clientRect.isEmpty());

            saveButton->setSize(h, h);


            GuiButton* zoomInButton = m_drawerPane->addButton(GuiText(zoomIcon, iconFont, h), 
                                                              GuiControl::Callback(m_textureBox, &GuiTextureBox::zoomIn), 
                                                              GuiTheme::TOOL_BUTTON_STYLE);
            zoomInButton->setSize(h, h);
            zoomInButton->moveBy(h/3, 0);

            GuiButton* fitToWindowButton = m_drawerPane->addButton(GuiText("fit", shared_ptr<GFont>(), h - 7), 
                                                                   GuiControl::Callback(m_textureBox, &GuiTextureBox::zoomToFit), GuiTheme::TOOL_BUTTON_STYLE);
            fitToWindowButton->setSize(h, h);

            GuiButton* zoom100Button = m_drawerPane->addButton(GuiText("1:1", shared_ptr<GFont>(), h - 8), 
                                                               GuiControl::Callback(m_textureBox, &GuiTextureBox::zoomTo1), GuiTheme::TOOL_BUTTON_STYLE);
            zoom100Button->setSize(h, h);

            GuiButton* zoomOutButton = m_drawerPane->addButton(GuiText(zoomIcon, iconFont, h/2), 
                                                               GuiControl::Callback(m_textureBox, &GuiTextureBox::zoomOut),
                                                               GuiTheme::TOOL_BUTTON_STYLE);
            zoomOutButton->setSize(h, h);

        }
        //m_drawerPane->moveRightOf(dataPane);
        m_drawerPane->pack();
        ///////////////////////////////////////////////////////////////////////////////
        GuiPane* dataPane = leftPane->addPane("", GuiTheme::NO_PANE_STYLE);

        int captionWidth = 55;
        m_xyLabel = addPair(dataPane, "xy =", "", 30);
        m_xyLabel->setWidth(70);
        if(texture->isCubeMap()){
            m_xyzLabel = addPair(dataPane, "xyz =", "", 30, m_xyLabel);
            m_xyzLabel->setWidth(160);
        } else {
            m_uvLabel = addPair(dataPane, "uv =", "", 30, m_xyLabel);
            m_uvLabel->setWidth(120);
        }

        m_rgbaLabel = addPair(dataPane, "rgba =", "", captionWidth);
        m_rgbaLabel->moveBy(-13,0);
        m_ARGBLabel = addPair(dataPane, "ARGB =", "", captionWidth);
        dataPane->addLabel(GuiText("Before gamma correction", shared_ptr<GFont>(), 8))->moveBy(Vector2(5, -5));
        if(texture->isCubeMap()){
            dataPane->addCheckBox("Show Cube Edges", &m_textureBox->m_showCubemapEdges);
        }
        dataPane->pack();  
        dataPane->moveRightOf(visPane);
        dataPane->moveBy(-20,-10);
        leftPane->pack();
        //////////////////////////////////////////////////////////////////////
        GuiScrollPane* scrollPane = leftPane->addScrollPane(true, false, GuiTheme::BORDERLESS_SCROLL_PANE_STYLE);

        GuiPane* infoPane = scrollPane->viewPane();

        addPair(infoPane, "Format:", texture->format()->name());
        if (texture->depth() > 1) {
            addPair(infoPane, "Size:"  , format("%dx%dx%d", texture->width(), texture->height(), texture->depth()));
        } else {
            addPair(infoPane, "Size:"  , format("%dx%d", texture->width(), texture->height()));
        }
        
        
        String dim;
        switch (texture->dimension()) {
        case Texture::DIM_2D: dim = "DIM_2D"; break;
        case Texture::DIM_3D: dim = "DIM_3D"; break;
        case Texture::DIM_2D_RECT: dim = "DIM_2D_RECT"; break;
        case Texture::DIM_CUBE_MAP: dim = "DIM_CUBE_MAP"; break;
        case Texture::DIM_2D_ARRAY: dim = "DIM_2D_ARRAY"; break;
        case Texture::DIM_CUBE_MAP_ARRAY: dim = "DIM_CUBE_MAP_ARRAY"; break;
        }
        addPair(infoPane, "Dimension:", dim);
        if (texture->depth() > 1) {
            Array<String> indexList;
            for (int i = 0; i < texture->depth(); ++i) {
                indexList.append(format("Layer %d", i));
            }
            m_layerDropDownList = infoPane->addDropDownList("", indexList, &(settings.layer) );
        }

        if (texture->hasMipMaps()) {
            Array<String> indexList;
            for (int i = 0; i < (int)log2(max(texture->width(), texture->height())); ++i) {
                indexList.append(format("Mip Level %d", i));
            }
            m_mipLevelDropDownList = infoPane->addDropDownList("", indexList, &(settings.mipLevel) );
        } else {
            infoPane->addLabel("No MipMaps");
        }
        

        addPair(infoPane, "Min Value:",  valToText(texture->min()), 80, NULL, 20);
        addPair(infoPane, "Mean Value:", valToText(texture->mean()), 80);
        addPair(infoPane, "Max Value:",  valToText(texture->max()), 80);
        addPair(infoPane, "ReadMultiplyFirst:", valToText(texture->encoding().readMultiplyFirst), 120);
        addPair(infoPane, "ReadAddSecond:", valToText(texture->encoding().readAddSecond), 120);
        infoPane->pack();
        scrollPane->pack();
        scrollPane->moveRightOf(dataPane);
        scrollPane->moveBy(0, -20);
        scrollPane->setHeight(110);
        scrollPane->setWidth(295.0f);
        /////////////////////////////////////////////////////////
        pack();
        moveTo(screenBounds / 2.0f - rect().center());
        setVisible(true);
    }


    virtual void render(RenderDevice* rd) const {
        GuiWindow::render(rd);

        // Keep our display in sync with the original one when a GUI control changes
        m_textureBox->setSettings(m_settings);

        // Update the xy/uv/rgba labels
        shared_ptr<Texture> tex = m_textureBox->texture();
        float w = 1, h = 1;
        if (tex) {
            w = (float)tex->width();
            h = (float)tex->height();
        }

        // Render child controls so that they slide under the canvas
        m_xyLabel->setCaption(format("(%d, %d)", m_textureBox->m_readbackXY.x, m_textureBox->m_readbackXY.y));
        float u = m_textureBox->m_readbackXY.x / w;
        float v = m_textureBox->m_readbackXY.y / h;

        if(tex->isCubeMap()){
            float theta = v * pif();
            float phi   = u * 2 * pif();
            float sinTheta = sin(theta);
            float x = cos(phi) * sinTheta;
            float y = cos(theta);
            float z = sin(phi) * sinTheta; 
            m_xyzLabel->setCaption(format("(%6.4f, %6.4f, %6.4f)", x, y, z));
        } else {
            m_uvLabel->setCaption(format("(%6.4f, %6.4f)", u, v));
        }
        m_rgbaLabel->setCaption(format("(%6.4f, %6.4f, %6.4f, %6.4f)", m_textureBox->m_texel.r, 
                        m_textureBox->m_texel.g, m_textureBox->m_texel.b, m_textureBox->m_texel.a));
        Color4unorm8 c(m_textureBox->m_texel);
        m_ARGBLabel->setCaption(format("0x%02x%02x%02x%02x", c.a.bits(), c.r.bits(), c.g.bits(), c.b.bits()));
    }


    virtual bool onEvent(const GEvent& event) {
        if (GuiWindow::onEvent(event)) {
            return true;
        }
        
        switch (event.type) {
        case GEventType::KEY_DOWN:
            if (event.key.keysym.sym == GKey::ESCAPE) {
                // Cancel this window
                manager()->remove(dynamic_pointer_cast<GuiTextureBoxInspector>(shared_from_this()));
                return true;
            }
            break;

        case GEventType::GUI_ACTION:
            if ((event.gui.control == m_modeDropDownList) && (m_modeDropDownList->selectedIndex() > 0)) {
                String preset = m_modeDropDownList->selectedValue().text();
                if (preset == "sRGB Image") {
                    m_settings = Texture::Visualization::sRGB();
                } else if (preset == "Radiance") {
                    // Choose the maximum value
                    m_settings = Texture::Visualization::defaults();
                    shared_ptr<Texture> tex = m_textureBox->texture();
                    if (tex) {
                        Color4 max = tex->max();
                        if (max.isFinite()) {
                            m_settings.max = G3D::max(max.r, max.g, max.b);
                        }
                    }
                } else if (preset == "Reflectivity") {
                    m_settings = Texture::Visualization::defaults();
                } else if (preset == "8-bit Normal/Dir") {
                    m_settings = Texture::Visualization::packedUnitVector();
                } else if (preset == "Float Normal/Dir") {
                    m_settings = Texture::Visualization::unitVector();
                } else if (preset == "Depth Buffer") {
                    m_settings = Texture::Visualization::depthBuffer();
                } else if (preset == "Bump Map (in Alpha)") {
                    m_settings = Texture::Visualization::bumpInAlpha();
                }

                // Switch back to <click to load>
                m_modeDropDownList->setSelectedIndex(0);
                return true;
            } 
            break;

        default:;
        }

        return false;
    }
};
void GuiTextureBox::save() {
    String filename = ScreenshotDialog::nextFilenameBase(m_app->settings().screenshotDirectory) + "_";
  
    // Make a sample filename, removing illegal or undesirable characters
    String temp = m_texture->name();
    for (int i = 0; i < (int)temp.size(); ++i) {
        switch (temp[i]) {
        case ' ':
        case '\r':
        case '\n':
        case '\t':
        case '.':
        case ':':
        case '/':
        case '\\':
        case '\'':
        case '\"':
            filename += "_";
            break;

        default:
            filename += temp[i];
        }
    }

    // Make sure this filename doesn't exist
    int i = 0;
    while (FileSystem::exists(format("%s-%d.png", filename.c_str(), i))) {
        ++i;
    }
    if (i > 0) {
        filename = format("%s-%d", filename.c_str(), i);
    }
    filename += ".png";

    //if (FileDialog::getFilename(filename)) {
        // save code
        shared_ptr<Framebuffer> fb = Framebuffer::create("GuiTextureBox: save");
        shared_ptr<Texture> color;
        bool generateMipMaps = false;
        if (m_texture->isCubeMap()) {
            //Stretch cubemaps appropriately
            color = Texture::createEmpty("GuiTextureBox: save", 
                m_texture->width()*4, m_texture->height()*2, ImageFormat::RGB8(), Texture::DIM_2D, generateMipMaps);
        }else{
            color = Texture::createEmpty("GuiTextureBox: save", 
                m_texture->width(), m_texture->height(), ImageFormat::RGB8(), Texture::DIM_2D, generateMipMaps);
        }
        fb->set(Framebuffer::COLOR0, color);
        
        RenderDevice* rd = RenderDevice::current;
        rd->push2D(fb);
        {
            rd->setColorClearValue(Color3::white());
            rd->clear();
            drawTexture(rd, rd->viewport());
        }
        rd->pop2D();
        shared_ptr<GuiTextureBoxInspector> ins(m_inspector.lock());
        if (notNull(m_app)) {
            if (ScreenshotDialog::create(window()->window(), theme(), FilePath::parent(filename), false)->getFilename(filename, m_texture->name(), color)) {
                filename = trimWhitespace(filename);
                if (!filename.empty()) {
                    color->toImage()->save(filename);
                }
            }
        }
   // }
}


void GuiTextureBox::setSizeFromInterior(const Vector2& dims) {
    // Find out how big the canvas inset is
    Rect2D big = Rect2D::xywh(0, 0, 100, 100);

    // Get the canvas bounds
    Rect2D small = theme()->canvasToClientBounds(canvasRect(big), m_captionHeight);
    
    // Offset is now big - small
    setSize(dims + big.wh() - small.wh() + Vector2(BORDER, BORDER) * 2.0f);
}


bool GuiTextureBox::onEvent(const GEvent& event) {
    if (! m_visible) {
        return false;
    }

    if (! m_enabled) {
        return false;
    }

    if (GuiContainer::onEvent(event)) {
        // Event was handled by base class
        return true;
    }

    if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && 
        m_clipBounds.contains(Vector2(event.button.x, event.button.y))) {
        if (m_embeddedMode) {
            m_dragStart = Vector2(event.button.x, event.button.y);
            m_dragging = true;
            m_offsetAtDragStart = m_offset;
        } else {
            showInspector();
        }
        return true;

    } else if (event.type == GEventType::MOUSE_BUTTON_UP) {

        // Stop drag
        m_dragging = false;
        return (m_clipBounds.contains(Vector2(event.button.x, event.button.y)));

    } else if (event.type == GEventType::MOUSE_MOTION) {
        if (m_dragging) {
            Vector2 mouse(event.mousePosition());
            
            // Move point, clamping adjacents        
            Vector2 delta = mouse - m_dragStart;

            // Hide weird mouse event delivery
            if (delta.squaredLength() < square(rect().width() + rect().height())) {
                m_offset = m_offsetAtDragStart + delta / m_zoom;
                return true;
            }
        } 
    }

    return false;
}


void GuiTextureBox::setRect(const Rect2D& rect) {
    debugAssert(! rect.isEmpty());
    GuiContainer::setRect(rect);
    debugAssert(! m_clientRect.isEmpty());

    m_clipBounds = theme()->canvasToClientBounds(canvasRect(), m_captionHeight);
    m_clickRect = m_clipBounds;
}

Rect2D GuiTextureBox::canvasRect(const Rect2D& rect) const {
    return rect;
}


Rect2D GuiTextureBox::canvasRect() const {
    return canvasRect(m_rect);
}





void GuiTextureBox::showInspector() {
    shared_ptr<GuiWindow>     myWindow = dynamic_pointer_cast<GuiWindow>(window()->shared_from_this());
    shared_ptr<WidgetManager> manager  = dynamic_pointer_cast<WidgetManager>(myWindow->manager()->shared_from_this());

    shared_ptr<GuiTextureBoxInspector> ins(m_inspector.lock());
    if (isNull(ins)) {
        computeSizeString();
        ins.reset(new GuiTextureBoxInspector(m_texture->name() + " (" + m_lastSizeCaption.text() + ")", m_texture, m_settings, myWindow, m_app));
        m_inspector = ins;

        manager->add(ins);
    }

    manager->setFocusedWidget(ins);
}


void GuiTextureBox::setShaderArgs(UniformTable& args, bool isCubemap){
    args.setMacro("IS_GL_TEXTURE_RECTANGLE", m_texture->dimension() == Texture::DIM_2D_RECT ? 1 : 0);
    args.setMacro("DRAW_INVERTED", m_drawInverted);
    m_texture->setShaderArgs(args, "tex_", m_texture->hasMipMaps() ? Sampler::visualization() : Sampler::buffer());
    m_settings.setShaderArgs(args);

    if(isCubemap){
        if(m_showCubemapEdges){
            // TODO: come up with a principled way to make this 1 pixel wide
            float thresholdValue = 2.0f - 10.0f/m_texture->width();
            args.setUniform("edgeThreshold", thresholdValue);
        } else {
            args.setUniform("edgeThreshold", 3.0f); // Anything over 2.0 turns off edge rendering
        }
            
    }
    args.setMacro("IS_2D_ARRAY", m_texture->dimension() == Texture::DIM_2D_ARRAY);
    args.setMacro("IS_3D", m_texture->dimension() == Texture::DIM_3D);
    
}


void GuiTextureBox::drawTexture(RenderDevice* rd, const Rect2D& r) const {
    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

    // If this is a depth texture, make sure we flip it into normal read mode.
 //   Texture::DepthReadMode oldReadMode = m_texture->settings().depthReadMode;
//    m_texture->setDepthReadMode(Texture::DEPTH_NORMAL);

    // The GuiTextureBox inspector can directly manipulate this value,
    // so it might not reflect the value we had at the last m_settings
    // call.
    const_cast<GuiTextureBox*>(this)->setSettings(m_settings);

    // Draw texture
    if (m_settings.needsShader()) {
        Args args;
        const_cast<GuiTextureBox*>(this)->setShaderArgs(args, m_texture->isCubeMap());
        args.setRect(r);
       
        if (m_texture->isCubeMap()) {
            LAUNCH_SHADER_WITH_HINT("GuiTextureBox_Cubemap.pix", args, m_texture->name());
        } else {
            LAUNCH_SHADER_WITH_HINT("GuiTextureBox_2D.pix", args, m_texture->name());
        }
        
    } else {
        Draw::rect2D(r, rd, Color3::white(), m_texture);
    }
}


void GuiTextureBox::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    if (! m_visible) {
        return;
    }  

    int w = 0;
    int h = 0;
    // Find the mouse position

    if (m_embeddedMode) {
        theme->renderCanvas(canvasRect(), m_enabled && ancestorsEnabled, focused(), m_caption, m_captionHeight);
    } else {
        //theme->renderButtonBorder(canvasRect(), mouseOver(), GuiTheme::TOOL_BUTTON_STYLE);
    }
    const CoordinateFrame& matrix = rd->objectToWorldMatrix();

    if (m_texture) {
        // Shrink by the border size to save space for the border,
        // and then draw the largest rect that we can fit inside.
        Rect2D r = m_texture->rect2DBounds();
        if (m_texture->isCubeMap()) {
            r = r * Vector2(2.0f, 1.0f);
        }
        r = r + (m_offset - r.center());
        r = r * m_zoom;
        r = r + m_clipBounds.center();
        
        theme->pauseRendering();
        {
            // Merge with existing clipping region!
            Rect2D oldClip = rd->clip2D();
            // Scissor region ignores transformation matrix
            Rect2D newClip = m_clipBounds + matrix.translation.xy();

            rd->setClip2D(oldClip.intersect(newClip));

            drawTexture(rd, r);

            if (m_texture) {
                w = m_texture->width();
                h = m_texture->height();
                GuiTheme::TextStyle style = theme->defaultStyle();
                const Color3& front = Color3::white();
                const Color3& back  = Color3::black();
                if (min(m_clipBounds.width(), m_clipBounds.height()) <= 128) {
                    style.size = 12;
                } else {
                    style.size = 14;
                }
                
                // Display coords and value when requested
                // (note that the manager may be null while we are waiting to be added)
                if (m_showInfo && (window()->manager() != NULL) && (window()->window()->mouseHideCount() < 1)) {
                    // Find the mouse position
                    Vector2 mousePos;
                    uint8 ignore;
                    window()->window()->getRelativeMouseState(mousePos, ignore);
                    // Make relative to the control
                    mousePos -= matrix.translation.xy();
                    if (m_clipBounds.contains(mousePos) && r.contains(mousePos)) {
                        mousePos -= r.x0y0();
                        // Convert to texture coordinates
                        mousePos *= Vector2((float)w, (float)h) / r.wh();
                        mousePos *= (1.0f / powf(2.0f, float(m_settings.mipLevel)));
                        //screenPrintf("w=%d h=%d", w, h);
                        int ix = iFloor(mousePos.x);
                        int iy = iFloor(mousePos.y);

                        if (ix >= 0 && ix < w && iy >= 0 && iy < h) {
                            if (m_readbackXY.x != ix || m_readbackXY.y != iy) {
                                m_readbackXY.x = ix;
                                m_readbackXY.y = iy;
                                m_texel = m_texture->readTexel(ix, iy, rd, m_settings.mipLevel, m_settings.layer);
                            }
                        }
                    }
                } 

                // Render the label
                computeSizeString();

                if (! m_embeddedMode) {
                    const float sizeHeight = theme->bounds(m_lastSizeCaption).y;
                    style.font->draw2D(rd, m_caption        , m_clipBounds.x0y1() - Vector2(-5, sizeHeight + theme->bounds(m_caption).y), style.size, front, back);
                    style.font->draw2D(rd, m_lastSizeCaption, m_clipBounds.x0y1() - Vector2(-5, sizeHeight), style.size, front, back);
                }

                //Draw::rect2DBorder(r, rd, Color3::black(), 0, BORDER);      

            }
        }
        theme->resumeRendering();      
    }
}


void GuiTextureBox::computeSizeString() const {
    const int w = m_texture->width();
    const int h = m_texture->height();
    const ImageFormat* fmt = m_texture->format();

    if ((m_lastSize.x != w) || (m_lastSize.y != h) || (fmt != m_lastFormat)) {
        m_lastSize.x = w;
        m_lastSize.y = h;
        m_lastFormat = fmt;

        // Avoid computing this every frame
        String s;
        if (w == h) {
            // Use ASCII squared character
            s = format("%d\xB2", w);
        } else {
            s = format("%dx%d", w, h);
        }

        s += " " + fmt->name();
        m_lastSizeCaption = GuiText(s, shared_ptr<GFont>(), 14, Color3::white(), Color3::black());
    }
}

#define ZOOM_FACTOR (sqrt(2.0f))

void GuiTextureBox::zoomIn() {
    m_zoom *= ZOOM_FACTOR;
}


void GuiTextureBox::zoomOut() {
    m_zoom /= ZOOM_FACTOR;
}


void GuiTextureBox::setViewZoom(float z) {
    m_zoom = z;
}


void GuiTextureBox::setViewOffset(const Vector2& x) {
    m_offset = x;
}


void GuiTextureBox::zoomToFit() {
    if (m_texture) {
        Vector2 w = m_texture->vector2Bounds();
        Rect2D r = m_clipBounds.expand(-BORDER).largestCenteredSubRect(w.x, w.y);
        m_zoom = r.width() / w.x;
        m_offset = Vector2::zero();
    } else {
        zoomTo1();
    }
}


void GuiTextureBox::zoomTo1() {
    m_zoom = 1.0;
    m_offset = Vector2::zero();
}
    

void GuiTextureBox::findControlUnderMouse(Vector2 mouse, GuiControl*& control) {
    if (! m_enabled || ! m_rect.contains(mouse) || ! m_visible) {
        return;
    }

    control = this;
    mouse -= m_clientRect.x0y0();

}

    
void GuiTextureBox::setTexture(const shared_ptr<Texture>& t) {
    if (m_texture == t) {
        // Setting back to the same texture
        return;
    }

    const bool firstTime = isNull(m_texture);

    m_texture = t;
    shared_ptr<GuiTextureBoxInspector> ins = m_inspector.lock();
    if (ins) {
        // The inspector now has the wrong texture in it and it would require a 
        // lot of GUI changes to update it, so we simply close that window.
        window()->manager()->remove(ins);
    }
    if (t) {
        setSettings(t->visualization);

        if (firstTime) {
            zoomToFit();
        }
    }
}

void GuiTextureBox::setCaption(const GuiText& text) {
    m_caption = text;
    setCaptionWidth(0);
    setCaptionHeight(0);
}

void GuiTextureBox::setSettings(const Texture::Visualization& s) {
    // Check the settings for this computer
    m_settings = s;
}



} // G3D

