/**
 @file GuiWindow.cpp
 
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2007-06-02
 \edited  2013-08-20

 G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#include "G3D/platform.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/GuiControl.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GLCaps.h"

namespace G3D {

shared_ptr<GuiWindow> GuiWindow::create
(const GuiText& label, 
 const shared_ptr<GuiTheme>& theme, 
 const Rect2D& rect, 
 GuiTheme::WindowStyle style, 
 CloseAction close) {

     return shared_ptr<GuiWindow>(new GuiWindow(label, isNull(theme) ? GuiTheme::lastThemeLoaded.lock() : theme, rect, style, close));
}


void GuiWindow::setCaption(const GuiText& text) {
    m_text = text;
}


GuiWindow::GuiWindow() 
    : modal(NULL), 
      m_text(""), 
      m_rect(Rect2D::xywh(0,0,0,0)),
      m_visible(false), 
      m_minSize(0, 0),
      m_resizable(false),
      m_style(GuiTheme::NO_WINDOW_STYLE),
      m_closeAction(NO_CLOSE), 
      inDrag(false),
      inResize(false),
      mouseOverGuiControl(NULL), 
      keyFocusGuiControl(NULL),
      m_enabled(true),
      m_focused(false),
      m_mouseVisible(false),
      m_rootPane(NULL) {
}


GuiWindow::GuiWindow(const GuiText& text, shared_ptr<GuiTheme> skin, const Rect2D& rect, GuiTheme::WindowStyle style, CloseAction close) 
    : modal(NULL),
      m_text(text),
      m_rect(rect),
      m_clientRect(rect), 
      m_visible(true), 
      m_minSize(0, 0),
      m_resizable(false),
      m_style(style),
      m_closeAction(close), 
      m_theme(skin), 
      inDrag(false),
      inResize(false),
      dragOriginalRect(Rect2D::empty()),
      mouseOverGuiControl(NULL), 
      keyFocusGuiControl(NULL),
      m_enabled(true),
      m_focused(false),
      m_mouseVisible(false) {

    debugAssertM(notNull(skin), "Cannot initialize a GuiWindow with a NULL theme");
    debugAssertM(! rect.isEmpty(), "Pass a non-empty rectangle for the initial bounds.  Rect2D() creates an empty rectangle, which now is different from a zero-area rectangle at zero.");
    setRect(rect);
    m_rootPane = new GuiPane(this, "", clientRect() - clientRect().x0y0(), GuiTheme::NO_PANE_STYLE);
}


GuiWindow::~GuiWindow() {
    delete m_rootPane;
}


void GuiWindow::changeKeyFocus(GuiControl* oldControl, GuiControl* newControl) {
    GEvent e;
    e.gui.type = GEventType::GUI_KEY_FOCUS;
    if (notNull(oldControl)) {
        e.gui.control = oldControl;
        fireEvent(e);
    }

    keyFocusGuiControl = newControl;
    if (notNull(keyFocusGuiControl)) {
        keyFocusGuiControl = newControl;
        e.gui.control = keyFocusGuiControl;
        fireEvent(e);
    }
}


void GuiWindow::setKeyFocusControl(GuiControl* c) {
    if (c->enabled() && c->visible() && (c != keyFocusGuiControl)) {
        changeKeyFocus(keyFocusGuiControl, c);
    }
}


void GuiWindow::increaseBounds(const Vector2& extent) {
    if ((m_clientRect.width() < extent.x) || (m_clientRect.height() < extent.y)) {
        // Create the new client rect
        Rect2D newRect = Rect2D::xywh(Vector2(0,0), extent.max(m_clientRect.wh()));

        // Transform the client rect into an absolute rect
        if (m_style != GuiTheme::NO_WINDOW_STYLE) {
            newRect = m_theme->clientToWindowBounds(newRect, GuiTheme::WindowStyle(m_style));
        }

        // The new window has the old position and the new width
        setRect(Rect2D::xywh(m_rect.x0y0(), newRect.wh()));
    }
}


void GuiWindow::morphTo(const Rect2D& r) {
    // Terminate any drag or resize
    inDrag = false;
    inResize = false;

    debugAssert(! r.isEmpty());
    m_morph.morphTo(rect(), r);
}


void GuiWindow::setRect(const Rect2D& r) {
    debugAssert(! r.isEmpty());

    m_rect = Rect2D::xywh( Vector2(r.x0(), r.y0() > 0 ? r.y0() : 0), r.wh() );
    m_morph.active = false;
    
    if (m_style == GuiTheme::NO_WINDOW_STYLE) {
        m_clientRect = m_rect;
    } else if (m_theme) {
        m_clientRect = m_theme->windowToClientBounds(m_rect, GuiTheme::WindowStyle(m_style));
    }
}


void GuiWindow::setKeyFocusOnNextControl() {
    // For now, just lose focus
    changeKeyFocus(keyFocusGuiControl, NULL);

    // TODO
    /*
     Find the pane containing the current control.
          Find the index of the control that has focus
          Go to the next control in that pane.  
          If there are no more controls, go to the parent's next pane.  
          If there are no more panes, go to the parent's controls.

     If we run out on the root pane, go to its deepest pane child's first control.

     ...
    */
}

