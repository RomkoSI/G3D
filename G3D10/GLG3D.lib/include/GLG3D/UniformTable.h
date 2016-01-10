/**
 \file GLG3D/UniformTable.h
  
 \maintainer Michael Mara, http://www.illuminationcodified.com

 \created 2012-06-16
 \edited  2015-07-07

 G3D Innovation Engine
 Copyright 2000-2015, Morgan McGuire.
 All rights reserved.
 */

#ifndef G3D_UniformTable_h
#define G3D_UniformTable_h

#include "G3D/platform.h"
#include "G3D/constants.h"
#include "G3D/SmallArray.h"
#include "G3D/Matrix.h"
#include "G3D/Vector4.h"
#include "G3D/Vector2uint32.h"
#include "G3D/Access.h"
#include "GLG3D/glheaders.h"
#include "GLG3D/Sampler.h"
#include "GLG3D/AttributeArray.h"

namespace G3D {
class BufferTexture;
class Texture;
class GLPixelTransferBuffer;
class GLSamplerObject;
class BindlessTextureHandle;


/** \brief Uniform and macro arguments for a G3D::Shader.

    This provides a mechanism for support classes like UniversalMaterial to
    provide additional arguments to a Shader.
    AttributeArray%s are usually set on Args and not UniformTable, however per-instance attributes
    may need to be specified for an ArticulatedModel::Pose or related support class.

	\sa Args
*/
class UniformTable {
public:
    friend class Shader;

    /** 8-byte storage used for all argument types */
    union Scalar {
        float  f;
        int32  i;
        uint32 ui;
		uint64 ui64;
        bool   b;
        double d;
        Scalar(float fl)    { f = fl; }
        Scalar(int32 in)    { i = in; }
        Scalar(uint32 uin)  { ui = uin; }
		Scalar(uint64 uin64){ ui64 = uin64; }
        Scalar(bool bo)     { b = bo; }
        Scalar(double db)   { d = db; }
        Scalar()            {  }
    }; 

    /** This contains the value of a uniform or macro argument passed to shader and its type 
        Macro variables can only be a subset of the possible values */
    class Arg {
    public:
        GLenum                  type;

        /** Empty unless this argument is an OpenGL Sampler or Image (that is not a buffer) */
        shared_ptr<Texture>     texture;

        /** Empty unless this argument is an OpenGL Sampler */
        shared_ptr<GLSamplerObject> sampler;

        shared_ptr<BindlessTextureHandle>   handle;

        shared_ptr<BufferTexture>   bufferTexture;
        SmallArray<Scalar, 4>   value;
        bool                    optional;

        /** If this arg is an element of a glsl array, this is its index in the array. 
            If this arg is not an element of an array, its index is -1 */
        int                     index;

        void set( bool val, bool optional);

        void set(int32 val, bool optional);
        void set(uint32 val, bool optional);
        void set(double val, bool optional);
        void set(float val, bool optional);

        void set(const Vector2& vec, bool optional);
        void set(const Vector3& vec, bool optional);
        void set(const Vector4& vec, bool optional);

        void set(const Color1& col, bool optional);
        void set(const Color3& col, bool optional);
        void set(const Color4& col, bool optional);

        void set(const Matrix2& mat, bool optional);
        void set(const Matrix3& mat, bool optional);
        void set(const Matrix4& mat, bool optional);
        void set(const CoordinateFrame& cframe, bool optional);

        void set(const Vector2int32& vec, bool optional);
        void set(const Vector2uint32& vec, bool optional);
        void set(const Vector3int32& vec, bool optional);

        void set(const Vector2int16& vec, bool optional);
        void set(const Vector3int16& vec, bool optional);
        void set(const Vector4int16& vec, bool optional);
        void set(const Vector4uint16& vec, bool optional);


        String toString() const;
        
        Arg() : index(-1) {}
        Arg(GLenum t) : type(t), index(-1) {}
        Arg(GLenum t, bool o) : type(t), optional(o), index(-1) {}
    };
    
    typedef Table<String, Arg>  ArgTable;

    class MacroArgPair {
    public:
        String name;
        String value;

		MacroArgPair() {}
        
		MacroArgPair(const String& n, const String& v) : name(n), value(v) {}

		bool operator<(const MacroArgPair& other) const {
			return name < other.name;
		}

		bool operator>(const MacroArgPair& other) const {
			return name > other.name;
		}
    };

