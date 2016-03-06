/**
 \file GLG3D/UniformTable.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-27
 \edited  2015-07-20

 */

#include "G3D/platform.h"
#include "G3D/Matrix2.h"
#include "G3D/Any.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Vector4int16.h"
#include "G3D/Vector4uint16.h"
#include "G3D/Vector3int32.h"
#include "GLG3D/UniformTable.h"
#include "GLG3D/Texture.h"
#include "GLG3D/BufferTexture.h"
#include "GLG3D/GLSamplerObject.h"
#include "GLG3D/getOpenGLState.h"
#include "G3D/PhysicsFrame.h"
#include "G3D/UprightFrame.h"
#include "GLG3D/BindlessTextureHandle.h"

namespace G3D {

static const String SYMBOL_SPACE = " ";
static const String SYMBOL_NEWLINE = "\n";
static const String SYMBOL_POUND_define = "#define ";

UniformTable::~UniformTable() {}


UniformTable::UniformTable(const Any& any) {
    for (Table<String, Any>::Iterator it = any.table().begin(); it.hasMore(); ++it) {
        const Any& v = it->value;
        switch (v.type()) {
        case Any::STRING:
            setMacro(it->key, v.string());
            break;

        case Any::NUMBER:
            setUniform(it->key, float(v.number()));
            break;

        case Any::BOOLEAN:
            setUniform(it->key, v.boolean());
            break;

        case Any::ARRAY:
        case Any::TABLE:
        case Any::EMPTY_CONTAINER:
            v.verify(! v.name().empty(), "Must be a named Any container (did you forget to say \"Color3\" or something like that?)");
			if (v.name() == "float") {
				setUniform(it->key, float(v[0].number()));
			} else if (v.name() == "double") {
				setUniform(it->key, double(v[0].number()));
			} else if ((v.name() == "int") || (v.name() == "int32")) {
				setUniform(it->key, int32(v[0].number()));
			} else if ((v.name() == "uint") || (v.name() == "uint32")) {
				setUniform(it->key, uint32(v[0].number()));
			} if (v.nameBeginsWith("Color3")) {
                setUniform(it->key, Color3(v));
            } else if (v.nameBeginsWith("Color4")) { 
                setUniform(it->key, Color4(v));
            } else if (v.nameBeginsWith("Vector2")) {
                setUniform(it->key, Vector2(v));
            } else if (v.nameBeginsWith("Vector3")) {
                setUniform(it->key, Vector3(v));
            } else if (v.nameBeginsWith("Vector4")) {
                setUniform(it->key, Vector4(v));
            } else if (v.nameBeginsWith("Matrix2")) {
                setUniform(it->key, Matrix2(v));
            } else if (v.nameBeginsWith("Matrix3")) {
                setUniform(it->key, Matrix3(v));
            } else if (v.nameBeginsWith("Matrix4")) {
                setUniform(it->key, Matrix4(v));
            } else if (v.nameBeginsWith("CFrame")) {
                setUniform(it->key, CFrame(v));
            } else if (v.nameBeginsWith("PhysicsFrame")) {
                setUniform(it->key, PhysicsFrame(v));
            } else if (v.nameBeginsWith("UprightFrame")) {
                setUniform(it->key, CFrame(UprightFrame(v)));
            } else {
                v.verify(false, "Unsupported value type name in UniformTable");
            }
            break;
       
        default:
            v.verify(false, "Unsupported value type in UniformTable");
        } // switch
    } // for
}


