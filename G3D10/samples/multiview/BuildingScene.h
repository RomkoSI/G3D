#ifndef BuildingScene_h
#define BuildingScene_h

#include <G3D/G3DAll.h>

/** Sample object */
class MyEntity : public ReferenceCountedObject {
private:

    MyEntity();

public:

    CFrame                          frame;
    shared_ptr<ArticulatedModel>    model;

    static shared_ptr<MyEntity> create(const CFrame& c, const shared_ptr<ArticulatedModel>& m);

    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray);
};


/** Sample scene graph */
class BuildingScene : public ReferenceCountedObject {
protected:

    LightingEnvironment        m_lighting;
    Array< shared_ptr<MyEntity> >   m_entityArray;
	
	BuildingScene() {}

public:

    static shared_ptr<BuildingScene> create();
    
    void onPose(Array< shared_ptr<Surface> >& surfaceArray);

    inline const LightingEnvironment& lighting() const {
        return m_lighting;
    }
};

#endif
