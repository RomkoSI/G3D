/**
  \file GLG3D/UprightSplineManipulator.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2007-06-01
  \edited  2015-01-02

  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
*/
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Camera.h"
#include "GLG3D/UserInput.h"
#include "GLG3D/Draw.h"
#include "G3D/Sphere.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "GLG3D/Camera.h"
#include "GLG3D/UprightSplineManipulator.h"

namespace G3D {

class UprightSplineSurface : public Surface {
private:

    UprightSpline*  spline;
    Color3          color;

    AttributeArray  vertex;
    int             numVertices;
    AABox           boxBounds;

public:

    UprightSplineSurface(UprightSpline* s, const Color3& c) : spline(s), color(c) {
        if (spline->control.size() > 1) {
            numVertices = spline->control.size() * 11 + 1;

            int count = spline->control.size();
            if (spline->extrapolationMode == SplineExtrapolationMode::CYCLIC) {
                ++count;
            }

            shared_ptr<VertexBuffer> area = VertexBuffer::create(sizeof(Vector3) * numVertices);
            Array<Vector3> v;
            v.resize(numVertices);
            
            for (int i = 0; i < numVertices; ++i) {
                float s = count * i / (float)(numVertices - 1);
                v[i] = spline->evaluate(s).translation;
                if (i == 0) {
                    boxBounds = AABox(v[i]);
                } else {
                    boxBounds.merge(v[i]);
                }
            }
            
            vertex = AttributeArray(v, area);
        }
    }

    void drawSplineCurve(RenderDevice* rd) const {
        rd->sendSequentialIndices(PrimitiveType::LINE_STRIP, numVertices);
    }

    virtual bool requiresBlending() const override {
        return false;
    }

    virtual bool anyUnblended() const override {
        return true;
    }

    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override {
        return false;
    }

    virtual void renderWireframeHomogeneous
    (RenderDevice*                          rd, 
     const Array<shared_ptr<Surface> >&     surfaceArray, 
     const Color4&                          color,
     bool                                   previous) const override {
         // intentionally empty
    }
    
    virtual void render
       (RenderDevice*                       rd, 
        const LightingEnvironment&          environment,
        RenderPassType                      passType, 
        const String&                       singlePassBlendedOutputMacro) const override {

        rd->pushState();
        
        // Draw control points
        if (spline->control.size() > 0) {
            Draw::sphere(Sphere(spline->control[0].translation, 0.1f), rd, Color3::green(), Color4::clear());
            Draw::sphere(Sphere(spline->control.last().translation, 0.1f), rd, Color3::black(), Color4::clear());
        }
        Vector3 extent(0.07f, 0.07f, 0.07f);
        //AABox box(spline->control[i].translation - extent, spline->control[i].translation + extent);
        AABox box(-extent, extent);
        for (int i = 1; i < spline->control.size() - 1; ++i) {
            rd->setObjectToWorldMatrix(spline->control[i].toCoordinateFrame());
            Draw::box(box, rd, color, Color4::clear());
        }
        rd->popState();

        if (spline->control.size() < 4) {
            return;
        }
        alwaysAssertM(false, "UprightSplineManipulator::render not yet implemented");
        /*
        rd->pushState(); {
            rd->setObjectToWorldMatrix(CoordinateFrame());
        
            Color4 c;
            glEnable(GL_FOG);
            glFogf(GL_FOG_START, 40.0f);
            glFogf(GL_FOG_END, 120.0);
            glFogi(GL_FOG_MODE, GL_LINEAR);

            rd->beginIndexedPrimitives();
            rd->setVertexArray(vertex);
        
            glEnable(GL_LINE_SMOOTH);
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            c = Color4((Color3::white() * 2.0f + color) / 3.0f, 0);
            glFogfv(GL_FOG_COLOR, (float*)&c);

            // Core
            glFogf(GL_FOG_START, 5.0f);
            glFogf(GL_FOG_END, 60.0);
            drawSplineCurve(rd);

            rd->setDepthWrite(false);
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);

            // Core:
            rd->setDepthWrite(false);
            c = Color3::black();

            rd->endIndexedPrimitives();
        
        } rd->popState();
        */
    }
    
