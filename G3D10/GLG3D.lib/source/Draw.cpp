/**
 \file Draw.cpp
  
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/platform.h"
#include "G3D/Rect2D.h"
#include "G3D/AABox.h"
#include "G3D/Box.h"
#include "G3D/Ray.h"
#include "G3D/Sphere.h"
#include "G3D/Line.h"
#include "G3D/LineSegment.h"
#include "G3D/Capsule.h"
#include "G3D/Cylinder.h"
#include "G3D/Frustum.h"
#include "GLG3D/Draw.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GLCaps.h"
#include "G3D/MeshAlg.h"
#include "G3D/PhysicsFrameSpline.h"
#include "GLG3D/Camera.h"
#include "GLG3D/Light.h"
#include "GLG3D/Shader.h"
#include "GLG3D/ShadowMap.h"
#include "GLG3D/Sampler.h"
#include "GLG3D/SlowMesh.h"

namespace G3D {

void Draw::points(const Array<Point3>& point, RenderDevice* rd, const Array<Color3>& colors, float pixelRadius) {
	SlowMesh mesh(PrimitiveType::POINTS);
    mesh.setPointSize(pixelRadius * 2.0f);
	mesh.reserveSpace(point.size());
	for (int i = 0; i < point.size(); ++i) {
		mesh.setColor(colors[i]);
		mesh.makeVertex(point[i]);
	}
	mesh.render(rd);
}

void Draw::points(const Array<Point3>& point, RenderDevice* rd, const Color4& color, float pixelRadius) {
	SlowMesh mesh(PrimitiveType::POINTS);
    mesh.setPointSize(pixelRadius * 2.0f);
	mesh.setColor(color);
	for (int i = 0; i < point.size(); ++i) {
		mesh.makeVertex(point[i]);
	}
	mesh.render(rd);
}


void Draw::points(const Array<Point2>& point, RenderDevice* rd, const Color4& color, float pixelRadius) {
	Array<Point3> p3;
	p3.resize(point.size());
	for (int i = 0; i < p3.size(); ++i) {
		p3[i] = Point3(point[i], 0);
	}
	points(p3, rd, color, pixelRadius);
}


void Draw::point(const Point3& point, RenderDevice* rd, const Color4& color, float pixelRadius) {
	points(Array<Point3>(point), rd, color, pixelRadius);
}


void Draw::point(const Point2& point, RenderDevice* rd, const Color4& color, float pixelRadius) {
	Draw::point(Point3(point, 0), rd, color, pixelRadius);
}


const int Draw::WIRE_SPHERE_SECTIONS = 26;
const int Draw::SPHERE_SECTIONS = 40;

const int Draw::SPHERE_PITCH_SECTIONS = 20;
const int Draw::SPHERE_YAW_SECTIONS = 40;

void Draw::physicsFrameSpline(const PhysicsFrameSpline& spline, RenderDevice* rd, int highlightedIndex) {
    if (spline.control.size() == 0) {
        return;
    }
    rd->pushState();
    rd->setObjectToWorldMatrix(CFrame());
    for (int i = 0; i < spline.control.size(); ++i) {
        const CFrame& c = spline.control[i];

        Draw::axes(c, rd, Color3::red(), Color3::green(), Color3::blue(), 0.5f);
        if (i == highlightedIndex) {
            Draw::sphere(Sphere(c.translation, 0.2f), rd, Color3::yellow(), Color4::clear());
        } else {
            Draw::sphere(Sphere(c.translation, 0.1f), rd, Color3::white(), Color4::clear());
        }
        
    }

    const int N = spline.control.size() * 30;
    CFrame last = spline.evaluate(0);
    const float a = 0.5f;
    SlowMesh mesh(PrimitiveType::LINES);

    const float t_0   = spline.time[0];
    const float t_end = spline.time.last() + spline.getFinalInterval();

    for (int i = 0; i < N; ++i) {
        float t = lerp(t_0, t_end, i / (N - 1.0f));
        const CFrame& cur = spline.evaluate(t);
        mesh.setColor(Color4(1,1,1,a));
        mesh.makeVertex(last.translation);
        mesh.makeVertex(cur.translation);

        mesh.setColor(Color4(1,0,0,a));
        mesh.makeVertex(last.rightVector() + last.translation);
        mesh.makeVertex(cur.rightVector() + cur.translation);

        mesh.setColor(Color4(0,1,0,a));
        mesh.makeVertex(last.upVector() + last.translation);
        mesh.makeVertex(cur.upVector() + cur.translation);

        mesh.setColor(Color4(0,0,1,a));
        mesh.makeVertex(-last.lookVector() + last.translation);
        mesh.makeVertex(-cur.lookVector() + cur.translation);

        last = cur;
    }
    mesh.render(rd);
    rd->popState();
}



void Draw::skyBox(RenderDevice* renderDevice, const shared_ptr<Texture>& cubeMap) {
    debugAssert(cubeMap->dimension() == Texture::DIM_CUBE_MAP);

    enum Direction {UP = 0, LT = 1, RT = 2, BK = 3, FT = 4, DN = 5};
    renderDevice->pushState();

    // Make a camera with an infinite view frustum to avoid clipping
    Matrix4 P = renderDevice->projectionMatrix();
    Projection projection(P);
    projection.setFarPlaneZ(-finf());
    projection.getProjectUnitMatrix(renderDevice->viewport(), P);
    renderDevice->setProjectionMatrix(P);   

    static Args args;
    static shared_ptr<Shader> shader;
    static bool init = true;

    if (init) {

        Shader::Specification shaderSpec;
        shaderSpec[Shader::VERTEX] = Shader::Source(STR(#version 330\n
                 out vec3 direction;
                 in vec4 g3d_Vertex;

                 void main() {
                     direction = g3d_Vertex.xyz;
                     gl_Position = g3d_Vertex * g3d_ObjectToScreenMatrixTranspose;
                 }\n
                 ));

        shaderSpec[Shader::PIXEL] = Shader::Source(STR(#version 330\n
                 in vec3 direction;\n
                 uniform samplerCube tex_buffer;\n
                 uniform vec4        tex_readMultiplyFirst;\n
                 uniform vec4        tex_readAddSecond;\n
                 out vec4 result;
                 void main() {
                     result.a = 1.0;
                     result.rgb = texture(tex_buffer, direction).rgb * tex_readMultiplyFirst.rgb + tex_readAddSecond.rgb;
                 }\n
                 ));

        shader = Shader::create(shaderSpec);

        args.setPrimitiveType(PrimitiveType::TRIANGLES);
        
        float s = 1;


        Array<Vector4> positions;
        positions.append(Vector4(-s, -s, -s, 0));
        positions.append(Vector4(-s, -s, +s, 0));
        positions.append(Vector4(-s, +s, -s, 0));
        positions.append(Vector4(-s, +s, +s, 0));
        positions.append(Vector4(+s, -s, -s, 0));
        positions.append(Vector4(+s, -s, +s, 0));
        positions.append(Vector4(+s, +s, -s, 0));
        positions.append(Vector4(+s, +s, +s, 0));

        Array<int> indices;
        indices.append(2, 0, 4, 2, 4, 6);
        indices.append(3, 1, 0, 3, 0, 2);
        indices.append(7, 5, 1, 7, 1, 3);
        indices.append(7, 6, 4, 7, 4, 5);
        indices.append(7, 3, 2, 7, 2, 6);
        indices.append(4, 0, 1, 4, 1, 5);

        shared_ptr<VertexBuffer> area = VertexBuffer::create(positions.size() * sizeof(Vector4), VertexBuffer::WRITE_ONCE);
        args.setAttributeArray("g3d_Vertex", AttributeArray(positions, area));
        area = VertexBuffer::create(indices.size() * sizeof(int), VertexBuffer::WRITE_ONCE);  
        args.setIndexStream(IndexStream(indices, area));

        init = false;
    }

    cubeMap->setShaderArgs(args, "tex_", Sampler::cubeMap());
    renderDevice->apply(shader, args);
    renderDevice->popState();        
}



void Draw::poly2DOutline(const Array<Vector2>& polygon, RenderDevice* renderDevice, const Color4& color) {
    if (polygon.length() == 0) {
        return;
    }
    SlowMesh slowMesh(PrimitiveType::LINES);
    slowMesh.setColor(color);
    
    for (int i = 0; i < polygon.length(); ++i) {
        slowMesh.makeVertex(polygon[i]);
        slowMesh.makeVertex(polygon[(i+1) % polygon.length()]);
    }
    slowMesh.render(renderDevice);
}


void Draw::poly2D(const Array<Vector2>& polygon, RenderDevice* renderDevice, const Color4& color) {
    if (polygon.length() == 0) {
        return;
    }
    
    SlowMesh slowMesh(PrimitiveType::TRIANGLE_FAN);
    slowMesh.setColor(color);
    for (int i = 0; i < polygon.length(); ++i) {
        slowMesh.makeVertex(polygon[i]);
    }
    slowMesh.render(renderDevice);
}




static void drawSpotLight(shared_ptr<Light> light, RenderDevice* rd){
  
    const float LIGHT_RADIUS = light->extent().length() / 2.0f;
    static const int   SLICES = 32;
    static const float delta = 2 * pif() / SLICES;
    static const float dist	 = 1.0f / (float)sqrt(2.0f);
    const bool square = light->spotSquare();
    const Color3& color = light->color / light->color.average();
    
    rd->pushState(); {
        rd->setObjectToWorldMatrix(light->frame());
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
        

        if (square) {
            SlowMesh slowMesh(PrimitiveType::TRIANGLES);
            slowMesh.setColor(color);

            //front and back of a square light
            slowMesh.makeVertex(Point3(-dist, -dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3( dist, -dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3(-dist,  dist, 0.0f) * LIGHT_RADIUS);

            slowMesh.makeVertex(Point3( dist, -dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3( dist,  dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3(-dist,  dist, 0.0f) * LIGHT_RADIUS);

            slowMesh.makeVertex(Point3(-dist, -dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3(-dist,  dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3( dist, -dist, 0.0f) * LIGHT_RADIUS);

            slowMesh.makeVertex(Point3( dist, -dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3(-dist,  dist, 0.0f) * LIGHT_RADIUS);
            slowMesh.makeVertex(Point3( dist,  dist, 0.0f) * LIGHT_RADIUS);

            slowMesh.render(rd);
        } else {
            SlowMesh slowMesh(PrimitiveType::TRIANGLES);
            slowMesh.setColor(color);

            // The front of the light itself
            for (int i = 0; i < SLICES; ++i) {
                const float angle = i * delta;
                slowMesh.makeVertex(Point3(cos(angle), sin(angle), 0.0f) * LIGHT_RADIUS);
                slowMesh.makeVertex(Point3::zero());
                slowMesh.makeVertex(Point3(cos(angle + delta), sin(angle + delta), 0.0f) * LIGHT_RADIUS);
            }

            // Back of the light
            slowMesh.setColor(Color3(0.15f));
            for (int i = 0; i < SLICES; ++i) {
                const float angle = i * delta;
                slowMesh.makeVertex(Point3(cos(angle), sin(angle), 0.0f) * LIGHT_RADIUS);
                slowMesh.makeVertex(Point3(cos(angle + delta), sin(angle + delta), 0.0f) * LIGHT_RADIUS);
                slowMesh.makeVertex(Point3::zero());
            }

            slowMesh.render(rd);
        }

        // Light cone 
        rd->setDepthWrite(false);
        const float distance = clamp(light->effectSphere().radius * 0.08f, 0.1f, 1.5f);
        const float innerRadius = LIGHT_RADIUS * 0.2f;
        const float outerRadius = max(innerRadius, (float)(tan(light->spotHalfAngle()) * distance));
        rd->setCullFace(CullFace::NONE);

        rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        SlowMesh slowMesh(PrimitiveType::TRIANGLES);
            if (square) {
                const float step = pif()/2;
                for(int i = 0; i < 4; ++i) {
                    const float angle = step * i + step/2;

                    slowMesh.setColor(Color4(color, 0.8f));
                    slowMesh.makeVertex(Point3(cos(angle + step) * innerRadius, sin(angle + step) * innerRadius, 0.0f));
                    slowMesh.makeVertex(Point3(cos(angle) * innerRadius, sin(angle) * innerRadius, 0.0f));

                    slowMesh.setColor(Color4(color, 0.0f));
                    slowMesh.makeVertex(Point3(cos(angle) * outerRadius, sin(angle) * outerRadius, -distance));

                    slowMesh.setColor(Color4(color, 0.8f));
                    slowMesh.makeVertex(Point3(cos(angle + step) * innerRadius, sin(angle + step) * innerRadius, 0.0f));

                    slowMesh.setColor(Color4(color, 0.0f));
                    slowMesh.makeVertex(Point3(cos(angle) * outerRadius, sin(angle) * outerRadius, -distance));
                    slowMesh.makeVertex(Point3(cos(angle + step) * outerRadius, sin(angle + step) * outerRadius, -distance));
                }
            } else {
                for (int i = 0; i < SLICES; ++i) {
                    const float angle = i * delta;
                    slowMesh.setColor(Color4(color, 0.8f));
                    slowMesh.makeVertex(Point3(cos(angle + delta) * innerRadius, sin(angle + delta) * innerRadius, 0.0f));
                    slowMesh.makeVertex(Point3(cos(angle) * innerRadius, sin(angle) * innerRadius, 0.0f));

                    slowMesh.setColor(Color4(color, 0.0f));
                    slowMesh.makeVertex(Point3(cos(angle) * outerRadius, sin(angle) * outerRadius, -distance));

                    slowMesh.setColor(Color4(color, 0.8f));
                    slowMesh.makeVertex(Point3(cos(angle + delta) * innerRadius, sin(angle + delta) * innerRadius, 0.0f));

                    slowMesh.setColor(Color4(color, 0.0f));
                    slowMesh.makeVertex(Point3(cos(angle) * outerRadius, sin(angle) * outerRadius, -distance));
                    slowMesh.makeVertex(Point3(cos(angle + delta) * outerRadius, sin(angle + delta) * outerRadius, -distance));
                }
            }
        slowMesh.render(rd);
    } rd->popState();
}


void Draw::visualizeCameraGeometry(shared_ptr<Camera> camera, RenderDevice* rd){
        Draw::frustum(camera->frustum(rd->viewport()), rd);
}

void Draw::camera(shared_ptr<Camera> camera, RenderDevice* rd){
    Color3 color = Color3(0.03f);

    // The box is 2 units long along the camera's Z axis,  1 unit along the other two
    // It is centered around the camera's Z axis, protruding zProtrusion distance out into the negative-Z half-space
    float boxUnitLength = 0.2f * camera->visualizationScale();
    float zProtrusion   = 0.06f * camera->visualizationScale();
    rd->pushState(); {
        rd->setObjectToWorldMatrix(camera->frame());
        
        // Draw box
        Point3 low(-boxUnitLength * 0.5f, -boxUnitLength * 0.5f, -zProtrusion);
        Point3 high(boxUnitLength * 0.5f, boxUnitLength * 0.5f, boxUnitLength * 2.0f -zProtrusion);
        Draw::box(AABox(low, high), rd, color);

    } rd->popState();


    /** Make a fake camera with near and far planes where we want our visualized frustum to be, 
            so that we can use the machinery of the camera class to build our frustum for us */
    Camera fakeCamera(*camera);
    fakeCamera.setNearPlaneZ(-0.000001f);
    /** Make the frustum get as wide as our box */
    float FOV = fakeCamera.projection().fieldOfViewAngle();
    float farPlaneZ = -boxUnitLength * (tan(0.5f * FOV) * 2.0f);
    fakeCamera.setFarPlaneZ(farPlaneZ);

    Draw::frustum(fakeCamera.frustum(rd->viewport()), rd, color);
}