	void UniformTable::setPreamble(const String& preamble) {
    m_preamble = preamble;
}


String UniformTable::preambleAndMacroString() const {

    String preambleAndMacroString = m_preamble;

    static const String minimalMacroString = "#define \n";
    preambleAndMacroString.append("\n");

    // Find the minimal macro argument set, and sort them alphabetically so that
    Array<const MacroArgPair*> macros;
    macros.resize(m_macroArgs.size());

    for (int i = 0; i < m_macroArgs.size(); ++i) {
        macros[i] = &m_macroArgs[i];
    }

    // Sort
    macros.sort();

    // Accumulate the minimal set and retain alphabetical order
    const MacroArgPair* prev = NULL;
    for (int i = 0; i < macros.size(); ++i) {
        if (isNull(prev) || (prev->name != macros[i]->name)) {
            prev = macros[i];
            // String::append is about 2x faster than String::operator+
            preambleAndMacroString.append("#define ").append(prev->name).append(" ").append(prev->value).append("\n");
        }
    }
    
    return preambleAndMacroString;
}


const UniformTable::Arg& UniformTable::uniform(const String& name) const {
	const Arg* argPointer = m_uniformArgs.getPointer(name);
    
    if (isNull(argPointer)) {
        throw UnboundArgument(name);
    } else {
        return *argPointer;
    }
}


/* http://www.opengl.org/wiki/GLSL_Types */
String UniformTable::Arg::toString() const {
    switch(type){
    case GL_UNSIGNED_INT:
        return format("%u", value[0].ui);
    case GL_FLOAT:
        return format("%f", value[0].f);
    case GL_FLOAT_VEC2:
        return format("vec2(%f, %f)", value[0].f, value[1].f);
    case GL_FLOAT_VEC3:
        return format("vec3(%f, %f, %f)", value[0].f, value[1].f, value[2].f);
    case GL_FLOAT_VEC4:
        return format("vec4(%f, %f, %f, %f)", value[0].f, value[1].f, value[2].f, value[3].f);
    case GL_INT:
        return format("%d", value[0].i);
    case GL_INT_VEC2:
        return format("ivec2(%d, %d)", value[0].i, value[1].i);
    case GL_INT_VEC3:
        return format("ivec3(%d, %d, %d)", value[0].i, value[1].i, value[2].i);
    case GL_INT_VEC4:
        return format("ivec4(%d, %d, %d %d)", value[0].i, value[1].i, value[2].i, value[3].i);
    case GL_BOOL:
        return format("%s", value[0].b ? "true" : " false");
    case GL_BOOL_VEC2:
        return format("bvec2(%s, %s)", value[0].b ? "true" : " false", value[1].b ? "true" : " false");
    case GL_BOOL_VEC3: 
        return format("bvec3(%s, %s, %s)", value[0].b ? "true" : " false", value[1].b ? "true" : " false",
                        value[2].b ? "true" : " false");
    case GL_BOOL_VEC4: 
        return format("bvec4(%s, %s, %s, %s)", value[0].b ? "true" : " false", value[1].b ? "true" : " false",
                        value[2].b ? "true" : " false", value[3].b ? "true" : " false");
        
    // Matrices are column-major in opengl
    case GL_FLOAT_MAT2:
        return format("mat2(%f, %f, %f, %f)",
            value[0].f, value[1].f, 
            value[2].f, value[3].f);

	case GL_FLOAT_MAT3:
        return format("mat3(%f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, value[2].f,  
            value[3].f, value[4].f, value[5].f, 
            value[6].f, value[7].f, value[8].f);

    case GL_FLOAT_MAT4:
        return format("mat4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f,  value[1].f,  value[2].f,  value[3].f,
            value[4].f,  value[5].f,  value[6].f,  value[7].f,
            value[8].f,  value[9].f,  value[10].f, value[11].f,
            value[12].f, value[13].f, value[14].f, value[15].f);
    
	case GL_FLOAT_MAT2x3:
        return format("mat2x3(%f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, value[2].f,  
            value[3].f, value[4].f, value[5].f);
    
	case GL_FLOAT_MAT2x4:
        return format("mat2x4(%f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f,  value[1].f,  value[2].f,  value[3].f,
            value[4].f,  value[5].f,  value[6].f,  value[7].f);
    
	case GL_FLOAT_MAT3x2:
        return format("mat3x2(%f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, 
            value[2].f, value[3].f,
            value[4].f, value[5].f);
    
	case GL_FLOAT_MAT3x4:
        return format("mat3x4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f,  value[1].f,  value[2].f,  value[3].f,
            value[4].f,  value[5].f,  value[6].f,  value[7].f,
            value[8].f,  value[9].f,  value[10].f, value[11].f);

	case GL_FLOAT_MAT4x2:
        return format("mat4x2(%f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f, 
            value[2].f, value[3].f,
            value[4].f, value[5].f,
            value[6].f, value[7].f);

    case GL_FLOAT_MAT4x3:
        return format("mat4x3(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", 
            value[0].f, value[1].f,  value[2].f,  
            value[3].f, value[4].f,  value[5].f, 
            value[6].f, value[7].f,  value[8].f,
            value[9].f, value[10].f, value[11].f);
		
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
        return "Texture " + texture->name() + format(": (%dx%d)", texture->width(), texture->height());

    default:        
        alwaysAssertM(false, format("Macro args with GLenum type: %s unsupported", GLenumToString(type)));
        return "ERROR: Currently unsupported";
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
///// SET MACRO METHODS

void UniformTable::setMacro(const String& name, const String& value) {
    debugAssertM(name.find_first_of("\r\n"), "Newlines are not allowed in macro arg names");
    debugAssertM(value.find_first_of("\r\n"), "Newlines are not allowed in macro arg values");

    for (int i = 0; i < m_macroArgs.size(); ++i){
        if (m_macroArgs[i].name == name) {
            m_macroArgs[i].value = value;
            return;
        }
    }

    m_macroArgs.append(MacroArgPair(name, value));
}


void UniformTable::setMacro(const String& name, bool val) {
    setMacro(name, val ? 1 : 0);
}


// Avoid runtime allocation of small macro argument integers
static Array<String> intToString;
static const int BOUND = 20;
void UniformTable::setMacro(const String& name, int val) {
    if ((val >= -BOUND) && (val <= BOUND)) {
        if (intToString.size() == 0) {
            intToString.resize(BOUND * 2 + 1);
            for (int i = -BOUND; i <= BOUND; ++i) {
                intToString[i + BOUND] = format("%d", i); 
            }
        }
        setMacro(name, intToString[val + BOUND]);
    } else {
        const String& value = format("%d", val);
        setMacro(name, value);
    }
}


void UniformTable::setMacro(const String& name, uint32 val) {
    if (val <= uint32(BOUND)) {
        setMacro(name, int(val));
    } else {
        const String& value = format("%u", val);
        setMacro(name, value);
    }
}


void UniformTable::setMacro(const String& name, double val) {
    const String& value = format("%f", val);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, float val) {
    String value = format("%f", val);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector2& vec){
    String value = format("vec2(%f, %f)", vec.x, vec.y);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector3& vec){
    String value = format("vec3(%f, %f, %f)", vec.x, vec.y, vec.z);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector4& vec){
    String value = format("vec4(%f, %f, %f, %f)", vec.x, vec.y, vec.z, vec.w);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Color1& col){
    const String& value = format("%f", col.value);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Color3& col){
    const String& value = format("vec3(%f, %f, %f)", col.r, col.g, col.b);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Color4& col){
    const String& value = format("vec4(%f, %f, %f, %f)", col.r, col.g, col.b, col.a);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector2int32& vec){
    const String& value = format("ivec2(%d, %d)", vec.x, vec.y);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector2uint32& vec) {
    const String& value = format("uvec2(%d, %d)", vec.x, vec.y);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector3int32& vec){
    const String& value = format("ivec3(%d, %d, %d)", vec.x, vec.y, vec.z);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector2int16& vec){
    const String& value = format("ivec2(%d, %d)", vec.x, vec.y);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector3int16& vec){
    const String& value = format("ivec3(%d, %d, %d)", vec.x, vec.y, vec.z);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Vector4int16& vec){
    const String& value = format("ivec4(%d, %d, %d, %d)", vec.x, vec.y, vec.z, vec.w);
    setMacro(name, value);
}

void UniformTable::setMacro(const String& name, const Vector4uint16& vec) {
    const String& value = format("uvec4(%d, %d, %d, %d)", vec.x, vec.y, vec.z, vec.w);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Matrix2& mat){
    const String& value = format("mat2(%f, %f, %f, %f)", mat[0][0], mat[1][0], mat[0][1], mat[1][1]);
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Matrix3& mat){
    String value = format("mat3(%f, %f, %f,",  mat[0][0], mat[1][0], mat[2][0]); // Column 0
    value += format(" %f, %f, %f,",                 mat[0][1], mat[1][1], mat[2][1]); // Column 1
    value += format(" %f, %f, %f)",                 mat[0][2], mat[1][2], mat[2][2]); // Column 2 
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Matrix4& mat){
    String value = format("mat4(%f, %f, %f, %f",   mat[0][0], mat[1][0], mat[2][0], mat[3][0]); // Column 0
    value += format(" %f, %f, %f, %f,",                 mat[0][1], mat[1][1], mat[2][1], mat[3][1]); // Column 1
    value += format(" %f, %f, %f, %f,",                 mat[0][2], mat[1][2], mat[2][2], mat[3][2]); // Column 2
    value += format(" %f, %f, %f, %f)",                 mat[0][3], mat[1][3], mat[2][3], mat[3][3]); // Column 3
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const Matrix& mat){
    const int rows = mat.rows();
    const int cols = mat.cols();
    alwaysAssertM(rows <= 4 && cols <= 4 && rows >= 2 && cols >= 2, 
        format("The number of rows and columns of a matrix bound as a uniform variable must be 2,3, or 4.\n"
                "This matrix was (%dx%d)", cols, rows));

    String value = format("mat%dx%d(", cols, rows);
    for (int i = 0; i < cols; ++i){
        for (int j = 0; j < rows; ++j){
            value += format("%f, ", mat.get(j,i));
        }
    }

    // Chop off the extra ", " at the end, and tack on the closing paren
    value = value.substr(0, value.size() - 2) + ")";
    setMacro(name, value);
}


void UniformTable::setMacro(const String& name, const CoordinateFrame& val){
    const Matrix3& mat = val.rotation;
    const Vector3& vec = val.translation;
    String value = format("mat4x3(%f, %f, %f,",  mat[0][0], mat[1][0], mat[2][0]); // Column 0
    value +=             format(" %f, %f, %f,",  mat[0][1], mat[1][1], mat[2][1]); // Column 1
    value +=             format(" %f, %f, %f,",  mat[0][2], mat[1][2], mat[2][2]); // Column 2
    value +=             format(" %f, %f, %f)",  vec.x,    vec.y,      vec.z); // Column 2
    setMacro(name, value);
}


///////////////////////////////////////////////////////////////////////////////////////////

void UniformTable::setUniform(const String& name, bool val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(val, optional);
}


void UniformTable::setUniform(const String& name, int32 val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(val, optional);
}


void UniformTable::setUniform(const String& name, uint32 val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(val, optional);
}


void UniformTable::setUniform(const String& name, double val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(val, optional);
}


void UniformTable::setUniform(const String& name, float val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(val, optional);
}


void UniformTable::setUniform(const String& name, const Color1& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(col, optional);
}


void UniformTable::setUniform(const String& name, const Vector2& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector3& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector4& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Color3& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(col, optional);
}


void UniformTable::setUniform(const String& name, const Color4& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(col, optional);
}


void UniformTable::setUniform(const String& name, const Matrix2& mat, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(mat, optional);
}


void UniformTable::setUniform(const String& name, const Matrix3& mat, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(mat, optional);
}


void UniformTable::setUniform(const String& name, const Matrix4& mat, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(mat, optional);
}


void UniformTable::setUniform(const String& name, const CoordinateFrame& cframe, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(cframe, optional);
}

void UniformTable::setUniform(const String& name, const Vector2int32& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector2uint32& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector3int32& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector2int16& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector3int16& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}


void UniformTable::setUniform(const String& name, const Vector4int16& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}

void UniformTable::setUniform(const String& name, const Vector4uint16& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.set(vec, optional);
}

void UniformTable::setUniform(const String& name, uint64 val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    arg.type = GL_UNSIGNED_INT64_NV;
    arg.optional = optional;
    arg.value.resize(1);
    arg.value[0] = val;
}

 
static GLenum toImageType(const shared_ptr<Texture> t) {
    Texture::TexelType tType = t->texelType();

    #define RETURN_TYPE(texelType, baseType) { \
        switch(texelType) {\
        case Texture::TexelType::FLOAT: \
            return GL_##baseType; \
        case Texture::TexelType::INTEGER: \
            return GL_INT_##baseType; \
        case Texture::TexelType::UNSIGNED_INTEGER: \
            return GL_UNSIGNED_INT_##baseType; \
        } \
    }

    switch (t->dimension()) {
    case Texture::DIM_CUBE_MAP:
        RETURN_TYPE(tType, IMAGE_CUBE);

    case Texture::DIM_2D:
        if(t->numSamples()>1){
			RETURN_TYPE(tType, IMAGE_2D_MULTISAMPLE);
		}else{
			RETURN_TYPE(tType, IMAGE_2D);
		}

    case Texture::DIM_2D_ARRAY:
        RETURN_TYPE(tType, IMAGE_2D_ARRAY);

    case Texture::DIM_2D_RECT:
        RETURN_TYPE(tType, IMAGE_2D_RECT);

    case Texture::DIM_3D:
        RETURN_TYPE(tType, IMAGE_3D);

    default:
        debugAssertM(false, "Invalid texture type for binding to image uniform");
    }
    return GL_NONE;
}


static GLenum toImageTypeFromSamplerType(const GLenum& textureType) {
    switch (textureType) {
    case GL_SAMPLER_1D:
        return GL_IMAGE_1D;
    case GL_SAMPLER_2D:
        return GL_IMAGE_2D;
    case GL_SAMPLER_2D_ARRAY:
        return GL_IMAGE_2D_ARRAY;
    case GL_SAMPLER_3D:
        return GL_IMAGE_3D;
    case GL_SAMPLER_CUBE:
        return GL_IMAGE_CUBE;
    case GL_SAMPLER_BUFFER:
        return GL_IMAGE_BUFFER;
    case GL_INT_SAMPLER_BUFFER:
        return GL_INT_IMAGE_BUFFER;
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        return GL_UNSIGNED_INT_IMAGE_BUFFER;
    default:
        alwaysAssertM(false, "GL Image type unsupported");
        return GL_NONE;
    }
}


void UniformTable::setImageUniform(const String& name, const shared_ptr<Texture>& t, Access access, int mipLevel, bool optional) {
    alwaysAssertM(t->format()->numComponents != 3, 
        format("SVO requires that all fields have 1, 2, or 4 components due to restrictions from OpenGL glBindImageTexture.  Error occured while binding texture %s to variable %s",
         t->name().c_str(), name.c_str()));
    Arg& arg = m_uniformArgs.getCreate(name);
    debugAssert(notNull(t));
    arg.type        = toImageType(t);
    arg.texture     = t; 
    arg.value.resize(2);
    debugAssert(mipLevel >= 0 && mipLevel < 1000);
    arg.value[0]    = (uint32)mipLevel;
    arg.value[1]    = (uint32)access;
    arg.optional    = optional;
}


void UniformTable::setUniform(const String& name, const shared_ptr<Texture> & val, const Sampler& sampler, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);

    debugAssert(notNull(val));
    alwaysAssertM(!sampler.interpolateMode.requiresMipMaps() || val->hasMipMaps(), "Tried to bind a Texture without mipmaps using a Sampler that requires them");
    arg.type        = val->openGLTextureTarget();
    arg.texture     = val;
    arg.sampler     = GLSamplerObject::create(sampler);
    arg.optional    = optional;
}


void UniformTable::setImageUniform(const String& name, const shared_ptr<BufferTexture>& t, Access access, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name);
    debugAssert(notNull(t));
    arg.type        = toImageTypeFromSamplerType(t->glslSamplerType());
    arg.bufferTexture     = t; 
    arg.value.resize(2);
    arg.value[0]    = 0;
    arg.value[1]    = (uint32)access;
    arg.optional    = optional;
}


