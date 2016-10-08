/**
  @file World.h
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef World_h
#define World_h

#include <G3D/G3DAll.h>

/** \brief The scene.*/
class World {
private:

    Array<shared_ptr<Surface> >     m_surfaceArray;
    TriTree                         m_triTree;
    CPUVertexArray                  m_cpuVertexArray;
    shared_ptr<CubeMap>             m_skybox;
    enum Mode {TRACE, INSERT}       m_mode;

public:

    Array<shared_ptr<Light> >       lightArray;
    Color3                          ambient;

    World();

    /** Returns true if there is an unoccluded line of sight from v0
        to v1.  This is sometimes called the visibilty function in the
        literature.*/
    bool lineOfSight(const Point3& v0, const Point3& v1) const;

    void begin();
    void insert(const shared_ptr<ArticulatedModel>& model, const CFrame& frame = CFrame());
    void insert(const shared_ptr<Surface>& m);
    void clearScene();
    void end();

    /**\brief Trace the ray into the scene and return the first
       surface hit.

       \param ray In world space 

       \return The surfel hit, or NULL if none.
     */
    shared_ptr<Surfel> intersect(const Ray& ray) const;
};

#endif
