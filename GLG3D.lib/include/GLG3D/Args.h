/**
 \file GLG3D/Args.h
  
 \maintainer Michael Mara, http://www.illuminationcodified.com

 \created 2012-06-16
 \edited  2014-03-27

 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#ifndef G3D_Args_h
#define G3D_Args_h


#include "G3D/platform.h"
#include "G3D/constants.h"
#include "G3D/SmallArray.h"
#include "G3D/Matrix.h"
#include "G3D/Vector4.h"
#include "G3D/Rect2D.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/UniformTable.h"

namespace G3D {

/** \brief Arguments to use when running a Shader. 
 
 Many classes (including Texture, Material, UniversalSurface, and Light) provide a setArgs method that
 assigns a set of arguments to an Args class.

 Many classes (including Framebuffer, ArticulatedModel::Pose, and LightingEnvironment) provide
 a UniformTable field that allows them to impose additional values on the
 Args used for shaders that incorporate them. See the documentation on VisibleEntity::create 
 for information about a common case of specifying shader arguments in a file.

 There are three major types of arguments stored:

 \section a Uniform Args
 These correspond exactly to glsl uniform variables. They are constant throughout one shader application.
 These are set using the overloaded setUniform() functions. The first parameter of this 
 function is the uniform name, the second is the value.
 
 We currently support a subset of the types in glsl, including all floating-point vector and 
 matrix types (vecX, matNxM), most scalar types (float, int, double, uint, bool), and sampler 
 types (which have the CPU type G3D::Texture).

 \section b Preamble and Macro Args
 The preamble is an arbitrary string that is prepended to the code of all stages of the shader before compilation 
 (before the \#version string, which must be the first line of any GLSL program, is prepended). This allows
 for the addition of arbitrary glsl code to your shader at compile-time.

 Macro Args are set using the overloaded setMacro() functions. The first parameter of this 
 function is the uniform name, the second is the value. The values all get formatted into strings internally,
 and we support string types as a value of macro arguments. This in theory allows you to write arbitrary glsl
 code inside the value of a macro arg, but it is more sensical to put such things in the preamble if they are necessary.

 Macro Args are appended to the preamble before the preamble is prepended to the shader code.

 Examples:

 <code>setMacro("USE_LAMBERTIAN", 0);</code>

 becomes

 <code>"#define USE_LAMBERTIAN 0\n"</code>

  (note that we also coerce booleans into 1 or 0)
 
 <code>setMacro("MACRO_VECTOR", Vector4::clear());</code>
 becomes
 <code>"#define MACRO_VECTOR vec4(0.0, 0.0, 0.0, 0.0)\n"</code>

 Since GLSL has a preprocessor that recognizes \#defines, macro args can be used as compile-time constants.

 NOTE: Shader, when being applied with an Args object, first checks the result preambleAndMacroString(), and uses that as a key into a cache of compiled shader program objects.
 If such an object is found, the compilation step is skipped, and Shader uses the program object from the cache, otherwise it is compiled and added to the cache. 

 The number of possible shaders to compile is exponential in the number of macro arguments, use them sparingly!


 \section c Stream Args
 Steam Args correspond to GLSL vertex attributes. 

    We support standard OpenGL attributes using pointers into VBOs (which correspond to G3D::AttributeArray), through the setAttributeArray() function
    The first parameter is the name of the (generic) vertex attribute. 

    The second parameter is the AttributeArray to use as the data for the attribute.

    Indexed rendering is used if the index stream is set with setIndexStream() before Shader application, otherwise, sequential indices are used up to the number of elements in
    the smallest set AttributeArray.

    Note that if no vertex shader is used we provide default.vrt, which uses g3d_Vertex and g3d_TexCoord0, transforming the position by the g3d_ObjectToScreenMatrix.
    This is useful mostly in the context of our alternate rendering mode using setRect, to set up a screen-space shader pass 
    (basically a compute shader masquerading as a compute shader).
 \sa Shader
 
 */
class Args : public UniformTable {
public:
private:
    
    int                 m_numInstances;

    /** Number of indices if explicitly set */
    int                 m_numIndices;

    /** For multi draw array rendering */
    Array<int>          m_indexOffsets;
    Array<int>          m_indexCounts;

    /** If empty, sequential indicies will be used. Non-immediate mode only. */
    IndexStream         m_indexStream;

    Array<IndexStream>  m_indexStreamArray;

    size_t              m_indirectOffset;

    shared_ptr<GLPixelTransferBuffer> m_indirectBuffer;

    /** If nonempty, rect mode will be used. */
    Rect2D              m_rect;

    /** Only used in rect mode. Defaults to (0,0), (1,1) */
    Rect2D              m_texCoordRect;

    /** Only used in rect mode. Defaults to -1.0 */
    float               m_rectZCoord;

    /** The primitive type input into the geometry or tesselation control shader. */
    PrimitiveType       m_primitiveType;

    /** 
        If true, G3D will set and bind its default arguments, including many matrix uniforms and macro variables.
        Default is true.
      */
    bool                m_useG3DArgs;
    
public:

    Vector3int32        computeGridDim;

    /** Number of vertices per patch sent to the geometry or tesselation control shader from the vertex shader.
        Only used if geometry input is PrimitiveType::PATCHES */
    GLint               patchVertices;

