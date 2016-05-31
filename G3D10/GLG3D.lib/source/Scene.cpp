/**
  \file GLG3D.lib/source/Scene.cpp

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2010-01-01
  \edited  2016-02-10
*/

#include "GLG3D/Scene.h"
#include "G3D/units.h"
#include "G3D/Table.h"
#include "G3D/FileSystem.h"
#include "G3D/Log.h"
#include "G3D/Ray.h"
#include "GLG3D/ArticulatedModel.h"
#include "GLG3D/VisibleEntity.h"
#include "GLG3D/ParticleSystem.h"
#include "GLG3D/Light.h"
#include "GLG3D/Camera.h"
#include "GLG3D/HeightfieldModel.h"
#include "GLG3D/MarkerEntity.h"
#include "GLG3D/Skybox.h"
#include "GLG3D/ParticleSystemModel.h"
#include "GLG3D/GFont.h"

using namespace G3D::units;

namespace G3D {

void Scene::onSimulation(SimTime deltaTime) {
    sortEntitiesByDependency();
    m_time += isNaN(deltaTime) ? 0 : deltaTime;
    for (int i = 0; i < m_entityArray.size(); ++i) {
        const shared_ptr<Entity> entity = m_entityArray[i];
        
        entity->onSimulation(m_time, deltaTime);

        if (dynamic_pointer_cast<Light>(entity)) {
            m_lastLightChangeTime = max(m_lastLightChangeTime, entity->lastChangeTime());
        } else if (dynamic_pointer_cast<VisibleEntity>(entity)) {
            m_lastVisibleChangeTime = max(m_lastVisibleChangeTime, entity->lastChangeTime());
        }
        // Intentionally ignoring the case of other Entity subclasses
    }

    if (m_editing) {
        m_lastEditingTime = System::time();
    }

}


void Scene::registerEntitySubclass(const String& name, EntityFactory factory, bool errorIfAlreadyRegistered) {
    alwaysAssertM(! m_entityFactory.containsKey(name) || ! errorIfAlreadyRegistered, name + " has already been registered as an entity subclass.");
    m_entityFactory.set(name, factory);
}


static Array<String> searchPaths;

void Scene::setSceneSearchPaths(const Array<String>& paths) {
    searchPaths.copyFrom(paths);
}


static void setDefaultSearchPaths() {
    // Add the current directory
    searchPaths.append(".");  

    // Add the directories specified by the environment variable G3D10DATA
    std::vector<String> g3dDataPaths;
    System::getG3DDataPaths(g3dDataPaths);
    for(size_t i = 0; i < g3dDataPaths.size(); i++) {
        searchPaths.append(g3dDataPaths[i]);
    }

    // If not already a subdirectoy of a search path, add the G3D scenes
    // directory, which is detected by the CornellBox-Glossy.scn.any file
    if (! _internal::g3dInitializationSpecification().deployMode) {
        const String& s = System::findDataFile("scene/CornellBox-glossy.Scene.Any", false);
        if (! s.empty()) {
            bool subDirectory = false;
            for (int i = 0; i < searchPaths.size(); ++i) {
                if (s.find(searchPaths[i]) != String::npos) {
                   subDirectory = true;
                }
            }
            if (!subDirectory) {
               searchPaths.append(FilePath::parent(s));
            }
        }
    }
}


/** Returns a table mapping scene names to filenames */
static Table<String, String>& filenameTable() {
    static Table<String, String> filenameTable;

    if (filenameTable.size() == 0) {
        if (searchPaths.size() == 0) {
            setDefaultSearchPaths();
        }

        // Create a table mapping scene names to filenames
        Array<String> filenameArray;

        FileSystem::ListSettings settings;
        settings.files = true;
        settings.directories = false;
        settings.includeParentPath = true;
        settings.recursive = true;

        for (int i = 0; i < searchPaths.size(); ++i) {
            FileSystem::list(FilePath::concat(searchPaths[i], "*.scn.any"), filenameArray, settings);
            FileSystem::list(FilePath::concat(searchPaths[i], "*.Scene.Any"), filenameArray, settings);
        }

        logLazyPrintf("Found scenes:\n");
        for (int i = 0; i < filenameArray.size(); ++i) {
            if (filenameArray[i].find('$') != String::npos) {
                logPrintf("Scene::filenameTable() skipped \"%s\" because it contained '$' which looked like an environment variable.\n",
                          filenameArray[i].c_str());
                continue;
            }
            Any a;
            String msg;
            try {
                a.load(filenameArray[i]);
                
                const String& name = a["name"].string();
                if (filenameTable.containsKey(name)) {
                    debugPrintf("%s", ("Warning: Duplicate scene names in " + filenameTable[name] +  " and " + filenameArray[i] +". The second was ignored.\n").c_str());
                } else {
                    msg = format("  \"%s\" (%s)\n", name.c_str(), filenameArray[i].c_str());
                    filenameTable.set(name, filenameArray[i]);
                }
            } catch (const ParseError& e) {
                msg = format("  <Parse error at %s:%d(%d): %s>\n", e.filename.c_str(), e.line, e.character, e.message.c_str());
            } catch (...) {
                msg = format("  <Error while loading %s>\n", filenameArray[i].c_str());
            }

            if (! msg.empty()) {
                logLazyPrintf("%s", msg.c_str());
                debugPrintf("%s", msg.c_str());
            }
        }
        logPrintf("");
    }

    return filenameTable;
}


Array<String> Scene::sceneNames() {
    Array<String> a;
    filenameTable().getKeys(a);
    a.sort(alphabeticalIgnoringCaseG3DLastLessThan);
    return a;
}


Scene::Scene(const shared_ptr<AmbientOcclusion>& ambientOcclusion) :
    m_time(0),
    m_lastStructuralChangeTime(0),
    m_lastVisibleChangeTime(0),
    m_lastLightChangeTime(0),
    m_editing(false),
    m_lastEditingTime(0),
    m_needEntitySort(false) {

    m_localLightingEnvironment.ambientOcclusion = ambientOcclusion;
    registerEntitySubclass("VisibleEntity",  &VisibleEntity::create);
    registerEntitySubclass("ParticleSystem", &ParticleSystem::create);
    registerEntitySubclass("Light",          &Light::create);
    registerEntitySubclass("Camera",         &Camera::create);
    registerEntitySubclass("MarkerEntity",   &MarkerEntity::create);
    registerEntitySubclass("Skybox",         &Skybox::create);
}


void Scene::clear() {
    shared_ptr<AmbientOcclusion> old = m_localLightingEnvironment.ambientOcclusion;

    // Entitys, cameras, lights, all settings back to intial defauls
    m_dependencyTable.clear();
    m_needEntitySort = false;
    m_entityTable.clear();
    m_entityArray.fastClear();
    m_cameraArray.fastClear();
    m_localLightingEnvironment = LightingEnvironment();
    m_localLightingEnvironment.ambientOcclusion = old;
    m_skybox.reset();
    m_time = 0;
    m_sourceAny = NULL;
    m_lastVisibleChangeTime = m_lastLightChangeTime = m_lastStructuralChangeTime = System::time();
}


shared_ptr<Scene> Scene::create(const shared_ptr<AmbientOcclusion>& ambientOcclusion) {
    return shared_ptr<Scene>(new Scene(ambientOcclusion));
}


void Scene::setEditing(bool b) {
    m_editing = b;
    m_lastEditingTime = System::time();
}


const shared_ptr<Camera> Scene::defaultCamera() const {
    shared_ptr<Camera> c = typedEntity<Camera>(m_defaultCameraName);
    if (isNull(c)) {
        c = m_cameraArray[0];
    }
    return c;
}


void Scene::setTime(const SimTime t) {
    m_time = t;
    // Called twice to wipe out the first-order time derivative

    onSimulation(fnan());
    onSimulation(fnan());
}


String Scene::sceneNameToFilename(const String& scene)  {
    const bool isFilename = endsWith(toLower(scene), ".scn.any") || endsWith(toLower(scene), ".scene.any");

    if (isFilename) {
        return scene;
    } else {
        const String* f = filenameTable().getPointer(scene);
        if (f == NULL) {
            throw "No scene with name '" + scene + "' found in (" +
                stringJoin(filenameTable().getKeys(), ", ") + ")";
        }

        return *f;
    }
}


Any Scene::load(const String& scene, const LoadOptions& loadOptions) {
    shared_ptr<AmbientOcclusion> old = m_localLightingEnvironment.ambientOcclusion;
    const String& filename = sceneNameToFilename(scene);
    
    clear();
    m_modelTable.clear();
    m_name = scene;

    Any any;
    any.load(filename);

    {
        const String& n = any.get("name", filename);

        // Ensure that this name appears in the filename table if it does not already,
        // so that it can be loaded by name in the future.
        if (! filenameTable().containsKey(n)) {
            filenameTable().set(n, filename);
        }
    }

    m_sourceAny = any;

    // Load the lighting environment (do this before loading entities, since some of them may
    // be lights that will enter this array)
    bool hasEnvironmentMap = false;
    if (any.containsKey("lightingEnvironment")) {
        m_localLightingEnvironment = any["lightingEnvironment"];
        hasEnvironmentMap = any["lightingEnvironment"].containsKey("environmentMap");
    }
        
    const String modelSectionName[]  = {"models", "models2"};
    const String entitySectionName[] = {"entities", "entities2"};

    m_modelsAny = Any(Any::TABLE);
    // Load the models
    for (int i = 0; i < 2; ++i) {
        if (any.containsKey(modelSectionName[i])) {
            Any models = any[modelSectionName[i]];
            if (models.size() > 0) {
                for (Any::AnyTable::Iterator it = models.table().begin(); it.isValid(); ++it) {
                    const String name = it->key;
                    const Any& v = it->value;
                    createModel(v, name);
                }
            }
        }
    }

    // Instantiate the entities
    // Try for both the current and extended format entity group names...intended to support using #include to merge
    // different files with entitys in them
    for (int i = 0; i < 2; ++i) {
        if (any.containsKey(entitySectionName[i])) { 
            const Any& entities = any[entitySectionName[i]];
            if (entities.size() > 0) {
                for (Table<String, Any>::Iterator it = entities.table().begin(); it.isValid(); ++it) {
                    const String& name = it->key;
                    createEntity(name, it->value, loadOptions);
                } // for each entity
            } // if there is anything in this table
        } // if this entity group name exists
    } 

    
    shared_ptr<Texture> skyboxTexture = Texture::whiteCube();

    Any skyAny;

    // Use the environment map as a skybox if there isn't one already, and vice versa
    Array<shared_ptr<Skybox> > skyboxes;
    getTypedEntityArray<Skybox>(skyboxes);
    if (skyboxes.size() == 0) {
        if (any.containsKey("skybox")) {
            // Legacy path
            debugPrintf("Warning: Use the Skybox entity now instead of a top-level skybox field in a Scene.Any file\n");
            createEntity("Skybox", "skybox", any["skybox"]);
            m_skybox = typedEntity<Skybox>("skybox");
        } else if (hasEnvironmentMap) {
            // Create the skybox from the environment map
            m_skybox = Skybox::create("skybox", this, Array<shared_ptr<Texture> >(m_localLightingEnvironment.environmentMapArray[0]), Array<SimTime>(0.0), 0, SplineExtrapolationMode::CLAMP, false, false);
            // GCC misunderstands overloading, necessitating this cast
            // DO NOT CHANGE without being aware of that
            insert(dynamic_pointer_cast<Entity>(m_skybox));
        } else {
            m_skybox = Skybox::create("skybox", this, Array<shared_ptr<Texture> >(Texture::whiteCube()), Array<SimTime>(0.0), 0, SplineExtrapolationMode::CLAMP, false, false);
            // GCC misunderstands overloading, necessitating this cast
            // DO NOT CHANGE without being aware of that
            insert(dynamic_pointer_cast<Entity>(m_skybox));
        }
    }

    if (any.containsKey("environmentMap")) {
        throw String("environmentMap field has been replaced with lightingEnvironment");
    }


    // Default to using the skybox as an environment map if none is specified.
    if (! hasEnvironmentMap) {
        if (m_skybox->keyframeArray()[0]->dimension() == Texture::DIM_CUBE_MAP) {
            m_localLightingEnvironment.environmentMapArray.append(m_skybox->keyframeArray()[0]);
        } else {
            m_localLightingEnvironment.environmentMapArray.append(Texture::whiteCube());
        }
    }
    any.verify(m_localLightingEnvironment.environmentMapArray[0]->dimension() == Texture::DIM_CUBE_MAP);


    //////////////////////////////////////////////////////

    if (m_cameraArray.size() == 0) {
        // Create a default camera, back it up from the origin
        m_cameraArray.append(Camera::create("camera"));
        m_cameraArray.last()->setFrame(CFrame::fromXYZYPRDegrees(0,1,-5,0,-5));
    }

    setTime(any.get("time", 0.0));
    m_lastVisibleChangeTime = m_lastLightChangeTime = m_lastStructuralChangeTime = System::time();

    m_defaultCameraName = (String)any.get("defaultCamera", "");

    m_localLightingEnvironment.ambientOcclusion = old;

    // Set the initial positions, repeating a few times to allow
    // objects defined relative to others to reach a fixed point
    for (int i = 0; i < 3; ++i) {
        for (int e = 0; e < m_entityArray.size(); ++e) {
            m_entityArray[e]->onSimulation(m_time, fnan());
        }
    }

    // Pose objects so that they have bounds.
    {
        Array< shared_ptr<Surface> > ignore;
        onPose(ignore);
    }

    return any;
}


lazy_ptr<Model> Scene::createModel(const Any& v, const String& name) {
    v.verify(! m_modelTable.containsKey(name), "A model named '" + name + "' already exists in this scene.");

    lazy_ptr<Model> m;

    if ((v.type() == Any::STRING) || v.nameBeginsWith("ArticulatedModel")) {
        m = ArticulatedModel::lazyCreate(v, name);
    } else if (v.nameBeginsWith("MD2Model")) {
        m = MD2Model::lazyCreate(v, name);
    } else if (v.nameBeginsWith("MD3Model")) {
        m = MD3Model::lazyCreate(v, name);
    } else if (v.nameBeginsWith("HeightfieldModel")) {
        m = HeightfieldModel::lazyCreate(v, name);
    } else if (v.nameBeginsWith("ParticleSystemModel")) {
        m = ParticleSystemModel::lazyCreate(v, name);
    }

    if (isNull(m)) {
        v.verify(false, "Unrecognized model type: " + v.name());
    }

    m_modelTable.set(name, m);
    m_modelsAny[name] = v;
    return m;
}


void Scene::getEntityNames(Array<String>& names) const {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        names.append(m_entityArray[e]->name());
    }
}


