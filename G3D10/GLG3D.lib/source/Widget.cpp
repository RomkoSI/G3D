/**
 \file Widget.cpp

 \maintainer Morgan McGuire, morgan3d@users.sourceforge.net

 \created 2006-04-22
 \edited  2014-07-30
*/

#include "GLG3D/Widget.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GEvent.h"
#include "GLG3D/GuiContainer.h"
#include "G3D/Ray.h"

namespace G3D {
    
void EventCoordinateMapper::update(RenderDevice* rd) {
    // Ignore depth-only rendering for shadow maps and early z
    if (rd->colorWrite()) {
        m_lastProjection = Projection(rd->projectionMatrix());
        m_lastCameraToWorldMatrix = rd->cameraToWorldMatrix();
        m_lastViewport = rd->viewport();
        
        OSWindow* osWindow = rd->window();
        m_lastGuardBandOffset = max(Point2(0.0f, 0.0f), (m_lastViewport.wh() - osWindow->clientRect().wh()) / 2.0f);
    }
}


Ray EventCoordinateMapper::eventPixelToCameraSpaceRay(const Point2& pixel) const {
	debugAssertM(! m_lastGuardBandOffset.isNaN(), "Invoked eventPixelToCameraSpaceRay before update");

	if (m_lastGuardBandOffset.isNaN()) {
		const_cast<EventCoordinateMapper*>(this)->m_lastGuardBandOffset = Vector2::zero();
	}
    return m_lastCameraToWorldMatrix.toWorldSpace(m_lastProjection.ray(pixel.x + m_lastGuardBandOffset.x + 0.5f, pixel.y + m_lastGuardBandOffset.y + 0.5f, m_lastViewport));
}


/////////////////////////////////////////////////////////////////////////////////////////////

void Widget::fireEvent(const GEvent& event) {
    if(notNull(m_manager)) {
        m_manager->fireEvent(event);
    }
}


OSWindow* Widget::window() const {
    if (isNull(m_manager)) {
        return NULL;
    } else {
        return m_manager->window();
    }
}


OSWindow* WidgetManager::window() const {
    return m_window;
}


WidgetManager::Ref WidgetManager::create(OSWindow* window) {
    WidgetManager::Ref m(new WidgetManager());
    m->m_window = window;
    return m;
}


void WidgetManager::fireEvent(const GEvent& event) {
    if (GEventType(event.type).isGuiEvent()) {
        debugAssertM(notNull(event.gui.control), "GUI events must have non-NULL controls.");
        GuiContainer* parent = event.gui.control->m_parent;
        if (parent != NULL) {
            if (parent->onChildControlEvent(event)) {
                // The event was suppressed by the GUI hierarchy
                return;
            }
        }
    }
    m_window->fireEvent(event);
}


WidgetManager::WidgetManager() : 
    m_locked(false) {
}


int WidgetManager::size() const {
    return m_moduleArray.size();
}


const shared_ptr<Widget>& WidgetManager::operator[](int i) const {
    return m_moduleArray[i];
}


void WidgetManager::beginLock() {
    debugAssert(! m_locked);
    m_locked = true;
}


void WidgetManager::endLock() {
    debugAssert(m_locked);
    m_locked = false;

    for (int e = 0; e < m_delayedEvent.size(); ++e) {
        DelayedEvent& event = m_delayedEvent[e];
        switch (event.type) {
        case DelayedEvent::REMOVE_ALL:
            clear();
            break;

        case DelayedEvent::REMOVE:
            remove(event.module);
            break;

        case DelayedEvent::ADD:
            add(event.module);
            break;
            
        case DelayedEvent::SET_FOCUS_AND_MOVE_TO_FRONT:
            setFocusedWidget(event.module, true);
            break;

        case DelayedEvent::SET_FOCUS:
            setFocusedWidget(event.module, false);
            break;

        case DelayedEvent::SET_DEFOCUS:
            defocusWidget(event.module);
            break;

        case DelayedEvent::MOVE_TO_BACK:
            moveWidgetToBack(event.module);
            break;
        }
    }

    m_delayedEvent.fastClear();
}


void WidgetManager::remove(const shared_ptr<Widget>& m) {
    debugAssertM(contains(m), "Tried to remove a Widget that was not in the manager.");
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::REMOVE, m));
    } else {
        if (m_focusedModule == m) {
            m_focusedModule.reset();
        }
        int j = m_moduleArray.findIndex(m);
        if (j != -1) {
            m_moduleArray.remove(j);
            m->setManager(NULL);
            updateWidgetDepths();
            return;
        }
        debugAssertM(false, "Removed a Widget that was not in the manager.");
    }
}


