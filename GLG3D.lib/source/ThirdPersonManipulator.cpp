/**
  \file GLG3D.lib/source/ThirdPersonManipulator.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2006-06-09
  \edited  2015-01-02
   
  Copyright 2000-2015, Morgan McGuire.
  All rights reserved.
 */

#include "GLG3D/ThirdPersonManipulator.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Draw.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/SlowMesh.h"
#include "GLG3D/UserInput.h"
#include "G3D/Sphere.h"
#include "G3D/AABox.h"

namespace G3D {

namespace _internal {

int dummyInt;

Vector3 UIGeom::segmentNormal(const LineSegment& seg, const Vector3& eye) {
    Vector3 E = eye - seg.point(0);
    Vector3 V = seg.point(1) - seg.point(0);
    Vector3 U = E.cross(V);
    Vector3 N = V.cross(U).direction();
    return N;
}


Vector3 UIGeom::computeEye(RenderDevice* rd) {
    Vector3 eye = rd->objectToWorldMatrix().pointToObjectSpace(rd->cameraToWorldMatrix().translation);
    return eye;
}


bool UIGeom::contains
   (const Vector2&  p, 
    float&          nearestDepth, 
    Vector2&        tangent2D,
    float&          projectionW,
    float           lineRadius) const {
    
    bool result = false;

    // Find the nearest containing polygon
    for (int i = 0; i < poly2D.size(); ++i) {
        bool reverse = m_twoSidedPolys && polyBackfacing[i];
        if ((polyDepth[i] < nearestDepth) && 
            poly2D[i].contains(p, reverse)) {
            
            nearestDepth = polyDepth[i];
            result = true;
        }
    }

    // Find the nearest containing line
    for (int i = 0; i < line2D.size(); ++i) {
        int segmentIndex = 0;
        float d = line2D[i].distance(p, segmentIndex);
        if (d <= lineRadius) {
            // Find the average depth of this segment
            float depth = (lineDepth[i][segmentIndex] + lineDepth[i][segmentIndex + 1]) / 2;
            if (depth < nearestDepth) {
                tangent2D = (line2D[i].segment(segmentIndex).point(1) - line2D[i].segment(segmentIndex).point(0)) / 
                            line3D[i].segment(segmentIndex).length();
                projectionW = (lineW[i][segmentIndex] + lineW[i][segmentIndex]) / 2;
                nearestDepth = depth;
                result = true;
            }
        }
    }

    return result;
}


void UIGeom::computeProjection(RenderDevice* rd) {
    Array<Vector2> vertex;

    // Polyline segments
    line2D.resize(line3D.size());
    lineDepth.resize(line2D.size());
    lineW.resize(line2D.size());
    for (int i = 0; i < line3D.size(); ++i) {
        const PolyLine& line = line3D[i];
        Array<float>& depth = lineDepth[i];
        Array<float>& w = lineW[i];

        // Project all vertices
        vertex.resize(line.numVertices(), false);
        depth.resize(vertex.size());
        w.resize(vertex.size());
        for (int j = 0; j < line.numVertices(); ++j) {
            Vector4 v = rd->project(line.vertex(j));
            vertex[j] = v.xy();
            depth[j] = v.z;
            w[j] = v.w;
        }

        line2D[i] = PolyLine2D(vertex);
    }

    // Polygons
    poly2D.resize(poly3D.size());
    polyDepth.resize(poly2D.size());
    if (poly3D.size() > 0) {
        polyBackfacing.resize(poly3D.size());

        // Used for backface classification
        Vector3 objEye = rd->modelViewMatrix().inverse().translation;
        
        float z = 0;
        for (int i = 0; i < poly3D.size(); ++i) {
            const ConvexPolygon& poly = poly3D[i];
            vertex.resize(poly3D[i].numVertices(), false);
            for (int j = 0; j < poly.numVertices(); ++j) {
                Vector4 v = rd->project(poly.vertex(j));
                vertex[j] = v.xy();
                z += v.z;
            }

            polyDepth[i] = z / poly.numVertices();
            poly2D[i] = ConvexPolygon2D(vertex);
            Vector3 normal = poly.normal();
            Vector3 toEye = objEye - poly.vertex(0);
            polyBackfacing[i] = normal.dot(toEye) < 0;
        }
    }
}

#define HIDDEN_LINE_ALPHA (0.1f)

void UIGeom::render(RenderDevice* rd, const Color3& color, float lineScale) const {
    if (! visible) {
        return;
    }

    if (m_twoSidedPolys) {
        rd->setCullFace(CullFace::NONE);
    }

    for (int i = 0; i < poly3D.size(); ++i) {
        const ConvexPolygon& poly = poly3D[i];
        SlowMesh mesh(PrimitiveType::TRIANGLE_FAN);
        mesh.setNormal(poly.normal());
        mesh.setColor(color);
        for (int v = 0; v < poly.numVertices(); ++v) {
            mesh.makeVertex(poly.vertex(v));
        }
        mesh.render(rd);
    }

    // Per segment normals
    Array<Vector3> normal;
    const Vector3 eye = computeEye(rd);
    for (int i = 0; i < line3D.size(); ++i) {
        const PolyLine& line = line3D[i];

        // Compute one normal for each segement
        normal.resize(line.numSegments(), false);
        for (int s = 0; s < line.numSegments(); ++s) {
            normal[s] = segmentNormal(line.segment(s), eye);
        }

        bool closed = line.closed();

        // Compute line width
        float L = 2;
        {
            const Vector4& v = rd->project(line.vertex(0));
            if (v.w > 0) {
                L = min(15.0f * v.w, 10.0f);
            } else {
                L = 10.0f;
            }
        }

        bool colorWrite = rd->colorWrite();
        bool depthWrite = rd->depthWrite();

        // Draw lines twice.  The first time we draw color information
        // but do not set the depth buffer.  The second time we write
        // to the depth buffer; this prevents line strips from
        // corrupting their own antialiasing with depth conflicts at
        // the line joint points.
        if (colorWrite) {
            rd->setDepthWrite(false);
            
            // Make two passes; first renders visible lines, the next 
            // renders hidden ones
            for (int i = 0; i < 2; ++i) {
                SlowMesh mesh(PrimitiveType::LINE_STRIP);

                if (i == 0) {
                    mesh.setColor(color);
                } else {
                    mesh.setColor(Color4(color, HIDDEN_LINE_ALPHA));
                }
                for (int v = 0; v < line.numVertices(); ++v) {
                    // Compute the smooth normal.  If we have a non-closed
                    // polyline, the ends are special cases.
                    Vector3 N;
                    if (! closed && (v == 0)) {
                        N = normal[0];
                    } else if (! closed && (v == line.numVertices())) {
                        N = normal.last();
                    } else {
                        // Average two adjacent normals
                        N = normal[v % normal.size()] + normal[(v - 1) % normal.size()];
                    }
                        
                    if (N.squaredLength() < 0.05) {
                        // Too small to normalize; revert to the
                        // nearest segment normal
                        mesh.setNormal(normal[iMin(v, normal.size() - 1)]);
                    } else {
                        mesh.setNormal(N.direction());
                    }

                    mesh.makeVertex(line.vertex(v));
                }
                mesh.render(rd);

                // Second pass, render hidden lines
                rd->setDepthTest(RenderDevice::DEPTH_GREATER);
            }

            // Restore depth test
            //glDisable(GL_LINE_STIPPLE);
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        }

        if (depthWrite) {
            rd->setColorWrite(false);
            rd->setDepthWrite(true);
            SlowMesh mesh(PrimitiveType::LINE_STRIP);
            mesh.setColor(Color4(color, HIDDEN_LINE_ALPHA));
            for (int v = 0; v < line.numVertices(); ++v) {
                mesh.makeVertex(line.vertex(v));
            }
            mesh.render(rd);
        }

        rd->setColorWrite(colorWrite);
        rd->setDepthWrite(depthWrite);
    }
}    
} // _internal


shared_ptr<ThirdPersonManipulator> ThirdPersonManipulator::create() {
    return shared_ptr<ThirdPersonManipulator>(new ThirdPersonManipulator());
}


void ThirdPersonManipulator::render3D(RenderDevice* rd) {
    rd->pushState();
    // Highlight the appropriate axis

    // X, Y, Z, XY, YZ, ZX, RX, RY, RZ
    Color3 color[] = 
        {Color3(0.9f, 0, 0), 
         Color3(0, 0.9f, 0.1f), 
         Color3(0.0f, 0.4f, 1.0f), 
         
         Color3(0.6f, 0.7f, 0.7f), 
         Color3(0.6f, 0.7f, 0.7f), 
         Color3(0.6f, 0.7f, 0.7f),
    
         Color3(0.9f, 0, 0), 
         Color3(0, 0.9f, 0.1f), 
         Color3(0.0f, 0.4f, 1.0f)}; 
         
    static const Color3 highlight[] = 
       {Color3(1.0f, 0.5f, 0.5f), 
        Color3(0.6f, 1.0f, 0.7f), 
        Color3(0.5f, 0.7f, 1.0f), 
        
        Color3::white(), 
        Color3::white(), 
        Color3::white(),
    
        Color3(1.0f, 0.5f, 0.5f), 
        Color3(0.6f, 1.0f, 0.7f), 
        Color3(0.5f, 0.7f, 1.0f)};

    static const Color3 usingColor = Color3::yellow();

    // Highlight whatever we're over
    if (m_overAxis != NO_AXIS) {
        color[m_overAxis] = highlight[m_overAxis];
    }

    // Show the selected axes
    for (int a = 0; a < NUM_GEOMS; ++a) {
        if (m_usingAxis[a]) {
            color[a] = usingColor;
        }
    }

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, 
                     RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
	
    rd->setObjectToWorldMatrix(CFrame());

    // Draw the background in LEQUAL mode
    // The Draw::axes command automatically doubles whatever scale we give it
    Draw::axes(m_controlFrame, rd, color[0], color[1], color[2], m_axisScale * 0.5f);

    // Hidden-line version
    rd->setDepthTest(RenderDevice::DEPTH_GREATER);
    Draw::axes(m_controlFrame, rd,
               Color4(color[0], HIDDEN_LINE_ALPHA), 
               Color4(color[1], HIDDEN_LINE_ALPHA), 
               Color4(color[2], HIDDEN_LINE_ALPHA),
               m_axisScale * 0.5f);
    rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);