void Scene::getCameraNames(Array<String>& names) const {
    for (int e = 0; e < m_cameraArray.size(); ++e) {
        names.append(m_cameraArray[e]->name());
    }
}


const shared_ptr<Entity> Scene::entity(const String& name) const {
    shared_ptr<Entity>* e = m_entityTable.getPointer(name);
    if (notNull(e)) {
        return *e;
    } else {
        return shared_ptr<Entity>();
    }
}


shared_ptr<Model> Scene::insert(const shared_ptr<Model>& model) {
    debugAssert(notNull(model));
    debugAssert(! m_modelTable.containsKey(model->name()));
//    m_modelTable.set(model->name(), lazy_ptr<Model>(model));
    m_modelTable.set(model->name(), model);
    return model;
}


void Scene::removeModel(const String& modelName) {
    debugAssert(m_modelTable.containsKey(modelName));
    m_modelTable.remove(modelName);
}


void Scene::remove(const shared_ptr<Model>& model) {
    debugAssert(notNull(model));
    removeModel(model->name());
}


void Scene::removeEntity(const String& entityName) {
    remove(entity(entityName));
}


void Scene::remove(const shared_ptr<Entity>& entity) {
    debugAssert(notNull(entity));
    m_entityTable.remove(entity->name());
    m_entityArray.remove(m_entityArray.findIndex(entity));


    const shared_ptr<VisibleEntity>& visible = dynamic_pointer_cast<VisibleEntity>(entity);
    if (notNull(visible)) {
        m_lastVisibleChangeTime = System::time();
    }

    const shared_ptr<Camera>& camera = dynamic_pointer_cast<Camera>(entity);
    if (notNull(camera)) {
        m_cameraArray.append(camera);
        m_cameraArray.remove(m_cameraArray.findIndex(camera));
    }

    const shared_ptr<Light>& light = dynamic_pointer_cast<Light>(entity);
    if (notNull(light)) {
        m_localLightingEnvironment.lightArray.remove(m_localLightingEnvironment.lightArray.findIndex(light));
        m_lastLightChangeTime = System::time();
    }
}


