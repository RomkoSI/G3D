/**
  @file GLCaps.cpp

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu
  @created 2004-03-28
  @edited  2012-06-21
*/

#include "G3D/TextOutput.h"
#include "G3D/RegistryUtil.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/glcalls.h"
#include "G3D/ImageFormat.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/NetworkDevice.h"
#include "G3D/Log.h"

#include <sstream>

namespace G3D {

// Global init flags for GLCaps.  Because this is an integer constant (equal to zero),
// we can safely assume that it will be initialized before this translation unit is
// entered.
bool GLCaps::m_loadedExtensions = false;
bool GLCaps::m_initialized = false;
bool GLCaps::m_checkedForBugs = false;

int GLCaps::m_glMajorVersion = 0;
int GLCaps::m_numTextureCoords = 0;
int GLCaps::m_numTextures = 0;
int GLCaps::m_numTextureUnits = 0;

int GLCaps::m_maxTextureSize = 0;
int GLCaps::m_maxTextureBufferSize = 0;
int GLCaps::m_maxCubeMapSize = 0;

bool GLCaps::m_hasTexelFetch = false;

float GLCaps::m_glslVersion = 0.0f;

/**
 Dummy function to which unloaded extensions can be set.
 */
static void __stdcall glIgnore(GLenum e) {
    (void)e;
}

/** Cache of values supplied to supportsImageFormat.
    Works on pointers since there is no way for users
    to construct their own ImageFormats.
 */
static Table<const ImageFormat*, bool>& _supportedImageFormat() {
    static Table<const ImageFormat*, bool> cache;
    return cache;
}

static Table<const ImageFormat*, bool>& _supportedTextureDrawBufferFormat() {
    static Table<const ImageFormat*, bool> cache;
    return cache;
}

Set<String> GLCaps::extensionSet;

void GLCaps::getExtensions(Array<String>& extensions) {
  extensionSet.getMembers(extensions);
}


const ImageFormat* GLCaps::smallHDRFormat() {
    static const ImageFormat* f = nullptr;

    if (!f) {
        Array<const ImageFormat*> preferredColorFormats(ImageFormat::R11G11B10F(), ImageFormat::RGB16F(), ImageFormat::RGBA16F(), ImageFormat::RGB32F(), ImageFormat::RGBA32F());
        for (int i = 0; i < preferredColorFormats.size(); ++i) {
            if (GLCaps::supportsTexture(preferredColorFormats[i])) {
                f = preferredColorFormats[i];
                return f;
            }
        }
    }

    return f;
}


GLCaps::Vendor GLCaps::computeVendor() {
    static bool initialized = false;
    static Vendor v;

    if (! initialized) {
        String s = vendor();
        String _glVersion = (char*)glGetString(GL_VERSION);

        if (s == "ATI Technologies Inc.") {
            v = ATI;
        } else if (s == "NVIDIA Corporation") {
            v = NVIDIA;
        } else if ((s == "Brian Paul") || (s == "Mesa project: www.mesa3d.org") || 
                   (_glVersion.find("Mesa ") != String::npos)) {
            v = MESA;
        } else {
            v = ARB;
        }
        initialized = true;
    }

    return v;
}

GLCaps::Vendor GLCaps::enumVendor() {
    return computeVendor();
}


#ifdef G3D_WINDOWS
/**
 Used by the Windows version of getDriverVersion().
 @cite Based on code by Ted Peck tpeck@roundwave.com http://www.codeproject.com/dll/ShowVer.asp
 */
struct VS_VERSIONINFO { 
    WORD                wLength; 
    WORD                wValueLength; 
    WORD                wType; 
    WCHAR               szKey[1]; 
    WORD                Padding1[1]; 
    VS_FIXEDFILEINFO    Value; 
    WORD                Padding2[1]; 
    WORD                Children[1]; 
};
#endif

String GLCaps::getDriverVersion() {
    if (computeVendor() == MESA) {
        // Mesa includes the driver version in the renderer version
        // e.g., "1.5 Mesa 6.4.2"
        
        static String _glVersion = (char*)glGetString(GL_VERSION);
        size_t i = _glVersion.rfind(' ');
        if (i == String::npos) {
            return "Unknown (bad MESA driver string)";
        } else {
            return _glVersion.substr(i + 1, _glVersion.length() - i);
        }
    }
    
#ifdef G3D_WINDOWS
 
    // locate the driver on Windows and get the version
    // this systems expects Windows 2000/XP/Vista
    String videoDriverKey;

    bool canCheckVideoDevice = RegistryUtil::keyExists("HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\VIDEO");

    if (canCheckVideoDevice) {

        // find the driver expected to load
        String videoDeviceKey = "HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\VIDEO";
        String videoDeviceValue = "\\Device\\Video";
        int videoDeviceNum = 0;

        while (RegistryUtil::valueExists(videoDeviceKey, format("%s%d", videoDeviceValue.c_str(), videoDeviceNum))) {
            ++videoDeviceNum;
        }

        // find the key where the installed driver lives
        String installedDriversKey;
        RegistryUtil::readString(videoDeviceKey, format("%s%d", videoDeviceValue.c_str(), videoDeviceNum - 1), installedDriversKey);

        // find and remove the "\Registry\Machine\" part of the key
        size_t subKeyIndex = installedDriversKey.find('\\', 1);
        subKeyIndex = installedDriversKey.find('\\', subKeyIndex + 1);

        installedDriversKey.erase(0, subKeyIndex);

        // read the list of driver files this display driver installed/loads
        // this is a multi-string value, but we only care about the first entry so reading one string is fine
        
        String videoDrivers;
        if ( ! RegistryUtil::valueExists("HKEY_LOCAL_MACHINE" + installedDriversKey,  "InstalledDisplayDrivers") ) {
            return "Unknown (Can't find driver)";
        }
        RegistryUtil::readString("HKEY_LOCAL_MACHINE" + installedDriversKey, "InstalledDisplayDrivers", videoDrivers);

        if (videoDrivers.find(',', 0) != String::npos) {
            videoDrivers = videoDrivers.substr(0, videoDrivers.find(',', 0));
        }

        char systemDirectory[512] = "";
        GetSystemDirectoryA(systemDirectory, sizeof(systemDirectory));

        String driverFileName = format("%s\\%s.dll", systemDirectory, videoDrivers.c_str());

        DWORD dummy;
        int size = GetFileVersionInfoSizeA((LPCSTR)driverFileName.c_str(), &dummy);
        if (size == 0) {
            return "Unknown (Can't find driver)";
        }

        void* buffer = new uint8[size];

        if (GetFileVersionInfoA((LPCSTR)driverFileName.c_str(), 0, size, buffer) == 0) {
            delete[] (uint8*)buffer;
            return "Unknown (Can't find driver)";
        }

        // Interpret the VS_VERSIONINFO header pseudo-struct
        VS_VERSIONINFO* pVS = (VS_VERSIONINFO*)buffer;
        debugAssert(! wcscmp(pVS->szKey, L"VS_VERSION_INFO"));

        uint8* pVt = (uint8*) &pVS->szKey[wcslen(pVS->szKey) + 1];

        #define roundoffs(a,b,r)    (((uint8*)(b) - (uint8*)(a) + ((r) - 1)) & ~((r) - 1))
        #define roundpos(b, a, r)    (((uint8*)(a)) + roundoffs(a, b, r))

        VS_FIXEDFILEINFO* pValue = (VS_FIXEDFILEINFO*) roundpos(pVt, pVS, 4);

        #undef roundoffs
        #undef roundpos

        String result = "Unknown (Can't find driver)";

        if (pVS->wValueLength) {
            result = format("%d.%d.%d.%d",
                pValue->dwProductVersionMS >> 16,
                pValue->dwProductVersionMS & 0xFFFF,
                pValue->dwProductVersionLS >> 16,
                pValue->dwProductVersionLS & 0xFFFF);
        }

        delete[] (uint8*)buffer;

        return result;

    } else {
        return "Unknown (Can't find driver)";
    }

    #else
        return "Unknown";
    #endif
}

void GLCaps::init() {

    const char* v = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    GLenum e = glGetError();
    if ((v != NULL) && (e == GL_NONE)) {
        sscanf((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION), "%f", &m_glslVersion);
    } else {
        m_glslVersion = 0;
    }
    // Remove all errors generated by glGetString
    while (glGetError() != GL_NONE) {}
    debugAssertGLOk();

    loadExtensions(Log::common());

    // Remove all pending errors
    while (glGetError() != GL_NONE) {}
    debugAssertGLOk();

    checkAllBugs();
    debugAssertGLOk();
    glClearColor(1,1,1,1);
    debugAssertGLOk();
    glClear(GL_COLOR_BUFFER_BIT);
    debugAssertGLOk();
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    debugAssertGLOk();
}

// We're going to need exactly the same code for each of 
// several extensions.
#define DECLARE_EXT(extname)    bool GLCaps::_supports_##extname = false; 
    DECLARE_EXT(GL_ARB_texture_float);
    DECLARE_EXT(GL_ARB_texture_non_power_of_two);
    DECLARE_EXT(GL_EXT_texture_rectangle);
    DECLARE_EXT(GL_ARB_vertex_program);
    DECLARE_EXT(GL_NV_vertex_program2);
    DECLARE_EXT(GL_ARB_vertex_buffer_object);
    DECLARE_EXT(GL_ARB_fragment_program);
    DECLARE_EXT(GL_ARB_multitexture);
    DECLARE_EXT(GL_EXT_texture_edge_clamp);
    DECLARE_EXT(GL_ARB_texture_border_clamp);
    DECLARE_EXT(GL_EXT_texture3D);
    DECLARE_EXT(GL_EXT_stencil_wrap);
    DECLARE_EXT(GL_EXT_stencil_two_side);
    DECLARE_EXT(GL_ATI_separate_stencil);    
    DECLARE_EXT(GL_EXT_texture_compression_s3tc);
    DECLARE_EXT(GL_EXT_texture_cube_map);
    DECLARE_EXT(GL_EXT_separate_specular_color);
    DECLARE_EXT(GL_ARB_shadow);
    DECLARE_EXT(GL_ARB_shader_objects);
    DECLARE_EXT(GL_ARB_shading_language_100);
    DECLARE_EXT(GL_ARB_fragment_shader);
    DECLARE_EXT(GL_ARB_vertex_shader);
    DECLARE_EXT(GL_EXT_geometry_shader4);
    DECLARE_EXT(GL_ARB_framebuffer_object);
    DECLARE_EXT(GL_ARB_framebuffer_sRGB);
    DECLARE_EXT(GL_SGIS_generate_mipmap);
    DECLARE_EXT(GL_EXT_texture_mirror_clamp);
    DECLARE_EXT(GL_EXT_framebuffer_object);
    DECLARE_EXT(GL_ARB_sync);
    DECLARE_EXT(GL_NV_fence);
    DECLARE_EXT(GL_ARB_texture_buffer_object);
#undef DECLARE_EXT


void GLCaps::loadExtensions(Log* debugLog) {
    // This is here to prevent a spurrious warning under gcc
    glIgnore(0);

    debugAssert(glGetString(GL_VENDOR) != NULL);

    if (m_loadedExtensions) {
        return;
    } else {
        m_loadedExtensions = true;
    }

    alwaysAssertM(! m_initialized, "Internal error.");

    // Require an OpenGL context to continue
    alwaysAssertM(glGetCurrentContext(), "Unable to load OpenGL extensions without a current context.");

    // Initialize statically cached strings
    vendor();
    renderer();
    glVersion();
    driverVersion();

    // Initialize cached GL major version pulled from glVersion() for extensions made into 2.0/3.0 core
    const String glver = glVersion();
    m_glMajorVersion = glver[0] - '0';
    bool hasGLMajorVersion2 = m_glMajorVersion >= 2;
    bool hasGLMajorVersion3 = m_glMajorVersion >= 3;

    // Turn on OpenGL 3.0
    glewExperimental = GL_TRUE;

    GLenum err = glewInit();
    alwaysAssertM(err == GLEW_OK, format("Error Initializing OpenGL Extensions (GLEW): %s\n", glewGetErrorString(err)));

    if (hasGLMajorVersion3) {
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	for (GLuint i = 0; i < (GLuint)numExtensions; ++i) {
	    extensionSet.insert(String((char*)glGetStringi(GL_EXTENSIONS, i)));
	}
    } else { // Old way to do it
        std::istringstream extensions;
	String extStringCopy = (char*)glGetString(GL_EXTENSIONS);
	extensions.str(extStringCopy.c_str());

        // Parse the extensions into the supported set
        std::string s;
        while (extensions >> s) {
            extensionSet.insert(String(s.c_str()));
        }

    }

    {

        // We're going to need exactly the same code for each of 
        // several extensions.
#       define DECLARE_EXT(extname) _supports_##extname = supports(#extname)
#       define DECLARE_EXT_GL2(extname) _supports_##extname = (supports(#extname) || hasGLMajorVersion2)
#       define DECLARE_EXT_GL3(extname) _supports_##extname = (supports(#extname) || hasGLMajorVersion3)
            DECLARE_EXT(GL_ARB_texture_float);
            DECLARE_EXT_GL2(GL_ARB_texture_non_power_of_two);
            DECLARE_EXT(GL_EXT_texture_rectangle);
            DECLARE_EXT(GL_ARB_vertex_program);
            DECLARE_EXT(GL_NV_vertex_program2);
            DECLARE_EXT(GL_ARB_vertex_buffer_object);
            DECLARE_EXT(GL_EXT_texture_edge_clamp);
            DECLARE_EXT_GL2(GL_ARB_texture_border_clamp);
            DECLARE_EXT(GL_EXT_texture3D);
            DECLARE_EXT_GL2(GL_ARB_fragment_program);
            DECLARE_EXT_GL2(GL_ARB_multitexture);
            DECLARE_EXT_GL2(GL_EXT_separate_specular_color);
            DECLARE_EXT(GL_EXT_stencil_wrap);
            DECLARE_EXT(GL_EXT_stencil_two_side);
            DECLARE_EXT(GL_ATI_separate_stencil);            
            DECLARE_EXT(GL_EXT_texture_compression_s3tc);
            DECLARE_EXT(GL_EXT_texture_cube_map);
            DECLARE_EXT_GL2(GL_ARB_shadow);
            DECLARE_EXT_GL2(GL_ARB_shader_objects);
            DECLARE_EXT_GL2(GL_ARB_shading_language_100);
            DECLARE_EXT(GL_ARB_fragment_shader);
            DECLARE_EXT(GL_ARB_vertex_shader);
            DECLARE_EXT(GL_EXT_geometry_shader4);
            DECLARE_EXT_GL3(GL_ARB_framebuffer_object);
            DECLARE_EXT_GL3(GL_ARB_framebuffer_sRGB);
            DECLARE_EXT(GL_SGIS_generate_mipmap);
            DECLARE_EXT(GL_EXT_texture_mirror_clamp);
            DECLARE_EXT(GL_EXT_framebuffer_object);
            DECLARE_EXT(GL_ARB_sync);
            DECLARE_EXT(GL_NV_fence);
            DECLARE_EXT(GL_ARB_texture_buffer_object);
#       undef DECLARE_EXT_GL3
#       undef DECLARE_EXT_GL2
#       undef DECLARE_EXT

        // Some extensions have aliases
         _supports_GL_EXT_texture_cube_map = _supports_GL_EXT_texture_cube_map || supports("GL_ARB_texture_cube_map");
         _supports_GL_EXT_texture_edge_clamp = _supports_GL_EXT_texture_edge_clamp || supports("GL_SGIS_texture_edge_clamp");


        // Verify that multitexture loaded correctly
        if (supports_GL_ARB_multitexture() &&
            ((glActiveTextureARB == NULL) ||
            (glMultiTexCoord4fvARB == NULL))) {
            _supports_GL_ARB_multitexture = false;
            #ifdef G3D_WINDOWS
                *((void**)&glActiveTextureARB) = (void*)glIgnore;
            #endif
        }

        _supports_GL_EXT_texture_rectangle =_supports_GL_EXT_texture_rectangle || supports("GL_NV_texture_rectangle");

        // GL_ARB_texture_cube_map doesn't work on Radeon Mobility
        // GL Renderer:    MOBILITY RADEON 9000 DDR x86/SSE2
        // GL Version:     1.3.4204 WinXP Release
        // Driver version: 6.14.10.6430

        // GL Vendor:      ATI Technologies Inc.
        // GL Renderer:    MOBILITY RADEON 7500 DDR x86/SSE2
        // GL Version:     1.3.3842 WinXP Release
        // Driver version: 6.14.10.6371

        if ((beginsWith(renderer(), "MOBILITY RADEON") || beginsWith(renderer(), "ATI MOBILITY RADEON")) &&
            beginsWith(driverVersion(), "6.14.10.6")) {
            logPrintf("WARNING: This ATI Radeon Mobility card has a known bug with cube maps.\n"
                      "   Put cube map texture coordinates in the normals and use ARB_NORMAL_MAP to work around.\n\n");
        }
    }

    if (GLCaps::supports_GL_ARB_multitexture()) {
        m_numTextureUnits = glGetInteger(GL_MAX_TEXTURE_UNITS_ARB);
    } else {
        m_numTextureUnits = 1;
    }

    // NVIDIA cards with GL_NV_fragment_program have different 
    // numbers of texture coords, units, and textures
    m_numTextureCoords = glGetInteger(GL_MAX_TEXTURE_COORDS_ARB);
    m_numTextures = glGetInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB);

