/**
  \file Camera.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2005-07-20
  \edited  2016-02-22

  G3D Library http://g3d.cs.williams.edu
  Copyright 2000-2016, Morgan McGuire morgan@cs.williams.edu
  All rights reserved.
  Use permitted under the BSD license
*/

#ifndef GLG3D_Camera_h
#define GLG3D_Camera_h

#include "G3D/platform.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Vector3.h"
#include "G3D/Plane.h"
#include "G3D/debugAssert.h"
#include "G3D/Projection.h"
#include "GLG3D/Scene.h"
#include "GLG3D/DepthOfFieldSettings.h"
#include "GLG3D/FilmSettings.h"
#include "GLG3D/Entity.h"
#include "GLG3D/MotionBlurSettings.h"

namespace G3D {

class Matrix4;
class Rect2D;
class Any;
class AnyTableReader;
class Args;
class Scene;


/**
  \brief Abstraction of a lens or pinhole camera.

  The area a camera sees is called a frustum.  It is bounded by the
  near plane, the far plane, and the sides of the view frame projected
  into the scene.  It has the shape of a pyramid with the top cut off.

  Cameras can project points from 3D to 2D.  The "unit" projection
  matches OpenGL.  It maps the entire view frustum to a cube of unit
  radius (i.e., edges of length 2) centered at the origin.  The
  non-unit projection then maps that cube to the specified pixel
  viewport in X and Y and the range [0, 1] in Z.  The projection is
  reversable as long as the projected Z value is known.

  All viewport arguments are the pixel bounds of the viewport-- e.g.,
  RenderDevice::viewport().

  See http://bittermanandy.wordpress.com/2009/04/10/a-view-to-a-thrill-part-one-camera-concepts/
  for a nice introduction to camera transformations.
 */
class Camera : public Entity {
private:
    /** Used for GUI */
    GApp*                       m_app;

protected:
    
    Projection                  m_projection;
    
    /** Non-negative, in seconds */
    float                       m_exposureTime;

    DepthOfFieldSettings        m_depthOfFieldSettings;
    MotionBlurSettings          m_motionBlurSettings;
    FilmSettings                m_filmSettings;

    /** Used to scale camera visualizations */
    float                       m_visualizationScale;

    /** \sa Entity::init.  Note that this is not an override; there is no case 
        where it is desirable to dynamically dispatch this method or invoke it externally. */
    void init(AnyTableReader&    propertyTable);    

    Camera();

public:

    static shared_ptr<Entity> create(const String& name, Scene* scene, AnyTableReader& propertyTable, const ModelTable& modelTable = ModelTable(),
                                     const Scene::LoadOptions& options = Scene::LoadOptions());

    static shared_ptr<Camera> create(const String& name = "Camera");

    Any toAny(const bool forceAll = false) const override;

    float visualizationScale() const {
        return m_visualizationScale;
    }

    /** The view frustum parameters */
    const Projection& projection() const {
        return m_projection;
    }

    void setProjection(const Projection& p) {
        m_projection = p;
    }

    /** Used for rendering GBuffer::SS_EXPRESSIVE_MOTION.  Interpolates between previous frame 
        and current frame based on  MotionBlurSettings::cameraMotionInfluence() */
    CFrame expressivePreviousFrame() const {
        return frame().lerp(previousFrame(), m_motionBlurSettings.cameraMotionInfluence());
    }

    /** \deprecated */
    Camera(const Matrix4& proj, const CFrame& frame);

    virtual ~Camera();
        
    /** Displacement from the upper left added in pixels in screen
        space to the projection matrix.  This is useful for shifting
        the sampled location from the pixel center (OpenGL convention)
        to other locations, such as the upper-left.
      
        The default is (0, 0).

        \deprecated Now on Projection
     */
    void setPixelOffset(const Vector2& p) {
        m_projection.setPixelOffset(p);
    }

    /** \deprecated Now on Projection */
    const Vector2& pixelOffset() const {
        return m_projection.pixelOffset();
    }
    
           
    /** Sets \a P equal to the camera's projection matrix. This is the
        matrix that maps points to the homogeneous clip cube that
        varies from -1 to 1 on all axes.  The projection matrix does
        not include the camera transform.

        For rendering direct to an OSWindow (vs. rendering to Texture/Framebuffer),
        multiply this matrix by <code>Matrix4::scale(1, -1, 1)</code>. 

        This is the matrix that a RenderDevice (or OpenGL) uses as the projection matrix.
        \sa RenderDevice::setProjectionAndCameraMatrix, RenderDevice::setProjectionMatrix, Matrix4::perspectiveProjection,
        gluPerspective

    \deprecated Now on Projection
    */
    void getProjectUnitMatrix(const Rect2D& viewport, Matrix4& P) const;

    /** Sets \a P equal to the matrix that transforms points to pixel
        coordinates on the given viewport.  A point correspoinding to
        the top-left corner of the viewport in camera space will
        transform to viewport.x0y0() and the bottom-right to viewport.x1y1().

       \deprecated Now on Projection
     */
    void getProjectPixelMatrix(const Rect2D& viewport, Matrix4& P) const;