shared_ptr<Entity> Scene::insert(const shared_ptr<Entity>& entity) {
    debugAssert(notNull(entity));

    debugAssertM(! m_entityTable.containsKey(entity->name()), "Two Entitys with the same name, \"" + entity->name() + "\"");
    m_entityTable.set(entity->name(), entity);
    m_entityArray.append(entity);
    m_lastStructuralChangeTime = System::time();
    
    const shared_ptr<VisibleEntity>& visible = dynamic_pointer_cast<VisibleEntity>(entity);
    if (notNull(visible)) {
        m_lastVisibleChangeTime = System::time();
    }

    const shared_ptr<Camera>& camera = dynamic_pointer_cast<Camera>(entity);
    if (notNull(camera)) {
        m_cameraArray.append(camera);
    }

    const shared_ptr<Light>& light = dynamic_pointer_cast<Light>(entity);
    if (notNull(light)) {
        m_localLightingEnvironment.lightArray.append(light);
        m_lastLightChangeTime = System::time();
    }

    const shared_ptr<Skybox>& skybox = dynamic_pointer_cast<Skybox>(entity);
    if (notNull(skybox)) {
        m_skybox = skybox;
    }

    // Simulate and pose the entity so that it has bounds
    entity->onSimulation(m_time, 0);
    Array< shared_ptr<Surface> > ignore;
    entity->onPose(ignore);

    return entity;
}