bool WidgetManager::contains(const shared_ptr<Widget>& m) const {
    // See if there is a delayed event
    bool found = false;
    for (int i = 0; i < m_delayedEvent.size(); ++i) {
        if ((m_delayedEvent[i].type == DelayedEvent::ADD) &&
            (m_delayedEvent[i].module == m)) {
            found = true;
        }
        if ((m_delayedEvent[i].type == DelayedEvent::REMOVE) &&
            (m_delayedEvent[i].module == m)) {
            found = false;
        }
    }
    return found || m_moduleArray.contains(m);
}


void WidgetManager::add(const shared_ptr<Widget>& m) {
    debugAssert(m);
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::ADD, m));
    } else {
        // Do not add elements that already are in the manager
        if (! m_moduleArray.contains(m)) {
            if (m_focusedModule && (m_focusedModule == m_moduleArray.last())) {
                // Cannot replace the focused module at the top of the priority list
                m_moduleArray[m_moduleArray.size() - 1] = m;
                m_moduleArray.append(m_focusedModule);
            } else {
                m_moduleArray.append(m);
            }
            m->setManager(this);
        }
    }
}


shared_ptr<Widget> WidgetManager::focusedWidget() const {
    return m_focusedModule;
}


void WidgetManager::moveWidgetToBack(const Widget::Ref& widget) {
   if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::MOVE_TO_BACK, widget));
   } else {
       int i = m_moduleArray.findIndex(widget);
       if (i > 0) {
           // Found and not already at the bottom
           m_moduleArray.remove(i);
           updateWidgetDepths();
           m_moduleArray.insert(0, widget);
           
       } 
   }
}


void WidgetManager::defocusWidget(const Widget::Ref& m) {
   if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_DEFOCUS, m));
   } else if (focusedWidget().get() == m.get()) {
       setFocusedWidget(Widget::Ref());
   }    
}


static inline bool __cdecl depthGreatherThan(const Widget::Ref& elem1, const Widget::Ref& elem2) {
    return elem1->depth() > elem2->depth();
}

void WidgetManager::updateWidgetDepths() {
    for (int i = 0; i < m_moduleArray.size(); ++i) {
        // Reserve depth 1 for the background and panels and depth 0 for menus and tooltips
        m_moduleArray[i]->setDepth(1.0f - float(i + 1) / (m_moduleArray.size() + 1));
    }
    m_moduleArray.sort(depthGreatherThan);
}


void WidgetManager::setFocusedWidget(const Widget::Ref& m, bool moveToFront) {
    if (m_locked) {
        if (moveToFront) {
            m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_FOCUS_AND_MOVE_TO_FRONT, m));
        } else {
            m_delayedEvent.append(DelayedEvent(DelayedEvent::SET_FOCUS, m));
        }
    } else {

        debugAssert(isNull(m) || m_moduleArray.contains(m));

        if (m && moveToFront) {
            // Move to the first event position and let updateWidgetOrder take it from there
            int i = m_moduleArray.findIndex(m);
            m_moduleArray.remove(i);
            m_moduleArray.append(m);
            updateWidgetDepths();
        }

        m_focusedModule = m;
    }
}


void WidgetManager::clear() {
    if (m_locked) {
        m_delayedEvent.append(DelayedEvent(DelayedEvent::REMOVE_ALL));
    } else {
        m_moduleArray.clear();
        m_focusedModule.reset();
    }
}

// Iterate through all modules in priority order.
// This same iteration code is used to implement
// all Widget methods concisely.
#define ITERATOR(body)\
    beginLock();\
        for (int i = m_moduleArray.size() - 1; i >=0; --i) {\
            body;\
        }\
    endLock();


void WidgetManager::onPose(
    Array<shared_ptr<Surface> >& posedArray, 
    Array<shared_ptr<Surface2D> >& posed2DArray) {

    if (m_locked) {
        // This must be onPose for the GApp being invoked during an event callback
        // that fired during rendering. Avoid posing again during this period.
        return; 
    }

    beginLock();
    for (int i = 0; i < m_moduleArray.size(); ++i) {
        m_moduleArray[i]->onPose(posedArray, posed2DArray);
    }
    endLock();

}


void WidgetManager::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    ITERATOR(m_moduleArray[i]->onSimulation(rdt, sdt, idt));
}

/** For use with G3D::Array::sort */
template<class T>
class SortWrapper {

public:
    T               value;
    float           key;

