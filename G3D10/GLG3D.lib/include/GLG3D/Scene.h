/**
  \file GLG3D/Scene.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2010-01-01
  \edited  2016-02-10
*/
#ifndef GLG3D_Scene_h
#define GLG3D_Scene_h

#include "G3D/platform.h"
#include "G3D/ReferenceCount.h"
#include "G3D/Array.h"
#include "G3D/SmallArray.h"
#include "G3D/lazy_ptr.h"
#include "GLG3D/Camera.h"
#include "GLG3D/LightingEnvironment.h"
#include "GLG3D/ArticulatedModel.h"

namespace G3D {

class GFont;
class Ray;
class Light;
class Camera;
class Model;
class Skybox;
class SceneVisualizationSettings;

/** \brief Base class for a scene graph.

    %G3D presents a layered API in which the scene graph is available for convenience but can be
    subclassed and overridden, completely replaced, or simply ignored by applications if it is 
    not a good fit.

    The major classes involved in the scene graph API are:

    - Scene holds the state of the scene, including the current time.
    - Entity is an object instance in the scene, including a root transformation and bounding boxes.  It may be visible (e.g., VisibleEntity) or invisible (e.g., MarkerEntity, Light, Camera) depending on its function.
    - Model is the template of the geometry that is instanced by a VisibleEntity.  ArticulatedModel is the most general subclass provided in the %G3D API.
    - Pose holds the state that a VisibleEntity provides to a Model to indicate the specifics of its current pose, except for the root transform.  The Pose subclass must be paired with the Model subclass.
    - Surface is the viewer-indpendent geometric information (as a mesh) for rendering a particular frame, obtained by combining information from an Entity, Model, and Pose at a specific time.  UniversalSurface is the most general subclass provided in the %G3D API.

    Although the analogy is imperfect, the Pose-Model-Entity design pattern is similar to the Model-Viewer-Controller design pattern.
    Entity combines the operations of a MVC "Viewer" and MVC "Controller" in abstracting an object from the user (here, Scene/GApp). Pose and Model
    represent the instance-specific and instance-independent state of the MVC "Model".

   \see G3D::Entity, G3D::VisibleEntity, G3D::Camera, G3D::SceneEditorWindow, G3D::SceneVisualizationSettings
*/
class Scene : public ReferenceCountedObject {
public:

    /** \sa registerEntityType */
    typedef shared_ptr<Entity> (*EntityFactory)(const String& name, Scene* scene, AnyTableReader& propertyTable, const ModelTable& modelTable);

protected:
    enum VisitorState {NOT_VISITED, VISITING, ALREADY_VISITED};

    // We expect one dependency per object maximum, but it is cheap to allocate two
    // elements in the off chance that we need them
    typedef SmallArray<String, 2> DependencyList;
   
    /** List of the names of all entities that must run before a given entity. 
        This is often empty. */
    typedef Table<String, DependencyList> DependencyTable;

    /** Used by setOrder and clearOrder */
    DependencyTable                     m_dependencyTable;

    /** When true, the m_entityArray needs to be re-sorted based on dependencies before iterating. */
    bool                                m_needEntitySort;

    String                              m_name;

    /** The Any from which this scene was constructed. */
    Any                                 m_sourceAny;

    /** Current time */
    SimTime                             m_time;
    
    LightingEnvironment                 m_localLightingEnvironment;

    /** All Entitys, including Cameras and Lights, by name */
    Table<String, shared_ptr<Entity> >  m_entityTable;

    /** All Entitys, including Cameras and Lights */
    Array< shared_ptr<Entity> >         m_entityArray;

    Array< shared_ptr<Camera> >         m_cameraArray;

    shared_ptr<Skybox>                  m_skybox;

    RealTime                            m_lastStructuralChangeTime;

    RealTime                            m_lastVisibleChangeTime;

    RealTime                            m_lastLightChangeTime;

    ModelTable                          m_modelTable;

    Any                                 m_modelsAny;

    bool                                m_editing;

    RealTime                            m_lastEditingTime;

    String                              m_defaultCameraName;
 
    Table<String, EntityFactory>        m_entityFactory;

    shared_ptr<GFont>                   m_font;

    Scene(const shared_ptr<AmbientOcclusion>& ambientOcclusion);

    const shared_ptr<Entity> _entity(const String& name) const;
     
    /** If m_needEntitySort, sort Entitys to resolve dependencies and set m_needEntitySort = false. Called fromOnSimulation */
    void sortEntitiesByDependency();

public:

    /** \brief Register a new subclass of G3D::Entity so that it can be constructed from a .Scene.Any file.
        You can also override Scene::createEntity to add support for new Entity types.
    */
    void registerEntitySubclass(const String& name, EntityFactory factory, bool errorIfAlreadyRegistered = true);

    /** Adds the model to the model table and returns it. */
    virtual lazy_ptr<Model> createModel(const Any& v, const String& name);

    const ModelTable& modelTable() const {
        return m_modelTable;
    }

    const String& name() const {
        return m_name;
    }