    rd->setObjectToWorldMatrix(m_controlFrame);

    // These do their own hidden-line rendering
    if (m_translationEnabled) {
        for (int g = FIRST_TRANSLATION; g <= LAST_TRANSLATION; ++g) {
            m_geomArray[g].render(rd, color[g]);
        }
    }

    if (m_rotationEnabled) {
        for (int g = FIRST_ROTATION; g <= LAST_ROTATION; ++g) {
            m_geomArray[g].render(rd, color[g], 1.5f);
        }
    }

    computeProjection(rd);
    m_originalWindow = Vector2(float(rd->width()), float(rd->height()));
    rd->popState();
}


/** The Surface that renders a ThirdPersonManipulator */
class TPMSurface : public Surface {
protected:
    friend class ThirdPersonManipulator;

    ThirdPersonManipulator* m_manipulator;

    TPMSurface(ThirdPersonManipulator* m) : m_manipulator(m) {}

    virtual void defaultRender(RenderDevice* rd) const override {
        alwaysAssertM(false, "Not implemented");
    }

public:

    virtual bool requiresBlending() const override {
        return true;
    }

    virtual bool anyOpaque() const override {
        return true;
    }

    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const override {
        return false;
    }

    virtual void renderWireframeHomogeneous
    (RenderDevice*                          rd, 
        const Array<shared_ptr<Surface> >&  surfaceArray, 
        const Color4&                       color,
        bool                                previous) const override {
            // Intentionally empty
    }
    
