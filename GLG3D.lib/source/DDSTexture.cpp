/**
 @file DDSTexture.cpp

 @author Corey Taylor

 Notes:
 <UL>
 <LI>http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/DDSFileReference/ddsfileformat.asp
 </UL>

 @created 2004-09-30
 @edited  2006-02-30
*/

#include "G3D/platform.h"
#include "G3D/Log.h"
#include "GLG3D/glcalls.h"
#include "G3D/ImageFormat.h"
#include "GLG3D/Texture.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/GLCaps.h"
#include "G3D/BinaryInput.h"

#include <cstring>

namespace G3D {

// Define the Windows data types for linux/OSX
#ifndef BYTE
#define BYTE uint8
#endif

#ifndef WORD
#define WORD uint16
#endif

#ifndef DWORD
#define DWORD uint32
#endif

#ifndef LONG
#define LONG uint32
#endif

#ifndef LPVOID
#define LPVOID void*
#endif

#define DUMMYUNIONNAMEN(n)

#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif //defined(MAKEFOURCC)


//FOURCC codes for DX compressed-texture pixel formats

#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))

// DDPIXELFORMAT flags
const uint32 DDPF_ALPHAPIXELS   = 0x00000001l;
const uint32 DDPF_FOURCC        = 0x00000004l;
const uint32 DDPF_RGB           = 0x00000040l;
 

//DDPIXELFORMAT

typedef struct _DDPIXELFORMAT
{
    DWORD       dwSize;                 // size of structure
    DWORD       dwFlags;                // pixel format flags
    DWORD       dwFourCC;               // (FOURCC code)
    union
    {
        DWORD   dwRGBBitCount;          // how many bits per pixel
        DWORD   dwYUVBitCount;          // how many bits per pixel
        DWORD   dwZBufferBitDepth;      // how many total bits/pixel in z buffer (including any stencil bits)
        DWORD   dwAlphaBitDepth;        // how many bits for alpha channels
        DWORD   dwLuminanceBitCount;    // how many bits per pixel
        DWORD   dwBumpBitCount;         // how many bits per "buxel", total
        DWORD   dwPrivateFormatBitCount;// Bits per pixel of private driver formats. Only valid in texture
                                        // format list and if DDPF_D3DFORMAT is set
    } DUMMYUNIONNAMEN(1);
    union
    {
        DWORD   dwRBitMask;             // mask for red bit
        DWORD   dwYBitMask;             // mask for Y bits
        DWORD   dwStencilBitDepth;      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
        DWORD   dwLuminanceBitMask;     // mask for luminance bits
        DWORD   dwBumpDuBitMask;        // mask for bump map U delta bits
        DWORD   dwOperations;           // DDPF_D3DFORMAT Operations
    } DUMMYUNIONNAMEN(2);
    union
    {
        DWORD   dwGBitMask;             // mask for green bits
        DWORD   dwUBitMask;             // mask for U bits
        DWORD   dwZBitMask;             // mask for Z bits
        DWORD   dwBumpDvBitMask;        // mask for bump map V delta bits
        struct
        {
            WORD    wFlipMSTypes;       // Multisample methods supported via flip for this D3DFORMAT
            WORD    wBltMSTypes;        // Multisample methods supported via blt for this D3DFORMAT
        } MultiSampleCaps;

    } DUMMYUNIONNAMEN(3);
    union
    {
        DWORD   dwBBitMask;             // mask for blue bits
        DWORD   dwVBitMask;             // mask for V bits
        DWORD   dwStencilBitMask;       // mask for stencil bits
        DWORD   dwBumpLuminanceBitMask; // mask for luminance in bump map
    } DUMMYUNIONNAMEN(4);
    union
    {
        DWORD   dwRGBAlphaBitMask;      // mask for alpha channel
        DWORD   dwYUVAlphaBitMask;      // mask for alpha channel
        DWORD   dwLuminanceAlphaBitMask;// mask for alpha channel
        DWORD   dwRGBZBitMask;          // mask for Z channel
        DWORD   dwYUVZBitMask;          // mask for Z channel
    } DUMMYUNIONNAMEN(5);
} DDPIXELFORMAT;


/*
 * DDSCAPS2
 */
typedef struct _DDSCAPS2
{
    DWORD       dwCaps;         // capabilities of surface wanted
    DWORD       dwCaps2;
    DWORD       dwCaps3;
    union
    {
        DWORD       dwCaps4;
        DWORD       dwVolumeDepth;
    } DUMMYUNIONNAMEN(1);
} DDSCAPS2;


const uint32 DDSCAPS2_VOLUME                        = 0x00200000L;
const uint32 DDSCAPS2_CUBEMAP                       = 0x00000200L;
const uint32 DDSCAPS2_CUBEMAP_POSITIVEX             = 0x00000400L;
const uint32 DDSCAPS2_CUBEMAP_NEGATIVEX             = 0x00000800L;
const uint32 DDSCAPS2_CUBEMAP_POSITIVEY             = 0x00001000L;
const uint32 DDSCAPS2_CUBEMAP_NEGATIVEY             = 0x00002000L;
const uint32 DDSCAPS2_CUBEMAP_POSITIVEZ             = 0x00004000L;
const uint32 DDSCAPS2_CUBEMAP_NEGATIVEZ             = 0x00008000L;



/*
 * DDCOLORKEY
 */
typedef struct _DDCOLORKEY
{
    DWORD       dwColorSpaceLowValue;   // low boundary of color space that is to
                                        // be treated as Color Key, inclusive
    DWORD       dwColorSpaceHighValue;  // high boundary of color space that is
                                        // to be treated as Color Key, inclusive
} DDCOLORKEY;

/*
 * DDSURFACEDESC2
 */
typedef struct _DDSURFACEDESC2
{
    DWORD               dwSize;                 // size of the DDSURFACEDESC structure
    DWORD               dwFlags;                // determines what fields are valid
    DWORD               dwHeight;               // m_height of surface to be created
    DWORD               dwWidth;                // m_width of input surface
    union
    {
        LONG            lPitch;                 // distance to start of next line (return value only)
        DWORD           dwLinearSize;           // Formless late-allocated optimized surface size
    } DUMMYUNIONNAMEN(1);
    union
    {
        DWORD           dwBackBufferCount;      // number of back buffers requested
        DWORD           dwDepth;                // the depth if this is a volume texture 
    } DUMMYUNIONNAMEN(5);
    union
    {
        DWORD           dwMipMapCount;          // number of mip-map levels requestde
                                                // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
        DWORD           dwRefreshRate;          // refresh rate (used when display mode is described)
        DWORD           dwSrcVBHandle;          // The source used in VB::Optimize
    } DUMMYUNIONNAMEN(2);
    DWORD               dwAlphaBitDepth;        // depth of alpha buffer requested
    DWORD               dwReserved;             // reserved
    LPVOID              lpSurface;              // pointer to the associated surface memory
    union
    {
        DDCOLORKEY      ddckCKDestOverlay;      // color key for destination overlay use
        DWORD           dwEmptyFaceColor;       // Physical color for empty cubemap faces
    } DUMMYUNIONNAMEN(3);
    DDCOLORKEY          ddckCKDestBlt;          // color key for destination blt use
    DDCOLORKEY          ddckCKSrcOverlay;       // color key for source overlay use
    DDCOLORKEY          ddckCKSrcBlt;           // color key for source blt use
    union
    {
        DDPIXELFORMAT   ddpfPixelFormat;        // pixel format description of the surface
        DWORD           dwFVF;                  // vertex format description of vertex buffers
    } DUMMYUNIONNAMEN(4);
    DDSCAPS2            ddsCaps;                // direct draw surface capabilities
    DWORD               dwTextureStage;         // stage in multitexture cascade
} DDSURFACEDESC2;

/*
 * ddsCaps field is valid.
 */
const uint32  DDSD_CAPS               = 0x00000001l;     // default

/*
 * dwHeight field is valid.
 */
const uint32  DDSD_HEIGHT             = 0x00000002l;

/*
 * dwWidth field is valid.
 */
const uint32  DDSD_WIDTH              = 0x00000004l;

/*
 * lPitch is valid.
 */
const uint32  DDSD_PITCH              = 0x00000008l;

/*
 * dwBackBufferCount is valid.
 */
const uint32  DDSD_BACKBUFFERCOUNT    = 0x00000020l;

/*
 * dwZBufferBitDepth is valid.  (shouldnt be used in DDSURFACEDESC2)
 */
const uint32  DDSD_ZBUFFERBITDEPTH    = 0x00000040l;

/*
 * dwAlphaBitDepth is valid.
 */
const uint32  DDSD_ALPHABITDEPTH      = 0x00000080l;


/*
 * lpSurface is valid.
 */
const uint32  DDSD_LPSURFACE          = 0x00000800l;

/*
 * ddpfPixelFormat is valid.
 */
const uint32  DDSD_PIXELFORMAT        = 0x00001000l;

/*
 * ddckCKDestOverlay is valid.
 */
const uint32  DDSD_CKDESTOVERLAY      = 0x00002000l;

/*
 * ddckCKDestBlt is valid.
 */
const uint32  DDSD_CKDESTBLT          = 0x00004000l;

/*
 * ddckCKSrcOverlay is valid.
 */
const uint32  DDSD_CKSRCOVERLAY       = 0x00008000l;

/*
 * ddckCKSrcBlt is valid.
 */
const uint32  DDSD_CKSRCBLT           = 0x00010000l;

/*
 * dwMipMapCount is valid.
 */
const uint32  DDSD_MIPMAPCOUNT        = 0x00020000l;

