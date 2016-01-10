/**
 \file GLG3D/VisibleEntity.h
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#ifndef GLG3D_VisibleEntity_h
#define GLG3D_VisibleEntity_h

#include "G3D/platform.h"
#include "GLG3D/Entity.h"
#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/MD2Model.h"
#include "GLG3D/MD3Model.h"


namespace G3D {

class HeightfieldModel;
class Scene;

/** \brief Base class for Entity%s that use a built-in G3D::Model subclass. 

    \sa G3D::Light, G3D::Camera, G3D::MarkerEntity, G3D::Scene
*/
class VisibleEntity : public Entity {
public:
    enum VisualizationMode {
        SKELETON,
        BOUNDS,
        SKELETON_AND_BOUNDS
    };

protected:

    class GuiDropDownList*          m_modelDropDownList;

    /** GUI callback */
    void onModelDropDownAction();

    enum ModelType {
        ARTICULATED_MODEL,
        MD2_MODEL,
        MD3_MODEL,
        HEIGHTFIELD_MODEL,
        NONE
    };

    // Duplicated within the poses
    Surface::ExpressiveLightScatteringProperties m_expressiveLightScatteringProperties;

    ModelType                       m_modelType;

    shared_ptr<Model>               m_model;

    /** Current pose */
    ArticulatedModel::Pose          m_artPose;

    /** Pose for the previous onSimulation.  The cframeTable field is copied
        from m_artPose every frame by the default simulatePose implementation, 
        but the other fields are unmodified (because copying an entire, unchanged
        material table would be inefficient). */
    ArticulatedModel::Pose          m_artPreviousPose;

    /** Pose over time. */
    ArticulatedModel::PoseSpline    m_artPoseSpline;

    shared_ptr<ArticulatedModel>    m_artModel;

    //////////////////////////////////////////////

    shared_ptr<MD2Model>            m_md2Model;
    MD2Model::Pose                  m_md2Pose;

    //////////////////////////////////////////////

    shared_ptr<MD3Model>            m_md3Model;
    MD3Model::Pose                  m_md3Pose;
    MD3Model::PoseSequence          m_md3PoseSequence;

    //////////////////////////////////////////////
    shared_ptr<HeightfieldModel>    m_heightfieldModel;

    /** Should this Entity currently be allowed to affect any part of the rendering pipeline (e.g., shadows, primary rays, indirect light)?  If false, 
    the Entity never returns any surfaces from onPose(). Does not necessarily mean that the underlying model is visible to primary rays.*/
    bool                            m_visible;

    VisibleEntity();

    /** \sa create */
    void init
        (AnyTableReader&                                              propertyTable,
         const ModelTable&                                            modelTable);

    /** \sa create */
    void init
       (const shared_ptr<Model>&                                      model,
        bool                                                          visible,
        const Surface::ExpressiveLightScatteringProperties&           expressiveLightScatteringProperties,
        const ArticulatedModel::PoseSpline&                           artPoseSpline,
        const MD3Model::PoseSequence&                                 md3PoseSequence = MD3Model::PoseSequence(),
        const ArticulatedModel::Pose&                                 artPose = ArticulatedModel::Pose());

    /** Animates the appropriate pose type for the model selected.
     Called from onSimulation.  Subclasses will frequently replace
     onSimulation but retain this helper method.*/
    virtual void simulatePose(SimTime absoluteTime, SimTime deltaTime);
    
    /** Called from VisibleEntity::onPose to extract surfaces from the model.
        If you subclass VisibleEntity to support a new model type, override 
        this method. onPose will then still compute bounds correctly for
        your model.

        \return True if the surfaces returned have different bounds than in the previous
        frame, e.g., due to animation.
    */
    virtual bool poseModel(Array<shared_ptr<Surface> >& surfaceArray) const;

public:

   /** \brief Construct a VisibleEntity.

       \param name The name of this Entity, e.g., "Player 1"

       \param propertyTable The form is given below.  It is intended that
       subclasses replace the table name and add new fields.

       \code
       VisibleEntity {
           model        = <modelname>;
           position     = <CFrame, Vector3, or PhysicsFrameSpline>;
           poseSpline   = <ArticulatedModel::PoseSpline>;
           articulatedModelPose = <ArticulatedModel::Pose>;
           md3Pose      = <MD2Model::PoseSequence>;
           visible      = <bool>;
           expressiveLightScatteringProperties = <object>;
           }
       }
       \endcode

       In particular, to pass a custom Shader argument (see also Args and UniformTable) for a VisibleEntity,
       use:

       \code
       VisibleEntity {
          ...
          articulatedModelPose = ArticulatedModel::Pose {
             ...
             uniformTable = {
                 myShaderArg = <value>;
             }
          };
       }
       \endcode

       See also the properties from Entity::init.

       expressiveLightScatteringProperties are stored on the pose object for the specific model chosen so that multiple instances of the
       same model can exist with different shadow-casting properties.

       The poseSpline, position, and castsShadows fields are optional.  The Entity base class
       reads these fields.  Other subclasses read their own fields.  Do not specify a poseSpline
       unless you want to override the defaults.  Specifying them reduces rendering performance slightly.

       If specified, the castsShadows field overwrites the pose's castsShadows field.  This is a load-time convenience. 

       \param modelTable Maps model names that are referenced in \a propertyTable
       to shared_ptr<ArticulatedModel>, MD2Model::Ref or MD3::ModelRef.
       
       \param scene May be NULL

       The original caller (typically, a Scene class) should invoke
       AnyTableReader::verifyDone to ensure that all of the fields 
       specified were read by some subclass along the inheritance chain.       

       See samples/starter/source/Scene.cpp for an example of use.
     */
    static shared_ptr<Entity> create 
        (const String&                           name,
         Scene*                                  scene,
         AnyTableReader&                         propertyTable,
         const ModelTable&                       modelTable);

    static shared_ptr<VisibleEntity> create
       (const String&                           name,
        Scene*                                  scene,
        const shared_ptr<Model>&                model,
        const CFrame&                           frame = CFrame(),
        const shared_ptr<Track>&                track = shared_ptr<Entity::Track>(),
        bool                                    canChange = true,
        bool                                    shouldBeSaved = true,
        bool                                    visible = true,
        const Surface::ExpressiveLightScatteringProperties& expressiveLightScatteringProperties = Surface::ExpressiveLightScatteringProperties(),
        const ArticulatedModel::PoseSpline&     artPoseSpline = ArticulatedModel::PoseSpline(),
        const ArticulatedModel::Pose&           artPose = ArticulatedModel::Pose());

    /** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "Entity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;

    /** Not all VisibleEntity subclasses will accept all models. If this model is not appropriate for this subclass,
        then the model() will not change. */
    virtual void setModel(const shared_ptr<Model>& model);
    
    /** 
     Invokes poseModel to compute the actual surfaces and then computes bounds on them when needed.
     \sa Entity::onPose
    */
    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray) override;

    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

    virtual bool intersect(const Ray& R, float& maxDistance, Model::HitInfo& info = Model::HitInfo::ignore) const override;

    bool visible() const {
        return m_visible;
    }

    virtual void setVisible(bool b) {
        m_visible = b;
    }

    shared_ptr<Model> model() const {
        return m_model;
    }

    ArticulatedModel::Pose& articulatedModelPose() {
        debugAssert(notNull(m_artModel));
        return m_artPose;
    }

    virtual void setPose(const ArticulatedModel::Pose& artPose) {
        debugAssert(notNull(m_artModel));
        m_artPose = artPose;
    }

    virtual void setPose(const MD2Model::Pose& md2Pose) {
        debugAssert(notNull(m_md2Model));
        m_md2Pose = md2Pose;
    }

    virtual void setPose(const MD3Model::Pose& md3Pose) {
        debugAssert(notNull(m_md3Model));
        m_md3Pose = md3Pose;
    }

    virtual void makeGUI(class GuiPane* pane, class GApp* app) override;

};

}
#endif