void UniformTable::setUniform(const String& name, const shared_ptr<BufferTexture>& val, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    debugAssert(notNull(val));
    arg.type            = val->glslSamplerType();
    arg.bufferTexture   = val;
    arg.optional        = optional;
}

void UniformTable::setUniform(const String& name, const shared_ptr<BindlessTextureHandle>& val, bool optional){
    Arg& arg = m_uniformArgs.getCreate(name);
    debugAssert(notNull(val));
    arg.type = GL_UNSIGNED_INT64_ARB;
    arg.handle = val;
    arg.optional = optional;
}


UniformTable::UniformTable() {}

void UniformTable::setArrayUniform(const String& name, int index, const shared_ptr<BindlessTextureHandle>& val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index = index;
    debugAssert(notNull(val));
    arg.type = GL_UNSIGNED_INT64_ARB;
    arg.handle = val;
    arg.optional = optional;
}

void UniformTable::setArrayUniform(const String& name, int index, int val, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index = index;
    arg.type = GL_INT;
    arg.optional = optional;
    arg.value.resize(1);
    arg.value[0] = val;
}

void UniformTable::setArrayUniform(const String& name, int index, float val, bool optional) {
	Arg& arg    = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type    = GL_FLOAT;
    arg.optional = optional;
    arg.value.resize(1);
    arg.value[0] = val;
}