void Draw::visualizeLightGeometry
   (const shared_ptr<Light>&                light, 
    RenderDevice*                           rd,
    RenderPassType                          passType, 
    const String&                           singlePassBlendedOutputMacro) {

    switch (light->type()) {
    case Light::Type::SPOT:
        {
            // Get the frustum
            shared_ptr<ShadowMap> shadowMap = light->shadowMap();
            if (notNull(shadowMap)) {
                Frustum frustum;
                shadowMap->projection().frustum(shadowMap->rect2DBounds(), frustum);
                frustum = light->frame().toWorldSpace(frustum);

                const Color3& color = light->bulbPower() / max(0.01f, light->bulbPower().max());
                Draw::frustum(frustum, rd, Color4(color, 0.5f), Color3::black());
            }

            const Sphere& s = light->effectSphere();
            if (s.radius < finf()) {
                Draw::sphere(s, rd, Color4::clear(), Color4(light->color / max(0.01f, light->color.max()), 0.5f));
            }

            break;
        }
    
    case Light::Type::OMNI:
        {
            const Sphere& s = light->effectSphere();
            if (s.radius < finf()) {
                Draw::sphere(s, rd, Color4::clear(), Color4(light->color / max(0.01f, light->color.max()), 0.5f));
            }
            break;
        }

    default:
        ;        // TODO: come up with a good visualization for directional lights
    }
}