    if (! GLCaps::supports_GL_ARB_multitexture()) {
        // No multitexture
        if (debugLog) {
            debugLog->println("No GL_ARB_multitexture support: "
                              "forcing number of texture units "
                              "to no more than 1");
        }
        m_numTextureCoords = iMax(1, m_numTextureCoords);
        m_numTextures      = iMax(1, m_numTextures);
        m_numTextureUnits  = iMax(1, m_numTextureUnits);
    }
    
    m_maxTextureSize       = glGetInteger(GL_MAX_TEXTURE_SIZE);
    
    bool supportsBufferTextures = (m_glslVersion >= 1.3) ||
                                  GLCaps::supports("GL_ARB_texture_buffer_object") ||
                                  GLCaps::supports("GL_EXT_texture_buffer_object");

    m_maxTextureBufferSize = supportsBufferTextures ? glGetInteger(GL_MAX_TEXTURE_BUFFER_SIZE) : 0;
    m_maxCubeMapSize       = glGetInteger(GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT);
    m_hasTexelFetch        = (m_glslVersion >= 1.3) || GLCaps::supports("GL_EXT_gpu_shader4");

    m_initialized = true;
}


void GLCaps::checkAllBugs() {
    if (m_checkedForBugs) {
        return;
    } else {
        m_checkedForBugs = true;
    }

    alwaysAssertM(m_loadedExtensions, "Cannot check for OpenGL bugs before extensions are loaded.");
    debugAssertGLOk();
}