void UniformTable::setArrayUniform(const String& name, int index, const Color1& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type = GL_FLOAT;
    arg.optional = optional;
    arg.value.resize(1);
    arg.value[0] = col.value;
}


void UniformTable::setArrayUniform(const String& name, int index, const Vector2& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type = GL_FLOAT_VEC2;
    arg.optional = optional;
    arg.value.resize(2);
    arg.value[0] = vec.x;
    arg.value[1] = vec.y;
}


void UniformTable::setArrayUniform(const String& name, int index, const Vector3& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type = GL_FLOAT_VEC3;
    arg.optional = optional;
    arg.value.resize(3);
    arg.value[0] = vec.x;
    arg.value[1] = vec.y;
    arg.value[2] = vec.z;
}


void UniformTable::setArrayUniform(const String& name, int index, const Vector4& vec, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type = GL_FLOAT_VEC4;
    arg.optional = optional;
    arg.value.resize(4);
    arg.value[0] = vec.x;
    arg.value[1] = vec.y;
    arg.value[2] = vec.z;
    arg.value[3] = vec.w;
}


void UniformTable::setArrayUniform(const String& name, int index, const Color3& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type = GL_FLOAT_VEC3;
    arg.optional = optional;
    arg.value.resize(3);
    arg.value[0] = col.r;
    arg.value[1] = col.g;
    arg.value[2] = col.b;
}