void Draw::light
   (const shared_ptr<Light>&                light, 
    RenderDevice*                           rd, 
    RenderPassType                          passType, 
    const String&                           singlePassBlendedOutputMacro,
    float                                   dirDist) {

    if (light->type() == Light::Type::SPOT) {       
        drawSpotLight(light, rd);
    } else if (light->type() == Light::Type::OMNI) {
        // Omni-directional light
        Draw::sphere(Sphere(light->position().xyz(), light->extent().length() / 2.0f), rd, light->color, Color4::clear());
    } else {
        // Directional light
        Draw::sphere(Sphere(-light->frame().lookVector() * dirDist, (light->extent().length() / 2.0f) * dirDist), rd, light->color / max(0.01f, light->color.max()), Color4::clear());
    }
}




void Draw::axes(
    RenderDevice*       renderDevice,
    const Color4&       xColor,
    const Color4&       yColor,
    const Color4&       zColor,
    float               scale) {

    axes(CFrame(), renderDevice, xColor, yColor, zColor, scale);
}


void Draw::arrow
   (const Point3&       start,
    const Vector3&      direction,
    RenderDevice*       renderDevice,
    const Color4&       color,
    float               scale) {
    

    const Point3& tip = start + direction;
    // Create a coordinate frame at the tip
    const Vector3& u = direction.direction();
    Vector3 v;
    if (abs(u.x) < abs(u.y)) {
        v = Vector3::unitX();
    } else {
        v = Vector3::unitY();
    }
    const Vector3& w = u.cross(v).direction();
    v = w.cross(u).direction();
    const Vector3& back = tip - u * 0.3f * scale;
    
    
    SlowMesh slowMesh(PrimitiveType::TRIANGLES);
    
    slowMesh.setColor(color);

    const float r = scale * 0.1f;
    // Arrow head.
    for (int a = 0; a < SPHERE_SECTIONS; ++a) {
        float angle0 = a * (float)twoPi() / SPHERE_SECTIONS;
        float angle1 = (a + 1) * (float)twoPi() / SPHERE_SECTIONS;
        Vector3 dir0(cos(angle0) * v + sin(angle0) * w);
        Vector3 dir1(cos(angle1) * v + sin(angle1) * w);
        slowMesh.setNormal(dir0);
        slowMesh.makeVertex(tip);

        slowMesh.makeVertex(back + dir0 * r);

        slowMesh.setNormal(dir1);
        slowMesh.makeVertex(back + dir1 * r);                
    }
    

    // Back of arrow head
    const Vector3& firstVertex = back + w * r;
    slowMesh.setNormal(-u);
    Vector3 prevVertex;
    for (int a = 0; a < SPHERE_SECTIONS; ++a) {
        const float angle = a * (float)twoPi() / SPHERE_SECTIONS;
        const Vector3& dir = sin(angle) * v + cos(angle) * w;
        if (a > 0) {
            slowMesh.makeVertex(firstVertex);
            slowMesh.makeVertex(prevVertex);
            prevVertex = back + dir * r;
            slowMesh.makeVertex(prevVertex);
        } else {
            prevVertex = back + dir * r;
        }
    }
    slowMesh.render(renderDevice);
    
    lineSegment(LineSegment::fromTwoPoints(start, back), renderDevice, color, scale);
    
}


