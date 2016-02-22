/**
 \file Draw.h
  
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 \created 2003-10-29
 \edited  2013-01-27

 G3D Library http://g3d.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire morgan@cs.williams.edu
 All rights reserved.
 Use permitted under the BSD license
 */

#ifndef G3D_Draw_h
#define G3D_Draw_h

#include "G3D/Color3.h"
#include "G3D/Color4.h"
#include "G3D/Vector3.h"
#include "G3D/MeshAlg.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Shader.h"
#include "Light.h"
#include "GLG3D/GFont.h"

namespace G3D {

class RenderDevice;
class Sphere;
class LineSegment;
class Box;
class AABox;
class Line;
class Capsule;
class Cylinder;
class Plane;
class Frustum;
class Camera;

/**
   \brief Useful routines for rendering primitives when debugging.  

  Compared
 to the optimized RenderDevice::beginIndexedPrimitives calls used
 by ArticulatedModel, <b>these routines are slow</b>.

 When rendering translucent bounding objects, depth write is automatically
 disabled.  Render from back to front for proper transparency.

 \sa debugDraw, Shape
 */
class Draw {
private:

    static const int WIRE_SPHERE_SECTIONS;
    static const int SPHERE_SECTIONS;
    static const int SPHERE_YAW_SECTIONS;
    static const int SPHERE_PITCH_SECTIONS;

    /** Called from wireSphere, wireCapsule */
    static void wireSphereSection(
        const Sphere&               sphere,
        class RenderDevice*         renderDevice,
        const class Color4&         color,
        bool                        top,
        bool                        bottom);


    static void sphereSection(
        const Sphere&       sphere,
        RenderDevice*       renderDevice,
        const Color4&       color,
        bool                top,
        bool                bottom);

    /**
     Returns the scale due to perspective at
     a point for a line.
     */
    static float perspectiveLineThickness(
        RenderDevice*       rd,
        const class Vector3&      pt);

public:

    /**
    Renders exact corners of a 2D polygon using lines.
    Assumes you already called push2D(). 
     */
    static void poly2DOutline(const Array<Vector2>& polygon, RenderDevice* renderDevice, const Color4& color = Color3::yellow());
    static void poly2D(const Array<Vector2>& polygon, RenderDevice* renderDevice, const Color4& color = Color3::yellow());

	/** Draw a colored point cloud. \param points and \param colors must be of equal size */
	static void points(const Array<Point3>& points, RenderDevice* rd, const Array<Color3>& colors, float pixelRadius = 0.5f);

	/** Draw a point cloud */
	static void points(const Array<Point3>& points, RenderDevice* rd, const Color4& color = Color3::white(), float pixelRadius = 0.5f);
	static void points(const Array<Point2>& points, RenderDevice* rd, const Color4& color = Color3::white(), float pixelRadius = 0.5f);

	/** Draw a single point */
	static void point(const Point3& point, RenderDevice* rd, const Color4& color = Color3::white(), float pixelRadius = 0.5f);
	static void point(const Point2& point, RenderDevice* rd, const Color4& color = Color3::white(), float pixelRadius = 0.5f);

    /** Visualize a single light (simple version) dirDist is the distance away to draw the geometry for a directional light */
    static void light(shared_ptr<Light> light, RenderDevice* rd,  float dirDist = 1000.0f);

    /** Visualize a the geometry of the effect of a single light. Currently does nothing for directional lights and shows a full effect sphere for both spot and omni lights */
    static void visualizeLightGeometry(shared_ptr<Light> light, RenderDevice* rd);

    /** Draws a symbolic representation of the camera */
    static void camera(shared_ptr<Camera> camera, RenderDevice* rd);
    
    /** Draws relevant geometry information for the camera. Currently simply draws the camera frustum. */
    static void visualizeCameraGeometry(shared_ptr<Camera> camera, RenderDevice* rd);

    /** Render a skybox using \a cubeMap, and the set of 6 cube map
        faces in \a texture if \a cubeMap is NULL. 

        \see SkyboxSurface
        */
    static void skyBox(RenderDevice* renderDevice, const shared_ptr<Texture>& cubeMap);

    static void physicsFrameSpline(const class PhysicsFrameSpline& spline, RenderDevice* rd, int highlightedIndex = -1);

    /**
     Set the solid color or wire color to Color4::clear() to
     prevent rendering of surfaces or lines.
     */
    static void boxes(
        const Array<Box>& boxes,
        RenderDevice*       renderDevice,
        const Color4&       solidColor = Color4(1,.2f,.2f,.5f),
        const Color4&       wireColor  = Color3::black());