void UniformTable::setArrayUniform(const String& name, int index, const Color4& col, bool optional) {
    Arg& arg = m_uniformArgs.getCreate(name + format("[%d]", index));
    arg.index   = index;
    arg.type = GL_FLOAT_VEC4;
    arg.optional = optional;
    arg.value.resize(4);
    arg.value[0] = col.r;
    arg.value[1] = col.g;
    arg.value[2] = col.b;
    arg.value[3] = col.a;
}


bool UniformTable::getMacro(const String& name, String& value) const {
    for (int i = 0; i < m_macroArgs.size(); ++i) {
        if (m_macroArgs[i].name == name) {
            value = m_macroArgs[i].value;
            return true;
        } 
    }
    return false;
}


void UniformTable::append(const UniformTable& other, const String& prefix) {
    if (! other.m_preamble.empty()) {
        appendToPreamble(other.m_preamble);
    }

    if (prefix.empty()) {
		for (int i = 0; i < other.m_macroArgs.size(); ++i) {
			setMacro(other.m_macroArgs[i].name, other.m_macroArgs[i].value);
		}

        if (other.m_uniformArgs.size() > 0) {
            for (ArgTable::Iterator it = other.m_uniformArgs.begin(); it.hasMore(); ++it) {
                m_uniformArgs.set(it->key, it->value);
            }
        }
    } else {
		for (int i = 0; i < other.m_macroArgs.size(); ++i) {
			setMacro(prefix + other.m_macroArgs[i].name, other.m_macroArgs[i].value);
		}

        for (ArgTable::Iterator it = other.m_uniformArgs.begin(); it.hasMore(); ++it) {
            m_uniformArgs.set(prefix + it->key, it->value);
        }
    }

    for (GPUAttributeTable::Iterator it = other.m_streamArgs.begin(); it.hasMore(); ++it) {
        m_streamArgs.set(it->key, it->value);
    }

}