    /** Converts projected points from OpenGL standards
        (-1, 1) to normal 3D coordinate standards (0, 1) 
    */
    Point3 convertFromUnitToNormal(const Point3& in, const Rect2D& viewport) const;

    /**
       Sets the field of view, in radians.  The 
       initial angle is toRadians(55).  Must specify
       the direction of the angle.

       This is the full angle, i.e., from the left side of the
       viewport to the right side.

       The field of view is specified for the pinhole version of the
       camera.

       \deprecated
    */
    void setFieldOfView(float edgeToEdgeAngleRadians, FOVDirection direction) {
        m_projection.setFieldOfView(edgeToEdgeAngleRadians, direction);
    }

    /** Returns the current full field of view angle (from the left side of the
       viewport to the right side) and direction \deprecated*/
    void getFieldOfView(float& angle, FOVDirection& direction) const {
        m_projection.getFieldOfView(angle, direction);
    }

    /** Set the edge-to-edge FOV angle along the current
        fieldOfViewDirection in radians. \deprecated*/
    void setFieldOfViewAngle(float edgeToEdgeAngleRadians) {
        m_projection.setFieldOfViewAngle(edgeToEdgeAngleRadians);
    }

    /** \deprecated */
    void setFieldOfViewAngleDegrees(float edgeToEdgeAngleDegrees) {
        m_projection.setFieldOfViewAngleDegrees(edgeToEdgeAngleDegrees);
    }

    /** \deprecated */
    void setFieldOfViewDirection(FOVDirection d) {
        m_projection.setFieldOfViewDirection(d);
    }

    /** \deprecated */
    float fieldOfViewAngle() const {
        return m_projection.fieldOfViewAngle();
    }

    /** \deprecated */
    float fieldOfViewAngleDegrees() const {
        return m_projection.fieldOfViewAngleDegrees();
    }

    /** \deprecated */
    FOVDirection fieldOfViewDirection() const {
        return m_projection.fieldOfViewDirection();
    } 

    /**
     Pinhole projects a world space point onto a width x height screen.  The
     returned coordinate uses pixmap addressing: x = right and y =
     down.  The resulting z value is 0 at the near plane, 1 at the far plane,
     and is a linear compression of unit cube projection.

     If the point is behind the camera, Point3::inf() is returned.

     \sa RenderDevice::invertY
     */
    Point3 project(const Point3& point,
                   const class Rect2D& viewport) const;

    /**
       Pinhole projects a world space point onto a unit cube.  The resulting
     x,y,z values range between -1 and 1, where z is -1
     at the near plane and 1 at the far plane and varies hyperbolically in between.

     If the point is behind the camera, Point3::inf() is returned.
     */
    Point3 projectUnit(const Point3& point,
                        const class Rect2D& viewport) const;

    /**
       Gives the world-space coordinates of screen space point v, where
       v.x is in pixels from the left, v.y is in pixels from
       the top, and v.z is on the range 0 (near plane) to 1 (far plane).
     */
    Point3 unproject(const Point3& v, const Rect2D& viewport) const;

     /**
       Gives the world-space coordinates of unit cube point v, where
       v varies from -1 to 1 on all axes.  The unproject first
       transforms the point into a pixel location for the viewport, then calls unproject
     */
    Point3 unprojectUnit(const Point3& v, const Rect2D& viewport) const;

    /**
     Returns the pixel area covered by a shape of the given
     world space area at the given z value (z must be negative)
     under pinhole projection.
     */
    float worldToScreenSpaceArea(float area, float z, const class Rect2D& viewport) const;

    /**
     Returns the world space 3D viewport corners under pinhole projection.  These
     are at the near clipping plane.  The corners are constructed
     from the nearPlaneZ, viewportWidth, and viewportHeight.
     "left" and "right" are from the Camera's perspective.
     */
    void getNearViewportCorners(const class Rect2D& viewport,
                                Point3& outUR, Point3& outUL,
                                Point3& outLL, Point3& outLR) const;

    /**
     Returns the world space 3D viewport corners under pinhole projection.  These
     are at the Far clipping plane.  The corners are constructed
     from the nearPlaneZ, farPlaneZ, viewportWidth, and viewportHeight.
     "left" and "right" are from the Camera's perspective.
     */
    void getFarViewportCorners(const class Rect2D& viewport,
                               Point3& outUR, Point3& outUL,
                               Point3& outLL, Point3& outLR) const;

    /**
      Returns the world space ray passing through pixel coordinates
      (x, y) on the image plane under pinhole projection.  The pixel x
      and y axes are opposite the 3D object space axes: (0,0) is the
      upper left corner of the screen.  They are in viewport
      coordinates, not screen coordinates.

      The ray origin is at the camera-space origin, that is, in world space
      it is at Camera::coordinateFrame().translation.  To start it at the image plane,
      move it forward by imagePlaneDepth/ray.direction.z along the camera's
      look vector.

      Integer (x, y) values correspond to
      the upper left corners of pixels.  If you want to cast rays
      through pixel centers, add 0.5 to x and y.        
      */
    Ray worldRay
       (float                                  x,
        float                                  y,
        const class Rect2D&                    viewport) const;

