/**
  \file G3D/Projection.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu

  \created 2005-07-20
  \edited  2016-03-23

  Copyright 2005-2016 Morgan McGuire
*/

#ifndef G3D_Projection_h
#define G3D_Projection_h

#include "G3D/platform.h"
#include "G3D/enumclass.h"
#include "G3D/Vector3.h"
#include "G3D/Plane.h"
#include "G3D/debugAssert.h"

namespace G3D {

class Matrix4;
class Rect2D;
class Any;
class Ray;
class Frustum;

/**
   Stores the direction of the field of view for a G3D::Projection
*/
class FOVDirection {
public:
    enum Value {HORIZONTAL, VERTICAL} value;
    static const char* toString(int i, Value& v);
    G3D_DECLARE_ENUM_CLASS_METHODS(FOVDirection);
};
}

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::FOVDirection);

namespace G3D {
/**
  \brief A 3D perspective projection with bounding planes in camera space.

  The area that a computer graphics camera sees is called a frustum.  It is bounded by the
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

  \see Matrix4, CoordinateFrame, Frustum
 */
class Projection  {    
private:

    /** Full field of view (in radians) */
    float                       m_fieldOfView;

    /** Clipping plane, *not* imaging plane.  Negative numbers. */
    float                       m_nearPlaneZ;

    /** Negative */
    float                       m_farPlaneZ;

    FOVDirection                m_direction;

    Vector2                     m_pixelOffset;

public:

    /** Must be of the format produced by the Any cast, e.g.,

    \code
        Projection {
            nearPlaneZ = -0.5,
            farPlaneZ = -50,
            fovDirection = "HORIZONTAL",
            fovAngleDegrees = 90
        }
    \endcode

     Missing fields are filled from the default Projection constructor.
    */
    Projection(const Any& any);

    Any toAny() const;

    Projection();

    /** \param viewportExtent Required if there is a pixel offset in \a proj (i.e., it has asymmetric clip planes) */
    Projection(const Matrix4& proj, const Vector2& viewportExtent = Vector2::nan());

    virtual ~Projection();
    
    /** Displacement relative to the pixel center measured to the right and down added in pixels in screen
        space to the projection matrix.  This is useful for shifting
        the sampled location from the pixel center (OpenGL convention)
        to other locations, such as the upper-left.
      
        The default is (0, 0).
     */
    void setPixelOffset(const Vector2& p) {
        m_pixelOffset = p;
    }

    const Vector2& pixelOffset() const {
        return m_pixelOffset;
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
    */
    void getProjectUnitMatrix(const Rect2D& viewport, Matrix4& P) const;

    /** Sets \a P equal to the matrix that transforms points to pixel
        coordinates on the given viewport.  A point correspoinding to
        the top-left corner of the viewport in camera space will
        transform to viewport.x0y0() and the bottom-right to viewport.x1y1(). */
    void getProjectPixelMatrix(const Rect2D& viewport, Matrix4& P) const;


    /**
       Sets the field of view, in radians.  The 
       initial angle is toRadians(55).  Must specify
       the direction of the angle.

       This is the full angle, i.e., from the left side of the
       viewport to the right side.

       The field of view is specified for the pinhole version of the
       camera.
    */
    void setFieldOfView(float edgeToEdgeAngleRadians, FOVDirection direction);

    /** Returns the current full field of view angle (from the left side of the
       viewport to the right side) and direction */
    void getFieldOfView(float& angle, FOVDirection direction) const {
        angle = m_fieldOfView;
        direction = m_direction;
    }

    /** Set the edge-to-edge FOV angle along the current
        fieldOfViewDirection in radians. */
    void setFieldOfViewAngle(float edgeToEdgeAngleRadians) {
        m_fieldOfView = edgeToEdgeAngleRadians;
    }

    void setFieldOfViewAngleDegrees(float edgeToEdgeAngleDegrees) {
        setFieldOfViewAngle(toRadians(edgeToEdgeAngleDegrees));
    }
    
    void setFieldOfViewDirection(FOVDirection d) {
        m_direction = d;
    }

    float fieldOfViewAngle() const {
        return m_fieldOfView;
    }

    float fieldOfViewAngleDegrees() const {
        return toDegrees(m_fieldOfView);
    }

    FOVDirection fieldOfViewDirection() const {
        return m_direction;
    } 

	/** Returns full horizontal and vertical field of view angles in radians. 
	Angle order is guaranteed to be: horizontal FOV, vertical FOV. */
	Vector2 fieldOfViewAngles(const Rect2D& viewport) const;

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
    
    /** Converts projected points from OpenGL standards
        (-1, 1) to normal 3D coordinate standards (0, 1) 
    */
    Point3 convertFromUnitToNormal(const Point3& in, const Rect2D& viewport) const;

    /**
     Returns the pixel area covered by a shape of the given
     world space area at the given z value (z must be negative)
     under pinhole projection.
     */
    float worldToScreenSpaceArea(float worldSpaceArea, float z, const class Rect2D& viewport) const;

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
      Returns the world space ray passing through pixel
      (x, y) on the image plane under pinhole projection.  The pixel x
      and y axes are opposite the 3D object space axes: (0,0) is the
      upper left corner of the screen.  They are in viewport
      coordinates, not screen coordinates.

      The ray origin is at the camera-space origin, that is, in world space
      it is at Camera::coordinateFrame().translation.  To start it at the image plane,
      move it forward by imagePlaneDepth/ray.direction.z along the camera's
      look vector.

      Integer (x, y) values correspond to
      the upper left corners of pixels.  <b>If you want to cast rays
      through pixel centers, add 0.5 to integer x and y. </b>

      The Rect2D::x0y0() coordinate is added to (x, y) before casting to account 
      for guard bands.
      */
    Ray ray
       (float                                  x,
        float                                  y,
        const class Rect2D&                    viewport) const;

    /**
      Returns a negative z-value.
     */
    inline float nearPlaneZ() const {
        return m_nearPlaneZ;
    }

    /**
     Returns a negative z-value.
     */
    inline float farPlaneZ() const {
        return m_farPlaneZ;
    }

    /**
     Sets a new value for the far clipping plane
     Expects a negative value
     */
    inline void setFarPlaneZ(float z) {
        debugAssert(z < 0);
        m_farPlaneZ = z;
    }
    
    /**
     Sets a new value for the near clipping plane
     Expects a negative value
     */
    inline void setNearPlaneZ(float z) {
        debugAssert(z < 0);
        m_nearPlaneZ = z;
    }
    
    /** \brief The number of pixels per meter at z=-1 for the given viewport.
       This is useful for performing explicit projections and for transforming world-space 
       values like circle of confusion into
       screen space.*/
    float imagePlanePixelsPerMeter(const class Rect2D& viewport) const;

    /**
     Returns the camera space width in meters of the viewport at the near plane.
     */
    float nearPlaneViewportWidth(const class Rect2D& viewport) const;

    /**
     Returns the camera space height of the viewport in meters at the near plane.
     */
    float nearPlaneViewportHeight(const class Rect2D& viewport) const;

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
    
    /** Read and Write projection parameters */
    void serialize(class BinaryOutput& bo) const;
    void deserialize(class BinaryInput& bi);

    /** Computes the clipInfo arg used in reconstructFromDepth.glsl */
    Vector3 reconstructFromDepthClipInfo() const;

    /** Computes the projInfo arg used in reconstructFromDepth.glsl */
    Vector4 reconstructFromDepthProjInfo(int width, int height) const;

};

} // namespace G3D

#endif