void Draw::axes
   (const CoordinateFrame&  cframe,
    RenderDevice*           renderDevice,
    const Color4&           xColor,
    const Color4&           yColor,
    const Color4&           zColor,
    float                   scale) {
    const Vector3& c = cframe.translation;
    const Vector3& x = cframe.rotation.column(0).direction() * 2 * scale;
    const Vector3& y = cframe.rotation.column(1).direction() * 2 * scale;
    const Vector3& z = cframe.rotation.column(2).direction() * 2 * scale;

    
    Draw::arrow(c, x, renderDevice, xColor, scale);   
    Draw::arrow(c, y, renderDevice, yColor, scale);
    Draw::arrow(c, z, renderDevice, zColor, scale);
  
    // Text label scale
    const float xx = -3;
    const float yy = xx * 1.4f;

    // Project the 3D locations of the labels
    Vector4 xc2D = renderDevice->project(c + x * 1.1f);
    Vector4 yc2D = renderDevice->project(c + y * 1.1f);
    Vector4 zc2D = renderDevice->project(c + z * 1.1f);

    // If coordinates are behind the viewer, transform off screen
    Vector2 x2D = (xc2D.w > 0) ? xc2D.xy() : Vector2(-2000, -2000);
    Vector2 y2D = (yc2D.w > 0) ? yc2D.xy() : Vector2(-2000, -2000);
    Vector2 z2D = (zc2D.w > 0) ? zc2D.xy() : Vector2(-2000, -2000);

    // Compute the size of the labels
    float xS = (xc2D.w > 0) ? clamp(10 * xc2D.w * scale, .1f, 5.0f) : 0;
    float yS = (yc2D.w > 0) ? clamp(10 * yc2D.w * scale, .1f, 5.0f) : 0;
    float zS = (zc2D.w > 0) ? clamp(10 * zc2D.w * scale, .1f, 5.0f) : 0;
    renderDevice->push2D(); {
        SlowMesh slowMesh(PrimitiveType::LINES);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        // X
        slowMesh.setColor(xColor * 0.8f);
        slowMesh.makeVertex(Vector2(-xx,  yy) * xS + x2D);
        slowMesh.makeVertex(Vector2( xx, -yy) * xS + x2D);
        slowMesh.makeVertex(Vector2( xx,  yy) * xS + x2D);
        slowMesh.makeVertex(Vector2(-xx, -yy) * xS + x2D);

        // Y
        slowMesh.setColor(yColor * 0.8f);
        slowMesh.makeVertex(Vector2(-xx,  yy) * yS + y2D);
        slowMesh.makeVertex(Vector2(  0,  0)  * yS + y2D);
        slowMesh.makeVertex(Vector2(  0,  0)  * yS + y2D);
        slowMesh.makeVertex(Vector2(  0, -yy) * yS + y2D);
        slowMesh.makeVertex(Vector2( xx,  yy) * yS + y2D);
        slowMesh.makeVertex(Vector2(  0,  0)  * yS + y2D);

        // Z
        slowMesh.setColor(zColor * 0.8f);    
        slowMesh.makeVertex(Vector2( xx,  yy) * zS + z2D);
        slowMesh.makeVertex(Vector2(-xx,  yy) * zS + z2D);
        slowMesh.makeVertex(Vector2(-xx,  yy) * zS + z2D);
        slowMesh.makeVertex(Vector2( xx, -yy) * zS + z2D);
        slowMesh.makeVertex(Vector2( xx, -yy) * zS + z2D);
        slowMesh.makeVertex(Vector2(-xx, -yy) * zS + z2D);
        slowMesh.render(renderDevice);
    } renderDevice->pop2D();
}


void Draw::ray(
    const Ray&          ray,
    RenderDevice*       renderDevice,
    const Color4&       color,
    float               scale) {
    Draw::arrow(ray.origin(), ray.direction(), renderDevice, color, scale);
}


void Draw::plane(
    const Plane&         plane, 
    RenderDevice*        renderDevice,
    const Color4&        solidColor,
    const Color4&        wireColor) {
		
    renderDevice->pushState();
    CoordinateFrame cframe0 = renderDevice->objectToWorldMatrix();

    Vector3 N, P;
    
    {
        float d;
        plane.getEquation(N, d);
        float distance = -d;
        P = N * distance;
    }

    CoordinateFrame cframe1(P);
    cframe1.lookAt(P + N);

    renderDevice->setObjectToWorldMatrix(cframe0 * cframe1);


    if (solidColor.a > 0.0f) {
        // Draw concentric circles around the origin.  We break them up to get good depth
        // interpolation and reasonable shading.

        renderDevice->setPolygonOffset(0.7f);

        if (solidColor.a < 1.0f) {
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        }
        SlowMesh slowMesh(PrimitiveType::TRIANGLE_STRIP);
        slowMesh.setNormal(Vector3::unitZ());
        slowMesh.setColor(solidColor);

        renderDevice->setCullFace(CullFace::NONE);
        // Infinite strip
        const int numStrips = 12;
        float r1 = 100;
        for (int i = 0; i <= numStrips; ++i) {
            float a = i * (float)twoPi() / numStrips;
            float c = cosf(a);
            float s = sinf(a);

            slowMesh.makeVertex(Vector3(c * r1, s * r1, 0));
            slowMesh.makeVertex(Vector4(c, s, 0, 0));
        }

        // Finite strips.  
        float r2 = 0;
        const int M = 4;
        for (int j = 0; j < M; ++j) {
            r2 = r1;
            r1 = r1 / 3;
            if (j == M - 1) {
                // last pass
                r1 = 0;
            }

            for (int i = 0; i <= numStrips; ++i) {
                float a = i * (float)twoPi() / numStrips;
                float c = cosf(a);
                float s = sinf(a);

                slowMesh.makeVertex(Vector3(c * r1, s * r1, 0));
                slowMesh.makeVertex(Vector3(c * r2, s * r2, 0));
            }
        }
        slowMesh.render(renderDevice);


    }

    if (wireColor.a > 0) {
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        
        SlowMesh slowMesh(PrimitiveType::LINES);
        {
            slowMesh.setColor(wireColor);
            slowMesh.setNormal(Vector3::unitZ());

            // Lines to infinity
            slowMesh.makeVertex(Vector4( 1, 0, 0, 0));
            slowMesh.makeVertex(Vector4( 0, 0, 0, 1));

            slowMesh.makeVertex(Vector4(-1, 0, 0, 0));
            slowMesh.makeVertex(Vector4( 0, 0, 0, 1));

            slowMesh.makeVertex(Vector4(0, -1, 0, 0));
            slowMesh.makeVertex(Vector4(0,  0, 0, 1));

            slowMesh.makeVertex(Vector4(0,  1, 0, 0));
            slowMesh.makeVertex(Vector4(0,  0, 0, 1));
        }


        // Horizontal and vertical lines
        const int numStrips = 10;
        const float space = 1;
        const float Ns = numStrips * space;
        for (int x = -numStrips; x <= numStrips; ++x) {
            float sx = x * space;
            slowMesh.makeVertex(Vector3( Ns, sx, 0));
            slowMesh.makeVertex(Vector3(-Ns, sx, 0));

            slowMesh.makeVertex(Vector3(sx,  Ns, 0));
            slowMesh.makeVertex(Vector3(sx, -Ns, 0));
        }

        slowMesh.render(renderDevice);
    }

    renderDevice->popState();
	
}
 


