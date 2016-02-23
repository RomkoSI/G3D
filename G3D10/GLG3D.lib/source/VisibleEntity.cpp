/*
  \file GLG3D.lib/source/VisibleEntity.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2012-07-27
  \edited  2016-02-22
 */
#include "GLG3D/VisibleEntity.h"
#include "G3D/Box.h"
#include "G3D/AABox.h"
#include "G3D/Sphere.h"
#include "G3D/Ray.h"
#include "G3D/LineSegment.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/HeightfieldModel.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/Scene.h"
#include "GLG3D/GuiPane.h"
#include "GLG3D/GApp.h"
#include "G3D/Pointer.h"

namespace G3D {

shared_ptr<Entity> VisibleEntity::create 
   (const String&                           name,
    Scene*                                  scene,
    AnyTableReader&                         propertyTable,
    const ModelTable&                       modelTable,
    const Scene::LoadOptions&               options) {

        
    bool canChange = false;
    propertyTable.getIfPresent("canChange", canChange);
    // Pretend that we haven't peeked at this value
    propertyTable.setReadStatus("canChange", false);

    if ((canChange && ! options.stripDynamicVisibleEntitys) || 
        (! canChange && ! options.stripStaticVisibleEntitys)) {

        shared_ptr<VisibleEntity> visibleEntity(new VisibleEntity());
        visibleEntity->Entity::init(name, scene, propertyTable);
        visibleEntity->VisibleEntity::init(propertyTable, modelTable);
        propertyTable.verifyDone();

        return visibleEntity;
    } else {
        return nullptr;
    }
}


shared_ptr<VisibleEntity> VisibleEntity::create
   (const String&                           name,
    Scene*                                  scene,
    const shared_ptr<Model>&                model,
    const CFrame&                           frame,
    const shared_ptr<Entity::Track>&        track,
    bool                                    canChange,
    bool                                    shouldBeSaved,
    bool                                    visible,
    const Surface::ExpressiveLightScatteringProperties& expressiveLightScatteringProperties,
    const ArticulatedModel::PoseSpline&     artPoseSpline,
    const ArticulatedModel::Pose&           artPose) {

    shared_ptr<VisibleEntity> visibleEntity(new VisibleEntity());

    visibleEntity->Entity::init(name, scene, frame, track, canChange, shouldBeSaved);
    visibleEntity->VisibleEntity::init(model, visible, expressiveLightScatteringProperties, artPoseSpline, MD3Model::PoseSequence(), artPose);
     
    return visibleEntity;
}


VisibleEntity::VisibleEntity() : Entity(), m_visible(true) {
}


void VisibleEntity::init
   (const shared_ptr<Model>&                model,
    bool                                    visible,
    const Surface::ExpressiveLightScatteringProperties& expressiveLightScatteringProperties,
    const ArticulatedModel::PoseSpline&     artPoseSpline,
    const MD3Model::PoseSequence&           md3PoseSequence,
    const ArticulatedModel::Pose&           artPose) {

    m_modelType = NONE;
    m_artPoseSpline = artPoseSpline;
    m_md3PoseSequence = md3PoseSequence;
    m_visible = visible;
    
    setModel(model);
    m_artPose = artPose;
    m_artPreviousPose = artPose;

    m_expressiveLightScatteringProperties = expressiveLightScatteringProperties;

    if (m_artModel) {
        m_artPose.expressiveLightScatteringProperties         = m_expressiveLightScatteringProperties;
        m_artPreviousPose.expressiveLightScatteringProperties = m_expressiveLightScatteringProperties;
    } else if (m_md2Model) {
        m_md2Pose.expressiveLightScatteringProperties = m_expressiveLightScatteringProperties;
    } else if (m_md3Model) {
        m_md3Pose.expressiveLightScatteringProperties = m_expressiveLightScatteringProperties;
    }
}


void VisibleEntity::init
   (AnyTableReader&    propertyTable,
    const ModelTable&  modelTable) {

    bool visible = true;
    propertyTable.getIfPresent("visible", visible);

    ArticulatedModel::Pose artPose;
    propertyTable.getIfPresent("articulatedModelPose", artPose);

    ArticulatedModel::PoseSpline artPoseSpline;
    propertyTable.getIfPresent("poseSpline", artPoseSpline);

	MD3Model::PoseSequence md3PoseSequence;
	propertyTable.getIfPresent("md3Pose", md3PoseSequence);

    Surface::ExpressiveLightScatteringProperties expressiveLightScatteringProperties;
    propertyTable.getIfPresent("expressiveLightScatteringProperties", expressiveLightScatteringProperties);
    
    if (propertyTable.getIfPresent("castsShadows", expressiveLightScatteringProperties.castsShadows)) {
        debugPrintf("Warning: castsShadows field is deprecated.  Use expressiveLightScatteringProperties");
    }    

    const lazy_ptr<Model>* model = NULL;
    Any modelNameAny;
    if (propertyTable.getIfPresent("model", modelNameAny)) {
        const String& modelName     = modelNameAny.string();
    
        modelNameAny.verify(modelTable.containsKey(modelName), 
                        "Can't instantiate undefined model named " + modelName + ".");
        model = modelTable.getPointer(modelName);
    }

    Any ignore;
    if (propertyTable.getIfPresent("materialTable", ignore)) {
        ignore.verify(false, "'materialTable' is deprecated. Specify materials on the articulatedModelPose field of VisibleEntity.");
    }

    init(notNull(model) ? model->resolve() : shared_ptr<Model>(), visible, expressiveLightScatteringProperties, artPoseSpline, md3PoseSequence, artPose);
}


void VisibleEntity::setModel(const shared_ptr<Model>& model) {
    m_model = model;

    m_artModel = dynamic_pointer_cast<ArticulatedModel>(m_model);
    m_md2Model = dynamic_pointer_cast<MD2Model>(m_model);
    m_md3Model = dynamic_pointer_cast<MD3Model>(m_model);
    m_heightfieldModel = dynamic_pointer_cast<HeightfieldModel>(m_model);

    if (m_artModel) {
        m_modelType = ARTICULATED_MODEL;
    } else if (m_md2Model) {
        m_modelType = MD2_MODEL;
    } else if (m_md3Model) {
        m_modelType = MD3_MODEL;
    } else if (m_heightfieldModel) {
        m_modelType = HEIGHTFIELD_MODEL;
    }

    m_lastChangeTime = System::time();
}


void VisibleEntity::simulatePose(SimTime absoluteTime, SimTime deltaTime) {
    switch (m_modelType) {
    case ARTICULATED_MODEL:
        if (isNaN(deltaTime) || (deltaTime > 0)) {
            m_artPreviousPose.frameTable = m_artPose.frameTable;
            m_artPreviousPose.uniformTable = m_artPose.uniformTable;
            if (notNull(m_artPose.uniformTable)) {
                m_artPose.uniformTable = shared_ptr<UniformTable>(new UniformTable(*m_artPreviousPose.uniformTable));
            }
        }

        if (m_artModel->usesSkeletalAnimation()) {
            Array<String> animationNames;
            m_artModel->getAnimationNames(animationNames);
            ArticulatedModel::Animation animation;
            m_artModel->getAnimation(animationNames[0], animation);
            animation.getCurrentPose(absoluteTime, m_artPose);
        } else {
            m_artPoseSpline.get(float(absoluteTime), m_artPose);
        }

        // Intentionally only compare cframeTables; materialTables don't change (usually)
        // and are more often non-empty, which could trigger a lot of computation here.
        if (m_artPreviousPose.frameTable != m_artPose.frameTable) {
            m_lastChangeTime = System::time();
        }
        break;

    case MD2_MODEL:
        {
            MD2Model::Pose::Action a;
            m_md2Pose.onSimulation(deltaTime, a);
            if (isNaN(deltaTime) || (deltaTime > 0)) {
                m_lastChangeTime = System::time();
            }
            break;
        }

    case MD3_MODEL:
        m_md3PoseSequence.getPose(float(absoluteTime), m_md3Pose);
        m_md3Model->simulatePose(m_md3Pose, deltaTime);
        if (isNaN(deltaTime) || (deltaTime > 0)) {
            m_lastChangeTime = System::time();
        }
        break;

    case HEIGHTFIELD_MODEL:
        break;

    case NONE:
        break;

    default:
        alwaysAssertM(false, "Tried to simulate a Visible Entity that was not one of the four supported model types.\n");
        break;
    }
}


void VisibleEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    Entity::onSimulation(absoluteTime, deltaTime);
    simulatePose(absoluteTime, deltaTime);
}