shared_ptr<Entity> Scene::createEntity(const String& name, const Any& any, const Scene::LoadOptions& options) {
    return createEntity(any.name(), name, any, options);
}


shared_ptr<Entity> Scene::createEntity(const String& entityType, const String& name, const Any& any, const Scene::LoadOptions& options) {
    shared_ptr<Entity> entity;
    AnyTableReader propertyTable(any);

    const EntityFactory* factory = m_entityFactory.getPointer(entityType);
    if (notNull(factory)) {
        entity = (**factory)(name, this, propertyTable, m_modelTable, options);
        if (notNull(entity)) {
            insert(entity);
        }            
    } else {
        any.verify(false, String("Unrecognized Entity type: \"") + entityType + "\"");
    }
        
    return entity;
}


void Scene::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        m_entityArray[e]->onPose(surfaceArray);
    }
}


shared_ptr<Entity> Scene::intersectBounds(const Ray& ray, float& distance, bool intersectMarkers, const Array<shared_ptr<Entity> >& exclude) const {
    shared_ptr<Entity> closest;
    
    for (int e = 0; e < m_entityArray.size(); ++e) {
        const shared_ptr<Entity>& entity = m_entityArray[e];
        if ((intersectMarkers || isNull(dynamic_pointer_cast<MarkerEntity>(entity))) &&
            ! exclude.contains(entity) && 
            entity->intersectBounds(ray, distance)) {
            closest = entity;
        }
    }

    return closest;
}