    virtual void render
    (RenderDevice*                          rd, 
     const LightingEnvironment&             environment,
     RenderPassType                         passType, 
     const String&                          singlePassBlendedOutputMacro) const override {
        m_manipulator->render3D(rd);
    }

    /** Force rendering after opaque objects so that it can try to
        always be on top. */
    virtual bool hasTransmission() const override {
        return true;
    }

    virtual String name() const override {
        return "ThirdPersonManipulator";
    }

    virtual void getCoordinateFrame(CoordinateFrame& c, bool previous = false) const override {
        m_manipulator->getControlFrame(c);
    }

    virtual void getObjectSpaceBoundingSphere(Sphere& s, bool previous = false) const override {
        s.radius = 2;
        s.center = Vector3::zero();
    }

    virtual void getObjectSpaceBoundingBox(AABox& b, bool previous = false) const override {
        b = AABox(Vector3(-2,-2,-2), Vector3(2,2,2));
    }
};


void ThirdPersonManipulator::onDragBegin(const Vector2& start) {
    (void)start;
    // Intentionally empty.
}


void ThirdPersonManipulator::onDragEnd(const Vector2& stop) {
    (void)stop;
    // Intentionally empty.
    for (int i = 0; i < NUM_GEOMS; ++i) {
        m_usingAxis[i] = false;
    }
}