    /** Defaults: triangle primitive, 1 instance, 3 vertices per patch, computeGridDim(0,0,0), and uses G3D Args */
    Args();

    String toString() const;
    
    /** 
        If true, G3D will set and bind its default arguments, including many matrix uniforms and macro variables.
        Default is true.
      */
    void enableG3DArgs(bool enable);

    bool useG3DArgs() const {
        return m_useG3DArgs;
    }
    
    /** \beta Specify an index stream to append in a multidraw call */
    void appendIndexStream(const IndexStream& indexStream);

    /** Determines the order vertex attribute streams are sent to the GPU */
    void setIndexStream(const IndexStream& indStream);

    PrimitiveType getPrimitiveType() const {
        return m_primitiveType;
    }

    void setPrimitiveType(PrimitiveType type);


    const Rect2D& rect() const {
        return m_rect;
    }

    const Rect2D& texCoordRect() const {
        return m_texCoordRect;
    }

    /** If set, perform a glDrawArraysIndirect or glDispatchComputeIndirect where the 
        parameters for the thread launch come from another device buffer instead of the host,
        thus avoiding a CPU-GPU synchronization.

        \param offset number of bytes to offset from 0 when reading the arguments.
    
        R32UI format.  For glDrawIndirect, count, primcount, first, reserveMustBeZero. 
        For glDispatchComputeIndirect, gridx, gridy, gridz.*/
    void setIndirectBuffer(const shared_ptr<GLPixelTransferBuffer>& b, size_t offset = 0) {
        m_indirectBuffer = b;
        m_indirectOffset = offset;
    }

    void setMultiDrawArrays(const Array<int>& offsets, const Array<int>& counts) {
        m_indexOffsets.fastClear();
        m_indexOffsets.appendPOD(offsets);
        m_indexCounts.fastClear();
        m_indexCounts.appendPOD(counts);
    }

    const shared_ptr<GLPixelTransferBuffer>& indirectBuffer() const {
        return m_indirectBuffer;
    }

    size_t indirectOffset() const {
        return m_indirectOffset;
    }

    /** 
        If the index array has size > 0, returns its size, otherwise,
        returns the length of the shortest attached vertex attribute
        stream (0 if there are none.)
        */
    int numIndices() const;
	
    void clearAttributeAndIndexBindings();

    /** @deprecated */
    void G3D_DEPRECATED clearAttributeArgs() {
        clearAttributeAndIndexBindings();
    }

    /** True if there is a nonzero compute grid set. If this is true, 
        it is invalid to set any index streams or attribute arrays, or an indirect buffer */
    bool hasComputeGrid() const {
        return (computeGridDim.x > 0 && computeGridDim.y > 0 && computeGridDim.z > 0);
    }

    /** If this is true, it is invalid to set any index streams or a compute grid. */
    bool hasIndirectBuffer() const {
        return notNull(m_indirectBuffer);
    }

    /** If this is true, it is invalid to set a compute grid or and cpu attribute arrays/indexstreams */
    bool hasStreamArgs() const {
        return m_streamArgs.size() > 0;
    }

    /** If this is true, it is invalid to set any CPU index streams, an indirect buffer, or a compute grid. */
    bool hasGPUIndexStream() const {
        return m_indexStream.valid() || m_indexStreamArray.size() > 0;
    }

    const Array<IndexStream>& indexStreamArray() const {
        return m_indexStreamArray;
    }

    const Array<int>& indexCountArray() const {
        return m_indexCounts;
    }

    const Array<int>& indexOffsetArray() const {
        return m_indexOffsets;
    }

    bool hasRect() const {
        return ! m_rect.isEmpty();
    }

    const IndexStream& indexStream() const {
        return m_indexStream;
    }

    /** 
    @deprecated
    */
    const IndexStream& getIndexStream() const {
        return m_indexStream;
    }

    /** 
    @deprecated
    */
    const IndexStream& getIndices() const {
        return m_indexStream;
    }
	
    const Rect2D& getRect() const {
        return m_rect;
    }

    const Rect2D& getTexCoordRect() const {
        return m_texCoordRect;
    }

    float getRectZCoord() const {
        return m_rectZCoord;
    }

    int numInstances() const {
        return m_numInstances;
    }

    /** If you change the number of instances in order to produce multiple
        copies of a model at different locations using a VisibleEntity subclass, 
        then ensure that you take the following steps to produce consistent results:
        
        - Modify UniversalSurface_customOSVertexTransformation in UniversalSurface_vertexHelpers.glsl
        - Override VisibleEntity::onPose to:
           - Compute Entity::m_lastObjectSpaceAABoxBounds, Entity::m_lastSphereBounds, Entity::m_lastBoxBounds, and m_lastBoxBoundArray
           - Mutate UniversalSurface::GPUGeom::boxBounds
           - Mutate UniversalSurface::GPUGeom::sphereBounds
     */
    void setNumInstances(int num) {
        m_numInstances = num;
    }

    /** When rendering without a vertex array or index array, this forces the number of indices */
    void setNumIndices(int n) {
        m_numIndices = n;
    }

	
	void setRect(const Rect2D& rect, float zCoord = -1, const Rect2D& texCoordRect = Rect2D(Vector2(1,1)));


}; // class Args

}

#endif
