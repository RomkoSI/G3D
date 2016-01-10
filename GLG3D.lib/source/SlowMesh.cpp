/**
 \file GLG3D.lib/source/SlowMesh.cpp

  \maintainer Michael Mara, http://www.illuminationcodified.com

  \created 2014-04-29
  \edited  2014-07-26
 */


#include "GLG3D/RenderDevice.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Shader.h"
#include "GLG3D/SlowMesh.h"

namespace G3D {

    /** Overrides the current PrimitiveType, all of the create vertices will be of 
        said type, whether made before or after this call */
    void SlowMesh::setPrimitveType(const PrimitiveType& p) {
        m_primitiveType = p;
    }

    /** Sets the texture to use for rendering */
    void SlowMesh::setTexture(const shared_ptr<Texture> t) {
        m_texture = t;
    }

    /** Change the currently set texCoord state, defaulted to (0,0) */
    void SlowMesh::setTexCoord(const Vector2& texCoord){
        m_currentTexCoord = texCoord;
    }

    /** Change the currently set color state, defaulted to black */
    void SlowMesh::setColor(const Color3& color) {
        m_currentColor = color;
    }

    void SlowMesh::setColor(const Color4& color) {
        m_currentColor = color;
    }

    /** Change the currently set color state, defaulted to (0,0,1) */
    void SlowMesh::setNormal(const Vector3& normal) {
        m_currentNormal = normal;
    }

    /** Construct a CPUVertex given the current texCoord, color, and normal state */
    void SlowMesh::makeVertex(const Vector2& vertex) {
        makeVertex(Vector3(vertex, 0));
    }

    void SlowMesh::makeVertex(const Vector3& vertex) {
        makeVertex(Vector4(vertex, 1));
    }

    void SlowMesh::makeVertex(const Vector4& vertex) {
        Vertex& v = m_cpuVertexArray.next();
        v.position  = vertex;
        v.normal    = m_currentNormal;
        v.texCoord  = m_currentTexCoord;
        v.color     = m_currentColor;
    }


    void SlowMesh::copyToGPU(
            AttributeArray&               vertex, 
            AttributeArray&               normal, 
            AttributeArray&               texCoord0,
            AttributeArray&               vertexColors) const {
        #   define OFFSET(field) ((size_t)(&dummy.field) - (size_t)&dummy)

        const int numVertices = m_cpuVertexArray.size();
        if (numVertices > 0) {
            int cpuVertexByteSize = sizeof(Vertex) * numVertices;

            const int padding = 16;

            shared_ptr<VertexBuffer> buffer = VertexBuffer::create(cpuVertexByteSize + padding, VertexBuffer::WRITE_ONCE);
        
            AttributeArray all(cpuVertexByteSize, buffer);

            Vertex dummy;
            vertex          = AttributeArray(dummy.position,    numVertices, all, OFFSET(position), (int)sizeof(Vertex));
            normal          = AttributeArray(dummy.normal,      numVertices, all, OFFSET(normal),   (int)sizeof(Vertex));
            texCoord0       = AttributeArray(dummy.texCoord,    numVertices, all, OFFSET(texCoord), (int)sizeof(Vertex));
            vertexColors    = AttributeArray(dummy.color,       numVertices, all, OFFSET(color),    (int)sizeof(Vertex));

            // Copy all interleaved data at once
            Vertex* dst = (Vertex*)all.mapBuffer(GL_WRITE_ONLY);

            System::memcpy(dst, m_cpuVertexArray.getCArray(), cpuVertexByteSize);

            all.unmapBuffer();
            dst = NULL;
        }

        #undef OFFSET
    }


    /** Constructs a VertexBuffer from the CPUVertexArray, and renders it using a simple shader that
        mimics the old fixed-function pipeline */
    void SlowMesh::render(RenderDevice* rd) const {
        debugAssertGLOk();
        if (m_cpuVertexArray.size() > 0) {
            AttributeArray vertex, normal, texCoord0, vertexColors;
            copyToGPU(vertex, normal, texCoord0, vertexColors);
            Args args;
            if (notNull(m_texture)) {
                args.setMacro("HAS_TEXTURE", 1);
                args.setUniform("textureMap", m_texture, Sampler::video());
            } else {
                args.setMacro("HAS_TEXTURE", 0);
            }

            args.setPrimitiveType(m_primitiveType);
            args.setAttributeArray("g3d_Vertex", vertex);
            args.setAttributeArray("g3d_Normal", normal);
            args.setAttributeArray("g3d_TexCoord0", texCoord0);
            args.setAttributeArray("g3d_Color", vertexColors);
            LAUNCH_SHADER("SlowMesh_render.*", args);
        }
    }

	void SlowMesh::reserveSpace(int numVertices) {
		m_cpuVertexArray.reserve(numVertices);
	}
}