void Draw::capsule(
    const Capsule&       capsule, 
    RenderDevice*        renderDevice,
    const Color4&        solidColor,
    const Color4&        wireColor) {

    CoordinateFrame cframe(capsule.point(0));

    Vector3 Y = (capsule.point(1) - capsule.point(0)).direction();
    Vector3 X = (abs(Y.dot(Vector3::unitX())) > 0.9) ? Vector3::unitY() : Vector3::unitX();
    Vector3 Z = X.cross(Y).direction();
    X = Y.cross(Z);        
    cframe.rotation.setColumn(0, X);
    cframe.rotation.setColumn(1, Y);
    cframe.rotation.setColumn(2, Z);

    float radius = capsule.radius();
    float height = (capsule.point(1) - capsule.point(0)).magnitude();

    // Always render upright in object space
    Sphere sphere1(Vector3::zero(), radius);
    Sphere sphere2(Vector3(0, height, 0), radius);

    Vector3 top(0, height, 0);

    renderDevice->pushState();

        renderDevice->setObjectToWorldMatrix(renderDevice->objectToWorldMatrix() * cframe);

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(CullFace::FRONT);
                renderDevice->setDepthWrite(false);
            }
            SlowMesh slowMesh(PrimitiveType:: TRIANGLE_STRIP);
            slowMesh.setColor(solidColor);
            for (int k = 0; k < numPasses; ++k) {
                sphereSection(sphere1, renderDevice, solidColor, false, true);
                sphereSection(sphere2, renderDevice, solidColor, true, false);

                // Cylinder faces
                
                
                for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                    const float yaw0 = y * (float)twoPi() / SPHERE_SECTIONS;
                    Vector3 v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));
                    slowMesh.setNormal(v0);
                    slowMesh.makeVertex(v0 * radius);
                    slowMesh.makeVertex(v0 * radius + top);
                }

                
            }
            slowMesh.render(renderDevice);
        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            wireSphereSection(sphere1, renderDevice, wireColor, false, true);
            wireSphereSection(sphere2, renderDevice, wireColor, true, false);
            
            // Line around center
            SlowMesh slowMesh(PrimitiveType::LINES);
            slowMesh.setColor(wireColor);
            Vector3 center(0, height / 2, 0);
            for (int y = 0; y < WIRE_SPHERE_SECTIONS; ++y) {
                const float yaw0 = y * (float)twoPi() / WIRE_SPHERE_SECTIONS;
                const float yaw1 = (y + 1) * (float)twoPi() / WIRE_SPHERE_SECTIONS;

                Vector3 v0(cosf(yaw0), 0, sinf(yaw0));
                Vector3 v1(cosf(yaw1), 0, sinf(yaw1));

                slowMesh.setNormal(v0);
                slowMesh.makeVertex(v0 * radius + center);
                slowMesh.setNormal(v1);
                slowMesh.makeVertex(v1 * radius + center);
            }

            // Edge lines
            for (int y = 0; y < 8; ++y) {
                const float yaw = y * (float)pi() / 4;
                const Vector3 x(cosf(yaw), 0, sinf(yaw));
        
                slowMesh.setNormal(x);
                slowMesh.makeVertex(x * radius);
                slowMesh.makeVertex(x * radius + top);
            }
            slowMesh.render(renderDevice);
        }

    renderDevice->popState();
}


void Draw::cylinder
   (const Cylinder&      cylinder, 
    RenderDevice*        renderDevice,
    const Color4&        solidColor,
    const Color4&        wireColor) {

    CoordinateFrame cframe;
    
    cylinder.getReferenceFrame(cframe);

    const float radius = cylinder.radius();
    const float height = cylinder.height();

    // Always render upright in object space
    const Vector3 bot(0, -height / 2.0f, 0);
    const Vector3 top(0, height / 2.0f, 0);

    renderDevice->pushState(); {

        renderDevice->setObjectToWorldMatrix(renderDevice->objectToWorldMatrix() * cframe);

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(CullFace::FRONT);
                renderDevice->setDepthWrite(false);
            }

            for (int k = 0; k < numPasses; ++k) {

                // Top
                {
                    SlowMesh mesh(PrimitiveType::TRIANGLE_FAN);
                    mesh.setColor(solidColor);
                    mesh.setNormal(Vector3::unitY());
                    mesh.makeVertex(top);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = -y * (float)twoPi() / SPHERE_SECTIONS;
                        const Vector3& v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        mesh.makeVertex(v0 * radius + top);
                    }
                    mesh.render(renderDevice);
                }

                // Bottom
                {
                    SlowMesh mesh(PrimitiveType::TRIANGLE_FAN);
                    mesh.setColor(solidColor);
                    mesh.setNormal(-Vector3::unitY());
                    mesh.makeVertex(bot);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / SPHERE_SECTIONS;
                        const Vector3& v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        mesh.makeVertex(v0 * radius + bot);
                    }
                    mesh.render(renderDevice);
                }

                // Cylinder faces
                {
                    SlowMesh mesh(PrimitiveType::TRIANGLE_STRIP);
                    mesh.setColor(solidColor);
                    for (int y = 0; y <= SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / SPHERE_SECTIONS;
                        const Vector3& v0 = Vector3(cosf(yaw0), 0, sinf(yaw0));

                        mesh.setNormal(v0);
                        mesh.makeVertex(v0 * radius + bot);
                        mesh.makeVertex(v0 * radius + top);
                    }
                    mesh.render(renderDevice);
                }
                renderDevice->setCullFace(CullFace::BACK);
            }

        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            // Line around center
            {
                SlowMesh mesh(PrimitiveType::LINES);
                mesh.setColor(wireColor);

                for (int z = 0; z < 3; ++z) {
                    for (int y = 0; y < WIRE_SPHERE_SECTIONS; ++y) {
                        const float yaw0 = y * (float)twoPi() / WIRE_SPHERE_SECTIONS;
                        const float yaw1 = (y + 1) * (float)twoPi() / WIRE_SPHERE_SECTIONS;

                        const Vector3 v0(cos(yaw0), 0, sin(yaw0));
                        const Vector3 v1(cos(yaw1), 0, sin(yaw1));

                        mesh.setNormal(v0);
                        mesh.makeVertex(v0 * radius);
                        mesh.setNormal(v1);
                        mesh.makeVertex(v1 * radius);
                    }
                }

                // Edge lines
                for (int y = 0; y < 8; ++y) {
                    const float yaw = y * (float)pi() / 4;
                    const Vector3 x(cos(yaw), 0, sin(yaw));
                    const Vector3 xr = x * radius;
    
                    // Side
                    mesh.setNormal(x);
                    mesh.makeVertex(xr + bot);
                    mesh.makeVertex(xr + top);

                    // Top
                    mesh.setNormal(Vector3::unitY());
                    mesh.makeVertex(top);
                    mesh.makeVertex(xr + top);

                    // Bottom
                    mesh.setNormal(Vector3::unitY());
                    mesh.makeVertex(bot);
                    mesh.makeVertex(xr + bot);
                }
                mesh.render(renderDevice);
            }
        }
    }
    renderDevice->popState();
}

    
void Draw::vertexNormals
   (const MeshAlg::Geometry&    geometry,
    RenderDevice*               renderDevice,
    const Color4&               color,
    float                       scale) {

		
    renderDevice->pushState();
        SlowMesh slowMesh(PrimitiveType::LINES);
        slowMesh.setColor(color);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        const Array<Vector3>& vertexArray = geometry.vertexArray;
        const Array<Vector3>& normalArray = geometry.normalArray;

        const float D = clamp(5.0f / ::powf((float)vertexArray.size(), 0.25f), 0.1f, .8f) * scale;
        
        for (int v = 0; v < vertexArray.size(); ++v) {
            slowMesh.makeVertex(vertexArray[v] + normalArray[v] * D);
            slowMesh.makeVertex(vertexArray[v]);
        }

        for (int v = 0; v < vertexArray.size(); ++v) {
            slowMesh.makeVertex(vertexArray[v] + normalArray[v] * D * .96f);
            slowMesh.makeVertex(vertexArray[v] + normalArray[v] * D * .84f);
        }

        for (int v = 0; v < vertexArray.size(); ++v) {
            slowMesh.makeVertex(vertexArray[v] + normalArray[v] * D * .92f);
            slowMesh.makeVertex(vertexArray[v] + normalArray[v] * D * .84f);
        }
        slowMesh.render(renderDevice);
    renderDevice->popState();
	
}


