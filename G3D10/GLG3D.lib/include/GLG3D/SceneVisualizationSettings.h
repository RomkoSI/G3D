#ifndef SceneVisualizationSettings_h
#define SceneVisualizationSettings_h

namespace G3D {

/** Visualization and debugging render options for Surface. Set by the SceneEditorWindow gui. */
class SceneVisualizationSettings {
public:
    /** Indicates MarkerEntity should be rendered visibly */
    bool            showMarkers;
    bool            showEntityBoxBounds;
    bool            showEntityBoxBoundArray;
    bool            showEntitySphereBounds;
    bool            showWireframe;
    bool            showEntityNames;

    SceneVisualizationSettings() : showMarkers(false), showEntityBoxBounds(false), showEntityBoxBoundArray(false), showEntitySphereBounds(false), showWireframe(false), showEntityNames(false) {}
};

}
#endif