void UniformTable::setAttributeArray(const String& name, const AttributeArray& arg, int divisor) {

    alwaysAssertM(divisor >= 0, "divisor cannot be negative");
    m_streamArgs.set(name, GPUAttribute(arg, divisor));
}


void UniformTable::Arg::set( bool val, bool optional) {
    type = GL_BOOL;
    this->optional = optional;
    value.resize(1);
    value[0] = val;
}

void UniformTable::Arg::set(int32 val, bool optional) {
    type = GL_INT;
    this->optional = optional;
    value.resize(1);
    value[0] = val;
}	

void UniformTable::Arg::set(uint32 val, bool optional) {
    type = GL_UNSIGNED_INT;
    this->optional = optional;
    value.resize(1);
    value[0] = val;
}

void UniformTable::Arg::set(double val, bool optional) {
    type = GL_DOUBLE;
    this->optional = optional;
    value.resize(1);
    value[0] = val;
}

void UniformTable::Arg::set(float val, bool optional) {
    type = GL_FLOAT;
    this->optional = optional;
    value.resize(1);
    value[0] = val;
}

void UniformTable::Arg::set(const Color1& col, bool optional) {
    type = GL_FLOAT;
    this->optional = optional;
    value.resize(1);
    value[0] = col.value;
}

void UniformTable::Arg::set(const Vector2& vec, bool optional) {
    type = GL_FLOAT_VEC2;
    this->optional = optional;
    value.resize(2);
    value[0] = vec.x;
    value[1] = vec.y;
}

