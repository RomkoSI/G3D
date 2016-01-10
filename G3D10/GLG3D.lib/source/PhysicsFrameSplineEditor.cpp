/**
  \file PhysicsFrameSplineEditor.h
  
  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2011-05-19
  \edited  2013-02-23
*/
#include "GLG3D/PhysicsFrameSplineEditor.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/Draw.h"

#define CONTROL_POINT_RADIUS .2

namespace G3D {

shared_ptr<PhysicsFrameSplineEditor> PhysicsFrameSplineEditor::create(const GuiText& caption, GuiPane* dockPane, const shared_ptr<GuiTheme>& theme) {    
    return shared_ptr<PhysicsFrameSplineEditor>(new PhysicsFrameSplineEditor(caption, dockPane, isNull(theme) ? (isNull(dockPane) ? GuiTheme::lastThemeLoaded.lock() : dockPane->theme()) : theme));
}


PhysicsFrameSplineEditor::PhysicsFrameSplineEditor(const GuiText& caption, GuiPane* dockPane, shared_ptr<GuiTheme> theme) : ControlPointEditor(caption, dockPane, theme)  {

    m_spline.append(CFrame());

    GuiPane* p = isNull(dockPane) ? m_rootPane : dockPane;

    cpPane->addNumberBox("Time", Pointer<float>(this, &PhysicsFrameSplineEditor::selectedNodeTime, &PhysicsFrameSplineEditor::setSelectedNodeTime), "s");

    cpPane->pack();
    GuiPane* exPane = p->addPane("Extrapolation Mode", GuiTheme::NO_PANE_STYLE);
    exPane->beginRow(); {
        GuiControl* linearButton  = exPane->addRadioButton("Linear", SplineExtrapolationMode::LINEAR, this, 
                                        &PhysicsFrameSplineEditor::extrapolationMode, &PhysicsFrameSplineEditor::setExtrapolationMode);
        GuiControl* clampedButton = exPane->addRadioButton("Clamped", SplineExtrapolationMode::CLAMP, this, 
                                        &PhysicsFrameSplineEditor::extrapolationMode, &PhysicsFrameSplineEditor::setExtrapolationMode);
        clampedButton->moveRightOf(linearButton, -145);
        GuiControl* cyclicButton  = exPane->addRadioButton("Cyclic", SplineExtrapolationMode::CYCLIC, this, 
                                        &PhysicsFrameSplineEditor::extrapolationMode, &PhysicsFrameSplineEditor::setExtrapolationMode);
        cyclicButton->moveRightOf(clampedButton);
        cyclicButton->moveBy(-140, 0);
    } exPane->endRow();
    exPane->pack();
    GuiPane* inPane = p->addPane("Interpolation Mode", GuiTheme::NO_PANE_STYLE);
    inPane->beginRow(); {
        GuiControl* linearButton = inPane->addRadioButton("Linear", SplineInterpolationMode::LINEAR, this, 
                                       &PhysicsFrameSplineEditor::interpolationMode, &PhysicsFrameSplineEditor::setInterpolationMode);
        GuiControl* cubicButton  = inPane->addRadioButton("Cubic", SplineInterpolationMode::CUBIC, this, 
                                       &PhysicsFrameSplineEditor::interpolationMode, &PhysicsFrameSplineEditor::setInterpolationMode); 
        cubicButton->moveRightOf(linearButton, -145);
    } inPane->endRow();
    inPane->pack();
    GuiPane* finalIntervalPane = p->addPane("Final Interval", GuiTheme::NO_PANE_STYLE);
    finalIntervalPane->moveRightOf(exPane, Vector2(-100, -5));
    static int m_explicitFinalInterval = 0;
    m_finalIntervalChoice[0] = finalIntervalPane->addRadioButton("automatic", 0, &m_explicitFinalInterval);
    finalIntervalPane->beginRow(); {
        m_finalIntervalChoice[1] = finalIntervalPane->addRadioButton("", 1, &m_explicitFinalInterval);
        m_finalIntervalBox = finalIntervalPane->addNumberBox("", &m_spline.finalInterval, "s", GuiTheme::NO_SLIDER, -1.0f, 10000.0f, 0.001f);
        m_finalIntervalBox->setWidth(76);
        m_finalIntervalBox->moveBy(-2, 0);
    } finalIntervalPane->endRow();
    pack();
}


float PhysicsFrameSplineEditor::selectedNodeTime() const {
    if (m_selectedControlPointIndex >= 0 && m_selectedControlPointIndex < m_spline.control.size()) {
        return m_spline.time[m_selectedControlPointIndex];
    } else {
        return 0.0f;
    }
}


void PhysicsFrameSplineEditor::setSelectedNodeTime(float t) {
    if (m_selectedControlPointIndex >= 0 && m_selectedControlPointIndex < m_spline.control.size()) {
        m_spline.time[m_selectedControlPointIndex] = t;
    }
}


SplineExtrapolationMode::Value PhysicsFrameSplineEditor::extrapolationMode() const {
    return m_spline.extrapolationMode.value;
}


void PhysicsFrameSplineEditor::setExtrapolationMode(SplineExtrapolationMode::Value m) {
    m_spline.extrapolationMode = m;
}


SplineInterpolationMode::Value PhysicsFrameSplineEditor::interpolationMode() const {
    return m_spline.interpolationMode.value; 
}


void PhysicsFrameSplineEditor::setInterpolationMode(SplineInterpolationMode::Value m) { 
    m_spline.interpolationMode = m; 
}


void PhysicsFrameSplineEditor::renderControlPoints
    (RenderDevice*                        rd, 
     const LightingEnvironment&           environment) const {
    Draw::physicsFrameSpline(m_spline, rd, m_selectedControlPointIndex);
}


void PhysicsFrameSplineEditor::setControlPoint(int index, const PhysicsFrame& frame) {
    m_spline.control[index] = frame;
}


PhysicsFrame PhysicsFrameSplineEditor::controlPoint(int index) const {
    return m_spline.control[index];
}


void PhysicsFrameSplineEditor::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    ControlPointEditor::onSimulation(rdt, sdt, idt);
    
