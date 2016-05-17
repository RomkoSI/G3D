/**
 \file GLG3D/GLCaps.h

 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2004-03-28
 \edited  2016-05-17

 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_GLCaps_h
#define G3D_GLCaps_h

#include "G3D/platform.h"
#include "G3D/Set.h"
#include "GLG3D/glheaders.h"
#include "G3D/NetworkDevice.h"
#include "G3D/G3DString.h"

namespace G3D {

/**
 Low-level wrapper for OpenGL extension management.
 Can be used without G3D::RenderDevice to load and
 manage extensions.

 OpenGL has a base API and an extension API.  All OpenGL drivers
 must support the base API.  The latest features may not 
 be supported by some drivers, so they are in the extension API
 and are dynamically loaded at runtime using GLCaps::loadExtensions.  
 Before using a specific extension you must test for its presence
 using the GLCaps::supports method.
 
 For convenience, frequently used extensions have fast tests, e.g.,
 GLCaps::supports_GL_EXT_texture_rectangle.

 Note that GL_NV_texture_rectangle and GL_EXT_texture_rectangle
 have exactly the same constants, so supports_GL_EXT_texture_rectangle
 returns true if either is supported.

 GLCaps assumes all OpenGL contexts have the same capabilities.

  The following extensions are shortcutted with a method named
  similarly to GLCaps::supports_GL_EXT_texture_rectangle():

  <UL>
    <LI>GL_ARB_texture_float
    <LI>GL_ARB_texture_non_power_of_two
    <LI>GL_EXT_texture_rectangle
    <LI>GL_ARB_vertex_program
    <LI>GL_NV_vertex_program2
    <LI>GL_ARB_vertex_buffer_object
    <LI>GL_ARB_fragment_program
    <LI>GL_ARB_multitexture
    <LI>GL_EXT_texture_edge_clamp
    <LI>GL_ARB_texture_border_clamp
    <LI>GL_EXT_texture_r
    <LI>GL_EXT_stencil_wrap
    <LI>GL_EXT_stencil_two_side
    <LI>GL_ATI_separate_stencil
    <LI>GL_EXT_texture_compression_s3tc
    <LI>GL_EXT_texture_cube_map, GL_ARB_texture_cube_map
    <LI>GL_ARB_shadow
    <LI>GL_ARB_shader_objects
    <LI>GL_ARB_shading_language_100
    <LI>GL_ARB_fragment_shader
    <LI>GL_ARB_vertex_shader
    <LI>GL_EXT_geometry_shader4
    <LI>GL_ARB_framebuffer_object
    <LI>GL_ARB_frambuffer_sRGB
    <LI>GL_SGIS_generate_mipmap
    <LI>GL_EXT_texture_mirror_clamp
    <LI>GL_EXT_framebuffer_object
    <LI>GL_ARB_sync
    <LI>GL_NV_fence
    </UL>

  These methods do not appear in the documentation because they
  are generated using macros.

  The <code>hasBug_</code> methods detect specific common bugs on
  graphics cards.  They can be used to switch to fallback rendering
  paths.
 */
class GLCaps {
public:

    enum Vendor {ATI, AMD = ATI, NVIDIA, MESA, ARB};

private:

    /** True when init has been called */
    static bool         m_initialized;

    /** True when loadExtensions has already been called */
    static bool         m_loadedExtensions;


    static int          m_glMajorVersion;

    /** True when checkAllBugs has been called. */
    static bool         m_checkedForBugs;

    static float        m_glslVersion;

    static int          m_numTextureCoords;
    static int          m_numTextures;
    static int          m_numTextureUnits;

    static int          m_maxTextureSize;
    static int          m_maxTextureBufferSize;
    static int          m_maxCubeMapSize;

    static bool         m_hasTexelFetch;

    static Vendor computeVendor();

    /**
       Returns the version string for the video driver
       for MESA or Windows drivers.
    */
    static String getDriverVersion();
  
 // We're going to need exactly the same code for each of 
 // several extensions, so we abstract the boilerplate into
 // a macro that declares a private variable and public accessor.
#define DECLARE_EXT(extname)                    \
private:                                        \
    static bool     _supports_##extname;        \
public:                                         \
    static bool inline supports_##extname() {   \
        return _supports_##extname;             \
    }                                           \
