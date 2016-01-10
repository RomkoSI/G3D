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
    enum Mode {TRACE, INSERT}       m_mode;

public:

    Array<shared_ptr<Light> >       lightArray;
    Color3                          ambient;

    World();

    /** Returns true if there is an unoccluded line of sight from v0
        to v1.  This is sometimes called the visibilty function in the
        literature.*/
    bool lineOfSight(const Vector3& v0, const Vector3& v1) const;

    void begin();
		void printTris();
		//static void printIfWanted(const Tri T, const CPUVertexArray& cpuVertexArray);
    void insert(const shared_ptr<ArticulatedModel>& model, const CFrame& frame = CFrame());
    void insert(const shared_ptr<Surface>& m);
    void end();

    /**\brief Trace the ray into the scene and return the first
       surface hit.

       \param ray In world space 

       \param distance On input, the maximum distance to trace to.  On
       output, the distance to the closest surface.

       \return The surfel hit, or NULL if none.
     */
    shared_ptr<Surfel> intersect(const Ray& ray, float& distance) const;
};

#endif