void Draw::vertexVectors(
    const Array<Vector3>&       vertexArray,
    const Array<Vector3>&       directionArray,
    RenderDevice*               renderDevice,
    const Color4&               color,
    float                       scale) {

    debugAssert(vertexArray.size() >= directionArray.size());
    for (int v = 0; v < vertexArray.size(); ++v) {
        arrow(vertexArray[v], directionArray[v], renderDevice, color, scale);
    }
}


void Draw::line(
    const Line&         line,
    RenderDevice*       renderDevice,
    const Color4&       color) {
	
    SlowMesh slowMesh(PrimitiveType::LINE_STRIP);
    slowMesh.setColor(color);
    renderDevice->pushState();
        renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        Vector3 v0 = line.point();
        Vector3 d  = line.direction();
        // Off to infinity
        slowMesh.makeVertex(Vector4(-d, 0));

        for (int i = -10; i <= 10; i += 2) {
            slowMesh.makeVertex(v0 + d * (float)i * 100.0f);
        }

        // Off to infinity
        slowMesh.makeVertex(Vector4(d, 0));
        slowMesh.render(renderDevice);
    renderDevice->popState();
	
}


void Draw::lineSegment(
    const LineSegment&  lineSegment,
    RenderDevice*       renderDevice,
    const Color4&       color,
    float               scale) {

    renderDevice->pushState(); {
        
        SlowMesh slowMesh(PrimitiveType::LINES);
        slowMesh.setColor(color);

        // Compute perspective line width
        Vector3 v0 = lineSegment.point(0);
        Vector3 v1 = lineSegment.point(1);

        // Find the object space vector perpendicular to the line
        // that points closest to the eye.
        const Vector3& eye = renderDevice->objectToWorldMatrix().pointToObjectSpace(renderDevice->cameraToWorldMatrix().translation);
        const Vector3& E = eye - v0;
        const Vector3& V = v1 - v0;
        const Vector3& U = E.cross(V);
        const Vector3& N = V.cross(U).direction();

        //renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
        
        slowMesh.setNormal(N);
        slowMesh.makeVertex(v0);
        slowMesh.makeVertex(v1);
        slowMesh.render(renderDevice);
        
    } renderDevice->popState();
}


void Draw::box(
    const AABox&        _box,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {

    box(Box(_box), renderDevice, solidColor, wireColor);
}

void Draw::boxes(
        const Array<AABox>& aaboxes,
        RenderDevice*       renderDevice,
        const Color4&       solidColor,
        const Color4&       wireColor) {
    Array<Box> boxen;
    boxen.reserve(aaboxes.size());
    for(int i = 0; i < aaboxes.size(); ++i) {
        Box& box = boxen.next();
        box = Box(aaboxes[i]);
    }
    Draw::boxes(boxen, renderDevice, solidColor, wireColor);
}

void Draw::boxes(
    const Array<Box>& boxes,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {
    renderDevice->pushState();

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(CullFace::FRONT);
                renderDevice->setDepthWrite(false);
            } else {
                renderDevice->setCullFace(CullFace::BACK);
            }
            SlowMesh slowMesh(PrimitiveType::TRIANGLES);
            slowMesh.setColor(solidColor);
            for (int k = 0; k < numPasses; ++k) {
                for (int j = 0; j < boxes.size(); ++j) {
                    for (int i = 0; i < 6; ++i) {
                        Vector3 v0, v1, v2, v3;
                        boxes[j].getFaceCorners(i, v0, v1, v2, v3);
                         
                        Vector3 n = (v1 - v0).cross(v3 - v0);
                        slowMesh.setNormal(n.direction());
                        slowMesh.makeVertex(v0);
                        slowMesh.makeVertex(v1);
                        slowMesh.makeVertex(v2);
                        slowMesh.makeVertex(v0);
                        slowMesh.makeVertex(v2);
                        slowMesh.makeVertex(v3);       
                    }
                }
                slowMesh.render(renderDevice);
                renderDevice->setCullFace(CullFace::BACK);
            }
        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            // Wire frame
            renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            SlowMesh slowMesh(PrimitiveType::LINES);
            slowMesh.setColor(wireColor);
            for (int b = 0; b < boxes.size(); ++b) {
                const Box& box = boxes[b];
                const Vector3& c = box.center();
                Vector3 v;
                // Front and back
                for (int i = 0; i < 8; i += 4) {
                    for(int j = 0; j < 4; j += 3){
                        v = box.corner(i + j);
                        slowMesh.setNormal((v - c).direction());
                        slowMesh.makeVertex(v);
                        
                        v = box.corner(i + 1);
                        slowMesh.setNormal((v - c).direction());
                        slowMesh.makeVertex(v);
                        
                        
                        v = box.corner(i + j);
                        slowMesh.setNormal((v - c).direction());
                        slowMesh.makeVertex(v);
                        
                        v = box.corner(i + 2);
                        slowMesh.setNormal((v - c).direction());
                        slowMesh.makeVertex(v);
                    }
                }
				
                // Sides
                for (int i = 0; i < 4; ++i) {
                    v = box.corner(i);
                    slowMesh.setNormal((v - c).direction());
                    slowMesh.makeVertex(v);

                    v = box.corner(i + 4);
                    slowMesh.setNormal((v - c).direction());
                    slowMesh.makeVertex(v);
                }
            }
				
            slowMesh.render(renderDevice);
        }
    renderDevice->popState();
}


