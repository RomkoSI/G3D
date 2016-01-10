/**
   \file EventViewer.cpp
 
   \created 2012-09-24
   \edited  2014-10-03
*/
#include "EventViewer.h"


EventViewer::EventViewer() : m_lastEventWasMouseMove(false),  m_showMouseMoveEvents(true) {
}


void EventViewer::onInit(const String& filename) {
    font = GFont::fromFile(System::findDataFile("arial.fnt"));
}


bool EventViewer::onEvent(const GEvent& e, App* app) {
    if (e.type == GEventType::MOUSE_MOTION) {
        if (! m_showMouseMoveEvents) {
            // Don't print this event
            return false;
        } else {
            // Condense mouse move events
            if (m_lastEventWasMouseMove) {
                // Replace the last event
                eventQueue.popBack();
            }

            m_lastEventWasMouseMove = true;
        }
    } else {
        m_lastEventWasMouseMove = false;
    }

    eventQueue.pushBack("[" + System::currentTimeString() + "]  " + e.toString());
    
    return false;
}


void EventViewer::onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) {
    app->colorClear = Color3::white();

    rd->push2D(); {
        printEventQueue(rd);
        printWindowInformation(rd);
        printJoystickInformation(rd);
    } rd->pop2D();
}


static String formatDimensions(const G3D::String& description, const Vector2& dimension) {
    return format("%s: %dx%d", description.c_str(), iRound(dimension.x), iRound(dimension.y));
}


void EventViewer::printWindowInformation(RenderDevice* rd) {
    const OSWindow* window = OSWindow::current();

    const float fontSize = 13;
    Point2 p(500, 10);

    p.y += font->draw2D(rd, format("G3D OSWindow: %s", window->className().c_str()), p, fontSize).y;
    p.y += font->draw2D(rd, format("Underlying API: %s %s", window->getAPIName().c_str(), window->getAPIVersion().c_str()), p, fontSize).y;
    p.y += font->draw2D(rd, format("numDisplays: %d", OSWindow::numDisplays()), p, fontSize).y;
    p.y += font->draw2D(rd, formatDimensions("primaryDisplaySize", OSWindow::primaryDisplaySize()), p, fontSize).y;
    const Vector2int32 w = OSWindow::primaryDisplayWindowSize();
    p.y += font->draw2D(rd, formatDimensions("primaryWindowSize",Vector2((float)w.x, (float)w.y)), p, fontSize).y;
    p.y += font->draw2D(rd, formatDimensions("virtualDisplaySize", OSWindow::virtualDisplaySize()), p, fontSize).y;
    p.y += font->draw2D(rd, format("numJoysticks: %d", window->numJoysticks()), p, fontSize).y;
}


void EventViewer::printJoystickInformation(RenderDevice* rd) {
    const OSWindow* window = OSWindow::current();

    const float fontSize = 10;

    const Vector2 indent(50, 0);

    for (int j = 0; j < window->numJoysticks(); ++j) {
        Point2 p(450 + j * 200.0f, 200.0f);

        p.y += font->draw2D(rd, format("Joystick %d", j), p, fontSize).y;
        {
            // Indent details
            p.y += font->draw2D(rd, format("Name: %s", window->joystickName(j).c_str()), p + indent, fontSize).y;

            Array<float> axis;
            Array<bool> button;
            window->getJoystickState(j, axis, button);

            p.y += font->draw2D(rd, "Axes:", p + indent, fontSize).y;
            for (int a = 0; a < axis.length(); ++a) {
                p.y += font->draw2D(rd, format("axis[%02d]: %f", a, axis[a]), p + indent * 2, fontSize).y;
            }

            p.y += font->draw2D(rd, "Buttons:", p + indent, fontSize).y;
            for (int b = 0; b < button.length(); ++b) {
                p.y += font->draw2D(rd, format("button[%02d]: %d", b, button[b]), p + indent * 2, fontSize).y;
            }
        }
        // Skip a line
        p.y += fontSize;
    }
}

void EventViewer::printEventQueue(RenderDevice* rd) {
    const Rect2D& windowBounds = rd->viewport();

    const float fontSize = 13;
    float y = windowBounds.y1() - fontSize * 1.5f;
    int i;
    for (i = eventQueue.length() - 1; (i >= 0) && (y > 0); --i) {
        y -= font->draw2D(rd, eventQueue[i], Point2(10, y), fontSize).y * 1.1f;
    }
    
    // Remove i elements from the top, since they've scrolled off the top of the screen.
    while (i > 0) {
        eventQueue.popFront();
        --i;
    }
}