void GuiWindow::onUserInput(UserInput* ui) {
    // Not in focus if the mouse is invisible
    m_mouseVisible = (ui->window()->mouseHideCount() <= 0);

    m_focused =
        m_enabled &&
        m_visible &&
        (m_manager->focusedWidget().get() == this) &&
        m_mouseVisible;
    Vector2 mouse = ui->mouseXY();
    m_mouseOver = contains(mouse);
    if (! focused()) {
        return;
    }

   
    mouseOverGuiControl = NULL;

    if (inDrag) {
        Rect2D windowRect = window()->clientRect();
        Rect2D newRect = dragOriginalRect + mouse - dragStart;
        float x0 = fmin(windowRect.width() - 30, newRect.x0());
        x0 = fmax(10.0f - newRect.width(), x0);
        float y0 = fmin(windowRect.height() - 30, newRect.y0());
        setRect(Rect2D::xywh(x0, y0, newRect.width(), newRect.height()));
        return;
    } else if (inResize) {
        setRect(Rect2D::xywh(dragOriginalRect.x0y0(), m_minSize.max(dragOriginalRect.wh() + mouse - dragStart)));
        return;
    }

    m_closeButton.mouseOver = false;
    if (m_mouseOver) {
        // The mouse is over this window, update the mouseOver control
        
        if ((m_closeAction != NO_CLOSE) && (m_style != GuiTheme::NO_WINDOW_STYLE) && (m_style != GuiTheme::PANEL_WINDOW_STYLE)) {
            m_closeButton.mouseOver = 
                m_theme->windowToCloseButtonBounds(m_rect, GuiTheme::WindowStyle(m_style)).contains(mouse);
        }

        mouse -= m_clientRect.x0y0();
        m_rootPane->findControlUnderMouse(mouse, mouseOverGuiControl);
    }
}


void GuiWindow::onPose(Array<shared_ptr<Surface> >& posedArray, Array<shared_ptr<Surface2D> >& posed2DArray) {
    (void)posedArray;
    if (m_visible) {
        posed2DArray.append(dynamic_pointer_cast<Surface2D>(shared_from_this()));
    }
}


static GEvent makeRelative(const GEvent& e, const Vector2& clientOrigin) {
    debugAssert(! isNaN(clientOrigin.x) || ! isNaN(clientOrigin.y));
    GEvent out(e);

    switch (e.type) {
    case GEventType::MOUSE_MOTION:
//        debugAssert(out.motion.x >= clientOrigin.x);  TODO: Enable and debug for G3D 9 release
//        debugAssert(out.motion.y >= clientOrigin.y);
        out.motion.x -= (uint16)clientOrigin.x;
        out.motion.y -= (uint16)clientOrigin.y;
        break;

    case GEventType::MOUSE_BUTTON_DOWN:
    case GEventType::MOUSE_BUTTON_UP:
    case GEventType::MOUSE_BUTTON_CLICK:
//        debugAssert(out.button.x >= clientOrigin.x);
//        debugAssert(out.button.y >= clientOrigin.y);
        out.button.x -= (uint16)clientOrigin.x;
        out.button.y -= (uint16)clientOrigin.y;        
        break;
    }

    return out;
}