shared_ptr<Entity> Scene::intersect(const Ray& ray, float& distance, bool intersectMarkers, const Array<shared_ptr<Entity> >& exclude, Model::HitInfo& info) const {
    shared_ptr<Entity> closest;
    
    for (int e = 0; e < m_entityArray.size(); ++e) {
        const shared_ptr<Entity>& entity = m_entityArray[e];
        if ((intersectMarkers || isNull(dynamic_pointer_cast<MarkerEntity>(entity))) &&
            ! exclude.contains(entity) &&
            entity->intersect(ray, distance, info)) {
            closest = entity;
        }
    }

    return closest;
}


Any Scene::toAny(const bool forceAll) const {
    Any a = m_sourceAny;

    // Overwrite the entity table
    Any entityTable(Any::TABLE);
    for (int i = 0; i < m_entityArray.size(); ++i) {
        const shared_ptr<Entity>& entity = m_entityArray[i];
        if (entity->shouldBeSaved()) {
            entityTable[entity->name()] = entity->toAny(forceAll);
        }
        //debugPrintf("Saving %s\n",  entity->name().c_str());
    }

    a["entities"] = entityTable;
    a["lightingEnvironment"] = m_localLightingEnvironment;
    a["models"] = m_modelsAny;
    
    return a;
}