bool GLCaps::supports(const String& extension) {
    return extensionSet.contains(extension);
}


bool GLCaps::hasBug_R11G11B10F() {
    // TODO: make a better test, actually trying to render to this format
    return GLCaps::enumVendor() == ATI;
}


bool GLCaps::supportsTexture(const ImageFormat* fmt) {
    
    alwaysAssertM(notNull(fmt), "Can't invoke GLCaps::supportsTexture on ImageFormat::AUTO()");

    if (fmt == ImageFormat::R11G11B10F() && hasBug_R11G11B10F()) {
        return false;
    }

    debugAssertGLOk();
    // First, check if we've already tested this format
    if (! _supportedImageFormat().containsKey(fmt)) {

        bool supportsFormat = false;
        
            // OS X 10.9 supports the ARB_internalformat_query on
            // all GPUs, independent of their built-in OpenGL Core
            // mode, but does not support ARB_internalformat_query2 which
            // works for base texture formats.
            // https://developer.apple.com/graphicsimaging/opengl/capabilities/
            //
            // https://www.opengl.org/registry/specs/ARB/internalformat_query2.txt
            // TODO: Re-enable. NSight under windows complains about the glGetInternalformati64v call.
            if (false && supports("GL_ARB_internalformat_query2")) {
                // GL 4.0 style check:
                int64 result = 0;
                debugAssertGLOk();
                
                glGetInternalformati64v(GL_TEXTURE_2D, fmt->openGLFormat,
                                        GL_INTERNALFORMAT_SUPPORTED, sizeof(result), &result);
                supportsFormat = (result == GL_TRUE);
                debugAssertGLOk();
            } else {

                debugAssertGLOk();
                // GL 3.0 style check:
                {
                    // Clear the error bit
                    glGetErrors();
                    
                    // See if we can create a texture in this format
                    unsigned int id;
                    glGenTextures(1, &id);
                    glBindTexture(GL_TEXTURE_2D, id);
                    
                    // Clear the old error flag
                    glGetErrors();

                    // 2D texture, level of detail 0 (normal),
                    // internal format, x size from image, y size from
                    // image, border 0 (normal), rgb color data,
                    // unsigned byte data, and finally the data
                    // itself.
                    glTexImage2D
                        (GL_TEXTURE_2D, 
                         0, 
                         fmt->openGLFormat, 8, 8, 0, 
                         fmt->openGLBaseFormat,
                         fmt->openGLDataFormat,
                         NULL);

                    supportsFormat = (glGetError() == GL_NO_ERROR);

                    glBindTexture(GL_TEXTURE_2D, 0);
                    glDeleteTextures(1, &id);
                }
                
            } // if old-style check
        
        _supportedImageFormat().set(fmt, supportsFormat);
    }
    glGetErrors();

    return _supportedImageFormat()[fmt];
}