bool GuiWindow::processMouseButtonDownEventForFocusChangeAndWindowDrag(const GEvent& event) {
    bool consumed = false;

    // Mouse down; change the focus
    Point2 mouse(event.button.x, event.button.y);

    if (! focused()) {
        // Set focus
        bool moveToFront = (m_style != GuiTheme::NO_WINDOW_STYLE) && (m_style != GuiTheme::PANEL_WINDOW_STYLE);
        m_manager->setFocusedWidget(dynamic_pointer_cast<Widget>(shared_from_this()), moveToFront);
        m_focused = true;

        // Most windowing systems do not allow the original click
        // to reach a control if it was consumed on focusing the
        // window.  However, we deliver events because, for most
        // 3D programs, the multiple windows are probably acting
        // like tool windows and should not require multiple
        // clicks for selection.
    }

    Rect2D titleRect;
    Rect2D closeRect;

    titleRect = m_theme->windowToTitleBounds(m_rect, GuiTheme::WindowStyle(m_style));
    closeRect = m_theme->windowToCloseButtonBounds(m_rect, GuiTheme::WindowStyle(m_style));


    if ((m_closeAction != NO_CLOSE) && closeRect.contains(mouse)) {
        close();
        return true;
    }

    GuiControl* oldFocusControl = keyFocusGuiControl;
    if (titleRect.contains(mouse) && (m_style != GuiTheme::MENU_WINDOW_STYLE)) {
        inDrag = true;
        keyFocusGuiControl = NULL;
        dragStart = mouse;
        dragOriginalRect = m_rect;
        return true;

    } else if (resizable() && resizeFrameContains(mouse)) {
        // Resizable border click
        inResize = true;
        keyFocusGuiControl = NULL;
        dragStart = mouse;
        dragOriginalRect = m_rect;
        return true;

    } else {
        // Interior click
        mouse -= m_clientRect.x0y0();

        keyFocusGuiControl = NULL;
        m_rootPane->findControlUnderMouse(mouse, keyFocusGuiControl);
    }

    if (oldFocusControl != keyFocusGuiControl) {
        // Tell the controls that focus changed
        changeKeyFocus(oldFocusControl, keyFocusGuiControl);
    }

    if (m_style != GuiTheme::NO_WINDOW_STYLE) {
        if (isNull(keyFocusGuiControl)) {
            onMouseButtonDown(event);
        }

        // Consume the click, since it was somewhere on this window (it may still
        // be used by another one of the controls on this window).
        return true;
    }

    return consumed;
}


void GuiWindow::onMouseButtonDown(const GEvent& event) {
    (void)event;
}


bool GuiWindow::resizeFrameContains(const Point2& pt) const {
    const GuiTheme::Window& prop = m_theme->m_window[m_style];

    if (prop.resizeMode == GuiTheme::Window::FRAME) {
        return (m_rect.contains(pt) &&
            ((pt.x <= m_rect.x0() + prop.resizeFrameThickness) ||
             (pt.x >= m_rect.x1() - prop.resizeFrameThickness)) &&
            ((pt.y <= m_rect.y0() + prop.resizeFrameThickness) ||
             (pt.y >= m_rect.y1() - prop.resizeFrameThickness)));
    } else {
        // SQUARE mode
        return Rect2D::xywh(m_rect.x1y1() - Vector2(prop.resizeFrameThickness, prop.resizeFrameThickness), Vector2(prop.resizeFrameThickness, prop.resizeFrameThickness)).contains(pt);
    }
}


bool GuiWindow::contains(const Point2& pt) const {
    return m_rect.contains(pt);
}