    /** Converts all AABox to Boxes first, so not optimized. */
    static void boxes(
        const Array<AABox>& aaboxes,
        RenderDevice*       renderDevice,
        const Color4&       solidColor = Color4(1,.2f,.2f,.5f),
        const Color4&       wireColor  = Color3::black());

    static void box(
        const Box&          box,
        RenderDevice*       rd,
        const Color4&       solidColor = Color4(1,.2f,.2f,.5f),
        const Color4&       wireColor  = Color3::black());

    static void box(
        const AABox&        box,
        RenderDevice*       rd,
        const Color4&       solidColor = Color4(1,.2f,.2f,.5f),
        const Color4&       wireColor  = Color3::black());

    static void sphere(
        const Sphere&       sphere,
        RenderDevice*       rd,
        const Color4&       solidColor = Color4(1, 1, 0, .5f),
        const Color4&       wireColor  = Color3::black());

    static void plane(
        const Plane&        plane,
        RenderDevice*       rd,
        const Color4&       solidColor = Color4(.2f, .2f, 1, .5f),
        const Color4&       wireColor  = Color3::black());

    static void line(
        const Line&         line,
        RenderDevice*       rd,
        const Color4&       color = Color3::black());

    static void lineSegment(
        const LineSegment&  lineSegment,
        RenderDevice*       rd,
        const Color4&       color = Color3::black(),
        float               scale = 1);
    
    /** The actual histogram will only be 95% as wide and 80% as tall as \param area due to labels */
    static void histogram(
        const Rect2D&       area,
        const Array<float>& values,
        float               binSize,
        RenderDevice*       rd,
        shared_ptr<GFont> font,
        const Color4&       boxColor = Color3::black(),
        const Color4&       labelColor = Color3::black(),
        float               fontSize = 12.0f,
        bool                useLogScale = false);

    /**
     Renders per-vertex normals as thin arrows.  The length
     of the normals is scaled inversely to the number of normals
     rendered.
     */
    static void vertexNormals(
        const G3D::MeshAlg::Geometry&    geometry,
        RenderDevice*               renderDevice,
        const Color4&               color = Color3::green() * .5,
        float                       scale = 1);

    /**
     Convenient for rendering tangent space basis vectors.
     */
    static void vertexVectors(
        const Array<Vector3>&       vertexArray,
        const Array<Vector3>&       directionArray,
        RenderDevice*               renderDevice,
        const Color4&               color = Color3::red() * 0.5,
        float                       scale = 1);

    static void capsule(
       const Capsule&       capsule, 
       RenderDevice*        renderDevice,
       const Color4&        solidColor = Color4(1,0,1,.5),
       const Color4&        wireColor = Color3::black());

    static void cylinder(
       const Cylinder&      cylinder, 
       RenderDevice*        renderDevice,
       const Color4&        solidColor = Color4(1,1,0,.5),
       const Color4&        wireColor = Color3::black());

    static void ray(
        const class Ray&    ray,
        RenderDevice*       renderDevice,
        const Color4&       color = Color3::orange(),
        float               scale = 1);
    
    /** \param vector Can be non-unit length. 
     \param scale Magnify vector by this amount.*/
    static void arrow(
        const Point3&       start,
        const Vector3&      vector,
        RenderDevice*       renderDevice,
        const Color4&       color = Color3::orange(),
        float               scale = 1.0f);

    static void axes(
        const class CoordinateFrame& cframe,
        RenderDevice*       renderDevice,
        const Color4&       xColor = Color3::red(),
        const Color4&       yColor = Color3::green(),
        const Color4&       zColor = Color3::blue(),
        float               scale = 1.0f);

    static void axes(
        RenderDevice*       renderDevice,
        const Color4&       xColor = Color3::red(),
        const Color4&       yColor = Color3::green(),
        const Color4&       zColor = Color3::blue(),
        float               scale = 1.0f);


    /** Draws a rectangle with dimensions specified by the input 
        rect that contains either the passed in texture or if the 
        textureMap is null then the color specified is drawn */
    static void rect2D
    (const class Rect2D& rect,
     RenderDevice* rd,
     const Color4& color = Color3::white(),
     const shared_ptr<Texture>& textureMap = shared_ptr<Texture>(),
     const Sampler& sampler = Sampler::video());

    /** Draws a border about the rectangle
        using polygons (since PrimitiveType::LINE_STRIP doesn't 
        guarantee pixel widths). */
    static void rect2DBorder
       (const class Rect2D& rect,
        RenderDevice* rd,
        const Color4& color = Color3::black(),
        float innerBorder = 0,
        float outerBorder = 1);

    static void frustum
       (const class Frustum& frustum,
        RenderDevice* rd,
        const Color4& color = Color4(1,.4f,.4f,0.2f),
        const Color4& wire = Color3::black());
};

}

#endif