    SortWrapper() : key(0) {}

    SortWrapper(const T& v, float k) : value(v), key(k) {}

    bool operator<(const SortWrapper& other) const {
        return key < other.key;
    }

    bool operator>(const SortWrapper& other) const {
        return key > other.key;
    }
};


bool WidgetManager::onEvent(const GEvent& event) {
    const bool motionEvent = 
        (event.type == GEventType::MOUSE_MOTION) ||
        (event.type == GEventType::JOY_AXIS_MOTION) ||
        (event.type == GEventType::JOY_HAT_MOTION) ||
        (event.type == GEventType::JOY_BALL_MOTION);

    const bool positionalEvent = 
        (event.type == GEventType::MOUSE_BUTTON_CLICK) ||
        (event.type == GEventType::MOUSE_BUTTON_DOWN) ||
        (event.type == GEventType::MOUSE_BUTTON_UP) ||
        (event.type == GEventType::MOUSE_MOTION);

    beginLock(); {
        // Deliver positional events in order of widget's depth preference
        if (positionalEvent) {
            const Point2& P = event.mousePosition();

            // Find the distance at which object believes that it is for this event. By default, this is
            // the focus array position.
            Array<SortWrapper<shared_ptr<Widget> > > widgetWithZ;
            widgetWithZ.resize(m_moduleArray.size());
            for (int i = 0; i < m_moduleArray.size(); ++i) {
                const shared_ptr<Widget>& w = m_moduleArray[i];
                SortWrapper<shared_ptr<Widget> >& s = widgetWithZ[i];

                s.value = w;
                s.key = w->positionalEventZ(P);

                if (isNaN(s.key)) {
                    // The module wishes to use its focus position as a "Depth"
                    if (w == m_focusedModule) {
                        // Focused module gets priority and pretends that it is 
                        // higher than the first one
                        s.key = float(m_moduleArray.size());
                    } else {
                        s.key = float(i);
                    }
                } // if default key
            } // for each widget

            widgetWithZ.sort(SORT_DECREASING);
                
            for (int i = 0; i < widgetWithZ.size(); ++i) {        
                if (widgetWithZ[i].value->onEvent(event)) {
                    // End the manager lock began at the top of this method before returning abruptly
                    endLock();
                    return true;
                }
            }

        } else {

            // Except for motion events, ensure that the focused module gets each event first
            if (m_focusedModule && ! motionEvent) {
                if (m_focusedModule->onEvent(event)) { 
                    // End the manager lock began at the top of this method before returning abruptly
                    endLock(); 
                    return true;
                }
            }

            for (int i = m_moduleArray.size() - 1; i >=0; --i) {
                // Don't double-deliver to the focused module
                if (motionEvent || (m_moduleArray[i] != m_focusedModule)) {
                    if (m_moduleArray[i]->onEvent(event) && ! motionEvent) {
                        // End the manager lock began at the top of this method before returning abruptly
                        endLock(); 
                        return true;
                    }
                }
            }
        }
    } endLock();

    return false;
}


void WidgetManager::onUserInput(UserInput* ui) {
    ITERATOR(m_moduleArray[i]->onUserInput(ui));
}


void WidgetManager::onNetwork() {
    ITERATOR(m_moduleArray[i]->onNetwork());
}

    
void WidgetManager::onAI() {
    ITERATOR(m_moduleArray[i]->onAI());
}


void WidgetManager::onAfterEvents() {
    ITERATOR(m_moduleArray[i]->onAfterEvents());
}

#undef ITERATOR

bool WidgetManager::onEvent(const GEvent& event, WidgetManager::Ref& a) {
    static shared_ptr<WidgetManager> s;
    return onEvent(event, a, s);
}


bool WidgetManager::onEvent(const GEvent& event, WidgetManager::Ref& a, WidgetManager::Ref& b) {
    a->beginLock();
    if (b) {
        b->beginLock();
    }

    int numManagers = isNull(b) ? 1 : 2;

    // Process each
    for (int k = 0; k < numManagers; ++k) {
        Array<Widget::Ref >& array = 
            (k == 0) ?
            a->m_moduleArray :
            b->m_moduleArray;
                
        for (int i = array.size() - 1; i >= 0; --i) {

            if (array[i]->onEvent(event)) {
                debugAssertGLOk();
                if (b) {
                    b->endLock();
                }
                a->endLock();
                return true;
            }
        }
    }
    
    if (b) {
        b->endLock();
    }
    a->endLock();

    return false;
}


} // G3D
