#ifndef G3D_Model_h
#define G3D_Model_h

#include "G3D/platform.h"
#include "G3D/Table.h"
#include "G3D/lazy_ptr.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Surface.h"
#include "GLG3D/Material.h"

namespace G3D {

class Entity;

/** Common base class for models */
class Model : public ReferenceCountedObject {
public:

    virtual const String& name() const = 0;

    virtual const String& className() const = 0;

    /** \sa Scene::intersect, Entity::intersect, ArticulatedModel::intersect, Tri::Intersector.
         All fields are const so as to require the use of the set method to set the value of the field
         while still keeping all fields public. */
    class HitInfo {
    public:

        /** In world space.  Point3::nan() if no object was hit. */
        const Point3                   point;

        /** In world space */
        const Vector3                  normal;

        const shared_ptr<Entity>       entity;

        const shared_ptr<Model>        model;

        const shared_ptr<Material>     material;

        /** If the model contains multiple meshes (e.g.,
            ArticulatedModel), this is an identifier for the
            underlying mesh or other surface in which primitiveIndex
            should be referenced.*/
        const String                   meshName;
        const int                      meshID;

        /** If the model has multiple primitives, this is the index of the one hit */
        const int                      primitiveIndex;

        /** Barycentric coords within the primitive hit if it is a triangle.*/
        const float                    u;
        const float                    v;
        
        static HitInfo                 ignore;

        HitInfo();

        void clear(); 

        void set( 
            const shared_ptr<Model>&    model,
            const shared_ptr<Entity>&   entity       = shared_ptr<Entity>(),
            const shared_ptr<Material>& material     = shared_ptr<Material>(),
            const Vector3&              normal       = Vector3::nan(),
            const Point3&               point        = Point3::nan(),
            const String&               meshName     = "",
            int                         meshID       = 0,
            int                         primIndex    = 0,
            float                       u            = 0,
            float                       v            = 0);

    };
    
    class Pose {
    public:

        Surface::ExpressiveLightScatteringProperties expressiveLightScatteringProperties;

        Pose() {}
        virtual ~Pose() {}
    };

};


//typedef Table< String, lazy_ptr<Model> >  ModelTable;
typedef Table< String, shared_ptr<Model> >  ModelTable;

} // namespace G3D

#endif