bool VisibleEntity::poseModel(Array<shared_ptr<Surface> >& surfaceArray) const {
    const shared_ptr<Entity>& me = dynamic_pointer_cast<Entity>(const_cast<VisibleEntity*>(this)->shared_from_this());
    switch (m_modelType) {
    case ARTICULATED_MODEL:
        m_artModel->pose(surfaceArray, m_frame, m_artPose, m_previousFrame, m_artPreviousPose, me);
        break;

    case MD2_MODEL:
        m_md2Model->pose(surfaceArray, m_frame, m_previousFrame, m_md2Pose, me);
        break;

    case MD3_MODEL:
        m_md3Model->pose(surfaceArray, m_frame, m_md3Pose, me);
        break;

    case HEIGHTFIELD_MODEL:
        m_heightfieldModel->pose(m_frame, m_previousFrame, surfaceArray, me, m_expressiveLightScatteringProperties);
        break;

    case NONE:
        break;

    default:
        alwaysAssertM(false, "Tried to pose a VisibleEntity that was not one of the four supported model types.\n");
        break;
    }

    
    return        !(
        (((m_modelType == ARTICULATED_MODEL) &&
         (m_artPose.frameTable.size() == 0) && 
         (m_artPreviousPose.frameTable.size() == 0)) || 
         (m_modelType == HEIGHTFIELD_MODEL)) &&
        (m_frame == m_previousFrame));
}