    /** Thrown when an argument is not bound */
    class UnboundArgument {
    public:
        String                      name;
        UnboundArgument(const String& name) : name(name) {}
    };
    
protected:

    /** https://www.opengl.org/sdk/docs/man3/xhtml/glVertexAttribDivisor.xml */
    class GPUAttribute {
    public:
        AttributeArray  attributeArray;

        /** https://www.opengl.org/sdk/docs/man3/xhtml/glVertexAttribDivisor.xml */
        int             divisor;
        GPUAttribute() : divisor(0) {}
        GPUAttribute(const AttributeArray& a, int d) : attributeArray(a), divisor(d) {}
    };

    typedef Table<String, GPUAttribute>        GPUAttributeTable;

    String                          m_preamble;
    
    /** We use a SmallArray to avoid heap allocation for every arg instance */
    SmallArray<MacroArgPair, 12>    m_macroArgs;

    ArgTable                        m_uniformArgs;

    /** Must be empty if m_immediateModeArgs is non-empty */
    GPUAttributeTable               m_streamArgs;
    
public:

    UniformTable();

	/**
	 Supports matrix, color, vector, float, string (as macro), boolean, int, and double arguments.
	 Example:
	 \code
	  {
	     height = float(3.1);
		 numIterations = int(7);

		 // Macro
		 USE_BUMP = "1";

		 position = Point3(2, 1, -4.5);
	  }
	 \endcode
	*/
    UniformTable(const Any& any);

    virtual ~UniformTable();

    bool hasUniform(const String& s) const {
        return m_uniformArgs.containsKey(s);
    }

    /** The preamble with macro arg definitions appended */
    String preambleAndMacroString() const;

    String preamble() const {
        return m_preamble;
    }

    void appendToPreamble(const String& extra) {
        m_preamble += extra;
    }

    /** Arbitrary string to append to beginning of the shader */
    void setPreamble(const String& preamble);

    void clearUniformBindings() {
        m_uniformArgs.clear();
    }

    void clearUniform(const String& s) {
        m_uniformArgs.remove(s);
    }

    bool hasPreambleOrMacros() const {
        return (m_preamble != "") || (m_macroArgs.size() > 0);
    }

    /** Returns the uniform value bound to this name or throws UnboundArgument. */
    const Arg& uniform(const String& name) const;

    /** Get the value of the macro arg \a name, and return its value in \a value,
        returns true if the macro arg exists */
    bool getMacro(const String& name, String& value) const;

    void setMacro(const String& name, const String& value);
    void setMacro(const String& name, const char* value) {
        setMacro(name, String(value));
    }

    // Supports bool, int, float, double, vec234, mat234, mat234x234, ivec234,
    /** false becomes 0 and true becomes 1 */
    void setMacro(const String& name, bool             val);
    void setMacro(const String& name, int              val);
    void setMacro(const String& name, uint32           val);
    void setMacro(const String& name, double           val);
    void setMacro(const String& name, float            val);

    void setMacro(const String& name, const Vector2&   val);
    void setMacro(const String& name, const Vector3&   val);
    void setMacro(const String& name, const Vector4&   val);
    /** Becomes float in GLSL */
    void setMacro(const String& name, const Color1&    val);
    void setMacro(const String& name, const Color3&    val);
    void setMacro(const String& name, const Color4&    val);

    void setMacro(const String& name, const Vector2int32&  val);
    void setMacro(const String& name, const Vector2uint32&  val);
    void setMacro(const String& name, const Vector3int32&  val);

    void setMacro(const String& name, const Vector2int16&  val);
    void setMacro(const String& name, const Vector3int16&  val);
    void setMacro(const String& name, const Vector4int16&  val);
    void setMacro(const String& name, const Vector4uint16&  val);

    void setMacro(const String& name, const Matrix2&   val);
    void setMacro(const String& name, const Matrix3&   val);
    void setMacro(const String& name, const Matrix4&   val);

    /** Becomes mat3x4 in GLSL */
    void setMacro(const String& name, const CoordinateFrame&   val);

    void setMacro(const String& name, const Matrix&   val);
 

    void setUniform(const String& name, bool             val, bool optional);
    void setUniform(const String& name, bool             val) {
        setUniform(name, val, false);
    }