bool GuiWindow::onEvent(const GEvent& event) {
    if (! m_mouseVisible || ! m_visible) {
        // Can't be using the GuiWindow if the mouse isn't visible or the gui isn't visible
        return false;
    }

    if (! m_enabled) {
        return false;
    }

    if ((inResize || inDrag) && (event.type == GEventType::MOUSE_MOTION)) {
        return true;
    }

    bool consumedForFocus = false;

    switch (event.type) {
    case GEventType::MOUSE_BUTTON_DOWN:
        if (! contains(event.mousePosition())) {
            // The click was not on this window.  Lose focus if we have it
            m_manager->defocusWidget(dynamic_pointer_cast<Widget>(shared_from_this()));
            return false;
        } else {
            consumedForFocus = processMouseButtonDownEventForFocusChangeAndWindowDrag(event);
        }
        break;

    case GEventType::MOUSE_BUTTON_UP:
        if (inDrag) {
            // We're dragging the entire window--the controls don't need to know
            inDrag = false;
            return true;
        } else if (inResize) {
            inResize = false;
            return true;
        }
        break;

    default:;
    }

    // If this window is not in focus, don't bother checking to see if
    // its controls will receive the event.
    if (! focused()) {
        return consumedForFocus;
    }
    
    bool consumed = false;

    if (keyFocusGuiControl != NULL) {
        // Deliver event to the control that has focus
      
        // Walk the GUI hierarchy  
        for (GuiControl* target = keyFocusGuiControl; (target != NULL) && ! consumed; target = target->m_parent) {
            if (event.isMouseEvent()) {

                Point2 origin = m_clientRect.x0y0();

                // Make the event relative by accumulating all of the transformations
                GuiContainer* p = target->m_parent;
                while (p != NULL) {
                    origin += p->clientRect().x0y0();
                    p = p->m_parent;
                }

                const GEvent& e2 = makeRelative(event, origin);
                const Point2& pt = e2.mousePosition();
                
                // Notify all controls that are parents of the one with focus of mouse up and motion events (since they may be in the middle of a drag)
                // but only deliver mouse down events to controls that are under the mouse.
                if ((event.type == GEventType::MOUSE_BUTTON_UP) || (event.type == GEventType::MOUSE_MOTION) || target->clickRect().contains(pt)) {
                    consumed = target->onEvent(e2);
                }

            } else {
                consumed = target->onEvent(event);
            }
        }
    }

    // If the controls inside the window didn't consume the event, still consume it if used for
    // focus or drag.
    consumed = consumed || consumedForFocus;

    // TODO: make the code below propagate back towards the GUI root, checking to see if we ever hit some parent of 
    // keyFocusGuiControl

    // If not consumed, also deliver mouse motion events to the control under the mouse
    if (! consumed && (event.type == GEventType::MOUSE_MOTION)) {
        // Deliver to the control under the mouse
        Vector2 mouse(event.button.x, event.button.y);
        mouse -= m_clientRect.x0y0();

        GuiControl* underMouseControl = NULL;
        m_rootPane->findControlUnderMouse(mouse, underMouseControl);

        if (underMouseControl && underMouseControl->enabled() && (underMouseControl != keyFocusGuiControl)) {
            Vector2 origin = m_clientRect.x0y0();
            GuiContainer* p = underMouseControl->m_parent;
            while (p != NULL) {
                origin += p->clientRect().x0y0();
                p = p->m_parent;
            }

            consumed = underMouseControl->onEvent(makeRelative(event, origin));
        }
    }

    return consumed;
}


void GuiWindow::close() {
    switch (m_closeAction) {
    case NO_CLOSE:
      debugAssertM(false, "Internal Error");
      break;
    
    case HIDE_ON_CLOSE:
        setVisible(false);

        // Intentionally fall through

    case IGNORE_CLOSE:
        {
            GEvent e;
            e.guiClose.type = GEventType::GUI_CLOSE;
            e.guiClose.window = this;
            fireEvent(e);
        }
        break;
    
    case REMOVE_ON_CLOSE:
        {
            GEvent e;
            e.guiClose.type = GEventType::GUI_CLOSE;
            e.guiClose.window = this;
            fireEvent(e);
            manager()->remove(dynamic_pointer_cast<Widget>(shared_from_this()));
        }
        break;
    }
}


void GuiWindow::pack() {
    setRect(Rect2D::xywh(m_rect.x0y0(), Vector2::zero()));
    m_rootPane->pack();
    increaseBounds(m_rootPane->rect().wh());
}

void GuiWindow::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    if (m_morph.active) {
        m_morph.update(this);
    }    
}