void Draw::box
   (const Box&          box,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {

    renderDevice->pushState();

        if (solidColor.a > 0) {
            int numPasses = 1;

            if (solidColor.a < 1) {
                // Multiple rendering passes to get front/back blending correct
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                numPasses = 2;
                renderDevice->setCullFace(CullFace::FRONT);
                renderDevice->setDepthWrite(false);
            } else {
                renderDevice->setCullFace(CullFace::BACK);
            }
            SlowMesh slowMesh(PrimitiveType::TRIANGLES);
            slowMesh.setColor(solidColor);
            for (int k = 0; k < numPasses; ++k) {
                for (int i = 0; i < 6; ++i) {
                    Vector3 v0, v1, v2, v3;
                    box.getFaceCorners(i, v0, v1, v2, v3);

                    Vector3 n = (v1 - v0).cross(v3 - v0);
                    slowMesh.setNormal(n.direction());
                    slowMesh.makeVertex(v0);
                    slowMesh.makeVertex(v1);
                    slowMesh.makeVertex(v2);
                    slowMesh.makeVertex(v0);
                    slowMesh.makeVertex(v2);
                    slowMesh.makeVertex(v3);
                }
                slowMesh.render(renderDevice);
                renderDevice->setCullFace(CullFace::BACK);
            }
        }

        if (wireColor.a > 0) {
            renderDevice->setDepthWrite(true);
            renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

            Vector3 c = box.center();
            Vector3 v;

            // Wire frame
            renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
            SlowMesh slowMesh(PrimitiveType::LINES);
            slowMesh.setColor(wireColor);
            // Front and back
            for (int i = 0; i < 8; i += 4) {
                for(int j = 0; j < 4; j += 3){
					v = box.corner(i + j);
					slowMesh.setNormal((v - c).direction());
					slowMesh.makeVertex(v);

					v = box.corner(i + 1);
					slowMesh.setNormal((v - c).direction());
					slowMesh.makeVertex(v);


					v = box.corner(i + j);
					slowMesh.setNormal((v - c).direction());
					slowMesh.makeVertex(v);

					v = box.corner(i + 2);
					slowMesh.setNormal((v - c).direction());
					slowMesh.makeVertex(v);
				}
                    
                    
            }
				
            // Sides
            for (int i = 0; i < 4; ++i) {
                v = box.corner(i);
                slowMesh.setNormal((v - c).direction());
                slowMesh.makeVertex(v);

                v = box.corner(i + 4);
                slowMesh.setNormal((v - c).direction());
                slowMesh.makeVertex(v);
            }
				
            slowMesh.render(renderDevice);
        }
    renderDevice->popState();
}


void Draw::wireSphereSection(
    const Sphere&       sphere,
    RenderDevice*       renderDevice,
    const Color4&       color,
    bool                top,
    bool                bottom) {
    
    int sections = WIRE_SPHERE_SECTIONS;
    int start = top ? 0 : (sections / 2);
    int stop = bottom ? sections : (sections / 2);

    renderDevice->pushState();
        renderDevice->setDepthTest(RenderDevice::DEPTH_LEQUAL);
        renderDevice->setCullFace(CullFace::BACK);
        renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

        float radius = sphere.radius;
        const Vector3& center = sphere.center;
        SlowMesh slowMesh(PrimitiveType::LINES);
        slowMesh.setColor(color);
        // Wire frame
        for (int y = 0; y < 8; ++y) {
            const float yaw = y * (float)pi() / 4;
            const Vector3 x(cos(yaw) * radius, 0, sin(yaw) * radius);
            //const Vector3 z(-sin(yaw) * radius, 0, cos(yaw) * radius);

                for (int p = start; p <= stop; ++p) {
                    const float pitch0 = p * (float)pi() / (sections * 0.5f);

                    Vector3 v0 = cosf(pitch0) * x + Vector3::unitY() * radius * sinf(pitch0);
                    slowMesh.setNormal(v0.direction());
                    slowMesh.makeVertex(v0 + center);
                    // Emulate line strip
                    if (p != start && p!= stop) {
                        slowMesh.makeVertex(v0 + center);
                    }
                }
        }

        
        int a = bottom ? -1 : 0;
        int b = top ? 1 : 0; 
        for (int p = a; p <= b; ++p) {
            const float pitch = p * (float)pi() / 6;

            for (int y = 0; y <= sections; ++y) {
                const float yaw0 = y * (float)twoPi() / (sections * 0.5f);
                Vector3 v0 = Vector3(cos(yaw0) * cos(pitch), sin(pitch), sin(yaw0) * cos(pitch)) * radius;
                slowMesh.setNormal(v0.direction());
                slowMesh.makeVertex(v0 + center);
                // Emulate line strip
                if (y != start && y != sections) {
                    slowMesh.makeVertex(v0 + center);
                }
            }
        }
        slowMesh.render(renderDevice);
    renderDevice->popState();
}


void Draw::sphereSection(
    const Sphere&       sphere,
    RenderDevice*       renderDevice,
    const Color4&       color,
    bool                top,
    bool                bottom) {

    // The first time this is invoked we create a series of quad strips in a vertex array.
    // Future calls then render from the array. 

    CoordinateFrame cframe = renderDevice->objectToWorldMatrix();

    // Auto-normalization will take care of normal length
    cframe.translation += cframe.rotation * sphere.center;
    cframe.rotation = cframe.rotation * sphere.radius;

    renderDevice->pushState();
        renderDevice->setObjectToWorldMatrix(cframe);

        static bool initialized = false;
        static AttributeArray vbuffer;
        static IndexStream bottomIndexBuffer;
        if (! initialized) {
            // The normals are the same as the vertices for a sphere
            Array<Vector3> vertex;


            for (int p = 0; p < SPHERE_PITCH_SECTIONS; ++p) {
                const float pitch0 = p * (float)pi() / (SPHERE_PITCH_SECTIONS*2);
                const float pitch1 = (p + 1) * (float)pi() / (SPHERE_PITCH_SECTIONS*2);

                const float sp0 = sin(pitch0);
                const float sp1 = sin(pitch1);
                const float cp0 = cos(pitch0);
                const float cp1 = cos(pitch1);

                for (int y = 0; y <= SPHERE_YAW_SECTIONS; ++y) {
                    const float yaw = -y * (float)twoPi() / SPHERE_YAW_SECTIONS;

                    const float cy = cos(yaw);
                    const float sy = sin(yaw);

                    const Vector3 v0(cy * sp0, cp0, sy * sp0);
                    const Vector3 v1(cy * sp1, cp1, sy * sp1);

                    vertex.append(v0, v1);
                    
                }

                const Vector3& degen = Vector3(1.0f * sp1, cp1, 0.0f * sp1);
                vertex.append(degen, degen);
                  
            }

            shared_ptr<VertexBuffer> area = VertexBuffer::create(vertex.size() * sizeof(Vector3), VertexBuffer::WRITE_ONCE);
            vbuffer = AttributeArray(vertex, area);

            Array<uint16> bottomIndices; // Top indices are just sequential
            for (int i = 0; i < vertex.size(); ++i) {
                bottomIndices.append(i);
            }
            area = VertexBuffer::create(bottomIndices.size() * sizeof(uint16), VertexBuffer::WRITE_ONCE);
            bottomIndexBuffer = IndexStream(bottomIndices, area);
            initialized = true;
        }
        Args args;
        args.setUniform("color", color);
        args.setAttributeArray("g3d_Vertex", vbuffer);
        args.setPrimitiveType(PrimitiveType::TRIANGLE_STRIP);
        if (top) {
            args.setMacro("BOTTOM", false);
            LAUNCH_SHADER("Draw_sphereSection.*", args);
        }
        if (bottom) {
            args.setMacro("BOTTOM", true);
            LAUNCH_SHADER("Draw_sphereSection.*", args);
        }

    renderDevice->popState();

}


void Draw::sphere(
    const Sphere&       sphere,
    RenderDevice*       renderDevice,
    const Color4&       solidColor,
    const Color4&       wireColor) {

    if (solidColor.a > 0) {
        renderDevice->pushState();

            int numPasses = 1;

            if (solidColor.a < 1) {
                numPasses = 2;
                renderDevice->setCullFace(CullFace::FRONT);
                renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
                renderDevice->setDepthWrite(false);
            } else {
                renderDevice->setCullFace(CullFace::BACK);
            }

            if (wireColor.a > 0) {
                renderDevice->setPolygonOffset(3);
            }

            for (int k = 0; k < numPasses; ++k) {
                sphereSection(sphere, renderDevice, solidColor, true, true);
                renderDevice->setCullFace(CullFace::BACK);
            }
        renderDevice->popState();
    }

    if (wireColor.a > 0) {
        wireSphereSection(sphere, renderDevice, wireColor, true, true);
    }
}


void Draw::rect2D
(const Rect2D&       rect,
 RenderDevice*       rd,
 const Color4&       color,
 const shared_ptr<Texture>& textureMap,
 const Sampler&      sampler,
 bool                invertY) {
    String hint;
    Args args;
    if (notNull(textureMap)) {
        args.setMacro("HAS_TEXTURE", 1);
        args.setUniform("textureMap", textureMap, sampler);
        hint = textureMap->name();
    } else {
        args.setMacro("HAS_TEXTURE", 0);
    }
    args.setUniform("color", color);
    debugAssertGLOk();

    if (invertY) {
        const size_t padding = 1000;
        const shared_ptr<VertexBuffer>& dataArea = VertexBuffer::create((sizeof(Vector2) + sizeof(Vector2)) * 4 + padding, VertexBuffer::WRITE_EVERY_FRAME);
        AttributeArray vertexArray(Point2(), 4, dataArea);
        AttributeArray texCoordArray(Point2(), 4, dataArea);

        {
            Point2* vertex = (Point2*)vertexArray.mapBuffer(GL_WRITE_ONLY);
            vertex[0] = rect.x0y0();
            vertex[1] = rect.x0y1();
            vertex[2] = rect.x1y0();
            vertex[3] = rect.x1y1();
            vertexArray.unmapBuffer();
        }

        {
            Point2* texCoord = (Point2*)texCoordArray.mapBuffer(GL_WRITE_ONLY);
            texCoord[0] = Point2(0, 1);
            texCoord[1] = Point2(0, 0);
            texCoord[2] = Point2(1, 1);
            texCoord[3] = Point2(1, 0);
            texCoordArray.unmapBuffer();
        }

        args.setAttributeArray("g3d_Vertex", vertexArray);
        args.setAttributeArray("g3d_TexCoord0", texCoordArray);
        args.setPrimitiveType(PrimitiveType::TRIANGLE_STRIP);
        args.setNumIndices(4);
    } else {
        args.setRect(rect, 0);
    }
    LAUNCH_SHADER_WITH_HINT("unlit.*", args, hint);
}


void Draw::rect2DBorder
   (const class Rect2D& rect,
    RenderDevice* rd,
    const Color4& color,
    float outerBorder,
    float innerBorder) {

    //
    //
    //   **************************************
    //   **                                  **
    //   * **                              ** *
    //   *   ******************************   *
    //   *   *                            *   *
    //
    //
    const Rect2D outer = rect.border(outerBorder);
    const Rect2D inner = rect.border(-innerBorder); 

    rd->pushState();
    SlowMesh slowMesh(PrimitiveType::TRIANGLE_STRIP);
    slowMesh.setColor(color);
    
    for (int i = 0; i < 5; ++i) {
        int j = i % 4;
        slowMesh.makeVertex(outer.corner(j));
        slowMesh.makeVertex(inner.corner(j));
    }

    slowMesh.render(rd);
    rd->popState();
}


void sendFrustumGeometry(const Frustum& frustum, RenderDevice* rd, const Color4& color, bool lines) {

    SlowMesh slowMesh(lines ? PrimitiveType::LINES : PrimitiveType::TRIANGLES);
    slowMesh.setColor(color);
    for (int f = 0; f < frustum.faceArray.size(); ++f) {
        const Frustum::Face& face = frustum.faceArray[f];
        slowMesh.setNormal(face.plane.normal());
        if (lines) {
            for (int v = 0; v < 4; ++v) {
                slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[v]]);
                slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[(v+1) % 4]]);
            } 
        } else {
            slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[0]]);
            slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[1]]);
            slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[3]]);

            slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[1]]);
            slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[2]]);
            slowMesh.makeVertex(frustum.vertexPos[face.vertexIndex[3]]);
        }
    }
    slowMesh.render(rd);
}


