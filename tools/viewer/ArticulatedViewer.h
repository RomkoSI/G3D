/**
 \file ArticulatedViewer.h
 
 Viewer for files that can be loaded by ArticulatedModel

 \maintainer Morgan McGuire
 \author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 \created 2007-05-31
 \edited  2015-01-08
 */
#ifndef ArticulatedViewer_h
#define ArticulatedViewer_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>
#include "Viewer.h"

class ArticulatedViewer : public Viewer {
private:

    class InstructionSurface : public Surface2D {
    private:
        shared_ptr<GFont>   m_font;
        shared_ptr<Texture> m_guide;

        InstructionSurface(const shared_ptr<Texture>& guide, const shared_ptr<GFont>& font) : m_font(font), m_guide(guide) {};
    
    public:
        
        static shared_ptr<InstructionSurface> create(const shared_ptr<Texture>& guide, const shared_ptr<GFont>& font) {
            return shared_ptr<InstructionSurface>(new InstructionSurface(guide, font));
        };
        
        virtual Rect2D bounds() const {
            return Rect2D::xywh(0, 0, (float)m_guide->width(), (float)m_guide->height());
        }
            
        virtual float depth() const {
            // Lowest possible depth
            return 0.0f;
        }

        virtual void render(RenderDevice *rd) const {
            const Rect2D& rect = 
                Rect2D::xywh(15.0f, float(rd->height() - m_guide->height() - 5), 
                             (float)m_guide->width(), (float)m_guide->height());

            m_font->draw2D(rd, "ESC - Quit  F3 - Toggle Hierarchy  F4 - Screenshot   F6 - Record Video   F8 - Render Cube Map   R - Reload", rect.x0y0() + Vector2(-10, -25), 10, Color3::black(), Color3::white());
            rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
            Draw::rect2D(rect, rd, Color4(Color3::white(), 0.8f), m_guide);
        }
    };

    String                                  m_filename;
    shared_ptr<ArticulatedModel>            m_model;
    int                                     m_numFaces;
    int                                     m_numVertices;

    ArticulatedModel::Part*                 m_selectedPart;

    ArticulatedModel::Mesh*                 m_selectedMesh;

    static shared_ptr<InstructionSurface>   m_instructions;

    /** In the Mesh's cpuIndexAray index array */
    int                                     m_selectedTriangleIndex;

    static shared_ptr<Surface>              m_skyboxSurface;

    static shared_ptr<GFont>                m_font;

    /** Scale applied to the model; stored only for printing the value as an overlay */
    float                                   m_scale;

    /** Translation applied to the model; stored only for printing the value as an overlay */
    Vector3                                 m_offset;

    /** True if the shadow map is out of date. This is true for the first frame and continues
        to be true if the model animates. */
    bool                                    m_shadowMapDirty;

    SimTime                                 m_time;

    ArticulatedModel::Pose                  m_pose;

    // Will be empty if the model does not have skeletal animations
    ArticulatedModel::Animation             m_animation;

    /** Saves the geometry for the first part to a flat file */
    void saveGeometry();

public:

    ArticulatedViewer();

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) override;
    virtual void onInit(const String& filename) override;
    virtual bool onEvent(const GEvent& e, App* app) override;
    virtual void onGraphics3D(RenderDevice* rd, App* app, const shared_ptr<LightingEnvironment>& lighting, Array<shared_ptr<Surface> >& surfaceArray) override;
    virtual void onPose(Array<shared_ptr<Surface> >& posed3D, Array<shared_ptr<Surface2D> >& posed2D) override;

};

#endif 
