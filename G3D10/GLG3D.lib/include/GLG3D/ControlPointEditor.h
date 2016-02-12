/**
 \file GLG3D/ControlPointEditor.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 \created 2011-06-05
 \edited  2014-07-30

 G3D Library http://g3d.codeplex.com
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license

 */
#ifndef G3D_ControlPointEditor_h
#define G3D_ControlPointEditor_h

#include "G3D/platform.h"
#include "G3D/AABox.h"
#include "G3D/Ray.h"
#include "G3D/Projection.h"
#include "G3D/PhysicsFrame.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/Surface.h"
#include "GLG3D/GuiNumberBox.h"

namespace G3D {

class GuiDropDownList;
class ThirdPersonManipulator;
class GuiButton;


class ControlPointEditor : public GuiWindow {
protected:

    shared_ptr<Surface>             m_surface;

    /** If outside of the legal range, indicates that no point is selected. */
    int                             m_selectedControlPointIndex;
    int                             m_lastNodeManipulatorControlPointIndex;

    shared_ptr<ThirdPersonManipulator> m_nodeManipulator;

    GuiNumberBox<int>*              m_selectedControlPointSlider;
    bool                            m_isDocked;
    GuiButton*                      m_removeSelectedButton;
    GuiPane*                        m_addRemoveControlPointPane;

    /** Used to avoid constantly unparsing the current physics frame in selectedNodePFrameAsString() */
    mutable PhysicsFrame            m_cachedPhysicsFrameValue;
    mutable String                  m_cachedPhysicsFrameString;

    GuiPane*                        cpPane;
    mutable EventCoordinateMapper   m_mapper;

    class ControlPointSurface : public Surface {
    public:
        ControlPointEditor* m_manipulator;
        
        ControlPointSurface(ControlPointEditor* m) : m_manipulator(m) {}
        
        virtual void render
        (RenderDevice*                        rd, 
         const LightingEnvironment&           environment,
         RenderPassType                       passType, 
         const String&                        singlePassBlendedOutputMacro) const override;

        virtual void renderWireframeHomogeneous
        (RenderDevice*                rd, 
         const Array<shared_ptr<Surface> >&   surfaceArray, 
         const Color4&                color,
         bool                         previous) const override {
             // Intentionally empty
        }

        virtual bool anyUnblended() const override {
            return true;
        }

        virtual bool requiresBlending() const override {
            return false;
        }

        virtual String name() const override {
            return "ControlPointSurface";
        }
        
        virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override {
            // Doesn't implement the renderIntoGBufferHomogeoneous method
            return false;
        }

        virtual void getCoordinateFrame(CoordinateFrame& c, bool previous = false) const override {
            c = CFrame();
        }

        virtual void getObjectSpaceBoundingBox(G3D::AABox& b, bool previous = false) const override {
            b = AABox::inf();
        }

        virtual void getObjectSpaceBoundingSphere(G3D::Sphere& s, bool previous = false) const override {
            s = Sphere(Point3::zero(), finf());
        }

    protected:
        virtual void defaultRender(RenderDevice* rd) const override {
            alwaysAssertM(false, "Not implemented");
        }
    };
    
    ControlPointEditor(const GuiText& caption, GuiPane* dockPane, const shared_ptr<GuiTheme>& theme);
    
    virtual void setControlPoint(int index, const PhysicsFrame& frame) = 0;

    virtual PhysicsFrame controlPoint(int index) const = 0;

    virtual int numControlPoints() const = 0;

    virtual void removeControlPoint(int i) = 0;

    virtual void addControlPointAfter(int i) = 0;

    virtual bool allowRotation() const {
        return true;
    }

    virtual bool allowTranslation() const {
        return true;
    }

    virtual bool allowAddingAndRemovingControlPoints() const {
        return true;
    }

    virtual void renderControlPoints
    (RenderDevice*                        rd, 
     const LightingEnvironment&           environment) const;

    void resizeControlPointDropDown(int i);

    /** Returns the camera-space z position of the first intersection of the ray through pixel with a control point,
      and the \a index of that control point (-1 if none was hit) */
    virtual float intersectRayThroughPixel(const Point2& pixel, int& index) const;

public:

    /** Given a world space ray, will determine if it goes through any of the control points. 
        If it does, it will set that control point as the the selectedControlPoint*/
    bool hitsControlPoint(const Ray& r);

    virtual float positionalEventZ(const Point2& pixel) const override;

    virtual bool onEvent(const GEvent& event) override;

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;

    virtual void onPose(Array<shared_ptr<Surface> >& surface, Array<shared_ptr<Surface2D> >& surface2D) override;

    virtual void setManager(WidgetManager* m) override;

    /** Gui Callback */
    void addControlPoint();

    void removeSelectedControlPoint();

    int selectedControlPointIndex() const {
        return m_selectedControlPointIndex;
    }

    /** Used by the GUI. */
    String selectedNodePFrameAsString() const;

    /** Used by the GUI. */
    void setSelectedNodePFrameFromString(const String& s);

    /** Used by the GUI. */
    float selectedNodeTime() const;

    /** Used by the GUI. */
    void setSelectedNodeTime(float t);
    
    virtual void setSelectedControlPointIndex(int i);
    
    virtual void setEnabled(bool e) override;

    void renderManipulator(RenderDevice* rd);

};

} // G3D

#endif