    void setUniform(const String& name, int              val, bool optional = false);
    void setUniform(const String& name, float            val, bool optional = false);
    void setUniform(const String& name, uint32           val, bool optional = false);
    void setUniform(const String& name, double           val, bool optional = false);
    void setUniform(const String& name, uint64			 val, bool optional = false);

    void setUniform(const String& name, const Vector2&   val, bool optional = false);
    void setUniform(const String& name, const Vector3&   val, bool optional = false);
    void setUniform(const String& name, const Vector4&   val, bool optional = false);

    /** Becomes float in GLSL */
    void setUniform(const String& name, const Color1&    val, bool optional = false);
    void setUniform(const String& name, const Color3&    val, bool optional = false);
    void setUniform(const String& name, const Color4&    val, bool optional = false);

    void setUniform(const String& name, const Vector2int32&  val, bool optional = false);
    void setUniform(const String& name, const Vector2uint32&  val, bool optional = false);
    void setUniform(const String& name, const Vector3int32&  val, bool optional = false);

    void setUniform(const String& name, const Vector2int16&  val, bool optional = false);
    void setUniform(const String& name, const Vector3int16&  val, bool optional = false);
    void setUniform(const String& name, const Vector4int16&  val, bool optional = false);
    void setUniform(const String& name, const Vector4uint16&  val, bool optional = false);

    
    void setUniform(const String& name, const Matrix2&   val, bool optional = false);
    void setUniform(const String& name, const Matrix3&   val, bool optional = false);
    void setUniform(const String& name, const Matrix4&   val, bool optional = false);

    void setUniform(const String& name, const Matrix&   val, bool optional = false);

    void setUniform(const String& name, const CoordinateFrame& val, bool optional = false);

    /** Uses the texture as the corresponding <b>image</b> type in the shader */
    void setImageUniform(const String& name, const shared_ptr<Texture>& val, Access access = Access::READ_WRITE, int mipLevel = 0, bool optional = false);

    void setUniform(const String& name, const shared_ptr<Texture> & val, const Sampler& settings, bool optional = false);

    /** Uses the texture as the corresponding *imageBuffer type in the shader */
    void setImageUniform(const String& name, const shared_ptr<BufferTexture>& val, Access access = Access::READ_WRITE, bool optional = false);

    void setUniform(const String& name, const shared_ptr<BufferTexture> & val, bool optional);
    void setUniform(const String& name, const shared_ptr<BufferTexture> & val) {
        // Needed because shared_ptr::operator bool makes this overload ambiguous
        setUniform(name, val, false);
    }

    void setUniform(const String& name, const shared_ptr<BindlessTextureHandle>& val, bool optional = false);

    void setArrayUniform(const String& name, int index, const shared_ptr<BindlessTextureHandle>& val, bool optional = false);


    void setArrayUniform(const String& name, int index, int val, bool optional = false);
    void setArrayUniform(const String& name, int index, float val, bool optional = false);

    void setArrayUniform(const String& name, int index, const Vector2&   val, bool optional = false);
    void setArrayUniform(const String& name, int index, const Vector3&   val, bool optional = false);
    void setArrayUniform(const String& name, int index, const Vector4&   val, bool optional = false);

    void setArrayUniform(const String& name, int index, const Color1&    val, bool optional = false);
    void setArrayUniform(const String& name, int index, const Color3&    val, bool optional = false);
    void setArrayUniform(const String& name, int index, const Color4&    val, bool optional = false);

    /**
     Sets all arguments from \a other on \a this.

     \param prefix If specified, prefix each uniform and macro name with this prefix.
     */
    void append(const UniformTable& other, const String& prefix = "");

    void append(const shared_ptr<UniformTable>& other, const String& prefix = "") {
        if (notNull(other)) {
            append(*other, prefix);
        }
    }

    UniformTable(const UniformTable& other) {
        append(other);
    }

    const GPUAttributeTable& gpuAttributeTable() const {
        return m_streamArgs;
    }

	/** Beta API */
	const UniformTable::ArgTable& uniformTable() const {
		return m_uniformArgs;
	}

    /**
      \param instanceDivisor Set to 0 for regular indexed rendering and 1 to increment once per instance. https://www.opengl.org/sdk/docs/man3/xhtml/glVertexAttribDivisor.xml
    */
    void setAttributeArray(const String& name, const AttributeArray& val, int instanceDivisor = 0);

}; // class UniformTable

} // G3D
#endif
