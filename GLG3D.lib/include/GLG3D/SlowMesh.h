/**
  \file GLG3D/SlowMesh.h

  Used to convert old immediate mode OpenGL code to OpenGL 3+ core. 
  Named as such to discourage use by end-users of G3D.

  Old immediate mode:
  rd->setTexture(0, t);
  glBegin(PrimitiveType::TRIANGLES); {
      glColor(...);
      glMultiTexCoord(0, ...);
      glVertex(...);
      glMultiTexCoord(0, ...);
      glVertex(...);
      glMultiTexCoord(0, ...);
      glVertex(...);
  } glEnd();

  New Code:
  SlowMesh m(PrimitiveType::TRIANGLES); {
      m.setTexture(t);
      m.setColor(...);
      m.setTexCoord(...);
      m.makeVertex(...);
      m.setTexCoord(...);
      m.makeVertex(...);
      m.setTexCoord(...);
      m.makeVertex(...);
  } m.render(rd);


  Alternatively, if you already have a CPUVertexArray:

  SlowMesh m(PrimitiveType::TRIANGLES, cpuVertexArray, texture);
  m.render(rd);

  \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2014-04-29
  \edited  2014-04-29
*/
#ifndef GLG3D_SlowMesh_h
#define GLG3D_SlowMesh_h

#include "G3D/constants.h"
#include "G3D/Color3.h"
#include "G3D/Color4.h"
#include "G3D/Vector2.h"
#include "G3D/Vector3.h"
#include "G3D/Vector4.h"

namespace G3D {

class RenderDevice;
class Texture;

class SlowMesh {

    struct Vertex {
        Vector4 position;
        Vector3 normal;
        Vector2 texCoord;
        Color4 color;
    };

    Array<Vertex>       m_cpuVertexArray;
    PrimitiveType       m_primitiveType;
    shared_ptr<Texture> m_texture;

    Color4              m_currentColor;
    Vector2             m_currentTexCoord;
    Vector3             m_currentNormal;
    float               m_pointSize;

    void copyToGPU(
            AttributeArray&   vertex, 
            AttributeArray&   normal, 
            AttributeArray&   texCoord0,
            AttributeArray&   vertexColors) const;

public:
    SlowMesh(PrimitiveType p, const shared_ptr<Texture>& t = shared_ptr<Texture>()) : m_primitiveType(p), m_texture(t),
        m_currentColor(Color3::black()), m_currentTexCoord(0,0), m_currentNormal(Vector3::unitZ()), m_pointSize(100.0f) {}

    /** Overrides the current PrimitiveType, all of the create vertices will be of 
        said type, whether made before or after this call */
    void setPrimitveType(const PrimitiveType& p);

    void setPointSize(const float size);

    /** Sets the texture to use for rendering */
    void setTexture(const shared_ptr<Texture> t);

    /** Change the currently set texCoord state, defaulted to (0,0) */
    void setTexCoord(const Vector2& texCoord);
    /** Change the currently set color state, defaulted to black */
    void setColor(const Color3& color);
    void setColor(const Color4& color);
    /** Change the currently set color state, defaulted to (0,0,1) */
    void setNormal(const Vector3& normal);

    /** Construct a Vertex given the current texCoord, color, and normal state */
    void makeVertex(const Vector2& vertex);
    void makeVertex(const Vector3& vertex);
    void makeVertex(const Vector4& vertex);

    /** Constructs a VertexBuffer from the vertex array, and renders it using a simple shader that
        mimics the old fixed-function pipeline */
    void render(RenderDevice* rd) const;

	/** Call to reserve space in the CPU array for \param numVertices Vertices, to avoid continous reallocation. 
		This is to make use of SlowMesh slightly faster for large vertex counts, but custom code bypassing SlowMesh should be used for
		optimal performance. */
	void reserveSpace(int numVertices);
};

}

#endif