#define DEBUG_SORT 0
void Scene::sortEntitiesByDependency() {
    if (! m_needEntitySort) { return; }

    if (m_dependencyTable.size() > 0) { 

        Table<Entity*, VisitorState> visitorStateTable;

        // Push all entities onto a stack. Fill the stack backwards, 
        // so that we don't change the order of
        // unconstrained objects.
        Array< shared_ptr<Entity> > stack;
        stack.reserve(m_entityArray.size());
        for (int e = m_entityArray.size() - 1; e >= 0; --e) {
            const shared_ptr<Entity>& entity = m_entityArray[e];
            stack.push(entity);
            visitorStateTable.set(entity.get(), NOT_VISITED);
        }
        
        m_entityArray.fastClear();

        // For each element of the stack that has not been visited, push all
        // of its dependencies on top of it.
        while (stack.size() > 0) {
#           if DEBUG_SORT
                debugPrintf("______________________________________________\nstack = ["); 
                for (int i = 0; i < stack.size(); ++i) { debugPrintf("\"%s\", ", stack[i]->name().c_str()); }; 
                debugPrintf("]\nm_entityArray = ["); 
                for (int i = 0; i < m_entityArray.size(); ++i) { debugPrintf("\"%s\", ", m_entityArray[i]->name().c_str()); };  
                debugPrintf("]\n\n");
#           endif
            const shared_ptr<Entity>& entity = stack.pop();
            VisitorState& state = visitorStateTable[entity.get()];

#           if DEBUG_SORT
                debugPrintf("entity = \"%s\", state = %s\n", entity->name().c_str(),
                    (state == NOT_VISITED) ? "NOT_VISITED" :
                    (state == VISITING) ? "VISITING" :
                    "ALREADY_VISITED");
#           endif
            switch (state) {
            case NOT_VISITED:
                {
                    state = VISITING;

                    // See if this node has any dependencies
                    const DependencyList* dependencies = m_dependencyTable.getPointer(entity->name());
                    if (notNull(dependencies)) {

                        // Push this node back on the stack
                        stack.push(entity);
                        debugAssertM(dependencies->size() > 0, "Empty dependency list stored");

                        // Process each dependency
                        for (int d = 0; d < dependencies->size(); ++d) {
                            const String& parentName = (*dependencies)[d];
                            shared_ptr<Entity> parent = this->entity(parentName);
                            debugAssertM(notNull(parent), entity->name() + " depends on " + parentName + ", which does not exist.");
                        
                            VisitorState& parentState = visitorStateTable[parent.get()];

                            debugAssertM(parentState != VISITING,
                                String("Dependency cycle detected containing ") + entity->name() + " and " + parentName);

                            if (parentState == NOT_VISITED) {
                                // Push the dependency on the stack so that it will be processed ahead of entity
                                // that depends on it.  The parent may already be in the stack
                                stack.push(parent);
                            } else {
                                // Do nothing, this parent was already processed and is in the
                                // entity array ahead of the child.
                                debugAssert(parentState == ALREADY_VISITED);
                            }
                        }

                    } else {
                        // There are no dependencies
                        state = ALREADY_VISITED;
                        m_entityArray.push(entity);
                    }
                } break;

            case VISITING:
                // We've come back to this entity after processing its dependencies, and are now ready to retire it
                state = ALREADY_VISITED;
                m_entityArray.push(entity);
                break;

            case ALREADY_VISITED:;
                // Ignore this entity because it was already processed            
            } // switch
        } // while
    } // if there are dependencies

    m_needEntitySort = false;
}