    virtual void getCoordinateFrame(CoordinateFrame& c, bool previous = false) const override {
        c = CoordinateFrame();
    }

    virtual void getObjectSpaceBoundingBox(AABox& b, bool previous = false) const override {
        b = boxBounds;
    }

    virtual void getObjectSpaceBoundingSphere(Sphere& s, bool previous = false) const override {
        boxBounds.getBounds(s);
    }
    
    virtual bool hasTransparency () const {
        return true;
    }

    virtual String name () const override {
        return "UprightSplineSurface";
    }

    virtual void defaultRender(G3D::RenderDevice*) const override {
        alwaysAssertM(false, "Not implemented");
    }
};


shared_ptr<UprightSplineManipulator> UprightSplineManipulator::create(const shared_ptr<Camera>& c) {
    shared_ptr<UprightSplineManipulator> manipulator(new UprightSplineManipulator());
    manipulator->setCamera(c);
    return manipulator;
}


UprightSplineManipulator::UprightSplineManipulator() 
    : m_time(0), 
      m_mode(INACTIVE_MODE), 
      m_showPath(true), 
      m_pathColor(Color3::red()), 
      m_sampleRate(1),
      m_recordKey(' ') {
}


CoordinateFrame UprightSplineManipulator::frame() const {
    return m_currentFrame;
}


void UprightSplineManipulator::getFrame(CoordinateFrame &c) const {
    c = m_currentFrame;
}


void UprightSplineManipulator::clear() {
    m_spline.clear();
    setTime(0);
}

void UprightSplineManipulator::setMode(Mode m) {
    m_mode = m;
    if (m_mode == RECORD_KEY_MODE || m_mode == RECORD_INTERVAL_MODE) {
        debugAssertM(notNull(m_camera), "Cannot enter record mode without first setting the camera");
    }
}


UprightSplineManipulator::Mode UprightSplineManipulator::mode() const {
    return m_mode;
}


void UprightSplineManipulator::onPose
(Array< shared_ptr<Surface> >& posedArray, 
 Array< Surface2DRef >& posed2DArray) {

    (void)posed2DArray;

    if (m_showPath && (m_spline.control.size() > 0)) {
        posedArray.append(shared_ptr<UprightSplineSurface>(new UprightSplineSurface(&m_spline, m_pathColor)));
    }
}


bool UprightSplineManipulator::onEvent(const GEvent &event) {
    if ((m_mode == RECORD_KEY_MODE) && 
        (event.type == (uint8)GEventType::KEY_DOWN) &&
        (event.key.keysym.sym == m_recordKey) &&
        m_camera) {

        // Capture data point
        m_spline.append(m_camera->frame());
        
        // Consume the event
        return true;
    }

    return false;
}


void UprightSplineManipulator::onSimulation (RealTime rdt, SimTime sdt, SimTime idt) {
    (void)rdt;
    (void)idt;
    if (m_mode != INACTIVE_MODE) {
        setTime(m_time + sdt);
    }
}


void UprightSplineManipulator::onUserInput(UserInput* ui) {
    (void)ui;
}
/*
void UprightSplineManipulator::setExtrapolationMode(SplineExtrapolationMode m) {
    m_spline.extrapolationMode = m;
} 

SplineExtrapolationMode UprightSplineManipulator::extrapolationMode() const {
    return m_spline.extrapolationMode;
}
*/
void UprightSplineManipulator::setTime(double t) {
    m_time = t;

    switch (m_mode) {
    case PLAY_MODE:
        if (m_spline.control.size() >= 4) {
            m_currentFrame = m_spline.evaluate((float)t * (float)m_sampleRate).toCoordinateFrame();
        } else {
            // Not enough points for a spline
            m_currentFrame = CoordinateFrame();
        }
        break;
        
    case RECORD_INTERVAL_MODE:
        if (m_camera) {
            // We have a camera
            if (m_time * m_sampleRate > m_spline.control.size()) {
                // Enough time has elapsed to capture a new data point
                m_spline.append(m_camera->frame());
            }
        }
        break;

    case RECORD_KEY_MODE:
        break;

    case INACTIVE_MODE:
        break;
    }
}

}