void GuiWindow::renderBackground(RenderDevice* rd) const {
    bool disappears = m_style == GuiTheme::FULL_DISAPPEARING_STYLE || m_style == GuiTheme::PARTIAL_DISAPPEARING_STYLE;
    bool hasClose = (m_closeAction != NO_CLOSE) && !(disappears && !(m_mouseOver || inDrag));

    GuiTheme::WindowStyle style = m_style;
    if (disappears) {
        if (style == GuiTheme::FULL_DISAPPEARING_STYLE) {
            style = (m_mouseOver || inDrag) ? GuiTheme::NORMAL_WINDOW_STYLE : GuiTheme::NO_WINDOW_STYLE;
        } else {
            style = (m_mouseOver || inDrag) ? GuiTheme::NORMAL_WINDOW_STYLE : GuiTheme::MENU_WINDOW_STYLE;
        }
    }
    if ((style != GuiTheme::NO_WINDOW_STYLE)) {
        m_theme->renderWindow(m_rect, focused(), hasClose, m_closeButton.down,
                            m_closeButton.mouseOver, m_text, style);
    } else {
        debugAssertM((m_style != style) || (m_closeAction == NO_CLOSE), "Windows without frames cannot have a close button.");
    }
}


void GuiWindow::render(RenderDevice* rd) const {
    debugAssertGLOk();
    m_theme->beginRendering(rd); {

        debugAssertGLOk();
        renderBackground(rd);
        debugAssertGLOk();
        
        // static const bool DEBUG_WINDOW_SIZE = false;
        
        m_theme->pushClientRect(m_clientRect); {

            m_rootPane->render(rd, m_theme, m_enabled);
            /*if (DEBUG_WINDOW_SIZE) {
                // Code for debugging window sizes
                rd->endPrimitive();
                rd->pushState();
                Draw::rect2D(Rect2D::xywh(-100, -100, 1000, 1000), rd, Color3::red());
                rd->popState();
                rd->beginPrimitive(PrimitiveType::QUADS);
            }*/

        } m_theme->popClientRect();
        /*
        if (DEBUG_WINDOW_SIZE) {        
            // Code for debugging window sizes
            rd->endPrimitive();
            rd->pushState();
            Draw::rect2D(m_rect + Vector2(20,0), rd, Color3::blue());
            rd->popState();
            rd->beginPrimitive(PrimitiveType::QUADS);
        }*/
    } m_theme->endRendering();
    
}


void GuiWindow::moveTo(const Vector2& position) {
    setRect(Rect2D::xywh(position, rect().wh()));
}


void GuiWindow::moveToCenter() {
    const OSWindow* w = (window() != NULL) ? window() : OSWindow::current();
    setRect(Rect2D::xywh((w->clientRect().wh() - rect().wh()) / 2.0f, rect().wh()));
}

////////////////////////////////////////////////////////////
// Modal support

void GuiWindow::showModal(OSWindow* osWindow, ModalEffect e) {
    modal = new Modal(osWindow, e);

    const int oldCount = osWindow->inputCaptureCount();
    osWindow->setInputCaptureCount(0);
    modal->run(dynamic_pointer_cast<GuiWindow>(shared_from_this()));
    osWindow->setInputCaptureCount(oldCount);
    delete modal;
    modal = NULL;
}


void GuiWindow::showModal(shared_ptr<GuiWindow> parent, ModalEffect e) {
    showModal(parent->window(), e);
}


GuiWindow::Modal::Modal(OSWindow* osWindow, ModalEffect e) : osWindow(osWindow), m_modalEffect(e) {
    manager = WidgetManager::create(osWindow);
    renderDevice = osWindow->renderDevice();
    userInput = new UserInput(osWindow);

    viewport = renderDevice->viewport();

    // Grab the screen texture
    bool generateMipMaps = false;
    if (GLCaps::supports_GL_ARB_texture_non_power_of_two()) {
        image = Texture::createEmpty("Old screen image", (int)viewport.width(), (int)viewport.height(), 
                                     ImageFormat::RGB8(), Texture::DIM_2D, generateMipMaps);
    } else {
        image = Texture::createEmpty("Old screen image", 512, 512,
                                     ImageFormat::RGB8(), Texture::DIM_2D, generateMipMaps);
    }

    RenderDevice::ReadBuffer old = renderDevice->readBuffer();
    renderDevice->setReadBuffer(RenderDevice::READ_FRONT);
    renderDevice->copyTextureFromScreen(image, viewport);
    renderDevice->setReadBuffer(old);
}