 /*
  * dwRefreshRate is valid
  */
const uint32  DDSD_REFRESHRATE        = 0x00040000l;

/*
 * dwLinearSize is valid
 */
const uint32  DDSD_LINEARSIZE         = 0x00080000l;

/*
 * dwTextureStage is valid
 */
const uint32  DDSD_TEXTURESTAGE       = 0x00100000l;
/*
 * dwFVF is valid
 */
const uint32  DDSD_FVF                = 0x00200000l;
/*
 * dwSrcVBHandle is valid
 */
const uint32  DDSD_SRCVBHANDLE        = 0x00400000l;

/*
 * dwDepth is valid
 */
const uint32  DDSD_DEPTH              = 0x00800000l;

/*
 * All input fields are valid.
 */
const uint32  DDSD_ALL                 = 0x00fff9eel;

Texture::DDSTexture::DDSTexture(const String& filename) :
    m_bytes(NULL),
    m_bytesFormat(ImageFormat::AUTO()),
    m_width(0),
    m_height(0),
    m_numMipMaps(0),
    m_numFaces(0) {

    BinaryInput ddsInput(filename, G3D_LITTLE_ENDIAN);

    String ddsString = ddsInput.readString(4);
    debugAssertM(ddsString == "DDS ", "Invalid DDS file");

    DDSURFACEDESC2 ddsSurfaceDesc;
    ddsInput.readBytes(&ddsSurfaceDesc, sizeof(ddsSurfaceDesc));

    if (!GLCaps::supports_GL_ARB_texture_non_power_of_two())
    {
        // Check the texture size for powers of 2.
        debugAssert( isPow2(ddsSurfaceDesc.dwWidth) );
        debugAssert( isPow2(ddsSurfaceDesc.dwHeight) );
    }

    // Check for enough valid header flags to import file
    debugAssert( ddsSurfaceDesc.dwFlags & DDSD_WIDTH );
    debugAssert( ddsSurfaceDesc.dwFlags & DDSD_HEIGHT );
    debugAssert( ddsSurfaceDesc.dwFlags & DDSD_PIXELFORMAT );

    // Check to make sure it isn't a volume texture
    debugAssert( !(ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME) ); 

    if ( ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP ) {
        debugAssert( (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) &&
            (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) &&
            (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) &&
            (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY) &&
            (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) &&
            (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) &&
            (ddsSurfaceDesc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) );

        m_numFaces = 6;
    } else {
        m_numFaces = 1;
    }

    debugAssertM(ddsSurfaceDesc.ddpfPixelFormat.dwFlags & DDPF_FOURCC, "Unsuported DDS format");

    if (ddsSurfaceDesc.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
        switch(ddsSurfaceDesc.ddpfPixelFormat.dwFourCC) {
            case FOURCC_DXT1:
                m_bytesFormat = ImageFormat::RGBA_DXT1();
                break;
            case FOURCC_DXT3:
                m_bytesFormat = ImageFormat::RGBA_DXT3();
                break;
            case FOURCC_DXT5:
                m_bytesFormat = ImageFormat::RGBA_DXT5();
                break;
                
            default:
                debugAssertM(false, "Unsupported DXT DDS format");
                break;
        }
    }

    uint8* ddsInputPos = const_cast<uint8*>(ddsInput.getCArray());
    debugAssert(ddsInputPos);

    ddsInputPos += sizeof(ddsSurfaceDesc) + 4;

    //Setup DDS data
    m_bytes = new uint8[size_t(ddsInput.size()) - size_t(ddsInput.getPosition())];
    memcpy(m_bytes, ddsInputPos, size_t(ddsInput.size() - ddsInput.getPosition()));

    if( ddsSurfaceDesc.dwFlags & DDSD_MIPMAPCOUNT ) {
        m_numMipMaps = ddsSurfaceDesc.dwMipMapCount;
    } else {
        m_numMipMaps = 1;
    }

    m_width = ddsSurfaceDesc.dwWidth;
    m_height = ddsSurfaceDesc.dwHeight;
}


Texture::DDSTexture::~DDSTexture() {
    delete[] m_bytes;
}

} //G3D
