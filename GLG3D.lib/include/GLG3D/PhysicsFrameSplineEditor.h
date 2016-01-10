/**
 \file GLG3D/PhysicsFrameSplineEditor.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-06-05
 \edited  2014-07-22

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef G3D_PhysicsFrameSplineEditor_h
#define G3D_PhysicsFrameSplineEditor_h

#include "GLG3D/GuiWindow.h"
#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/GuiButton.h"
#include "GLG3D/Surface.h"
#include "G3D/PhysicsFrameSpline.h"
#include "G3D/AABox.h"
#include "G3D/Ray.h"
#include "GLG3D/ControlPointEditor.h"
#include "GLG3D/GuiNumberBox.h"

namespace G3D {

/** 2D/3D control for editing PhysicsFrameSpline%s.  
    Used by SceneEditorWindow.  Not intended for general use and likely to be private in a future release.

    The spline manipulator is a GuiWindow that displays its controls.
    It also creates additional Widget%s.

    Invoking setVisible(false) on the PhysicsFrameSplineEditor hides the
    control window, but not the 3D controls.  Use setEnabled(false)
    to hide the 3D controls.

    \beta
*/
class PhysicsFrameSplineEditor : public ControlPointEditor {
protected:

    shared_ptr<Surface>             m_surface;

    PhysicsFrameSpline              m_spline;
    GuiRadioButton*                 m_finalIntervalChoice[2];
    GuiNumberBox<float>*            m_finalIntervalBox;

    virtual int numControlPoints() const override {
        return m_spline.control.size();
    }

    virtual void removeControlPoint(int i) override;

    virtual void addControlPointAfter(int i) override;

    virtual void setControlPoint(int index, const PhysicsFrame& frame) override;

    virtual PhysicsFrame controlPoint(int index) const override;

    virtual void renderControlPoints
    (RenderDevice*                        rd, 
     const LightingEnvironment&           environment) const override;

    PhysicsFrameSplineEditor(const GuiText& caption, GuiPane* dockPane, shared_ptr<GuiTheme> theme);
        
public:

    /** \param dockPane If not NULL, the 2D GUI is placed into this pane and no visible window is created */
    static shared_ptr<PhysicsFrameSplineEditor> create(const GuiText& caption = "Spline Editor", GuiPane* dockPane = NULL, const shared_ptr<GuiTheme>& theme = shared_ptr<GuiTheme>());

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
        
    /** Used by the GUI. */
    float selectedNodeTime() const;

    /** Used by the GUI. */
    void setSelectedNodeTime(float t);
    
    virtual void setSelectedControlPointIndex(int i) override;

    /** Extrapolation mode of the underlying spline */
    SplineExtrapolationMode::Value extrapolationMode() const;

    /** Sets the extrapolationMode of the underlying spline */
    void setExtrapolationMode(SplineExtrapolationMode::Value m);

    /** Interpolation mode of the underlying spline */
    SplineInterpolationMode::Value interpolationMode() const; 

    /** Sets the interpolationMode of the underlying spline */
    void setInterpolationMode(SplineInterpolationMode::Value m);

    const PhysicsFrameSpline& spline() const {
        return m_spline;
    }

    virtual void setSpline(const PhysicsFrameSpline& p);

    virtual void setEnabled(bool e) override {
        GuiWindow::setEnabled(e);

        // If enabled, also make visible (so that the window can be seen)
        if (e && ! m_isDocked) {
            setVisible(true);
        }
    }
};

} // G3D

#endif // PhysicsFrameSplineEditor_h