Vector3 ThirdPersonManipulator::singleAxisTranslationDrag(int a, const Vector2& delta) {
    // Project the mouse delta onto the drag axis to determine how far to drag
    const LineSegment2D& axis = m_geomArray[a].line2D[0].segment(0);

    Vector2 vec = axis.point(1) - axis.point(0);
    float length2 = max(0.5f, vec.squaredLength());

    // Divide by squared length since we not only normalize but need to take into
    // account the angular foreshortening.
    float distance = vec.dot(delta) / length2;
    return m_controlFrame.rotation.column(a) * distance;
}


Vector3 ThirdPersonManipulator::doubleAxisTranslationDrag(int a0, int a1, const Vector2& delta) {

    Vector2 axis[2];
    int   a[2] = {a0, a1};

    // The two dot products represent points on a non-orthogonal set of axes.
    float dot[2];

    for (int i = 0; i < 2; ++i) {
        // Project the mouse delta onto the drag axis to determine how far to drag
        const LineSegment2D& seg = m_geomArray[a[i]].line2D[0].segment(0);

        axis[i] = seg.point(1) - seg.point(0);
        float length2 = max(0.5f, axis[i].squaredLength());
        axis[i] /= length2;

        dot[i] = axis[i].dot(delta);
    }

    // Distance along both
    float common = axis[0].dot(axis[1]) * dot[0] * dot[1];
    float distance0 = dot[0] - common;
    float distance1 = dot[1] - common;

    return m_controlFrame.rotation.column(a0) * distance0 + 
           m_controlFrame.rotation.column(a1) * distance1;
}


void ThirdPersonManipulator::onDrag(const Vector2& delta) {
    if ((m_dragAxis >= XY) && (m_dragAxis <= ZX)) {

        // Translation, multiple axes
        m_controlFrame.translation += doubleAxisTranslationDrag(m_dragAxis - 3, (m_dragAxis - 2) % 3, delta);

    } else if ((m_dragAxis >= X) && (m_dragAxis <= Z)) {

        // Translation, single axis
        m_controlFrame.translation += singleAxisTranslationDrag(m_dragAxis, delta);

    } else {
        // Rotation
    
        // Drag distance.  We divide by the W coordinate so that
        // rotation distance is independent of the size of the widget
        // on screen.
        float angle = delta.dot(m_dragTangent) * 0.00004f / m_dragW;

        // Axis about which to rotate
        Vector3 axis = Vector3::zero();
        axis[m_dragAxis - RX] = 1.0f;

        Matrix3 R = Matrix3::fromAxisAngle(axis, angle);

        m_controlFrame.rotation = m_controlFrame.rotation * R;
        // Prevent accumulated error
        m_controlFrame.rotation.orthonormalize();

    }
}


void ThirdPersonManipulator::computeProjection(RenderDevice* rd) const {
    // TODO: clip to the near plane; modify LineSegment to go through a projection

    for (int g = 0; g < NUM_GEOMS; ++g) {
        m_geomArray[g].computeProjection(rd);
    }
}


bool ThirdPersonManipulator::enabled() const {
    return m_enabled;
}


void ThirdPersonManipulator::setEnabled(bool e) {
    if (m_enabled == e) {
        // Don't stop drag if we were previously enabled
        return;
    }

    m_dragging = false;
    m_enabled = e;
}