void Scene::setOrder(const String& entity1Name, const String& entity2Name) {
    debugAssert(entity1Name != entity2Name);
    bool ignore;
    DependencyList& list = m_dependencyTable.getCreate(entity2Name, ignore);

    debugAssertM(! m_dependencyTable.containsKey(entity1Name) || ! m_dependencyTable[entity1Name].contains(entity2Name), 
        "Tried to specify a cyclic dependency between " + entity1Name + " and " + entity2Name);
    debugAssertM(! list.contains(entity1Name), "Duplicate dependency specified");
    list.append(entity1Name);
    m_needEntitySort = true;
}


void Scene::clearOrder(const String& entity1Name, const String& entity2Name) {
    debugAssert(entity1Name != entity2Name);
    bool created = false;
    DependencyList& list = m_dependencyTable.getCreate(entity2Name, created);
    debugAssertM(! created, "Tried to remove a dependency that did not exist");

    int i = list.findIndex(entity1Name);
    debugAssertM(i != -1, "Tried to remove a dependency that did not exist");
    list.fastRemove(i);

    if (list.size() == 0) {
        // The list is empty, don't store it any more
        m_dependencyTable.remove(entity2Name);
    }
    m_needEntitySort = true;
}


Ray Scene::eyeRay(const shared_ptr<Camera>& camera, const Vector2& pixel, const Rect2D& viewport, const Vector2int16 guardBandThickness) const {
    return camera->worldRay(pixel.x + guardBandThickness.x, pixel.y + guardBandThickness.y, Rect2D(Vector2(viewport.width() + 2 * guardBandThickness.x, viewport.height() + 2 * guardBandThickness.y)));
}


void Scene::visualize(RenderDevice* rd, const shared_ptr<Entity>& selectedEntity, const Array<shared_ptr<Surface> >& allSurfaces, const SceneVisualizationSettings& settings, const shared_ptr<Camera>& camera) {
    if (settings.showWireframe) {
        Surface::renderWireframe(rd, allSurfaces);
    }
    
    if (isNull(m_font)) {
        m_font = GFont::fromFile(System::findDataFile(System::findDataFile("arial.fnt")));
    }
    // Visualize markers and other features
    for (int i = 0; i < m_entityArray.size(); ++i) {
        m_entityArray[i]->visualize(rd, m_entityArray[i] == selectedEntity, settings, m_font, camera);
    }
}

} // namespace G3D