    /**
      Returns a negative z-value.

      \deprecated
     */
    inline float nearPlaneZ() const {
        return m_projection.nearPlaneZ();
    }

    /**
     Returns a negative z-value.
         \deprecated
     */
    inline float farPlaneZ() const {
        return m_projection.farPlaneZ();
    }

    /**
     Sets a new value for the far clipping plane
     Expects a negative value

     \deprecated
     */
    inline void setFarPlaneZ(float z) {
        m_projection.setFarPlaneZ(z);
    }
    
    /**
     Sets a new value for the near clipping plane
     Expects a negative value

       \deprecated
     */
    inline void setNearPlaneZ(float z) {
        m_projection.setNearPlaneZ(z);
    }
    
    /** \brief The number of pixels per meter at z=-1 for the given viewport.
       This is useful for performing explicit projections and for transforming world-space 
       values like circle of confusion into
       screen space.
      \deprecated*/
    float imagePlanePixelsPerMeter(const class Rect2D& viewport) const;

    /**
     Returns the camera space width in meters of the viewport at the near plane.
      \deprecated*/
    float nearPlaneViewportWidth(const class Rect2D& viewport) const;

    /**
     Returns the camera space height of the viewport in meters at the near plane.
      \deprecated*/
    float nearPlaneViewportHeight(const class Rect2D& viewport) const;

    void setPosition(const Point3& t);

    /** Rotate the camera in place to look at the target.  Does not
        persistently look at that location when the camera moves;
        i.e., if you move the camera and still want it to look at the
        old target, you must call lookAt again after moving the
        camera.)*/
    void lookAt(const Point3& position, const Vector3& up = Vector3::unitY());

    /**
       Returns the clipping planes of the frustum, in world space.  
       The planes have normals facing <B>into</B> the view frustum.
       
       The plane order is guaranteed to be:
       Near, Right, Left, Top, Bottom, [Far]
       
       If the far plane is at infinity, the resulting array will have 
       5 planes, otherwise there will be 6.
       
       The viewport is used only to determine the aspect ratio of the screen; the
       absolute dimensions and xy values don't matter.
    */
    void getClipPlanes
    (const Rect2D& viewport,
     Array<Plane>& outClip) const;

    /**
      Returns the world space view frustum, which is a truncated pyramid describing
      the volume of space seen by this camera.
    */
    void frustum(const Rect2D& viewport, Frustum& f) const;

    Frustum frustum(const Rect2D& viewport) const;
    
	DepthOfFieldSettings& depthOfFieldSettings() {
    	return m_depthOfFieldSettings;
    }

    const DepthOfFieldSettings& depthOfFieldSettings() const {
        return m_depthOfFieldSettings;
    }

	MotionBlurSettings& motionBlurSettings() {
    	return m_motionBlurSettings;
    }

    const MotionBlurSettings& motionBlurSettings() const {
        return m_motionBlurSettings;
    }

    FilmSettings& filmSettings() {
    	return m_filmSettings;
    }

    const FilmSettings& filmSettings() const {
        return m_filmSettings;
    }

    /** World space ray from a lens camera.  (\a u, \a v) are signed
        (-1, 1) that should lie within a unit-radius disc.*/
    Ray worldRay(float x, float y, float u, float v, const class Rect2D &viewport) const;
    
    /** Circle of confusion radius, in pixels, for a point at negative
        position \a z from the center of projection along the
        camera-space z axis.*/
    float circleOfConfusionRadiusPixels(float z, const class Rect2D& viewport) const;

    virtual void copyParametersFrom(const shared_ptr<Camera>& camera);

    /** Limits the radius to \a viewportFractionMax for physical blur, which has no inherent limit.*/
    float maxCircleOfConfusionRadiusPixels(const Rect2D& viewport, float viewportFractionMax = 0.02f);

    /**
    Binds:

    * mat4x3 prefix##previousFrame
    * mat4x3 prefix##frame
    * float3 prefix##clipInfo
    * float4 prefix##projInfo

    to \a args.

    Inside a shader, these arguments can be automatically defined using the macro
        
    \code
    #include <Camera/Camera.glsl> 
    uniform_Camera(name_)
    \endcode

    \sa uniform_Texture, uniform_GBuffer
    */
    virtual void setShaderArgs(class UniformTable& args, const Vector2& screenSize, const String& prefix = "") const;

    virtual void makeGUI(GuiPane* p, GApp* app) override;

protected:

    void onOverwriteCameraFromDebug();

    void onOverwriteDebugFromCamera();

    friend class CameraControlWindow;

    void _setFieldOfViewDirectionInt(int d) {
        setFieldOfViewDirection(FOVDirection(d));
    }

    int _fieldOfViewDirectionInt() const {
        return fieldOfViewDirection().value;
    }


};

} // namespace G3D

#endif