GuiWindow::Modal::~Modal() {
    delete userInput;
}


void GuiWindow::Modal::run(shared_ptr<GuiWindow> dialog) {
    this->dialog = dialog.get();
    manager->add(dialog);
    manager->setFocusedWidget(dialog);
    dialog->setVisible(true);

    if (osWindow->requiresMainLoop()) {
        osWindow->pushLoopBody(loopBody, this);
    } else {
        do {
            oneFrame();
        } while (dialog->visible());
    }

}


void GuiWindow::Modal::loopBody(void* me) {
    reinterpret_cast<GuiWindow::Modal*>(me)->oneFrame();
}


void GuiWindow::Modal::oneFrame() {
    double desiredFrameDuration = 1 / 60.0;

    processEventQueue();

    manager->onUserInput(userInput);

    manager->onNetwork();

    {
        // Pretend that we acheive our frame rate
        RealTime rdt = desiredFrameDuration;
        SimTime  sdt = float(desiredFrameDuration);
        SimTime  idt = float(desiredFrameDuration);

        manager->onSimulation(rdt, sdt, idt);
    }

    // Logic
    manager->onAI();

    // Sleep to keep the frame rate at about the desired frame rate
    System::sleep(0.9 * desiredFrameDuration);

    // Graphics
    renderDevice->beginFrame();
    {
        renderDevice->push2D();
        {
            // Draw the background
            const bool oldY = renderDevice->invertY();
            renderDevice->setInvertY(! oldY);
            switch (m_modalEffect) {
            case MODAL_EFFECT_NONE:
                Draw::rect2D(viewport, renderDevice, Color3::white(), image);
                break;

            case MODAL_EFFECT_DARKEN:
                Draw::rect2D(viewport, renderDevice, Color3::white() * 0.5f, image);
                break;

            case MODAL_EFFECT_DESATURATE:
                Draw::rect2D(viewport, renderDevice, Color3::white(), image);
                // Desaturate the image by drawing white over it
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                Draw::rect2D(viewport, renderDevice, Color4(Color3::white(), 0.9f));            
                break;

            case MODAL_EFFECT_LIGHTEN:
                Draw::rect2D(viewport, renderDevice, Color3::white() * 1.5f, image);            
                break;
            }
            renderDevice->setInvertY(oldY);
        }
        renderDevice->pop2D();

        renderDevice->pushState();
        {
            Array<shared_ptr<Surface> > posedArray; 
            Array<shared_ptr<Surface2D> > posed2DArray;

            manager->onPose(posedArray, posed2DArray);
            
            if (posed2DArray.size() > 0) {
                renderDevice->push2D();
                Surface2D::sort(posed2DArray);
                for (int i = 0; i < posed2DArray.size(); ++i) {
                    posed2DArray[i]->render(renderDevice);
                }
                renderDevice->pop2D();
            }
        }
        renderDevice->popState();
    }
    
    renderDevice->endFrame();
    renderDevice->swapBuffers();     
    if (! dialog->visible() && osWindow->requiresMainLoop()) {
        osWindow->popLoopBody();
    }
}

void GuiWindow::Modal::processEventQueue() {
    userInput->beginEvents();

    // Event handling
    GEvent event;
    while (osWindow->pollEvent(event)) {
        if (WidgetManager::onEvent(event, manager)) {
            continue;
        }

        switch(event.type) {
        case GEventType::QUIT:
            exit(0);
            break;

        default:;
        }

        userInput->processEvent(event);
    }

    userInput->endEvents();
}


Rect2D GuiWindow::bounds() const {
    return m_rect;
}


float GuiWindow::depth() const {
    if ((m_style == GuiTheme::NO_WINDOW_STYLE) || (m_style == GuiTheme::PANEL_WINDOW_STYLE)) {
        // Draw in back, regardless of where we are in the focus stack
        return 1.0;
    } else {
        return m_depth;
    }
}

}