ThirdPersonManipulator::ThirdPersonManipulator() : 
    m_axisScale(1), 
    m_dragging(false), 
    m_dragKey(GKey::LEFT_MOUSE),
    m_doubleAxisDrag(false),
    m_overAxis(NO_AXIS),
    m_maxAxisDistance2D(15),
    m_maxRotationDistance2D(10),
    m_rotationEnabled(true),
    m_translationEnabled(true),
    m_enabled(true) {

    for (int i = 0; i < NUM_GEOMS; ++i) {
        m_usingAxis[i] = false;
    }

    // Size of the 2-axis control widget
    const float hi = 0.80f, lo = 0.60f;

    // Single axis (cut a hole out of the center for grabbing all)
    const float cut = 0.20f;

    Array<Vector3> vertex;

    // Create the translation axes.  They will be rendered separately.
    vertex.clear();
    vertex.append(Vector3(cut, 0, 0), Vector3(1.1f, 0, 0));
    m_geomArray[X].line3D.append(_internal::PolyLine(vertex));
    m_geomArray[X].visible = false;

    vertex.clear();
    vertex.append(Vector3(0, cut, 0), Vector3(0, 1.1f, 0));
    m_geomArray[Y].line3D.append(_internal::PolyLine(vertex));
    m_geomArray[Y].visible = false;

    vertex.clear();
    vertex.append(Vector3(0, 0, cut), Vector3(0, 0, 1.1f));
    m_geomArray[Z].line3D.append(_internal::PolyLine(vertex));
    m_geomArray[Z].visible = false;

    // Create the translation squares that lie inbetween the axes
    if (m_doubleAxisDrag) {
        vertex.clear();
        vertex.append(Vector3(lo, hi, 0), Vector3(lo, lo, 0), Vector3(hi, lo, 0), Vector3(hi, hi, 0));
        m_geomArray[XY].poly3D.append(ConvexPolygon(vertex));

        vertex.clear();
        vertex.append(Vector3(0, hi, hi), Vector3(0, lo, hi), Vector3(0, lo, lo), Vector3(0, hi, lo));
        m_geomArray[YZ].poly3D.append(ConvexPolygon(vertex));

        vertex.clear();
        vertex.append(Vector3(lo, 0, lo), Vector3(lo, 0, hi), Vector3(hi, 0, hi), Vector3(hi, 0, lo));
        m_geomArray[ZX].poly3D.append(ConvexPolygon(vertex));
    }

    // Create the rotation circles
    const int ROT_SEGMENTS = 40;
    const float ROT_RADIUS = 0.65f;
    vertex.resize(ROT_SEGMENTS + 1);
    for (int v = 0; v <= ROT_SEGMENTS; ++v) {
        float a = float(twoPi() * v) / ROT_SEGMENTS;
        vertex[v].x = 0;
        vertex[v].y = cos(a) * ROT_RADIUS;
        vertex[v].z = sin(a) * ROT_RADIUS;
    }
    m_geomArray[RX].line3D.append(_internal::PolyLine(vertex));

    for (int v = 0; v <= ROT_SEGMENTS; ++v) {
        vertex[v] = vertex[v].zxy();
    }
    m_geomArray[RY].line3D.append(_internal::PolyLine(vertex));

    for (int v = 0; v <= ROT_SEGMENTS; ++v) {
        vertex[v] = vertex[v].zxy();
    }
    m_geomArray[RZ].line3D.append(_internal::PolyLine(vertex));

    m_posedModel.reset(new TPMSurface(this));
}


void ThirdPersonManipulator::setRotationEnabled(bool r) {
    m_rotationEnabled = r;
}


bool ThirdPersonManipulator::rotationEnabled() const {
    return m_rotationEnabled;
}


void ThirdPersonManipulator::setTranslationEnabled(bool r) {
    m_translationEnabled = r;
}


bool ThirdPersonManipulator::translationEnabled() const {
    return m_translationEnabled;
}


CoordinateFrame ThirdPersonManipulator::computeOffsetFrame(
    const CoordinateFrame& controlFrame, 
    const CoordinateFrame& objectFrame) {
    return objectFrame * controlFrame.inverse();
}


CoordinateFrame ThirdPersonManipulator::frame() const {
    CoordinateFrame c;
    getFrame(c);
    return c;
}


