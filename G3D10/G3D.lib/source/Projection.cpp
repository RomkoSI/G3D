/**
  \file G3D/source/Projection.cpp

  \author Morgan McGuire, http://graphics.cs.williams.edu
 
  \created 2005-07-20
  \edited  2012-07-23
*/
#include "G3D/Projection.h"
#include "G3D/Rect2D.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/Ray.h"
#include "G3D/Matrix4.h"
#include "G3D/Any.h"
#include "G3D/Frustum.h"
#include "G3D/stringutils.h"

namespace G3D {

const char* FOVDirection::toString(int i, FOVDirection::Value& v) {
    static const char* str[] = {"HORIZONTAL", "VERTICAL", NULL};
    static const Value val[] = {HORIZONTAL, VERTICAL};
    const char* s = str[i];
    if (s) {
        v = val[i];
    }
    return s;
}


Projection::Projection(const Any& any) {
    *this = Projection();

    AnyTableReader reader("Projection", any);

    String s;
    float f = 0;;

    reader.getIfPresent("fovDirection", m_direction);

    if (reader.getIfPresent("fovDegrees", f)) {
        m_fieldOfView = toRadians(f);
    }

    reader.getIfPresent("nearPlaneZ", m_nearPlaneZ);
    reader.getIfPresent("farPlaneZ", m_farPlaneZ);

    reader.getIfPresent("pixelOffset", m_pixelOffset);
    reader.verifyDone();
}


Any Projection::toAny() const {
    Any any(Any::TABLE, "Projection");

    any["fovDirection"]     = m_direction;
    any["fovDegrees"]       = toDegrees(m_fieldOfView);
    any["nearPlaneZ"]       = nearPlaneZ();
    any["farPlaneZ"]        = farPlaneZ();
    any["pixelOffset"]      = pixelOffset();

    return any;
}
    

Projection::Projection() {

    setNearPlaneZ(-0.1f);
    setFarPlaneZ(-finf());
    setFieldOfView((float)toRadians(50.0f), FOVDirection::HORIZONTAL);
}


Projection::Projection(const Matrix4& proj, const Vector2& viewportExtent) {
    double left, right, bottom, top, nearval, farval;
    proj.getPerspectiveProjectionParameters(left, right, bottom, top, nearval, farval);
    setNearPlaneZ(-(float)nearval);
    setFarPlaneZ(-(float)farval);
    
    //const double halfX = (right - left) / 2.0;
    const double halfY = (bottom - top) / 2.0;

    // From Graphics Codex: [P] Perspective Projection Matrix
    //
    // proj[0][2] = u * proj[1][1] / (nearval * viewportExtent.x)
    // proj[1][2] = v * proj[1][1] / (nearval * viewportExtent.y)
    //
    // u = proj[0][2] * (nearval * viewportExtent.x) / proj[1][1]
    // v = proj[1][2] * (nearval * viewportExtent.y) / proj[1][1]
    // 
    // (u, v) = proj.column(2).xy() *  viewportExtent * (nearval / proj[1][1])

    if (! fuzzyEq(left, -right) || ! fuzzyEq(bottom, -top)) {
        alwaysAssertM(viewportExtent.isFinite(), "Must specify the viewportExtent when constructing a Projection from a Matrix4 with pixelOffsets");
        setPixelOffset(proj.column(2).xy() * viewportExtent * Vector2(-0.5f, 0.5f));
    }

    // Assume vertical field of view, and if the Y-axis scale is positive, restore it to a G3D convention of
    // negating the Y axis
    setFieldOfView(float(atan2(abs(halfY), -m_nearPlaneZ) * 2.0), FOVDirection::VERTICAL);
}


Projection::~Projection() {
}


void Projection::setFieldOfView(float angle, FOVDirection dir) {
    debugAssert((angle < pi()) && (angle > 0));
    
    m_fieldOfView = angle;
    m_direction = dir;
}

 
float Projection::nearPlaneViewportWidth(const Rect2D& viewport) const {
    // Compute the side of a square at the near plane based on our field of view
    float s = 2.0f * -m_nearPlaneZ * tan(m_fieldOfView * 0.5f);

    if (m_direction == FOVDirection::VERTICAL) {
        s *= viewport.width() / viewport.height();
    }

    return s;
}


float Projection::nearPlaneViewportHeight(const Rect2D& viewport) const {
    // Compute the side of a square at the near plane based on our field of view
    float s = 2.0f * -m_nearPlaneZ * tan(m_fieldOfView * 0.5f);

    debugAssert(m_fieldOfView < toRadians(180));
    if (m_direction == FOVDirection::HORIZONTAL) {
        s *= viewport.height() / viewport.width();
    }

    return s;
}


float Projection::imagePlanePixelsPerMeter(const class Rect2D& viewport) const {
    const float scale = -2.0f * tan(m_fieldOfView * 0.5f);
    
    if (m_direction == FOVDirection::HORIZONTAL) {
        return viewport.width() / scale;        
    } else {
        return viewport.height() / scale;
    }
}



Ray Projection::ray(float x, float y, const Rect2D& viewport) const {

    int screenWidth  = iFloor(viewport.width());
    int screenHeight = iFloor(viewport.height());

    Vector3 origin = Point3::zero();

    float cx = screenWidth  / 2.0f - viewport.x0();
    float cy = screenHeight / 2.0f - viewport.y0();

    float vw = nearPlaneViewportWidth(viewport);
    float vh = nearPlaneViewportHeight(viewport);

    const Vector3& direction = Vector3( (x - cx) * vw / screenWidth,
                            -(y - cy) * vh / screenHeight,
                             m_nearPlaneZ).direction();

    return Ray::fromOriginAndDirection(origin, direction);
}


void Projection::getProjectPixelMatrix(const Rect2D& viewport, Matrix4& P) const {
    getProjectUnitMatrix(viewport, P);
    float screenWidth  = viewport.width();
    float screenHeight = viewport.height();

    float sx = screenWidth / 2.0f;
    float sy = screenHeight / 2.0f;

    P = Matrix4(sx, 0, 0, sx + viewport.x0() - m_pixelOffset.x,
                0, -sy, 0, sy + viewport.y0() + m_pixelOffset.y,
                0, 0,  1, 0,
                0, 0,  0, 1) * P;
}


void Projection::getProjectUnitMatrix(const Rect2D& viewport, Matrix4& P) const {

    // Uses double precision because the division operations may otherwise 
    // significantly hurt prevision.
    const double screenWidth  = viewport.width();
    const double screenHeight = viewport.height();

    double r, l, t, b, n, f, x, y;

    if (m_direction == FOVDirection::VERTICAL) {
        y = -m_nearPlaneZ * tan(m_fieldOfView / 2);
        x = y * (screenWidth / screenHeight);
    } else { //m_direction == HORIZONTAL
        x = -m_nearPlaneZ * tan(m_fieldOfView / 2);
        y = x * (screenHeight / screenWidth);
    }

    n = -m_nearPlaneZ;
    f = -m_farPlaneZ;

    // Scale the pixel offset relative to the (non-square!) pixels in the unit frustum
    r =  x - m_pixelOffset.x * (2 * x) / screenWidth;
    l = -x - m_pixelOffset.x * (2 * x) / screenWidth;
    t =  y + m_pixelOffset.y * (2 * y) / screenHeight;
    b = -y + m_pixelOffset.y * (2 * y) / screenHeight;

    P = Matrix4::perspectiveProjection(l, r, b, t, n, f);
}


Vector2 Projection::fieldOfViewAngles(const Rect2D& viewport) const {
	if (m_direction == FOVDirection::HORIZONTAL) {
		return Vector2(m_fieldOfView, 2.0f * atan((viewport.height() / viewport.width()) * tan(m_fieldOfView / 2.0f)));
	} else {
		return Vector2(2.0f * atan((viewport.width() / viewport.height()) * tan(m_fieldOfView / 2.0f)), m_fieldOfView);
	}
}


Vector3 Projection::projectUnit(const Point3& point, const Rect2D& viewport) const {
    Matrix4 M;
    getProjectUnitMatrix(viewport, M);

    const Vector4& screenSpacePoint = M * Vector4(point, 1.0f);

    return Vector3(screenSpacePoint.xyz() / screenSpacePoint.w);
}


Point3 Projection::project(const Point3& point, const Rect2D& viewport) const {

    // Find the point in the homogeneous cube
    const Point3& cube = projectUnit(point, viewport);

    return convertFromUnitToNormal(cube, viewport);
}


Point3 Projection::unprojectUnit(const Vector3& v, const Rect2D& viewport) const {

    const Vector3& projectedPoint = convertFromUnitToNormal(v, viewport);

    return unproject(projectedPoint, viewport);
}


Point3 Projection::unproject(const Point3& v, const Rect2D& viewport) const {
    
    const float n = m_nearPlaneZ;
    const float f = m_farPlaneZ;

    float z;

    if (-f >= finf()) {
        // Infinite far plane
        z = 1.0f / (((-1.0f / n) * v.z) + 1.0f / n);
    } else {
        z = 1.0f / ((((1.0f / f) - (1.0f / n)) * v.z) + 1.0f / n);
    }

    const Ray& ray = Projection::ray(v.x - m_pixelOffset.x, v.y - m_pixelOffset.y, viewport);

    // Find out where the ray reaches the specified depth.
    const Vector3& out = ray.origin() + ray.direction() * z / ray.direction().z;

    return out;
}


float Projection::worldToScreenSpaceArea(float area, float z, const Rect2D& viewport) const {
    (void)viewport;
    if (z >= 0) {
        return finf();
    }
    return area * (float)square(-nearPlaneZ() / z);
}


void Projection::getClipPlanes
   (const Rect2D&       viewport,
    Array<Plane>&       clip) const {

    Frustum fr;
    frustum(viewport, fr);
    clip.resize(fr.faceArray.size(), DONT_SHRINK_UNDERLYING_ARRAY);
    for (int f = 0; f < clip.size(); ++f) {
        clip[f] = fr.faceArray[f].plane;
    }
}
 

Frustum Projection::frustum(const Rect2D& viewport) const {
    Frustum f;
    frustum(viewport, f);
    return f;
}


void Projection::frustum(const Rect2D& viewport, Frustum& fr) const {
    fr.vertexPos.clear();
    fr.faceArray.clear();
    // The volume is the convex hull of the vertices definining the view
    // frustum and the light source point at infinity.

    const float x               = nearPlaneViewportWidth(viewport) / 2;
    const float y               = nearPlaneViewportHeight(viewport) / 2;
    const float zn              = m_nearPlaneZ;
    const float zf              = m_farPlaneZ;
    float xx, zz, yy;

    float halfFOV = m_fieldOfView * 0.5f;

    // This computes the normal, which is based on the complement of the 
    // halfFOV angle, so the equations are "backwards"
    if (m_direction == FOVDirection::VERTICAL) {
        yy = -cosf(halfFOV);
        xx = yy * viewport.height() / viewport.width();
        zz = -sinf(halfFOV);
    } else {
        xx = -cosf(halfFOV);
        yy = xx * viewport.width() / viewport.height();
        zz = -sinf(halfFOV);
    } 

    // Near face (ccw from UR)
    fr.vertexPos.append(
        Vector4( x,  y, zn, 1),
        Vector4(-x,  y, zn, 1),
        Vector4(-x, -y, zn, 1),
        Vector4( x, -y, zn, 1));

    // Far face (ccw from UR, from origin)
    if (m_farPlaneZ == -finf()) {
        fr.vertexPos.append(Vector4( x,  y, zn, 0),
                            Vector4(-x,  y, zn, 0),
                            Vector4(-x, -y, zn, 0),
                            Vector4( x, -y, zn, 0));
    } else {
        // Finite
        const float s = zf / zn;
        fr.vertexPos.append(Vector4( x * s,  y * s, zf, 1),
                            Vector4(-x * s,  y * s, zf, 1),
                            Vector4(-x * s, -y * s, zf, 1),
                            Vector4( x * s, -y * s, zf, 1));
    }

    Frustum::Face face;

    // Near plane (wind backwards so normal faces into frustum)
    // Recall that nearPlane, farPlane are positive numbers, so
    // we need to negate them to produce actual z values.
    face.plane = Plane(Vector3(0,0,-1), Vector3(0,0,m_nearPlaneZ));
    face.vertexIndex[0] = 3;
    face.vertexIndex[1] = 2;
    face.vertexIndex[2] = 1;
    face.vertexIndex[3] = 0;
    fr.faceArray.append(face);

    // Right plane
    face.plane = Plane(Vector3(xx, 0, zz), Vector3::zero());
    face.vertexIndex[0] = 0;
    face.vertexIndex[1] = 4;
    face.vertexIndex[2] = 7;
    face.vertexIndex[3] = 3;
    fr.faceArray.append(face);

    // Left plane
    face.plane = Plane(Vector3(-fr.faceArray.last().plane.normal().x, 0, fr.faceArray.last().plane.normal().z), Vector3::zero());
    face.vertexIndex[0] = 5;
    face.vertexIndex[1] = 1;
    face.vertexIndex[2] = 2;
    face.vertexIndex[3] = 6;
    fr.faceArray.append(face);

    // Top plane
    face.plane = Plane(Vector3(0, yy, zz), Vector3::zero());
    face.vertexIndex[0] = 1;
    face.vertexIndex[1] = 5;
    face.vertexIndex[2] = 4;
    face.vertexIndex[3] = 0;
    fr.faceArray.append(face);

    // Bottom plane
    face.plane = Plane(Vector3(0, -fr.faceArray.last().plane.normal().y, fr.faceArray.last().plane.normal().z), Vector3::zero());
    face.vertexIndex[0] = 2;
    face.vertexIndex[1] = 3;
    face.vertexIndex[2] = 7;
    face.vertexIndex[3] = 6;
    fr.faceArray.append(face);

    // Far plane
    if (-m_farPlaneZ < finf()) {
        face.plane = Plane(Vector3(0, 0, 1), Vector3(0, 0, m_farPlaneZ));
        face.vertexIndex[0] = 4;
        face.vertexIndex[1] = 5;
        face.vertexIndex[2] = 6;
        face.vertexIndex[3] = 7;
        fr.faceArray.append(face);
    }
}


void Projection::getNearViewportCorners
(const Rect2D& viewport,
 Point3&      outUR,
 Point3&      outUL,
 Point3&      outLL,
 Point3&      outLR) const {
    
    // Must be kept in sync with getFrustum()
    const float w  = nearPlaneViewportWidth(viewport) / 2.0f;
    const float h  = nearPlaneViewportHeight(viewport) / 2.0f;
    const float z  = nearPlaneZ();

    // Compute the points
    outUR = Point3( w,  h, z);
    outUL = Point3(-w,  h, z);
    outLL = Point3(-w, -h, z);
    outLR = Point3( w, -h, z);
}


void Projection::getFarViewportCorners
   (const Rect2D& viewport,
    Point3& outUR,
    Point3& outUL,
    Point3& outLL,
    Point3& outLR) const {

    // Must be kept in sync with getFrustum()
    const float w = nearPlaneViewportWidth(viewport) * m_farPlaneZ / m_nearPlaneZ;
    const float h = nearPlaneViewportHeight(viewport) * m_farPlaneZ / m_nearPlaneZ;
    const float z = m_farPlaneZ;
    
    // Compute the points
    outUR = Point3( w/2,  h/2, z);
    outUL = Point3(-w/2,  h/2, z);
    outLL = Point3(-w/2, -h/2, z);
    outLR = Point3( w/2, -h/2, z);
}


void Projection::serialize(BinaryOutput& bo) const {
    bo.writeFloat32(m_fieldOfView);
    debugAssert(nearPlaneZ() < 0.0f);
    bo.writeFloat32(nearPlaneZ());
    debugAssert(farPlaneZ() < 0.0f);
    bo.writeFloat32(farPlaneZ());
    bo.writeInt8(m_direction);
    m_pixelOffset.serialize(bo);
}


void Projection::deserialize(BinaryInput& bi) {
    m_fieldOfView = bi.readFloat32();
    m_nearPlaneZ = bi.readFloat32();
    debugAssert(m_nearPlaneZ < 0.0f);
    m_farPlaneZ = bi.readFloat32();
    debugAssert(m_farPlaneZ < 0.0f);
    m_direction = (FOVDirection)bi.readInt8();
    m_pixelOffset.deserialize(bi);
}


Point3 Projection::convertFromUnitToNormal(const Point3& in, const Rect2D& viewport) const{
    return (in + Vector3(1.0f, 1.0f, 1.0f)) * 0.5f * Vector3(viewport.width(), viewport.height(), 1.0f) + 
            Vector3(viewport.x0(), viewport.y0(), 0.0f);
}


Vector3 Projection::reconstructFromDepthClipInfo() const {
    const double z_f = farPlaneZ();
    const double z_n = nearPlaneZ();

    return
        (z_f == -inf()) ? 
            Vector3(float(z_n), -1.0f, 1.0f) : 
            Vector3(float(z_n * z_f),  float(z_n - z_f),  float(z_f));
}


Vector4 Projection::reconstructFromDepthProjInfo(int width, int height) const {
    Matrix4 P;
    getProjectUnitMatrix(Rect2D::xywh(0.0f, 0.0f, (float)width, (float)height), P);
    return Vector4
        (float(-2.0 / (width * P[0][0])), 
         float(-2.0 / (height * P[1][1])),
         float((1.0 - (double)P[0][2]) / P[0][0]), 
         float((1.0 - (double)P[1][2]) / P[1][1]));
}
} // namespace