void VisibleEntity::onPose(Array<shared_ptr<Surface> >& surfaceArray) {

    // We have to pose in order to compute bounds that are used for selection in the editor
    // and collisions in simulation, so pose anyway if not visible,
    // but then roll back.
    debugAssert(isFinite(m_frame.translation.x));
    debugAssert(! isNaN(m_frame.rotation[0][0]));
    const int oldLen = surfaceArray.size();

    const bool boundsChangedSincePreviousFrame = poseModel(surfaceArray);

    // Compute bounds for objects that moved
    if (m_lastAABoxBounds.isEmpty() || boundsChangedSincePreviousFrame || (m_lastChangeTime > m_lastBoundsTime)) {

        m_lastSphereBounds = Sphere(m_frame.translation, 0);
        
        const CFrame& myFrameInverse = frame().inverse();

        m_lastObjectSpaceAABoxBounds = AABox::empty();
        m_lastBoxBoundArray.fastClear();

        // Look at all surfaces produced
        for (int i = oldLen; i < surfaceArray.size(); ++i) {
            AABox b;
            Sphere s;
            const shared_ptr<Surface>& surf = surfaceArray[i];

            // body to world transformation for the surface
            CoordinateFrame cframe;
            surf->getCoordinateFrame(cframe, false);
            debugAssertM(cframe.translation.x == cframe.translation.x, "NaN translation");


            surf->getObjectSpaceBoundingSphere(s);
            s = cframe.toWorldSpace(s);
            m_lastSphereBounds.radius = max(m_lastSphereBounds.radius,
                                            (s.center - m_lastSphereBounds.center).length() + s.radius);


            // Take the entity's frame out of consideration, so that we get tight AA bounds 
            // in the Entity's frame
            CFrame osFrame = myFrameInverse * cframe;

            surf->getObjectSpaceBoundingBox(b);

            m_lastBoxBoundArray.append(cframe.toWorldSpace(b));
            const Box& temp = osFrame.toWorldSpace(b);
            m_lastObjectSpaceAABoxBounds.merge(temp);
        }

        // Box can't represent an empty box, so we make empty boxes into real boxes with zero volume here
        if (m_lastObjectSpaceAABoxBounds.isEmpty()) {
            m_lastObjectSpaceAABoxBounds = AABox(Point3::zero());
            m_lastAABoxBounds = AABox(frame().translation);
        }

        m_lastBoxBounds = frame().toWorldSpace(m_lastObjectSpaceAABoxBounds);
        m_lastBoxBounds.getBounds(m_lastAABoxBounds);
        m_lastBoundsTime = System::time();
    }

    if (! m_visible) {
        // Discard my surfaces if I'm invisible; they were only needed for bounds
        surfaceArray.resize(oldLen, false);
    }
}

