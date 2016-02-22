/**
 \file GLG3D/Skybox.h
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  G3D Innovation Engine
  Copyright 2000-2016, Morgan McGuire.
  All rights reserved.
 */
#ifndef GLG3D_Skybox_h
#define GLG3D_Skybox_h

#include "G3D/SplineExtrapolationMode.h"
#include "GLG3D/Entity.h"
#include "GLG3D/Scene.h"
#include "GLG3D/Texture.h"

namespace G3D {

class Surface;
class Any;
class AnyTableReader;

/**
 \brief An infinite box with a cube-map texture. 

  Ignores its frame()

  Can interpolate through a set of keyframes.  Any format:

  If the individual keyframes are Texture::DIM_2D, then they will be rendered as 
  2D spherical maps. If they are Texture::CUBE, then they will be rendered
  as cube maps.

\code
Skybox {
  keyframeArray = [  "sky1*.jpg", "sky2*.jpg", "sky2*.jpg"];
  timeArray     = [0, 1, 2];
  finalInterval = 1;
  extrapolationMode = CYCLIC;
}
\endcode

Simple Any format has one field, which is texture = Texture::Specification
*/
class Skybox : public Entity {
protected:
    
    /** If true, cycle through the KeyFrames rather than running through them once. */
    SplineExtrapolationMode     m_extrapolationMode;
    SimTime                     m_finalInterval;
    Array<shared_ptr<Texture> > m_keyframeArray;
    Array<SimTime>              m_timeArray;

    Skybox() : m_extrapolationMode(SplineExtrapolationMode::CLAMP), m_finalInterval(1.0) {}
    void init(AnyTableReader& propertyTable);
    void init(const Array<shared_ptr<Texture> >& k, const Array<SimTime>& t, SimTime f, SplineExtrapolationMode e);

public:

    static shared_ptr<Entity> create 
       (const String&                           name,
        Scene*                                  scene,
        AnyTableReader&                         propertyTable,
        const ModelTable&                       modelTable,
        const Scene::LoadOptions&               options = Scene::LoadOptions());

    static shared_ptr<Skybox> create
       (const String& name,
        Scene*                                  scene,
        const Array<shared_ptr<Texture> >&      keyframeArray, 
        const Array<SimTime>&                   timeArray, 
        SimTime                                 finalInterval,          
        SplineExtrapolationMode                 extrapolationMode,
        bool                                    canChange,
        bool                                    shouldBeSaved);

    virtual void onPose(Array< shared_ptr<Surface> >& surfaceArray) override;

    virtual Any toAny(const bool forceAll = false) const override;

    virtual const Array<shared_ptr<Texture> >& keyframeArray() const {
        return m_keyframeArray;
    }
};

}

#endif