bool GLCaps::supportsTextureDrawBuffer(const ImageFormat* fmt) {
   
    // First, check if we've already tested this format
    if (! _supportedTextureDrawBufferFormat().containsKey(fmt)) {

        bool supportsFormat = true;
        
        // Mask off the unused channels
        Color4 mask = fmt->channelMask();

        if ( (! supports_GL_ARB_framebuffer_object() && ! supports_GL_EXT_framebuffer_object()) // No frame buffers
                || (fmt->floatingPoint && ! supports_GL_ARB_texture_float())  // No floating point texture
                || (fmt->depthBits > 0 || fmt->stencilBits > 0) // Depth and stencil, always fail test
                || (mask.sum() <= 0) // No channels would be read back!
                ){
            supportsFormat = false;
        } else {
            if (true) {
                // GL 3.0 style
                GLint result = GL_FALSE;
                glGetInternalformativ(GL_TEXTURE_2D, fmt->openGLFormat, GL_FRAMEBUFFER_RENDERABLE, 1, &result);
                supportsFormat = (result != GL_NONE);
            } else {
                // GL 1.0 style
                glPushAttrib(GL_COLOR_BUFFER_BIT); {
                    // Clear the error bit
                    glGetErrors();
                    
                    // Set up a frame buffer to render to
                    GLuint testBufferGLName = 0;
                    glGenFramebuffers(1, &testBufferGLName);
                    glBindFramebuffer(GL_FRAMEBUFFER, testBufferGLName);
                    
                    // The texture we're going to render to
                    GLuint testTexture;
                    glGenTextures(1, &testTexture);
                    
                    // "Bind" the newly created texture : all future texture functions will modify this texture
                    glBindTexture(GL_TEXTURE_2D, testTexture);
                    
                    // Make a 4x4 texture, (automatically 4-byte-aligned no matter the format), with no itialized data
                    const GLsizei   width  = 4;
                    const GLsizei   height = 4;
                    GLint           border = 0;
                    GLint           mipLevel  = 0;
                    
                    glTexImage2D(GL_TEXTURE_2D, mipLevel, fmt->openGLFormat, width, height, border, fmt->openGLBaseFormat, GL_UNSIGNED_BYTE, NULL);
                    
                    // Attach the texture to the framebuffer
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, testTexture, mipLevel);

                    // Early out test: If we can't even get this far without errors, don't try writing!
                    if (glGetError() == GL_NO_ERROR) {
                        // Setup successful
                        
                        // Set up a color array, for testing that we can validly render each color into a 4x4 texture bound to a framebuffer
                        // This could be static
                        const int testColorNum = 2;
                        
                        // The test colors (white and clear);
                        Color4 testColors[testColorNum] = {
                        Color4(1.0f, 1.0f, 1.0f, 1.0f), // White
                        Color4(0.0f, 0.0f, 0.0f, 0.0f)  // Clear (Transparent Black)
                        };
                        
                        // We will read the pixel at 0,0.
                        const int x = 0;
                        const int y = 0;
                        
                        // For each color in the test color array, render that color to the texture, read back the pixels,
                        // and make sure the color was written correctly
                        for(int i = 0; i < testColorNum; ++i){
                            const Color4& testColor = testColors[i];
                            
                            // Render the test color to the texture
                            
                            glClearColor(testColor.r, testColor.g, testColor.b, testColor.a);
                            glClear(GL_COLOR_BUFFER_BIT);
                            
                            Color4 readBackPixel;
                            
                            // Read back the pixel, need to have a
                            // different code path for integer
                            // formats, since otherwise glReadPixels
                            // will read all zeros.
                            if( !fmt->isIntegerFormat() ){
                                glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &readBackPixel);  
                            } else {
                                // We use a signed int8, since it has
                                // the lowest maxValue of all number
                                // formats.  glClear transforms 1.0f
                                // to the maxValue of the internal
                                // integer format: 127 for signed
                                // byte, 65535 for unsigned short,
                                // etc. By reading back in GL_BYTE
                                // format, we will always get 127 when
                                // we cleared to 1. If we used any
                                // other format, we'd run into cases
                                // such as R8UI being reaad back as
                                // 255 after being cleared, but R8I
                                // read back as 127. BAD! 
                                //
                                //  A more principled approach would
                                //  to actually store max values for
                                //  the various formats
                                // and use them instead.
                                int8 readBackByte[4] = {0,0,0,0};
                                glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_BYTE, &readBackByte);
                                for(int k = 0; k < 4; ++k){
                                    // A signed byte (GL_BYTE) has a
                                    // max value of 127. Since glClear
                                    // maps [0,1] to [0,maxValue], at
                                    // least for zero and one we can
                                    // obtain the original value by
                                    // dividing by 127
                                    readBackPixel[k] = float(readBackByte[k]) / float(127.0f);
                                }
                            }
                            supportsFormat &= (readBackPixel * mask == testColor * mask);
                        }
                        
                    } else { // Couldn't even set up texture and framebuffer
                        
                        supportsFormat = false;
                    }
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, mipLevel);
                    glDeleteFramebuffers(1, &testBufferGLName);
                    
                } glPopAttrib();
            } // if old style
        }
        // Clear error bit again
        glGetErrors();
        _supportedTextureDrawBufferFormat().set(fmt, supportsFormat);
    }

    return _supportedTextureDrawBufferFormat()[fmt];
}