void Draw::frustum
   (const Frustum& frustum, 
    RenderDevice*  rd,
    const Color4&  solidColor, 
    const Color4&  wireColor) {
    rd->pushState();

    rd->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);

    if (wireColor.a > 0) {
        sendFrustumGeometry(frustum, rd, wireColor, true);
    }

    if (solidColor.a > 0) {
        rd->setCullFace(CullFace::FRONT);
        if (solidColor.a < 1) {
            rd->setDepthWrite(false);
        }

        for (int i = 0; i < 2; ++i) {
            sendFrustumGeometry(frustum, rd, solidColor, false);
            rd->setCullFace(CullFace::BACK);
        }
    }

    rd->popState();
}


void Draw::histogram(
        const Rect2D&       area,
        const Array<float>& values,
        float               binSize,
        RenderDevice*       rd,
        const shared_ptr<GFont>      font,
        const Color4&       boxColor,
        const Color4&       labelColor,
        float               fontSize,
        bool                logScale) {
    alwaysAssertM(values.size() > 1, "Can't draw a histogram of 0 values");
    float minVal = finf();
    float maxVal = -finf();
    float meanVal = 0.0f;
    for (int i = 0; i < values.size(); ++i) {
        minVal = min(minVal, values[i]);
        maxVal = max(maxVal, values[i]);
        meanVal += values[i];
    }
    meanVal /= float(values.size());
    const int numBins = (int)ceil(maxVal / binSize);

    
    Array<int> freqData;
    freqData.resize(numBins);
    freqData.setAll(0);
    int maxFreq = 0;
    for (int i = 0; i < values.size(); ++i) {
        for (int j = 0; j < numBins; ++j) {
            if (minVal + (j*binSize) <= values[i] && values[i] < minVal + (j + 1)*binSize) {
                ++freqData[j];
                maxFreq = max(maxFreq, freqData[j]);
            }
        }
    }
    /** Convert to float if using log scales
      * also add 1 to non-zero heights so that 1 and 0 can be distinguished.
      */
    Array<float> freqDataLog;
    if (logScale) {
        freqDataLog.resize(freqData.size());
        for (int i = 0; i < freqData.size(); ++i) {
            if (freqData[i] > 0) {
                freqDataLog[i] = log10(float(freqData[i] + 1.0f));
            } else {
                freqDataLog[i] = 0;
            }
        }
    }
    const float logMaxFreq = log10(float(maxFreq) + 1.0f);
    const float bottomHeight= font->bounds(format("bin size=%f %f", minVal + (binSize / 2), maxVal + (binSize / 2)), fontSize).y;
    const float leftWidth   = font->bounds(format("%d", maxFreq), fontSize).x;
    const float binWidth    = (area.width()  - leftWidth)  / numBins;
    const float heightScale = (area.height() - bottomHeight) / (logScale ? logMaxFreq : maxFreq);
    for(int i = 0; i < freqData.size(); ++i) {
        float freq = logScale ? freqDataLog[i] : float(freqData[i]);
        Draw::rect2D(Rect2D::xywh(area.x0() + i*binWidth, (area.y1() - bottomHeight) - (freq * heightScale), max(1.0f, binWidth - 2.0f), freq * heightScale), rd, boxColor);
    }
    
    font->draw2D(rd, format("%f", minVal + (binSize / 2)), Point2(area.x0(), area.y1() - bottomHeight), fontSize, labelColor);
    font->draw2D(rd, format("%f", maxVal - (binSize / 2)), Point2(area.x1() - leftWidth - binWidth, area.y1() - bottomHeight), fontSize, labelColor);
    font->draw2D(rd, format("bin size = %f", binSize), Point2(area.x0() + binWidth * (numBins / 2), area.y1() - bottomHeight), fontSize, labelColor);
    font->draw2D(rd, format("%d", maxFreq), Point2(area.x1() - leftWidth, area.y0() + bottomHeight), fontSize, labelColor);
    font->draw2D(rd, "0", Point2(area.x1() - leftWidth, area.y1() - bottomHeight), fontSize, labelColor);
}

}