private:

    // New extensions must be added in 3 places: 1. here.
    // 2. at the top of GLCaps.cpp.  3. underneath the LOAD_EXTENSION code.
    DECLARE_EXT(GL_ARB_texture_float)
    DECLARE_EXT(GL_ARB_texture_non_power_of_two)
    DECLARE_EXT(GL_EXT_texture_rectangle)
    DECLARE_EXT(GL_ARB_vertex_program)
    DECLARE_EXT(GL_NV_vertex_program2)
    DECLARE_EXT(GL_ARB_vertex_buffer_object)
    DECLARE_EXT(GL_ARB_fragment_program)
    DECLARE_EXT(GL_ARB_multitexture)
    DECLARE_EXT(GL_EXT_texture_edge_clamp)
    DECLARE_EXT(GL_ARB_texture_border_clamp)
    DECLARE_EXT(GL_EXT_texture3D)
    DECLARE_EXT(GL_EXT_stencil_wrap)
    DECLARE_EXT(GL_EXT_separate_specular_color)
    DECLARE_EXT(GL_EXT_stencil_two_side)
    DECLARE_EXT(GL_ATI_separate_stencil)
    DECLARE_EXT(GL_EXT_texture_compression_s3tc)
    DECLARE_EXT(GL_EXT_texture_cube_map)
    DECLARE_EXT(GL_ARB_shadow)
    DECLARE_EXT(GL_ARB_shader_objects)
    DECLARE_EXT(GL_ARB_shading_language_100)
    DECLARE_EXT(GL_ARB_fragment_shader)
    DECLARE_EXT(GL_ARB_vertex_shader)
    DECLARE_EXT(GL_EXT_geometry_shader4)
    DECLARE_EXT(GL_ARB_framebuffer_object)
    DECLARE_EXT(GL_ARB_framebuffer_sRGB)
    DECLARE_EXT(GL_SGIS_generate_mipmap)
    DECLARE_EXT(GL_EXT_texture_mirror_clamp)
    DECLARE_EXT(GL_EXT_framebuffer_object)
    DECLARE_EXT(GL_ARB_sync)
    DECLARE_EXT(GL_NV_fence)
    DECLARE_EXT(GL_ARB_texture_buffer_object);
#undef DECLARE_EXT

    static Set<String>         extensionSet;

    /** Runs all of the checkBug_ methods. Called from loadExtensions(). */
    static void checkAllBugs();

    /** Loads OpenGL extensions (e.g. glBindBufferARB).
    Call this once at the beginning of the program,
    after a video device is created.  This is called
    for you if you use G3D::RenderDevice.    
    */
    static void loadExtensions(class Log* debugLog = NULL);

public:
    /** Loads OpenGL extensions (e.g. glBindBufferARB).
        Call this once at the beginning of the program,
        after a video device is created.  This is called
        for you if you use G3D::RenderDevice.*/
    static void init();

    static bool supports(const String& extName);

    /** Returns true if the given texture format is supported on this
        device for Textures.*/
    static bool supportsTexture(const class ImageFormat* fmt);

    /** Returns true if the given texture format is supported on this
        device for draw FrameBuffers. Note: always false for depth compressed formats */
    static bool supportsTextureDrawBuffer(const class ImageFormat* fmt);

    /** Returns the first element of \a prefs for which
        supportsTexture() returns true. Returns NULL if none are
        supported.*/
    static const class ImageFormat* firstSupportedTexture(const Array<const class ImageFormat*>& prefs);

    static const String& glVersion();

    static const String& driverVersion();

    /** e.g., 1.50 or 4.00 */
    static float glslVersion() {
        return m_glslVersion;
    }

    static const String& vendor();

    static Vendor enumVendor();

    /** Returns a small high-dynamic range (float) RGB format supported on this machine.
        Prefers: R11G11B10F, RGB16F, RGB32F
      */
    static const class ImageFormat* smallHDRFormat();

    /** 
        Returns true if this GPU/driver supports the features needed
        for the G3D  10 release, which raises the minimum
        standards for GPUs.  This call is intended to give developers
        some guidance in what to expect from the new API, however, it
        is not guaranteed to match the G3D 10.xx specification and
        requirements because that API is still under design.

        \param explanation Detailed explanation of which extensions
        are needed.
     */
    static bool supportsG3D10(String& explanation);

    static const String& renderer();

    /** Between 8 and 16 on most cards.  Can be more than number of textures. */
    static int numTextureCoords() {
        return m_numTextureCoords;
    }

    /** Between 16 and 32 on most cards. Can be more than number of fixed-function texture units. */
    static int numTextures() {
        return m_numTextures;
    }

    /** 4 on most cards. Only affects fixed function. See http://developer.nvidia.com/object/General_FAQ.html#t6  for a discussion of the number of texture units.*/
    static int numTextureUnits() {
        return m_numTextureUnits;
    }

    static int maxTextureSize() {
        return m_maxTextureSize;
    }

    static int maxTextureBufferSize() {
        return m_maxTextureBufferSize;
    }

    static int maxCubeMapSize() {
        return m_maxCubeMapSize;
    }


    /** Some ATI cards claim to support ImageFormat::R11G10B10F but render to it incorrectly. */
    static bool hasBug_R11G11B10F();

    /** Fills the array with the currently available extension names */
    static void getExtensions(Array<String>& extensions);
};


/**
 Prints a human-readable description of this machine
 to the text output stream.  Either argument may be NULL.
 */
void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    class TextOutput& t);

void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    String&        s);

} // namespace

#endif
