/**
 \file GLG3D/Shader.cpp
  
 \maintainer Morgan McGuire, Michael Mara http://graphics.cs.williams.edu
 
 \created 2012-06-13
 \edited  2015-01-03

 */

#include "G3D/platform.h"
#include "GLG3D/Shader.h"
#include "GLG3D/GLCaps.h"
#include "G3D/fileutils.h"
#include "G3D/stringutils.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "G3D/prompt.h"
#include "G3D/g3dmath.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/BufferTexture.h"
#include "GLG3D/BindlessTextureHandle.h"
#include "GLG3D/GLSamplerObject.h"
#include "GLG3D/GApp.h"

namespace G3D {
Array< weak_ptr<Shader> > Shader::s_allShaders;

Shader::FailureBehavior Shader::s_failureBehavior = Shader::PROMPT;

GLenum Shader::toGLType(const String& s) {
    if (s == "float") {
        return GL_FLOAT;
    } else if (s == "vec2" || s == "float2") {
        return GL_FLOAT_VEC2;
    } else if (s == "vec3" || s == "float3") {
        return GL_FLOAT_VEC3;
    } else if (s == "vec4" || s == "float4") {
        return GL_FLOAT_VEC4;
    } else if (s == "int") {
        return GL_INT;
    } else if (s == "ivec2" || s == "int2") {
        return GL_INT_VEC2;
    } else if (s == "ivec3" || s == "int3") {
        return GL_INT_VEC3;
    } else if (s == "ivec4" || s == "int4") {
        return GL_INT_VEC4;
    } else if (s == "unsigned int" || s == "uint") {
        return GL_UNSIGNED_INT;
    } else if (s == "uint2" || s == "uvec2" ) {
        return GL_UNSIGNED_INT_VEC2;
    } else if (s == "uint3" || s == "uvec3" ) {
        return GL_UNSIGNED_INT_VEC3;
    } else if (s == "uint4" || s == "uvec4" ) {
        return GL_UNSIGNED_INT_VEC4;
    } else if (s == "bool") {
        return GL_BOOL;
    } else if (s == "bvec2" || s == "bool2") {
        return GL_BOOL_VEC2;
    } else if (s == "bvec3" || s == "bool3") {
        return GL_BOOL_VEC3;
    } else if (s == "bvec4" || s == "bool4") {
        return GL_BOOL_VEC4;
    } else if (s == "mat2") {
        return GL_FLOAT_MAT2;
    } else if (s == "mat3") {
        return GL_FLOAT_MAT3;
    } else if (s == "mat4") {
        return GL_FLOAT_MAT4;
    } else if (s == "mat4x3") {
        return GL_FLOAT_MAT4x3;
    } else if (s == "mat4x2") {
        return GL_FLOAT_MAT4x2;
    } else if (s == "mat3x4") {
        return GL_FLOAT_MAT3x4;
    } else if (s == "mat3x2") {
        return GL_FLOAT_MAT3x2;
    } else if (s == "mat2x4") {
        return GL_FLOAT_MAT2x4;
    } else if (s == "mat2x3") {
        return GL_FLOAT_MAT2x3;
    } else if (s == "sampler1D") {
        return GL_SAMPLER_1D;
    } else if (s == "isampler1D") {
        return GL_INT_SAMPLER_1D;
    } else if (s == "usampler1D") {
        return GL_UNSIGNED_INT_SAMPLER_1D;
    } else if (s == "sampler2D") {
        return GL_SAMPLER_2D;
	} else if (s == "sampler2DMS") {
        return GL_SAMPLER_2D_MULTISAMPLE;
    } else if (s == "sampler2DArray") {
        return GL_SAMPLER_2D_ARRAY;
    } else if (s == "isampler2D") {
        return GL_INT_SAMPLER_2D;
    } else if (s == "isampler2DMS") {     //CC: Added
        return GL_INT_SAMPLER_2D_MULTISAMPLE;
    } else if (s == "usampler2D") {
        return GL_UNSIGNED_INT_SAMPLER_2D;
    } else if (s == "usampler2DMS") {     //CC: Added
        return GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE;
    } else if (s == "sampler3D") {
        return GL_SAMPLER_3D;
    } else if (s == "isampler3D") {
        return GL_INT_SAMPLER_3D;
    } else if (s == "usampler3D") {
        return GL_UNSIGNED_INT_SAMPLER_3D;
    } else if (s == "samplerCube") {
        return GL_SAMPLER_CUBE;
    } else if (s == "isamplerCube") {
        return GL_INT_SAMPLER_CUBE;
    } else if (s == "usamplerCube") {
        return GL_UNSIGNED_INT_SAMPLER_CUBE;
    } else if (s == "sampler2DRect") {
        return GL_SAMPLER_2D_RECT;
    } else if (s == "usampler2DRect") {
        return GL_UNSIGNED_INT_SAMPLER_2D_RECT;
    } else if (s == "sampler2DShadow") {
        return GL_SAMPLER_2D_SHADOW;
    } else if (s == "sampler2DRectShadow") {
        return GL_SAMPLER_2D_RECT_SHADOW;
    } else if (s == "image2D") {
        return GL_IMAGE_2D;
	 } else if (s == "image2DMS") {
        return GL_IMAGE_2D_MULTISAMPLE;
    } else if (s == "uimage2D") {
        return GL_UNSIGNED_INT_IMAGE_2D;
    } else if (s == "uimage2DMS") {   //CC: Added
        return GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE;
    } else if (s == "iimage2D") {
        return GL_INT_IMAGE_2D;
    } else if (s == "iimage2DMS") {   //CC: Added
        return GL_INT_IMAGE_2D_MULTISAMPLE;
    } else if (s == "image3D") {
        return GL_IMAGE_3D;
    } else if (s == "uimage3D") {
        return GL_UNSIGNED_INT_IMAGE_3D;
    } else if (s == "iimage3D") {
        return GL_INT_IMAGE_3D;
    } else if (s == "image1D") {
        return GL_IMAGE_1D;
    } else if (s == "uimage1D") {
        return GL_UNSIGNED_INT_IMAGE_1D;
    } else if (s == "iimage1D") {
        return GL_INT_IMAGE_3D;
    } else if (s == "image2DRect") {
        return GL_IMAGE_2D_RECT;
    } else if (s == "uimage2DRect") {
        return GL_UNSIGNED_INT_IMAGE_2D_RECT;
    } else if (s == "iimage2DRect") {
        return GL_INT_IMAGE_2D_RECT;
    } else if (s == "atomic_uint") {
        return GL_UNSIGNED_INT_ATOMIC_COUNTER;
    } else if (s == "uimageBuffer") {
        return GL_UNSIGNED_INT_IMAGE_BUFFER;
    } else if (s == "imageBuffer") {
        return GL_IMAGE_BUFFER;
    } else if (s == "iimageBuffer") {
        return GL_INT_IMAGE_BUFFER;
    } else {
        return GL_NONE;
    }
}


static String stageName(int s) {
    switch(s) {
    case Shader::VERTEX:
        return "Vertex";
    case Shader::TESSELLATION_CONTROL:
        return "Tesselation Control";
    case Shader::TESSELLATION_EVAL:
        return "Tesselation Evaluation";
    case Shader::GEOMETRY:
        return "Geometry";
    case Shader::PIXEL:
        return "Pixel";
    case Shader::COMPUTE:
        return "Compute";
    default:
        return "Invalid Stage";
    }
}


Shader::Shader(const Specification& s) : m_isCompute(false) {
    m_specification         = s;
    m_nextUnusedFileIndex   = 1;

    m_name = "???";

    // Find the first non-empty name
    for (int i = 0; i < STAGE_COUNT; ++i) {
        if ((s.shaderStage[i].type == FILE) && (s.shaderStage[i].val != "")) {
            m_name = FilePath::baseExt(s.shaderStage[i].val) + ", etc.";

            const String& b = FilePath::base(s.shaderStage[i].val);
            // See if all other stages match
            for (++i; i < STAGE_COUNT; ++i) {
                if (((s.shaderStage[i].type == STRING) && (s.shaderStage[i].val != "")) || 
                    ((s.shaderStage[i].type == FILE) && (FilePath::base(s.shaderStage[i].val) != b) && (s.shaderStage[i].val != ""))) {
                    // Inconsistent names
                    return;
                }
            }

            m_name = b + ".*";
            return;
        }
    }
}


void Shader::reloadAll() {
    debugPrintf("____________________________________________________________________\n\n");
    debugPrintf("Reloading all shaders...\n\n");

    for (int i = 0; i < s_allShaders.length(); ++i) {
        shared_ptr<Shader> s = s_allShaders[i].lock();
        if (notNull(s)) {
            s->reload();
        } else {
            // Remove element i from list, since that shader has been garbage collected
            s_allShaders.fastRemove(i);
            --i;
        }
    }
}


bool Shader::isImageType(GLenum e) {
    return e == GL_IMAGE_1D ||
            e == GL_IMAGE_2D ||
			e == GL_IMAGE_2D_MULTISAMPLE ||
            e == GL_IMAGE_2D_ARRAY ||
            e == GL_IMAGE_3D ||
            e == GL_IMAGE_2D_RECT ||
            e == GL_IMAGE_CUBE ||
            e == GL_IMAGE_BUFFER ||
            e == GL_INT_IMAGE_1D ||
            e == GL_INT_IMAGE_2D ||
			e == GL_INT_IMAGE_2D_MULTISAMPLE ||
            e == GL_INT_IMAGE_3D ||
            e == GL_INT_IMAGE_2D_RECT ||
            e == GL_INT_IMAGE_CUBE ||
            e == GL_INT_IMAGE_BUFFER ||
            e == GL_UNSIGNED_INT_IMAGE_1D ||
            e == GL_UNSIGNED_INT_IMAGE_2D ||
			e == GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE ||
            e == GL_UNSIGNED_INT_IMAGE_3D ||
            e == GL_UNSIGNED_INT_IMAGE_2D_RECT ||
            e == GL_UNSIGNED_INT_IMAGE_CUBE ||
            e == GL_UNSIGNED_INT_IMAGE_BUFFER;
}


bool Shader::isSamplerType(GLenum e) {
    return
       (e == GL_SAMPLER_1D) ||
       (e == GL_INT_SAMPLER_1D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_1D) ||

       (e == GL_SAMPLER_2D) ||
       (e == GL_INT_SAMPLER_2D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_2D) ||
       (e == GL_SAMPLER_2D_RECT) ||
	   (e == GL_SAMPLER_2D_MULTISAMPLE) ||
	   (e == GL_INT_SAMPLER_2D_MULTISAMPLE) ||
	   (e == GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE) ||

       (e == GL_SAMPLER_2D_ARRAY) ||

       (e == GL_SAMPLER_3D) ||
       (e == GL_INT_SAMPLER_3D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_3D) ||

       (e == GL_SAMPLER_CUBE) ||
       (e == GL_INT_SAMPLER_CUBE) ||
       (e == GL_UNSIGNED_INT_SAMPLER_CUBE) ||

       (e == GL_SAMPLER_1D_SHADOW) ||

       (e == GL_SAMPLER_2D_SHADOW) ||
       (e == GL_SAMPLER_2D_RECT_SHADOW) ||

       (e == GL_SAMPLER_BUFFER) ||
       (e == GL_INT_SAMPLER_BUFFER) ||
       (e == GL_UNSIGNED_INT_SAMPLER_BUFFER);
}


GLenum Shader::canonicalType(GLenum e) {
#   define IMAGES_AS_TEXTURES 1

    switch (e) {
#   if IMAGES_AS_TEXTURES
    case GL_IMAGE_1D:
    case GL_INT_IMAGE_1D:
    case GL_UNSIGNED_INT_IMAGE_1D:
#   endif
    case GL_SAMPLER_1D_ARB:
    case GL_INT_SAMPLER_1D_EXT:
    case GL_UNSIGNED_INT_SAMPLER_1D_EXT:
        return GL_TEXTURE_1D;

#   if IMAGES_AS_TEXTURES
    case GL_IMAGE_2D:
    case GL_INT_IMAGE_2D:
    case GL_UNSIGNED_INT_IMAGE_2D:
	//case GL_IMAGE_2D_MULTISAMPLE:
#   endif
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D:
	//case GL_SAMPLER_2D_MULTISAMPLE:
    case GL_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
	//case GL_TEXTURE_2D_MULTISAMPLE:
        return GL_TEXTURE_2D;
        
#   if IMAGES_AS_TEXTURES
    case GL_IMAGE_CUBE:
    case GL_INT_IMAGE_CUBE:
    case GL_UNSIGNED_INT_IMAGE_CUBE:
#   endif
    case GL_SAMPLER_CUBE_ARB:
    case GL_INT_SAMPLER_CUBE_EXT:
    case GL_UNSIGNED_INT_SAMPLER_CUBE_EXT:
        return GL_TEXTURE_CUBE_MAP_ARB;
        
#   if IMAGES_AS_TEXTURES
    case GL_IMAGE_2D_RECT:
    case GL_INT_IMAGE_2D_RECT:
    case GL_UNSIGNED_INT_IMAGE_2D_RECT:
#   endif
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:
    case GL_SAMPLER_2D_RECT_ARB:
        return GL_TEXTURE_RECTANGLE_EXT;

#   if IMAGES_AS_TEXTURES
    case GL_IMAGE_3D:
    case GL_INT_IMAGE_3D:
    case GL_UNSIGNED_INT_IMAGE_3D:
#   endif
    case GL_SAMPLER_3D_ARB:
    case GL_INT_SAMPLER_3D_EXT:
    case GL_UNSIGNED_INT_SAMPLER_3D_EXT:
        return GL_TEXTURE_3D;

    case GL_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        return GL_TEXTURE_2D_ARRAY;

    case GL_SAMPLER_BUFFER:
        return GL_TEXTURE_BUFFER;

	//NV specific
	case GL_UNSIGNED_INT64_NV :
	case GL_GPU_ADDRESS_NV :
		return GL_GPU_ADDRESS_NV;

	//Texture multi-sample
#   if IMAGES_AS_TEXTURES
	case GL_IMAGE_2D_MULTISAMPLE:
	case GL_INT_IMAGE_2D_MULTISAMPLE:
	case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
#   endif
	case GL_SAMPLER_2D_MULTISAMPLE:
	case GL_INT_SAMPLER_2D_MULTISAMPLE:
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
	case GL_TEXTURE_2D_MULTISAMPLE:
        return GL_TEXTURE_2D_MULTISAMPLE;

    default:
        // Return the input
        return e;    
    }
}


void Shader::bindUniformArg(const Args::Arg& arg, const ShaderProgram::UniformDeclaration& decl, int& maxModifiedTextureUnit){
    const GLint location = decl.location;
    // Bind based on the *declared* type
    if (isNull(arg.handle)) { // Passing by handle is not type safe, so doing a type check here doesn't make much sense.
        debugAssertM((canonicalType(decl.type) == canonicalType(arg.type)),
            format("Mismatching types for uniform arg %s. Program requires %s, tried to bind %s.  Note that Textures passed as Images must use setImageUniform.",
            decl.name.c_str(), GLenumToString(decl.type), GLenumToString(arg.type)));
    }
    switch (decl.type) {
    case GL_IMAGE_1D:
    case GL_IMAGE_2D:
	case GL_IMAGE_2D_MULTISAMPLE:
    case GL_IMAGE_2D_ARRAY:
	case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
	case GL_IMAGE_3D:
    case GL_IMAGE_CUBE:
    case GL_INT_IMAGE_1D:
    case GL_INT_IMAGE_2D:
	case GL_INT_IMAGE_2D_MULTISAMPLE:
	case GL_INT_IMAGE_2D_ARRAY:
	case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_INT_IMAGE_3D:
    case GL_INT_IMAGE_CUBE:
    case GL_UNSIGNED_INT_IMAGE_1D:
    case GL_UNSIGNED_INT_IMAGE_2D:
	case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
	case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
	case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_3D:
    case GL_UNSIGNED_INT_IMAGE_CUBE:
        {
#if 1 //Non bindless
            // Images are bound as if they were integers.  The
            // value of the image is the image unit into which
            // the texture was bound.
            debugAssert(decl.glUnit >= 0);
            glUniform1i(decl.location, decl.glUnit);
			bool layered    =  (decl.type==GL_IMAGE_3D) ? GL_TRUE : GL_FALSE;
            GLint layer     = 0;
            GLint mipLevel  = arg.value[0].ui;
            GLenum access   = arg.value[1].ui;
            GLenum glformat = arg.texture->format()->openGLShaderImageFormat();
            glBindImageTexture(decl.glUnit, arg.texture->openGLID(), mipLevel, layered, layer, access, glformat);

#else		//New bindless image suport

			//debugAssert(decl.glUnit >= 0);
			//glUniform1i(decl.location, decl.glUnit);
			bool layered = (decl.type == GL_IMAGE_3D) ? GL_TRUE : GL_FALSE;
			GLint layer = 0;
			GLint mipLevel = arg.value[0].ui;
			GLenum access = arg.value[1].ui;
			GLenum glformat = arg.texture->format()->openGLShaderImageFormat();

			uint64 ui64 = arg.texture->getBindlessHandle(mipLevel, access);
		
			glUniformHandleui64ARB(decl.location, ui64);
			debugAssertGLOk();

#endif
            break;
        }
	

    case GL_IMAGE_BUFFER:
    case GL_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
        {
            debugAssert(decl.glUnit >= 0);
            glUniform1i(decl.location, decl.glUnit);
            /** TODO: Support layered textures */
            GLint mipLevel  = arg.value[0].ui;
            alwaysAssertM(mipLevel == 0, "Texture Buffers only have a single mip level");
            GLenum access   = arg.value[1].ui;
            glBindImageTexture(decl.glUnit, arg.bufferTexture->openGLID(), mipLevel, GL_FALSE, 0, access, arg.bufferTexture->format()->openGLShaderImageFormat() );
            break;
        }

    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
	case GL_SAMPLER_2D_MULTISAMPLE:
	case GL_SAMPLER_2D_ARRAY:

    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:

    case GL_INT_SAMPLER_1D:
    case GL_INT_SAMPLER_2D:
	case GL_INT_SAMPLER_2D_ARRAY:
	case GL_INT_SAMPLER_2D_MULTISAMPLE:
	case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:

    case GL_UNSIGNED_INT_SAMPLER_1D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
	case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
        {
            if (arg.type == GL_UNSIGNED_INT64_ARB) { // Handles
                glUniformHandleui64ARB(decl.location, arg.handle->glHandle());
            } else {

                // Textures are bound as if they were integers.  The
                // value of the texture is the texture unit into which
                // the texture is placed.
                debugAssert(decl.glUnit >= 0);

                glUniform1i(decl.location, decl.glUnit);
                // Directly make the OpenGL binding call
                glActiveTexture(decl.glUnit + GL_TEXTURE0);
                glBindTexture(arg.texture->openGLTextureTarget(), arg.texture->openGLID());
                const shared_ptr<GLSamplerObject>& sampler = arg.sampler;
                static const bool hasSampler = GLCaps::supports("GL_ARB_sampler_objects");
                if (hasSampler) {
                    if (notNull(sampler)) {
                        glBindSampler(decl.glUnit, sampler->openGLID());
                        maxModifiedTextureUnit = max(decl.glUnit, maxModifiedTextureUnit);
                    }
                    else {
                        glBindSampler(decl.glUnit, 0);
                    }
                }
                else if (notNull(sampler)) {
                    arg.texture->updateSamplerParameters(sampler->sampler());
                }
            }
        }
        break;

    case GL_SAMPLER_BUFFER:
    case GL_INT_SAMPLER_BUFFER:
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        // Textures are bound as if they were integers.  The
        // value of the texture is the texture unit into which
        // the texture is placed.
        debugAssert(decl.glUnit >= 0);
        glUniform1i(decl.location, decl.glUnit);
       
        // Directly make the OpenGL binding call
        glActiveTexture(decl.glUnit + GL_TEXTURE0);
        glBindTexture(arg.bufferTexture->openGLTextureTarget(), arg.bufferTexture->openGLID());
        
        break;
    

    case GL_INT:
        {
            int32 i = arg.value[0].i;
            glUniform1i(location, i);
        }
        break;

    case GL_BOOL:
        {
            bool b = arg.value[0].b;
            glUniform1i(location, b);
        }
        break;

    case GL_UNSIGNED_INT:
        {
            uint32 ui = arg.value[0].ui;
            glUniform1ui(location, ui);
        }
        break;

    case GL_FLOAT:
        {
            float f = arg.value[0].f;
            glUniform1f(location, f);
        }
        break;

    case GL_FLOAT_VEC2:
        {
            float f0 = arg.value[0].f;
            float f1 = arg.value[1].f;
            glUniform2f(location, f0, f1);
        }
        break;

    case GL_FLOAT_VEC3:
        {
            float f0 = arg.value[0].f;
            float f1 = arg.value[1].f;
            float f2 = arg.value[2].f;
            glUniform3f(location, f0, f1, f2);
        }
        break;

    case GL_FLOAT_VEC4:
        {
            float f0 = arg.value[0].f;
            float f1 = arg.value[1].f;
            float f2 = arg.value[2].f;
            float f3 = arg.value[3].f;
            glUniform4f(location, f0, f1, f2, f3);
        }
        break;

    case GL_INT_VEC2:
        {
            int32 i0 = arg.value[0].i;
            int32 i1 = arg.value[1].i;
            glUniform2i(location, i0, i1);
        }
        break;

    case GL_UNSIGNED_INT_VEC2:
    {
        uint32 i0 = arg.value[0].ui;
        uint32 i1 = arg.value[1].ui;
        glUniform2ui(location, i0, i1);
    }
    break;

    case GL_BOOL_VEC2:
        {
            bool b0 = arg.value[0].b;
            bool b1 = arg.value[1].b;
            glUniform2i(location, b0, b1);
        }
        break;

    case GL_INT_VEC3:
        {
            int32 i0 = arg.value[0].i;
            int32 i1 = arg.value[1].i;
            int32 i2 = arg.value[2].i;
            glUniform3i(location, i0, i1, i2);
        }
        break;

    case GL_BOOL_VEC3:
        {
            bool b0 = arg.value[0].b;
            bool b1 = arg.value[1].b;
            bool b2 = arg.value[2].b;
            glUniform3i(location, b0, b1, b2);
        }
        break;

    case GL_INT_VEC4:
        {
            int32 i0 = arg.value[0].i;
            int32 i1 = arg.value[1].i;
            int32 i2 = arg.value[2].i;
            int32 i3 = arg.value[3].i;
            glUniform4i(location, i0, i1, i2, i3);
        }
        break;

    case GL_BOOL_VEC4:
        {
            bool b0 = arg.value[0].b;
            bool b1 = arg.value[1].b;
            bool b2 = arg.value[2].b;
            bool b3 = arg.value[3].b;
            glUniform4i(location, b0, b1, b2, b3);
        }
        break;

    case GL_FLOAT_MAT2:
        {
            float m[4];
            for (int i = 0; i < 4; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix2fv(location, 1, GL_FALSE, m);
        }            
        break;

    case GL_FLOAT_MAT3_ARB:
        {
            float m[9];
            for (int i = 0; i < 9; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix3fv(location, 1, GL_FALSE, m);
        }            
        break;

    case GL_FLOAT_MAT4:
        {
            float m[16];
            for (int i = 0; i < 16; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix4fv(location, 1, GL_FALSE, m);
        }
        break;

    case GL_FLOAT_MAT4x3:
        {
            float m[12];
            for (int i = 0; i < 12; ++i) {
                m[i] = arg.value[i].f;
            }
            glUniformMatrix4x3fv(location, 1, GL_FALSE, m);
        }
        break;

	//NVIDIA specific
	case GL_GPU_ADDRESS_NV:
		{
			uint64 ui64 = arg.value[0].ui64;
			glUniformui64NV(location, ui64);
		}
		break;

    default:

        alwaysAssertM(false, format("Unsupported argument type: %s", GLenumToString(decl.type)));
    } // switch on type
}

G3D_DECLARE_SYMBOL(g3d_);
G3D_DECLARE_SYMBOL(_noset_);

#ifndef G3D_WINDOWS
   // We don't use SSESmallString on OS X, unfortunately
    bool beginsWith_g3d_(const String& s) {
        return beginsWith(s, SYMBOL_g3d_);
    }
#else
    __forceinline bool beginsWith_g3d_(const SSESmallString<64>& s) {
        // We know that G3D strings are safe to read at least 8 bytes of at once because they
        // allocate an internal "stack" buffer when short. We only need four bytes for this
        // test.
        return *reinterpret_cast<const uint32*>(&s[0]) == *reinterpret_cast<const uint32*>(&SYMBOL_g3d_[0]);
    }
#endif

void Shader::bindUniformArgs(const shared_ptr<ShaderProgram>& program, const Args& args, bool allowG3DArgs, int& maxModifiedTextureUnit){
	
    // Iterate through the formal parameter list
    const ShaderProgram::UniformDeclarationTable& t = program->uniformDeclarationTable;
    for (ShaderProgram::UniformDeclarationTable::Iterator i = t.begin(); i != t.end(); ++i){

        const ShaderProgram::UniformDeclaration& decl  = (*i).value;
    
        // Normal user defined variable
        // Variables with g3d_ are allowed here if useG3DArgs is disabled
        if (!decl.dummy && (allowG3DArgs || !beginsWith_g3d_(decl.name))) {
            try {
                const Args::Arg& arg = args.uniform(decl.name);
                bindUniformArg(arg, decl, maxModifiedTextureUnit);
            } catch (const UniformTable::UnboundArgument& e) {
                alwaysAssertM(false, 
                              format("Shader uniform %s was not bound when applying shader %s", 
                                     e.name.c_str(), 
                                     name().c_str()));
            } // try
        } // if
    } // for
}


static bool isG3DAttribute(const String& name) {
    static Array<String> g3dAttributes;
    static bool initialized = false;

    if (! initialized) {
        initialized = true;
        g3dAttributes.append("g3d_Vertex", "g3d_Normal", "g3d_Color", "g3d_TexCoord0", "g3d_TexCoord1", "g3d_PackedTangent");
        g3dAttributes.append("g3d_VertexColor", "g3d_BoneWeights", "g3d_BoneIndices");
    }
    return g3dAttributes.contains(name);
}


// TODO: Note, we only use program for the attribute table. Can we abstract this to make it unneccessary to pass a program?
void Shader::bindStreamArgs(const shared_ptr<ShaderProgram>& program, const Args& args, RenderDevice* rd) {
    debugAssertGLOk();
    // Iterate through the formal parameter list
    const Args::GPUAttributeTable& t   = args.gpuAttributeTable();
    const ShaderProgram::AttributeDeclarationTable& attributeInformationTable = program->attributeDeclarationTable;

    for (Args::GPUAttributeTable::Iterator i = t.begin(); i != t.end(); ++i){

        const Args::GPUAttribute&   v    = (*i).value;
        const String&               name = (*i).key;

        if (beginsWith_g3d_(name)) { 
            // Our "built-ins", which we will assign them even if the shader doesn't use them
            if (isG3DAttribute(name)) {
				ShaderProgram::AttributeDeclaration* declPtr = attributeInformationTable.getPointer(name);
				if (notNull(declPtr)) {
					const ShaderProgram::AttributeDeclaration& decl = *declPtr;
                    debugAssertM(decl.name == name, format("%s != %s\n", decl.name.c_str(), name.c_str()));
                    if (decl.location >= 0) {
                        rd->setVertexAttribArray(decl.location, v.attributeArray);
                        debugAssertGLOk();
                        glVertexAttribDivisor(decl.location, v.divisor);
                    }
                }
            } else {
                alwaysAssertM(false, format("There is no built-in G3D attribute named %s.\n", name.c_str()));
            }
        } else  {
			ShaderProgram::AttributeDeclaration* declPtr = attributeInformationTable.getPointer(name);
			if (notNull(declPtr)) {
				const ShaderProgram::AttributeDeclaration& decl = *declPtr;
                if (decl.location >= 0) { 
                    // Not a  dummy arg
                    rd->setVertexAttribArray(decl.location, v.attributeArray);

                    if (v.divisor > 0) {
                        alwaysAssertM(v.attributeArray.size() >= args.numInstances(), 
                            format("Instance attribute array %s has only %d elements, but it must have at least as "
                                   "many elements as the number of instances (%d) for the draw call.",
                            decl.name.c_str(), v.attributeArray.size(), args.numInstances()));
                    }
                    glVertexAttribDivisor(decl.location, v.divisor);
                }
            } else {
                alwaysAssertM(false, format("Tried to assign attribute to %s that was not used in the shader.\n", name.c_str()));
            }            
        } 
    }

    if (args.getPrimitiveType() == PrimitiveType::PATCHES) {
        glPatchParameteri(GL_PATCH_VERTICES, args.patchVertices);
    }
}

void Shader::unbindStreamArgs(const shared_ptr<ShaderProgram>& program, const Args& args, RenderDevice* rd) {
    // Iterate through the formal parameter list
    const Args::GPUAttributeTable& t   = args.gpuAttributeTable();
    const ShaderProgram::AttributeDeclarationTable& attributeInformationTable = program->attributeDeclarationTable;

    for (Args::GPUAttributeTable::Iterator i = t.begin(); i != t.end(); ++i){
        const String&               name = (*i).key;
		ShaderProgram::AttributeDeclaration* declPtr = attributeInformationTable.getPointer(name);
		if (notNull(declPtr)) {
			const ShaderProgram::AttributeDeclaration& decl = *declPtr;
            if (decl.location >= 0) { 
                // Not a dummy arg
                rd->unsetVertexAttribArray(decl.location);
                debugAssertGLOk();
                glVertexAttribDivisor(decl.location, 0);
            }
        }     
    }
    
    // glPatchParamter not set
}


shared_ptr<Shader::ShaderProgram> Shader::compileAndBind(const Args& args, RenderDevice* rd, int& maxModifiedTextureUnit){
    String messages;
    
    // May be overwritten below by handleRecoverableError
    shared_ptr<ShaderProgram> program = shaderProgram(args, messages);

    if (isNull(program)) {
        handleRecoverableError(COMPILATION_ERROR, args, messages, program);
    }
    
    if (notNull(program)) {
        glUseProgram(program->glShaderProgramObject());

        maxModifiedTextureUnit = -1;
        if ( args.useG3DArgs() ) {
            bindG3DArgs(program, rd, args, maxModifiedTextureUnit);
        }
        bindUniformArgs(program, args, !args.useG3DArgs(), maxModifiedTextureUnit);
    }
	
    return program;
}

bool Shader::sameSource(const Source& a, const Source& b) {
    return a.type == b.type && a.val == b.val;
}

bool Shader::sameSpec(const Specification& a, const Specification& b) {
    for (int s = 0; s < STAGE_COUNT; ++s) {
        if (!sameSource(a.shaderStage[s], b.shaderStage[s])) {
	    return false;
        }
    }
    return true;
}

shared_ptr<Shader> Shader::getShaderFromCacheOrCreate(const Specification& spec) {
    for (int i = 0; i < s_allShaders.size(); ++i) {
        shared_ptr<Shader> s = s_allShaders[i].lock();
        if (notNull(s) && sameSpec(s->m_specification, spec)) {
            return s;
        }
    }

    return create(spec);
}

static void filterInvalidShaders(Array<String>& files) {
    static Array<String> validExtensions;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;
        validExtensions.append("vrt", "vtx", "ctl", "hul", "evl", "dom");
        validExtensions.append("geo", "pix", "frg", "glc", "glsl");
    }

    for (int i = 0; i < files.size(); ++i) {
        const String extension = toLower(FilePath::ext(files[i]));
        if (!validExtensions.contains(extension)) {
            files.remove(i);
            --i;
        }
    }    
}

shared_ptr<Shader> Shader::getShaderFromPattern(const String& pattern) {
    String absolutePattern = System::findDataFile(pattern, false);

    if (absolutePattern.empty()) {
        size_t prefixLoc = pattern.find("_");
	if (prefixLoc == String::npos) {
	  // Intentionally throw error so we get details
	  System::findDataFile(pattern);
	}
        alwaysAssertM(prefixLoc != 0, format("LAUNCH_SHADER pattern (%s) must not begin with an underscore, since that implies an empty directory name.", pattern.c_str()));

        const String& directory = pattern.substr(0,prefixLoc);
        absolutePattern = System::findDataFile(FilePath::concat(directory, pattern));
    }
    
    Array<String> files;
    FileSystem::getFiles(absolutePattern, files, true);

    filterInvalidShaders(files);

    Specification s(files);
    return getShaderFromCacheOrCreate(s);
}

shared_ptr<Shader> Shader::fromFiles
(const String& f0, 
 const String& f1, 
 const String& f2, 
 const String& f3, 
 const String& f4) {

    const Specification s(f0, f1, f2, f3, f4);
    return create(s);
}


bool Shader::ShaderProgram::containsNonDummyUniform(const String& name) {
	const UniformDeclaration* decl = uniformDeclarationTable.getPointer(name);
	return notNull(decl) && decl->dummy == false;
}

G3D_DECLARE_SYMBOL(g3d_ObjectToWorldMatrix);
G3D_DECLARE_SYMBOL(g3d_ObjectToCameraMatrix);
G3D_DECLARE_SYMBOL(g3d_ProjectionMatrix);
G3D_DECLARE_SYMBOL(g3d_ProjectToPixelMatrix);
G3D_DECLARE_SYMBOL(g3d_ObjectToScreenMatrix);
G3D_DECLARE_SYMBOL(g3d_CameraToWorldMatrix);
G3D_DECLARE_SYMBOL(g3d_ObjectToWorldNormalMatrix);
G3D_DECLARE_SYMBOL(g3d_ObjectToCameraNormalMatrix);
G3D_DECLARE_SYMBOL(g3d_CameraToObjectNormalMatrix);
G3D_DECLARE_SYMBOL(g3d_WorldToObjectNormalMatrix);
G3D_DECLARE_SYMBOL(g3d_WorldToObjectMatrix);
G3D_DECLARE_SYMBOL(g3d_WorldToCameraMatrix);
G3D_DECLARE_SYMBOL(g3d_WorldToCameraNormalMatrix);
G3D_DECLARE_SYMBOL(g3d_InvertY);
G3D_DECLARE_SYMBOL(g3d_FragCoordExtent);
G3D_DECLARE_SYMBOL(g3d_FragCoordMin);
G3D_DECLARE_SYMBOL(g3d_FragCoordMax);
G3D_DECLARE_SYMBOL(g3d_SceneTime);

void Shader::bindG3DArgs(const shared_ptr<ShaderProgram>& p, RenderDevice* renderDevice, const Args& sourceArgs, int& maxModifiedTextureUnit) {
    const CoordinateFrame& o2w = renderDevice->objectToWorldMatrix();
    const CoordinateFrame& c2w = renderDevice->cameraToWorldMatrix();

    // The one arg, continually reused.
    static UniformTable::Arg arg;
	const ShaderProgram::UniformDeclaration* decl;

#   define ARG(nameString, val)\
	decl = p->uniformDeclarationTable.getPointer(nameString);\
	if (notNull(decl) && decl->dummy == false) {\
        arg.value.clear(false);\
        arg.set((val), false);\
		bindUniformArg(arg, *decl, maxModifiedTextureUnit); \
    }

    // Bind matrices
    ARG("g3d_ObjectToWorldMatrix", o2w);    
    ARG("g3d_ProjectionMatrix", renderDevice->projectionMatrix());
    ARG("g3d_CameraToWorldMatrix", c2w);

    // The whole section below should just be replaced with something that
    // takes the projection matrix and scales it by the screen resolution.
    // However, doing so didn't work during debugging and this was the easiest
    // patch before release.
    const Matrix4& P = renderDevice->projectionMatrix();
    Matrix4 projectionPixelMatrix;
    if (P[3][2] != 0.0f) {
        // Perspective projection
        Projection projection(P);
        projection.getProjectPixelMatrix(renderDevice->viewport(), projectionPixelMatrix);
        projectionPixelMatrix = projectionPixelMatrix * Matrix4::scale(1, -1 * renderDevice->invertYMatrix()[1][1], 1);
    } else {
        // Likely orthographic
        projectionPixelMatrix = P;
    }

    ARG("g3d_ProjectToPixelMatrix", projectionPixelMatrix);
    ARG("g3d_ObjectToWorldNormalMatrix", o2w.rotation);
    ARG("g3d_ObjectToCameraMatrix", c2w.inverse() * o2w);
    ARG("g3d_ObjectToCameraNormalMatrix", c2w.inverse().rotation * o2w.rotation);
    ARG("g3d_CameraToObjectNormalMatrix", (c2w.inverse().rotation * o2w.rotation).inverse());
    ARG("g3d_WorldToObjectNormalMatrix", o2w.rotation.transpose());
    ARG("g3d_WorldToObjectMatrix", o2w.inverse());
    ARG("g3d_WorldToCameraMatrix", c2w.inverse());
    ARG("g3d_WorldToCameraNormalMatrix", c2w.rotation.inverse());
    ARG("g3d_InvertY", renderDevice->invertY());
    const Matrix4& M = renderDevice->objectToScreenMatrix();
    ARG("g3d_ObjectToScreenMatrix", M);
    ARG("g3d_ObjectToScreenMatrixTranspose", M.transpose());

    if (p->containsNonDummyUniform("g3d_SceneTime")) {
        float time;
		if (notNull(GApp::current()) && notNull(GApp::current()->scene())) {
			time = (float)GApp::current()->scene()->time();
        } else {
            static const RealTime initTime = System::time();
            time = (float)(System::time() - initTime);
        }
        ARG("g3d_SceneTime", time);
    }

    if (sourceArgs.hasRect()) {
        ARG("g3d_FragCoordMin", sourceArgs.rect().x0y0());
        ARG("g3d_FragCoordExtent", sourceArgs.rect().wh());
        ARG("g3d_FragCoordMax", sourceArgs.rect().x1y1());
    }
    
    ARG("g3d_NumInstances", sourceArgs.numInstances());
#   undef ARG
}


GLuint Shader::toGLEnum(ShaderStage s) {
    const GLuint name[] = {GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER};
    return name[s];
}


void Shader::load() {
    String loadMessages;
    bool ok = true;

    // Map code source 0 to generated code
    m_indexToFilenameTable.set(0, "G3D Inserted Code");     
    m_fileNameToIndexTable.set("G3D Inserted Code", 0);
	
    for (int s = 0; s < STAGE_COUNT; ++s) {
        debugAssertGLOk();

        const Source& source = m_specification.shaderStage[s];

        PreprocessedShaderSource pSource;
        String& code = pSource.preprocessedCode;
        String& name = pSource.filename;
        String dir = "";

        // Read the code into a string
        if (source.type == STRING) {
            code = source.val;
            name = format("<:%s:>", stageName(s).c_str());
            if (s == COMPUTE) {
                m_isCompute = true;
            }
        } else {
            name = source.val;
            if (! name.empty()) {
                name = FileSystem::resolve(name);
                code = readWholeFile(name);
                dir = filenamePath(name);

                if (s == COMPUTE) {
                    m_isCompute = true;
                }
            } 
        }

        debugAssertGLOk();
        // There is no code, then there is nothing to preprocess
        if (! code.empty()) {
            ok = g3dLoadTimePreprocessor(dir, pSource, loadMessages, toGLEnum(ShaderStage(s))) && ok;
            if (! ok) {
                break;
            }
        }
        m_preprocessedSource.append(pSource);
    }

    if (m_preprocessedSource[VERTEX].preprocessedCode.empty()
        && ! m_preprocessedSource[PIXEL].preprocessedCode.empty()) {
        // Use the default vertex shader
        static String defaultVertexShaderFile = FileSystem::resolve(System::findDataFile("default.vrt"));
        static String defaultVertexShaderCode = readWholeFile(defaultVertexShaderFile);

        m_preprocessedSource[VERTEX].filename         = defaultVertexShaderFile;
        m_preprocessedSource[VERTEX].preprocessedCode = defaultVertexShaderCode;
        m_preprocessedSource[VERTEX].versionString    = "#version 330\n";

        String ignore;
        processExtensions(ignore, m_preprocessedSource[VERTEX].extensionsString);
    }

    if (! ok) {
        // A loading error occured
        shared_ptr<ShaderProgram> dummy;
        Args ignore;
        handleRecoverableError(LOAD_ERROR, ignore, loadMessages, dummy); 
    }
}


void Shader::handleRecoverableError(RecoverableErrorType eType,  const Args& args, const String& messages, shared_ptr<ShaderProgram>& program){
    if (s_failureBehavior == PROMPT) {
        const int cRetry = 0;
        const int cDebug = 1;
        const int cExit = 2;
        const char* options[3];

        options[cRetry] = "Reload";
        options[cDebug] = "Debug";
        options[cExit] = "Exit";

        // Copy the result into a std::string temporarily in case G3D::String is the cause of the problem
        const std::string& m = messages.c_str();

        // the output after the message string has had all warnings removed
        std::string output;

        size_t begining = 0;
        size_t split = m.find("\n");

        while (split != std::string::npos) {
            const std::string singleError = m.substr(begining, split);
            String lowerCaseError = toLower(singleError.c_str());
            //The error is considered valid if it does not both the words extensioin and warning
            if (lowerCaseError.find("warning") == String::npos || lowerCaseError.find("extension") == String::npos) {
                output.append(singleError + "\n");
            }
            begining = split + 1;
            split = m.find("\n", begining);
        }

        //the full error message is debug printed.  
        debugPrintf("%s\n", m.c_str());

        //The parsed message is displayed on the debug dialog
        int userAction = prompt("Shader Compilation Failed",
            output.c_str(),
            (const char**)options,
            3,
            true);

        switch (userAction) {
        case cDebug:
            rawBreak();
            break;

        case cRetry:
            if (eType == LOAD_ERROR) {
                reload();
            } else if (eType == COMPILATION_ERROR) {
                program = retry(args);
            }
            break;

        case cExit:
            exit(-1);
            break;
        }

    } else if (s_failureBehavior == EXCEPTION) {
        if (eType == LOAD_ERROR) {
            alwaysAssertM(false, format("Shader Load Error (see log): \n %s\n", messages.c_str()));
        } else if (eType == COMPILATION_ERROR) {
            alwaysAssertM(false, format("Shader Compilation Error (see log): \n %s\n", messages.c_str()));
        }

    } // else failureBehavior is silent, so don't do anything 
}


shared_ptr<Shader::ShaderProgram> Shader::shaderProgram(const Args& args, String& messages){
    const String& preambleAndMacroString = args.preambleAndMacroString();

    shared_ptr<ShaderProgram>* sp = m_compilationCache.getPointer(preambleAndMacroString);
    shared_ptr<ShaderProgram> s;

    if (isNull(sp)) { 
        // There was no cached value
		debugAssertGLOk();

        s = ShaderProgram::create(m_preprocessedSource, preambleAndMacroString, args, m_indexToFilenameTable);
        debugAssertGLOk();
        sp = &s;        
        
        if (s->ok) {
            m_compilationCache.set(preambleAndMacroString, s);
        } else {
            messages = s->messages;
            return shared_ptr<ShaderProgram>();
        }                
    }

    return *sp;
}


void Shader::setFailureBehavior(FailureBehavior f){
    s_failureBehavior = f;
}
    

void Shader::reload(){
    m_compilationCache.clear();
    m_preprocessedSource.clear();
    m_g3dUniformArgs.clearUniformBindings();
    load();
}


shared_ptr<Shader::ShaderProgram> Shader::retry(const Args& args){
    reload(); 
    String messages;
    shared_ptr<ShaderProgram> program = shaderProgram(args, messages);
    
    if (isNull(program)) {
        handleRecoverableError(COMPILATION_ERROR, args, messages, program);
    }

    return program;
}


shared_ptr<Shader> Shader::create(const Specification& s){
    shared_ptr<Shader> shader(new Shader(s));
    shader->load();

    s_allShaders.append(shader);

    return shader;
}


Shader::DomainType Shader::domainType(const shared_ptr<Shader>& s, const Args& args) {
    if (s->isCompute()) {
        if (args.hasIndirectBuffer()) {
            return INDIRECT_COMPUTE_MODE;
        } else if (args.hasComputeGrid()) {
            return STANDARD_COMPUTE_MODE;
        } else {
            return ERROR_MODE;
        }
    } else if (args.hasIndirectBuffer()) {
        return INDIRECT_RENDERING_MODE;
    } else if (args.hasStreamArgs()) {
        if (args.hasGPUIndexStream()) {
            if (args.indexStreamArray().size() > 0) {
                return MULTIDRAW_INDEXED_RENDERING_MODE;
            } else {
                return STANDARD_INDEXED_RENDERING_MODE;
            }
        } else {
            if (args.indexCountArray().size() > 0) {
                return MULTIDRAW_NONINDEXED_RENDERING_MODE;
            } else {
                return STANDARD_NONINDEXED_RENDERING_MODE;
            }
            
        }
    } else if (args.hasRect()) {
	    return RECT_MODE;
    } else if (args.numIndices() > 0) {  
        // Note: this case must come last since it is an error to call
        // numIndices in any other mode.
        return STANDARD_NONINDEXED_RENDERING_MODE;
    } else {
        // Also note that this case is unreachable since numIndices
        // either returns a positive number or throws an error.
        return ERROR_MODE;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////

Shader::Source::Source(const String& value) {
    // All valid shader code has a semicolon, no filenames do if you're sane.
    alwaysAssertM(value.find(';') != String::npos, format("The Source(string) constructor only \
        accepts GLSL code, not filenames. The passed in string was:\n %s \nIf this looks like code, look \
        for missing semicolons (all valid code should have at least one).\nIf this is a filename, use \
        the Source(SourceType, string) constructor instead, with a SourceType of FILE.\n", value.c_str()));
    type = STRING;
    val = value;
}


Shader::Source::Source(SourceType t, const String& value) :
    type(t), val(value) {
}


// These dummy values are ignored by Shader::load
Shader::Source::Source() :
    type(FILE), val("") {
}


////////////////////////////////////////////////////////////////////////////////////////


void Shader::Specification::setStages(const Array<String>& filenames) {
    for (int i = 0; i < filenames.size(); ++i) {
        const String& fname = filenames[i];

        if (fname != "") { // Skip blanks
            alwaysAssertM(fname.size() > 4, format("Invalid filename given to Shader::Specification():\n%s", fname.c_str()));
            const String extension = toLower(FilePath::ext(fname));

            // Determine the stage
            ShaderStage stage;
            if (extension == "vrt" || extension == "vtx") {
                stage = VERTEX;
            } else if (extension == "ctl" || extension == "hul") {
                stage = TESSELLATION_CONTROL;
            } else if (extension == "evl" || extension == "dom") {
                stage = TESSELLATION_EVAL;
            } else if (extension == "geo") {
                stage = GEOMETRY;
            } else if (extension == "pix" || extension == "frg") {
                stage = PIXEL;
            } else if (extension == "glc" || extension == "glsl") {
                stage = COMPUTE;
            } else {
		stage = COMPUTE;
                alwaysAssertM(false, format("Invalid filename given to Shader::Specification():\n%s", fname.c_str()));
            }

            // Finally, set the source
            shaderStage[stage] = Source(FILE, fname);
            
        } // if filename not empty
    } // For each filename
}

Shader::Specification::Specification(const Array<String>& filenames) {
    setStages(filenames);
}

Shader::Specification::Specification
(const String& f0, 
 const String& f1, 
 const String& f2, 
 const String& f3, 
 const String& f4) {

    const Array<String> filenames(f0, f1, f2, f3, f4);

    setStages(filenames);    
}


Shader::Specification::Specification() {}


Shader::Specification::Specification(const Any& any) {
    if (any.containsKey("vertexFile")){
        shaderStage[VERTEX].val  = any["vertexFile"].string();
        shaderStage[VERTEX].type = FILE;
    } else if(any.containsKey("vertexString")){
        shaderStage[VERTEX] = any["vertexString"].string();
    }

    if (any.containsKey("tessellationEvalFile")){
        shaderStage[TESSELLATION_EVAL].val  = any["tessellationEvalFile"].string();
        shaderStage[TESSELLATION_EVAL].type = FILE;
    } else if(any.containsKey("tessellationEvalString")){
        shaderStage[TESSELLATION_EVAL].val = any["tessellationEvalString"].string();
    }

    if (any.containsKey("tessellationControlFile")){
        shaderStage[TESSELLATION_CONTROL].val  = any["tessellationControlFile"].string();
        shaderStage[TESSELLATION_CONTROL].type = FILE;
    } else if(any.containsKey("tessellationControlString")){
        shaderStage[TESSELLATION_CONTROL].val = any["tessellationControlString"].string();
    } 

    if (any.containsKey("geometryFile")){
        shaderStage[GEOMETRY].val  = any["geometryFile"].string();
        shaderStage[GEOMETRY].type = FILE;
    } else if (any.containsKey("geometryString")){
        shaderStage[GEOMETRY].val = any["geometryString"].string();
    } 

    if (any.containsKey("pixelFile")){
        shaderStage[PIXEL].val  = any["pixelFile"].string();
        shaderStage[PIXEL].type = FILE;
    } else if (any.containsKey("pixelString")){
        shaderStage[PIXEL].val = any["pixelString"].string();
    } 

    if (any.containsKey("computeFile")){
        shaderStage[COMPUTE].val  = any["computeFile"].string();
        shaderStage[COMPUTE].type = FILE;
    } else if (any.containsKey("computeString")){
        shaderStage[COMPUTE].val = any["computeString"].string();
    } 
}


Shader::Source& Shader::Specification::operator[](ShaderStage s){
    return shaderStage[s];
}


const Shader::Source& Shader::Specification::operator[](ShaderStage s) const {
    return shaderStage[s];
}


shared_ptr<Shader> Shader::unlit() {
    static shared_ptr<Shader> s = Shader::fromFiles(System::findDataFile("unlit.vrt"), System::findDataFile("unlit.pix"));
    return s;
}

} // namespace G3D