void ThirdPersonManipulator::onPose(
    Array<shared_ptr<Surface> >& posedArray, 
    Array<Surface2DRef>& posed2DArray) {

    if (m_enabled) {
        (void)posed2DArray;
        posedArray.append(m_posedModel);
    }
}


void ThirdPersonManipulator::setControlFrame(const CoordinateFrame& c) {
    // Compute the offset for the new control frame, and then
    // change the control frame.
    m_offsetFrame = c.inverse() * frame();
    m_controlFrame = c;
}


void ThirdPersonManipulator::getControlFrame(CoordinateFrame& c) const {
    c = m_controlFrame;
}


void ThirdPersonManipulator::setFrame(const CoordinateFrame& c) {
    m_controlFrame = m_offsetFrame.inverse() * c;
}


void ThirdPersonManipulator::getFrame(CoordinateFrame& c) const {
    c = m_controlFrame * m_offsetFrame;
}


void ThirdPersonManipulator::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)rdt;
    (void)idt;
    (void)sdt;
}


bool ThirdPersonManipulator::onEvent(const GEvent& event) {
    if (! m_enabled) {
        return false;
    }

    if (event.type == GEventType::MOUSE_MOTION) {
        const Vector2 guardBandThickness = Vector2((m_originalWindow.x - window()->width()) / 2.0f, (m_originalWindow.y - window()->height()) / 2.0f); 
        const Vector2 mouseXY(event.motion.x + guardBandThickness.x, event.motion.y + guardBandThickness.y);
        if (m_dragging) {
            // TODO: could use event.motion.xrel/event.motion.yrel if all OSWindows supported it
            // All but CocoaWindow are confirmed as supporting it 
            //onDrag(Vector2(event.motion.xrel, event.motion.yrel));
            onDrag(mouseXY - m_oldMouseXY);

        } else {
            
            // Highlight the axis closest to the mouse
            m_overAxis = NO_AXIS;
            float nearestDepth = finf();

            if (m_translationEnabled) {
                for (int g = FIRST_TRANSLATION; g <= LAST_TRANSLATION; ++g) {
                    const _internal::UIGeom& geom = m_geomArray[g];

                    if (geom.contains(mouseXY, nearestDepth, m_dragTangent, m_dragW, m_maxAxisDistance2D)) {
                        m_overAxis = g;
                    }
                }
            }

            if (m_rotationEnabled) {
                for (int g = FIRST_ROTATION; g <= LAST_ROTATION; ++g) {
                    const _internal::UIGeom& geom = m_geomArray[g];

                    if (geom.contains(mouseXY, nearestDepth, m_dragTangent, m_dragW, m_maxRotationDistance2D)) {
                        m_overAxis = g;
                    }
                }
            }
        }
        m_oldMouseXY = mouseXY;

        // Do not consume the motion event
        return false;
    } 

    if ((event.type == GEventType::MOUSE_BUTTON_UP) && (event.button.button == 0) && m_dragging) {
        // Stop dragging
        m_dragging = false;
        const Vector2 mouseXY(event.button.x, event.button.y);
        onDragEnd(mouseXY);

        // Consume the mouse up
        return true;

    } else if ((event.type == GEventType::MOUSE_BUTTON_DOWN) && (event.button.button == 0)) {

        // Maybe start a drag
        if (m_overAxis != NO_AXIS) {
            
            // The user clicked on an axis
            m_dragAxis = m_overAxis;
            m_dragging = true;
            m_usingAxis[m_overAxis] = true;

            // Translation along two axes
            if ((m_overAxis >= XY) && (m_overAxis <= ZX)) {
                // Select the two adjacent axes as well as the
                // box that was clicked on
                m_usingAxis[m_overAxis - 3] = true;
                m_usingAxis[(m_overAxis - 3 + 1) % 3] = true;
            }

            const Vector2 mouseXY(event.button.x, event.button.y);
            onDragBegin(mouseXY);
            return true;
        } // if drag
    } // if already dragging

    return false;
}


void ThirdPersonManipulator::onUserInput(UserInput* ui) {
    (void)ui;
}


void ThirdPersonManipulator::onNetwork() {
}


void ThirdPersonManipulator::onAI() {
}

}