#if 0
void VisibleEntity::debugDrawVisualization(RenderDevice* rd, VisualizationMode mode) {
    alwaysAssertM(mode == SKELETON, "Bounds visualization not implemented");
    Array<Point3> skeletonLines;
    switch (m_modelType) {
    case ARTICULATED_MODEL:
        m_artModel->getSkeletonLines(m_artPose, m_frame, skeletonLines);
        break;
    default:
        break;
    }
    rd->pushState(); {
        rd->setObjectToWorldMatrix(CFrame());
        rd->setDepthTest(RenderDevice::DEPTH_ALWAYS_PASS);
        for (int i = 0; i < skeletonLines.size(); i += 2) {
            Draw::lineSegment(LineSegment::fromTwoPoints(skeletonLines[i], skeletonLines[i+1]), rd, Color3::red());
        }
    } rd->popState();
}
#endif


bool VisibleEntity::intersect(const Ray& R, float& maxDistance, Model::HitInfo& info) const {
    switch (m_modelType) {
    case ARTICULATED_MODEL:
        return m_artModel->intersect(R, m_frame, m_artPose, maxDistance, info, dynamic_pointer_cast<Entity>(const_cast<VisibleEntity*>(this)->shared_from_this()));

    case MD2_MODEL:
        return m_md2Model->intersect(R, m_frame, m_md2Pose, maxDistance, info, dynamic_pointer_cast<Entity>(const_cast<VisibleEntity*>(this)->shared_from_this()));

    case MD3_MODEL:
        return m_md3Model->intersect(R, m_frame, m_md3Pose, maxDistance, info, dynamic_pointer_cast<Entity>(const_cast<VisibleEntity*>(this)->shared_from_this()));

    case HEIGHTFIELD_MODEL: 
        return m_heightfieldModel->intersect(R, m_frame, maxDistance, info, dynamic_pointer_cast<Entity>(const_cast<VisibleEntity*>(this)->shared_from_this()));

    case NONE:
        return false;
    
    default:
        alwaysAssertM(false, "Tried to intersect a Visible Entity that was not one of the four supported model types.\n");
        break;
    }

    return false;
}


Any VisibleEntity::toAny(const bool forceAll) const {
    Any a = Entity::toAny(forceAll);
    a.setName("VisibleEntity");

    AnyTableReader oldValues(a);
    bool visible;
    if (forceAll || (oldValues.getIfPresent("visible", visible) && visible != m_visible)) {
        a.set("visible", m_visible);
    }

    // Model and pose must already have been set, so no need to change anything
    return a;
}


void VisibleEntity::onModelDropDownAction() {
    const String& choice = m_modelDropDownList->selectedValue().text();
    if (choice == "<none>") {
        setModel(shared_ptr<Model>());
    } else {
        // Parse the name
        const size_t i = choice.rfind('(');
        const String& modelName = choice.substr(0, i - 1);

        // Find the model with that name
        const lazy_ptr<Model>* model = m_scene->modelTable().getPointer(modelName);
        if (notNull(model)) {
            setModel(model->resolve());
        } else {
            setModel(shared_ptr<Model>());
        } // not null
    }
}


void VisibleEntity::makeGUI(class GuiPane* pane, class GApp* app) {
    Entity::makeGUI(pane, app);

    Array<String> modelNames("<none>");
    int selected = 0;
    for (ModelTable::Iterator it = m_scene->modelTable().begin(); it.hasMore(); ++it) {      
        const lazy_ptr<Model>& m = it->value;

        if (m.resolved()) {
            modelNames.append(it->key + " (" + it->value.resolve()->className() + ")");
        } else {
            // The model type is unknown because it hasn't been loaded yet
            modelNames.append(it->key);
        }

        if (it->value == m_model) {
            selected = modelNames.size() - 1;
        }
    }
    m_modelDropDownList = pane->addDropDownList("Model", modelNames, NULL, GuiControl::Callback(this, &VisibleEntity::onModelDropDownAction));
    m_modelDropDownList->setSelectedIndex(selected);
    pane->addCheckBox("Visible", &m_visible);
}


}
