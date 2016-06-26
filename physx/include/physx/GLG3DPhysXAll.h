#pragma once
#include <G3D/G3DAll.h>

#include <physx/PxPhysicsAPI.h>
#include <physx/extensions/PxDefaultSimulationFilterShader.h>


#ifdef G3D_DEBUG

#   pragma comment(lib, "LowLevelDEBUG.lib")
#   pragma comment(lib, "LowLevelClothDEBUG.lib")
#   pragma comment(lib, "PhysX3CharacterKinematicDEBUG_x64.lib")
#   pragma comment(lib, "PhysX3DEBUG_x64.lib")
#   pragma comment(lib, "PhysX3CommonDEBUG_x64.lib")
#   pragma comment(lib, "PhysX3CookingDEBUG_x64.lib")
#   pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")
#   pragma comment(lib, "PhysX3VehicleDEBUG.lib")
#   pragma comment(lib, "PhysXProfileSDKDEBUG.lib")
#   pragma comment(lib, "PhysXVisualDebuggerSDKDEBUG.lib")
#   pragma comment(lib, "PvdRuntimeDEBUG.lib")
#   pragma comment(lib, "PxTaskDEBUG.lib")
#   pragma comment(lib, "SceneQueryDEBUG.lib")
#   pragma comment(lib, "SimulationControllerDEBUG.lib")

#else

#define PHYSX_CHECKED

#ifdef PHYSX_CHECKED

#   pragma comment(lib, "LowLevelCHECKED.lib")
#   pragma comment(lib, "LowLevelClothCHECKED.lib")
#   pragma comment(lib, "PhysX3CharacterKinematicCHECKED_x64.lib")
#   pragma comment(lib, "PhysX3CHECKED_x64.lib")
#   pragma comment(lib, "PhysX3CommonCHECKED_x64.lib")
#   pragma comment(lib, "PhysX3CookingCHECKED_x64.lib")
#   pragma comment(lib, "PhysX3ExtensionsCHECKED.lib")
#   pragma comment(lib, "PhysX3VehicleCHECKED.lib")
#   pragma comment(lib, "PhysXProfileSDKCHECKED.lib")
#   pragma comment(lib, "PhysXVisualDebuggerSDKCHECKED.lib")
#   pragma comment(lib, "PvdRuntimeCHECKED.lib")
#   pragma comment(lib, "PxTaskCHECKED.lib")
#   pragma comment(lib, "SceneQueryCHECKED.lib")
#   pragma comment(lib, "SimulationControllerCHECKED.lib")

#else

#   pragma comment(lib, "LowLevel.lib")
#   pragma comment(lib, "LowLevelCloth.lib")
#   pragma comment(lib, "PhysX3CharacterKinematic_x64.lib")
#   pragma comment(lib, "PhysX3_x64.lib")
#   pragma comment(lib, "PhysX3Common_x64.lib")
#   pragma comment(lib, "PhysX3Cooking_x64.lib")
#   pragma comment(lib, "PhysX3Extensions.lib")
#   pragma comment(lib, "PhysX3Vehicle.lib")
#   pragma comment(lib, "PhysXProfileSDK.lib")
#   pragma comment(lib, "PhysXVisualDebuggerSDK.lib")
#   pragma comment(lib, "PvdRuntime.lib")
#   pragma comment(lib, "PxTask.lib")
#   pragma comment(lib, "SceneQuery.lib")
#   pragma comment(lib, "SimulationController.lib")

#endif

#endif

using namespace physx;

inline Vector3 toVector3(const PxVec3& v) {
    return Vector3(v.x, v.y, v.z);
}

inline Quat toQuat(const PxQuat& q) {
    return Quat(q.x, q.y, q.z, q.w);
}

inline CFrame toCFrame(const PxTransform& t) {
    CFrame frame;
    frame.translation = toVector3(t.p);
    frame.rotation = Matrix3(toQuat(t.q));
    return frame;
}

inline PhysicsFrame toPhysicsFrame(const PxTransform& t) {
    PhysicsFrame frame;
    frame.translation = toVector3(t.p);
    frame.rotation = toQuat(t.q);
    return frame;
}


inline PxVec3 toPxVec3(const Vector3& v) {
    return PxVec3(v.x, v.y, v.z);
}

inline PxQuat toPxQuat(const Quat& q) {
    return PxQuat(q.x, q.y, q.z, q.w);
}

inline PxQuat toPxQuat(const Matrix3& r) {
    Quat q(r);
    return PxQuat(q.x, q.y, q.z, q.w);
}

inline PxTransform toPxTransform(const CFrame& t) {
    return PxTransform(toPxVec3(t.translation), toPxQuat(t.rotation));
}

inline PxTransform toPxTransform(const PhysicsFrame& t) {
    return PxTransform(toPxVec3(t.translation), toPxQuat(t.rotation));
}
