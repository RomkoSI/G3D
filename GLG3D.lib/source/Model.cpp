#include "GLG3D/Model.h"

namespace G3D {

Model::HitInfo Model::HitInfo::ignore;

Model::HitInfo::HitInfo() : point(Vector3::nan()),
                            normal(Vector3::nan()),
                            entity(shared_ptr<Entity>()),
                            model(shared_ptr<Model>()),
                            meshName(""),
                            meshID(0),
                            primitiveIndex(0),
                            u(0),
                            v(0) {
}


void Model::HitInfo::clear() {
    const_cast<shared_ptr<Entity>&>(this->entity).reset();
    const_cast<shared_ptr<Model> &>(this->model).reset();
    const_cast<shared_ptr<Material> &>(this->material).reset();
    *const_cast<String *>(&this->meshName)    = "";
    *const_cast<int *>(&this->meshID)              = 0;
    *const_cast<int *>(&this->primitiveIndex)      = 0; 
    *const_cast<float *>(&this->u)                 = 0; 
    *const_cast<float *>(&this->v)                 = 0;
    *const_cast<Point3 *>(&this->point)            = Vector3::nan();
    *const_cast<Vector3 *>(&this->normal)          = Vector3::nan();
}

void Model::HitInfo::set( 
    const shared_ptr<Model>&    model,
    const shared_ptr<Entity>&   entity,
    const shared_ptr<Material>& material,
    const Vector3&              normal,
    const Point3&               point,
    const String&          meshName,
    int                         meshID,
    int                         primIndex,
    float                       u,
    float                       v ) {

    *const_cast<shared_ptr<Entity> *>(&this->entity)        = entity;
    *const_cast<shared_ptr<Model> *>(&this->model)          = model;
    *const_cast<shared_ptr<Material> *>(&this->material)    = material;
    *const_cast<String *>(&this->meshName)             = meshName;
    *const_cast<int *>(&this->meshID)                       = meshID;
    *const_cast<int *>(&this->primitiveIndex)               = primIndex; 
    *const_cast<float *>(&this->u)                          = u; 
    *const_cast<float *>(&this->v)                          = v;
    *const_cast<Point3 *>(&this->point)                     = point;
    *const_cast<Vector3 *>(&this->normal)                   = normal;

}


} // G3D
