/**
   \file EventViewer.h
 
   \created 2012-09-24
   \edited  2014-09-27
 */
#ifndef EventViewer_h
#define EventViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class EventViewer : public Viewer {
private:
    Queue<String>       eventQueue;
    bool                m_lastEventWasMouseMove;

    shared_ptr<GFont>   font;

    bool                m_showMouseMoveEvents;


    /** Prints the event queue, flushing events that are not displayed
        on screen */
    void printEventQueue(RenderDevice* rd);

    void printWindowInformation(RenderDevice* rd);

    void printJoystickInformation(RenderDevice* rd);

public:

    EventViewer();
    virtual void onInit(const String& filename) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override;
    virtual bool onEvent(const GEvent& e, App* app) override;
};

#endif 