    /** Add an Entity to the Scene (and return it).

        Assumes that no entity with the same name is present in the scene.

     \sa createEntity, remove */
    virtual shared_ptr<Entity> insert(const shared_ptr<Entity>& entity);

    /** Add a Model to the Scene's modelTable() (and return it).

        Assumes that no model with the same name is present in the scene.
      \sa createModel, modelTable*/
    virtual shared_ptr<Model> insert(const shared_ptr<Model>& model);

    /** Remove an entity that must already be in the scene.
        Does not change the order of the other Entity%s in the scene
        (i.e., entityArray())

        Note that removal occurs immediately, so be avoid invoking this
        in the middle of iterating through entityArray().

      \sa insert, createEntity */
    virtual void remove(const shared_ptr<Entity>& entity);
    virtual void removeEntity(const String& entityName);

    /** Remove a model from modelTable. Does not affect Entitys currently
     using that model, only ones later instantiated with createEntity().
      \sa remove, createModel, modelTable
     */
    virtual void remove(const shared_ptr<Model>& model);
    virtual void removeModel(const String& modelName);

    /** 
     Directories in which loadScene() and getSceneNames() will search for .scn.any
     files.

     By default, the search path for scene files is the current directory
     (as of the time that the first scene is loaded or getSceneNames() is
     invoked), the paths specified by the G3D10DATA environment variable,
     and the directory containing System::findDataFile("scene/CornellBox-Glossy.scn.any")
     if not already invluded in G3D10Data.

     This does not affect the directories that models and include statements
     search to load other resources.
    */
    static void setSceneSearchPaths(const Array<String>& paths);

    /** Create an Entity and insert() it into the Scene.
        \sa registerEntitySubclass, m_modelTable
     */
    virtual shared_ptr<Entity> createEntity(const String& name, const Any& any);

    /** For creating an entity with an explicit type such as Skybox
        where the \a any may be a simple
        filename instead of a named Any::TABLE. */
    virtual shared_ptr<Entity> createEntity(const String& entityType, const String& name, const Any& any);

    /** \param ambientOcclusion Object to use for the LightingEnvironment */
    static shared_ptr<Scene> create(const shared_ptr<AmbientOcclusion>& ambientOcclusion);

    /** Remove all objects */
    virtual void clear();

    /**
      When true, even entities with Entity::canChange = false are allowed to change.
      This is primarily used when the developer is actively editing the scene, versus
      when the program is in its normal operation mode.  The SceneEditorWindow GUI
      will automatically modify this.
     */
    bool editing() const {
        return m_editing;
    }

    // Cannot be virtual because SceneEditorWindow needs to create a 
    // pointer to this method.
    void setEditing(bool b);

    /** Last (wall clock) time that the Scene was in editing() mode.
        If the scene is currently being edited, this continuously changes.
        Allows other code that references the scene to know whether values
        that it has cached are stale.
    */
    RealTime lastEditingTime() const {
        return m_lastEditingTime;
    }

    class LoadOptions {
    public:
        /** Remove Entitys for which canChange = true. Default = false */
        bool        stripStaticEntitys;

        /** Remove Entitys for which canChange = false. Default = false */
        bool        stripDynamicEntitys;

        LoadOptions() : stripStaticEntitys(false), stripDynamicEntitys(false) {}
    };

    /** \brief Replace the current scene with a new one parsed from a file.  See the starter project
        for examples
        Entity%s may have already moved since creation because they are simulated for 0s at start. 
        

        \param sceneName The 'name' field specified inside the scene file, or the filename of the scene file

        \return The Any from which the scene was parsed.  This is useful for reading your own custom fields from.
    */
    virtual Any load(const String& sceneName, const LoadOptions& loadOptions = LoadOptions());
    
    /** Returns the default camera, set by defaultCamera = "name" in the Scene file. */
    const shared_ptr<Camera> defaultCamera() const;

    /** Creates an Any representing this scene by updating the one
     from which it was loaded with the current Entity positions.  This
     will overwrite any \htmlonly <code>#include</code>\endhtmlonly entries that appeared in
     the original source Any.

     You can obtain the original filename as a.source().filename().
    */
    Any toAny(const bool forceAll = false) const;

    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray);

    virtual void onSimulation(SimTime deltaTime);

    const LightingEnvironment& lightingEnvironment() const {
        return m_localLightingEnvironment;
    }

    LightingEnvironment& lightingEnvironment() {
        return m_localLightingEnvironment;
    }

    /** Append all Entity%s (which include Cameras and Lights) to the array. 
        \sa getTypedEntityArray
    */
    void getEntityArray(Array<shared_ptr<Entity> >& array) const {
        array.append(m_entityArray);
    }

    /** 
    Append all Entity%s that are subclasses of \a EntitySubclass to \a array.
        \sa getEntityArray
    */
    template<class EntitySubclass>
    void getTypedEntityArray(Array< shared_ptr<EntitySubclass> >& array) const {
        for (int e = 0; e < m_entityArray.size(); ++e) {
            const shared_ptr<EntitySubclass>& entity = dynamic_pointer_cast<EntitySubclass>(m_entityArray[e]);
            if (notNull(entity)) {
                array.append(entity);
            }
        }
    }