const String& GLCaps::glVersion() {
    alwaysAssertM(m_loadedExtensions, "Cannot call GLCaps::glVersion before GLCaps::init().");
    static String _glVersion = (char*)glGetString(GL_VERSION);
    return _glVersion;
}


const String& GLCaps::driverVersion() {
    alwaysAssertM(m_loadedExtensions, "Cannot call GLCaps::driverVersion before GLCaps::init().");
    static String _driverVersion = getDriverVersion().c_str();
    return _driverVersion;
}


const String& GLCaps::vendor() {
    alwaysAssertM(m_loadedExtensions, "Cannot call GLCaps::vendor before GLCaps::init().");
    const static String _driverVendor = 

        // For debuging, force a specific vendor:
        //"ATI Technologies Inc.";
        //"NVIDIA Corporation";
        

        (char*)glGetString(GL_VENDOR);



    return _driverVendor;
}


const String& GLCaps::renderer() {
    alwaysAssertM(m_loadedExtensions, "Cannot call GLCaps::renderer before GLCaps::init().");
    static String _glRenderer = (char*)glGetString(GL_RENDERER);
    return _glRenderer;
}

 
void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    TextOutput& t) {

    System::describeSystem(t);
    rd->describeSystem(t);
    nd->describeSystem(t);
}