    if (enabled()) {
        m_finalIntervalChoice[0]->setEnabled(m_spline.extrapolationMode == SplineExtrapolationMode::CYCLIC);
        m_finalIntervalChoice[1]->setEnabled(m_spline.extrapolationMode == SplineExtrapolationMode::CYCLIC);
        m_finalIntervalBox->setEnabled(m_spline.extrapolationMode == SplineExtrapolationMode::CYCLIC); //  && (m_spline.finalInterval != -1.0f)
    }
}


void PhysicsFrameSplineEditor::setSpline(const PhysicsFrameSpline& s) {
    m_spline = s;

    // Ensure that there is at least one node
    if (m_spline.control.size() == 0) {
        m_spline.control.append(PFrame());
    }

    m_selectedControlPointIndex = iClamp(m_selectedControlPointIndex, 0, m_spline.control.size() - 1);
    resizeControlPointDropDown(m_spline.control.size());

    m_nodeManipulator->setFrame(m_spline.control[m_selectedControlPointIndex]);
}


void PhysicsFrameSplineEditor::setSelectedControlPointIndex(int i) {
    ControlPointEditor::setSelectedControlPointIndex(i);

    // Move the manipulator to the new control point
    m_nodeManipulator->setFrame(m_spline.control[m_selectedControlPointIndex]);
}


void PhysicsFrameSplineEditor::removeControlPoint(int i) {
    m_spline.time.remove(i);
    m_spline.control.remove(i);
}


void PhysicsFrameSplineEditor::addControlPointAfter(int i) {
    if (numControlPoints() == 0) {
        m_spline.append(CFrame());
    } else if (numControlPoints() == 1) {
        // Adding the 2nd point
        CFrame f = m_spline.control.last();
        f.translation += f.lookVector();
        m_spline.append(f);
    } else {
        // Adding between two points
        float t0 = m_spline.time[i];
        float newT = t0;
        float evalT = 0;
        if (i < m_spline.control.size() - 1) {
            // Normal interval
            newT = m_spline.time[m_selectedControlPointIndex + 1];
            evalT = (t0 + newT) / 2;
        } else if (m_spline.extrapolationMode == SplineExtrapolationMode::CYCLIC) {
            // After the end on a cyclic spline
            newT = m_spline.getFinalInterval() + t0;
            evalT = (t0 + newT) / 2;
        } else {
            // After the end on a non-cyclic spline of length at least
            // 2; assume that we want to step the distance of the previous
            // interval
            newT = evalT = 2.0f * t0 - m_spline.time[i - 1];
        }

        const PhysicsFrame& f = m_spline.evaluate(evalT);
        m_spline.control.insert(i + 1, f);
        m_spline.time.insert(i + 1, newT);

        // Fix the rest of the times to be offset by the inserted duration
        float shift = newT - t0;
        for (int j = i + 1; j < m_spline.time.size(); ++j) {
            m_spline.time[j] += shift;
        }
    }
}

}