    SimTime time() const {
        return m_time;
    }

    /** Discontinuously change the current time.
        Also invokes <code>onSimulation(time(), nan())</code> <i>twice<i> so that scene positions
        and previous positions are updated. */
    virtual void setTime(const SimTime t);

    /** \sa getCameraNames, entity */
    void getEntityNames(Array<String>& names) const;

    /** Note that because Camera%s are Entity%s, these also appear in the entity array. 
       \sa getEntityNames, camera */
    void getCameraNames(Array<String>& names) const;

    /** \sa typedEntity */
    const shared_ptr<Entity> entity(const String& name) const;

    /** Get an entity by name and downcast to the desired type.  Returns NULL if the entity
        does not exist or does not have that type. */
    template<class EntitySubclass>
    const shared_ptr<EntitySubclass> typedEntity(const String& name) const {
        return dynamic_pointer_cast<EntitySubclass>(entity(name));
    }

    /** Enumerate the names of all available scenes. This is recomputed each time that it is called. */
    static Array<String> sceneNames();

    /** If scene is a filename, returns it, else look up the string in the filename table and return the value.
        If no entry exists, throw an exception */
    static String sceneNameToFilename(const String& scene);

    /** Returns the Entity whose conservative bounds are first
        intersected by \a ray, excluding Entity%s in \a exclude.  
        Useful for mouse selection and coarse hit-scan collision detection.  
        Returns NULL if none are intersected.
        
        Note that this may not return the closest Entity if another's bounds
        project in front of it.

        \param ray World space ray

        \param distance Maximum distance at which to allow selection
        (e.g., finf()).  On return, this is the distance to the
        object.

        \param exclude Entities to ignore when searching for occlusions.  This is
        convenient to use when avoiding self-collisions, for example.

        \param intersectMarkers If true, allow MarkerEntity instances
        to be intersected.  Default is false.
     */  
    virtual shared_ptr<Entity> intersectBounds(const Ray& ray, float& distance = ignoreFloat, bool intersectMarkers = false, const Array<shared_ptr<Entity> >& exclude = Array<shared_ptr<Entity> >()) const;

    /** Performs very precise (usually, ray-triangle) intersection, and is much slower
        than intersectBounds.  

        \param intersectMarkers If true, allow MarkerEntity instances
        to be intersected.  Default is false.

		\param ray The Ray in world space.
    */
    virtual shared_ptr<Entity> intersect(const Ray& ray, float& distance = ignoreFloat, bool intersectMarkers = false, const Array<shared_ptr<Entity> >& exclude = Array<shared_ptr<Entity> >(), Model::HitInfo& info = Model::HitInfo::ignore) const;

    /**
     Helper for calling intersect() with an eye ray.  
     \param pixel The pixel centers are at (0.5, 0.5).  Pixel is taken relative to viewport before the guard band was applied.
     */
    Ray eyeRay(const shared_ptr<Camera>& camera, const Vector2& pixel, const Rect2D& viewport, const Vector2int16 guardBandThickness) const;

    /** Wall-clock time at which the scene contents last changed, e.g., by
        reloading, adding or removing Entitys, etc.  Moving objects do
        not count as structural changes.  This is used by the GUI for
        tracking when to update the SceneEditorWindow, for example. */
    RealTime lastStructuralChangeTime() const {
        return m_lastStructuralChangeTime;
    }

    /** Wall-clock time at which a VisibleEntity in scene last
        changed at all, e.g., by moving. */
    RealTime lastVisibleChangeTime() const {
        return m_lastVisibleChangeTime;
    }

    /** Does not track changes to environment lighting */
    RealTime lastLightChangeTime() const {
        return m_lastLightChangeTime;
    }

    /**     
      Places a constraint on the Scene that when iterating for
      simulation, posing, etc., \a entity1 always be processed 
      before \a entity2 (although not necessarily immediately before).
      It is an error to create a cycle between multiple entities.
      
      This method takes names instead of pointers so that it can be invoked
      before both entities exist, e.g., during entity creation. If one of 
      the entities is later removed, this constraint is removed.  
      
      Useful when \a entity2 is a child of \a entity1 and defines itself relative, e.g.,
      \a entity1 is a jeep and \a entity2 is a character driving the jeep.

      Do not invoke this method unless you actually require an ordering constraint
      because it causes extra work for the iteration system.
    */
    void setOrder(const String& entity1Name, const String& entity2Name);

    /** 
       Removes an existing constraint that \a entity1 simulate before \a entity2.
       It is an error to remove a constraint that does not exist
      */
    void clearOrder(const String& entity1Name, const String& entity2Name);


    /** Draws debugging information about the current scene to the render device. */
    void visualize(RenderDevice* rd, const shared_ptr<Entity>& selectedEntity,
        const Array<shared_ptr<Surface> >& allSurfaces, const SceneVisualizationSettings& v, const shared_ptr<Camera>& camera);
};

} // namespace G3D

#endif