void UniformTable::Arg::set(const Vector3& vec, bool optional) {
    type = GL_FLOAT_VEC3;
    this->optional = optional;
    value.resize(3);
    value[0] = vec.x;
    value[1] = vec.y;
    value[2] = vec.z;
}

void UniformTable::Arg::set(const Vector4& vec, bool optional) {
    type = GL_FLOAT_VEC4;
    this->optional = optional;
    value.resize(4);
    value[0] = vec.x;
    value[1] = vec.y;
    value[2] = vec.z;
    value[3] = vec.w;
}

void UniformTable::Arg::set(const Color3& col, bool optional) {
    type = GL_FLOAT_VEC3;
    this->optional = optional;
    value.resize(3);
    value[0] = col.r;
    value[1] = col.g;
    value[2] = col.b;
}

void UniformTable::Arg::set(const Color4& col, bool optional) {
    type = GL_FLOAT_VEC4;
    this->optional = optional;
    value.resize(4);
    value[0] = col.r;
    value[1] = col.g;
    value[2] = col.b;
    value[3] = col.a;
}

void UniformTable::Arg::set(const Matrix2& mat, bool optional) {    
    type = GL_FLOAT_MAT2;
    value.clear(); 
    this->optional = optional;
    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 2; ++j){
            value.append(mat[j][i]);
        }
    }   
}

void UniformTable::Arg::set(const Matrix3& mat, bool optional) {
    type = GL_FLOAT_MAT3;
    value.clear();
    this->optional = optional;
    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 3; ++j){
            value.append(mat[j][i]);
        }
    }	
}

void UniformTable::Arg::set(const Matrix4& mat, bool optional) {    
    type = GL_FLOAT_MAT4;
    value.clear();
    this->optional = optional;
    for(int i = 0; i < 4; ++i){
        for(int j = 0; j < 4; ++j){
            value.append(mat[j][i]);
	    }
    }   
}

void UniformTable::Arg::set(const CoordinateFrame& cframe, bool optional) {    
    type = GL_FLOAT_MAT4x3;
    value.clear();
    this->optional = optional;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            value.append(cframe.rotation[j][i]);
        }
    }
    for (int j = 0; j < 3; ++j) {
        value.append(cframe.translation[j]);
    } 
}


void UniformTable::Arg::set(const Vector2int32& vec, bool optional) {
    type = GL_INT_VEC2;
    this->optional = optional;
    value.resize(2);
    value[0] = vec.x;
    value[1] = vec.y;    
}


void UniformTable::Arg::set(const Vector2uint32& vec, bool optional) {
    type = GL_UNSIGNED_INT_VEC2;
    this->optional = optional;
    value.resize(2);
    value[0] = vec.x;
    value[1] = vec.y;
}


void UniformTable::Arg::set(const Vector3int32& vec, bool optional) {
    type = GL_INT_VEC3;
    this->optional = optional;
    value.resize(3);
    value[0] = vec.x;
    value[1] = vec.y;
    value[2] = vec.z;
}


void UniformTable::Arg::set(const Vector2int16& vec, bool optional) {
    type = GL_INT_VEC2;
    this->optional = optional;
    value.resize(2);
    value[0] = (int32)vec.x;
    value[1] = (int32)vec.y;    
}


void UniformTable::Arg::set(const Vector3int16& vec, bool optional) {
    type = GL_INT_VEC3;
    this->optional = optional;
    value.resize(3);
    value[0] = (int32)vec.x;
    value[1] = (int32)vec.y;
    value[2] = (int32)vec.z;
}


void UniformTable::Arg::set(const Vector4int16& vec, bool optional) {
    value.clear();
    type = GL_INT_VEC4;
    this->optional = optional;
    value.append((int32)vec.x);
    value.append((int32)vec.y);
    value.append((int32)vec.z);
    value.append((int32)vec.w);
}

void UniformTable::Arg::set(const Vector4uint16& vec, bool optional) {
    value.clear();
    type = GL_UNSIGNED_INT_VEC4;
    this->optional = optional;
    value.append((uint32)vec.x);
    value.append((uint32)vec.y);
    value.append((uint32)vec.z);
    value.append((uint32)vec.w);
}

} // end namespace G3D