void describeSystem(
    class RenderDevice*  rd, 
    class NetworkDevice* nd, 
    String&        s) {
    
    TextOutput t;
    describeSystem(rd, nd, t);
    t.commitString(s);
}


const class ImageFormat* GLCaps::firstSupportedTexture(const Array<const class ImageFormat*>& prefs) {
    for (int i = 0; i < prefs.size(); ++i) {
        if (supportsTexture(prefs[i])) {
            return prefs[i];
        }
    }

    return NULL;
}


bool GLCaps::supportsG3D10(String& explanation) {
    bool supported = false;
    
    int major = 1;
    int minor = 0;
    sscanf((const char*)glGetString(GL_VERSION), "%d.%d", &major, &minor);

    int smajor = 1;
    int sminor = 0;
    
    bool hasGLSL330 = false;
    if (glGetString(GL_SHADING_LANGUAGE_VERSION)) { 
        sscanf((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION), "%d.%d", &smajor, &sminor);
        hasGLSL330 = ((smajor > 3) || (smajor == 3 && sminor >= 30));
    }
    
    supported = supported && hasGLSL330;
    explanation += format("GLSL version 3.30                   %s - GLSL version on this driver is %d.%d\n",
                          hasGLSL330 ? "yes" : "NO", smajor, sminor);
    
    if (major > 3 || (major == 3 && minor >= 3)) {
        supported = true;
        explanation += format("GPU Supports OpenGL 3.3 or later    yes - OpenGL version on this driver is %d.%d\n", major, minor);
    } else {
        explanation += format("                                   OpenGL version on this driver is %d.%d\n", major, minor);
#       define REQUIRE(ext, alt) \
        { bool has = supports(ext) || supports(alt);\
            explanation += format("%33s  %s\n", ext, (has ? " yes " : " NO - Required for G3D 9.0!"));\
            supported = supported && has; \
        }
#       define RECOMMEND(ext, alt) \
        { bool has = supports(ext) || supports(alt);\
            explanation += format("%33s  %s\n", ext, (has ? " yes - Optional" : " NO - Recommended but not required."));\
        }

        // This is an older OpenGL, but we can support it through extensions
        REQUIRE("GL_EXT_stencil_two_side", "");
        REQUIRE("GL_ARB_depth_clamp", "GL_NV_depth_clamp");
        REQUIRE("GL_ARB_texture_non_power_of_two", "");
        REQUIRE("GL_ARB_texture_float", "");
        REQUIRE("GL_ARB_geometry_shader4", "GL_EXT_geometry_shader4");
        REQUIRE("GL_ARB_sync", "");
        REQUIRE("GL_ARB_draw_buffers_blend", "");
        REQUIRE("GL_EXT_framebuffer_blit", "");

#       ifdef G3D_WINDOWS
            if (wglGetProcAddress("wglCreateContextAttribsARB") == NULL) {
                REQUIRE("WGL_ARB_create_context", "");
            }
#       else
            REQUIRE("GLX_ARB_create_context", "");
#       endif
        REQUIRE("GL_ARB_vertex_array_object", "GL_APPLE_vertex_array_object");
        REQUIRE("GL_ARB_instanced_arrays", "");

        REQUIRE("GL_ARB_shader_objects", "");
        REQUIRE("GL_ARB_shading_language_100", "");
        REQUIRE("GL_ARB_map_buffer_range", "");

        REQUIRE("GL_EXT_texture_integer", "");
        REQUIRE("GL_ARB_texture_rg", "");
    }
   
    RECOMMEND("GL_ARB_sample_shading", "");

    return supported;
}


}

