/**
 \file GLG3D.lib/source/Texture.cpp

 \author Morgan McGuire, http://graphics.cs.williams.edu

 \created 2001-02-28
 \edited  2016-02-16

 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
 */
#include "G3D/Log.h"
#include "G3D/Any.h"
#include "G3D/Matrix3.h"
#include "G3D/Rect2D.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"
#include "G3D/ThreadSet.h"
#include "G3D/ImageFormat.h"
#include "G3D/CoordinateFrame.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/Texture.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/Framebuffer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/BumpMap.h"
#include "GLG3D/Shader.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/GLPixelTransferBuffer.h"
#include "G3D/format.h"
#include "GLG3D/GApp.h"
#include "GLG3D/VideoRecordDialog.h"

#ifdef verify
#undef verify
#endif

namespace G3D {

void Texture::clearCache() {
    s_cache.clear();
}

WeakCache<uint64, shared_ptr<Texture> > Texture::s_allTextures;

WeakCache<Texture::Specification, shared_ptr<Texture> > Texture::s_cache;

shared_ptr<Texture> Texture::getTextureByName(const String& name) {
    Array<shared_ptr<Texture> > allTextures;
    getAllTextures(allTextures);
    for (int i = 0; i < allTextures.size(); ++i) {
        const shared_ptr<Texture>& t = allTextures[i];
        if (t->name() == name) {
            return t;
        }
    }
    return shared_ptr<Texture>();
}


size_t Texture::Specification::hashCode() const {
    return HashTrait<String>::hashCode(filename) ^ HashTrait<String>::hashCode(alphaFilename); 
}

/** Used by various Texture methods when a framebuffer is needed */
static const shared_ptr<Framebuffer>& workingFramebuffer() {
    static shared_ptr<Framebuffer> fbo = Framebuffer::create("Texture FBO");
    return fbo;
}

void Texture::getAllTextures(Array<shared_ptr<Texture> >& textures) {
    s_allTextures.getValues(textures);
}

void Texture::getAllTextures(Array<weak_ptr<Texture> >& textures) {
    Array<shared_ptr<Texture> > sharedTexturePointers;
    getAllTextures(sharedTexturePointers);
    for (int i = 0; i < sharedTexturePointers.size(); ++i) {
        textures.append(sharedTexturePointers[i]);
    }
    
}


Color4 Texture::readTexel(int x, int y, RenderDevice* rd, int mipLevel, int z) const {
    debugAssertGLOk();
    const shared_ptr<Framebuffer>& fbo = workingFramebuffer();

    if (rd == NULL) {
        rd = RenderDevice::current;
    }

    Color4 c;

    // Read back 1 pixel
    shared_ptr<Texture> me = dynamic_pointer_cast<Texture>(const_cast<Texture*>(this)->shared_from_this());
    bool is3D = dimension() == DIM_2D_ARRAY || dimension() == DIM_3D;
    int layer = is3D ? z : -1;
    if (format()->isIntegerFormat()) {
        int ints[4];
        fbo->set(Framebuffer::COLOR0, me, CubeFace::POS_X, mipLevel, layer);
        rd->pushState(fbo);
        glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_INT, ints);
        c = Color4(float(ints[0]), float(ints[1]), float(ints[2]), float(ints[3]));
        rd->popState();
    } else if (format()->depthBits == 0) {
        fbo->set(Framebuffer::COLOR0, me, CubeFace::POS_X, mipLevel, layer);
        rd->pushState(fbo);
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &c);
        rd->popState();
    } else {
        // This is a depth texture
        fbo->set(Framebuffer::DEPTH, me, CubeFace::POS_X, mipLevel, layer);
        rd->pushState(fbo);
        glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &c.r);
        rd->popState();
        c.g = c.b = c.a = c.r;
    }
    fbo->clear();
    return c;
}


const CubeMapConvention::CubeMapInfo& Texture::cubeMapInfo(CubeMapConvention convention) {
    static CubeMapConvention::CubeMapInfo cubeMapInfo[CubeMapConvention::COUNT];
    static bool initialized = false;
    if (! initialized) {
        initialized = true;
        cubeMapInfo[CubeMapConvention::QUAKE].name = "Quake";
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_X].suffix = "bk";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_X].suffix = "ft";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Y].suffix = "up";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Y].suffix = "dn";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::POS_Z].suffix = "rt";

        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::QUAKE].face[CubeFace::NEG_Z].suffix = "lf";


        cubeMapInfo[CubeMapConvention::UNREAL].name = "Unreal";
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_X].suffix = "east";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_X].suffix = "west";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Y].suffix = "up";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Y].suffix = "down";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::POS_Z].suffix = "south";

        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::UNREAL].face[CubeFace::NEG_Z].suffix = "north";


        cubeMapInfo[CubeMapConvention::G3D].name = "G3D";
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_X].suffix = "+x";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_X].suffix = "-x";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Y].suffix = "+y";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Y].suffix = "-y";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::POS_Z].suffix = "+z";

        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::G3D].face[CubeFace::NEG_Z].suffix = "-z";


        cubeMapInfo[CubeMapConvention::DIRECTX].name = "DirectX";
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_X].suffix = "PX";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_X].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_X].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_X].suffix = "NX";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Y].suffix = "PY";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Y].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Y].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Y].suffix = "NY";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::POS_Z].suffix = "PZ";

        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Z].flipX  = true;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Z].flipY  = false;
        cubeMapInfo[CubeMapConvention::DIRECTX].face[CubeFace::NEG_Z].suffix = "NZ";
    }

    return cubeMapInfo[convention];
}


CubeMapConvention Texture::determineCubeConvention(const String& filename) {
    String filenameBase, filenameExt;
    Texture::splitFilenameAtWildCard(filename, filenameBase, filenameExt);
    if (FileSystem::exists(filenameBase + "east" + filenameExt)) {
        return CubeMapConvention::UNREAL;
    } else if (FileSystem::exists(filenameBase + "lf" + filenameExt)) {
        return CubeMapConvention::QUAKE;
    } else if (FileSystem::exists(filenameBase + "+x" + filenameExt)) {
        return CubeMapConvention::G3D;
    } else if (FileSystem::exists(filenameBase + "PX" + filenameExt) || FileSystem::exists(filenameBase + "px" + filenameExt)) {
        return CubeMapConvention::DIRECTX;
    }
    throw String("File not found");
    return CubeMapConvention::G3D;
}

static void generateCubeMapFilenames(const String& src, String realFilename[6], CubeMapConvention::CubeMapInfo& info) {
    String filenameBase, filenameExt;
    Texture::splitFilenameAtWildCard(src, filenameBase, filenameExt);

    CubeMapConvention convention = Texture::determineCubeConvention(src);

    info = Texture::cubeMapInfo(convention);
    for (int f = 0; f < 6; ++f) {
        realFilename[f] = filenameBase + info.face[f].suffix + filenameExt;
    }
}

int64 Texture::m_sizeOfAllTexturesInMemory = 0;

/**
 Legacy: sets the active texture to zero
 */
static void glStatePush() {
    glActiveTexture(GL_TEXTURE0);
}

/**
 Legacy: unneeded
 */
static void glStatePop() {
}


shared_ptr<Texture> Texture::singleChannelDifference(RenderDevice* rd, shared_ptr<Texture> t0, shared_ptr<Texture> t1, int channel) {
    debugAssertM(t0->width() == t1->width() && t0->height() == t1->height(), "singleChannelDifference requires the input textures to be of the same size");
    debugAssertM(channel >= 0 && channel < 4, "singleChannelDifference requires the input textures to be of the same size");
    shared_ptr<Framebuffer> fb = Framebuffer::create(Texture::createEmpty(t0->name() + "-" + t1->name(), t0->width(), t0->height(), ImageFormat::RG32F()));
    rd->push2D(fb); {
        Args args;
        args.setUniform("input0_buffer", t0, Sampler::buffer());
        args.setUniform("input1_buffer", t1, Sampler::buffer());
        args.setMacro("CHANNEL", channel);
        args.setRect(rd->viewport());
        LAUNCH_SHADER_WITH_HINT("Texture_singleChannelDiff.*", args, t0->name() + "->" + t1->name());
    } rd->pop2D();
    return fb->texture(0);
}


const shared_ptr<Texture>& Texture::white() {
    static shared_ptr<Texture> t;
    if (isNull(t)) {
        // Cache is empty
        const shared_ptr<CPUPixelTransferBuffer>& imageBuffer = CPUPixelTransferBuffer::create(4, 4, ImageFormat::RGB8());
        System::memset(imageBuffer->buffer(), 0xFF, imageBuffer->size());
        t = Texture::fromPixelTransferBuffer("G3D::Texture::white", imageBuffer, ImageFormat::RGB8(), Texture::DIM_2D);
    }

    return t;
}


const shared_ptr<Texture>& Texture::opaqueBlackCube() {
    static shared_ptr<Texture> t;
    if (isNull(t)) {
        // Cache is empty
        shared_ptr<CPUPixelTransferBuffer> imageBuffer = CPUPixelTransferBuffer::create(4, 4, ImageFormat::RGB8());
        System::memset(imageBuffer->buffer(), 0x00, imageBuffer->size());
        Array< Array<const void*> > bytes;
        Array<const void*>& cubeFace = bytes.next();
        for (int i = 0; i < 6; ++i)  {
            cubeFace.append(imageBuffer->buffer());
        }
        t = Texture::fromMemory("G3D::Texture::opaqueBlackCube", bytes, imageBuffer->format(), imageBuffer->width(), imageBuffer->height(), 1, 1, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP);

        // Store in cache
        //cache = t;
    }

    return t;
}


const shared_ptr<Texture>& Texture::whiteCube() {
    static shared_ptr<Texture> t;
    if (isNull(t)) {
        // Cache is empty
        shared_ptr<CPUPixelTransferBuffer> imageBuffer = CPUPixelTransferBuffer::create(4, 4, ImageFormat::RGB8());
        System::memset(imageBuffer->buffer(), 0xFF, imageBuffer->size());
        Array< Array<const void*> > bytes;
        Array<const void*>& cubeFace = bytes.next();
        for (int i = 0; i < 6; ++i)  {
            cubeFace.append(imageBuffer->buffer());
        }
        t = Texture::fromMemory("G3D::Texture::whiteCube", bytes, imageBuffer->format(), imageBuffer->width(), imageBuffer->height(), 1, 1, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP);
    }

    return t;
}


shared_ptr<Texture> Texture::createColorCube(const Color4& color) {
    // Get the white cube and then make another texture using the same handle 
    // and a different encoding.
    shared_ptr<Texture> w = whiteCube();

    Encoding e;
    e.format = w->encoding().format;
    e.readMultiplyFirst = color;
    return fromGLTexture(color.toString(), w->openGLID(), e, AlphaHint::ONE, DIM_CUBE_MAP);
}


const shared_ptr<Texture>& Texture::zero(Dimension d) {
    alwaysAssertM(d == DIM_2D || d == DIM_3D || d == DIM_2D_ARRAY, "Dimension must be 2D, 3D, or 2D Array");
    static Table<int, shared_ptr<Texture> > textures;
    if (!textures.containsKey(d)) {
        // Cache is empty                                                                                      
        shared_ptr<CPUPixelTransferBuffer> imageBuffer = CPUPixelTransferBuffer::create(8, 8, ImageFormat::RGBA8());
        System::memset(imageBuffer->buffer(), 0x00, imageBuffer->size());
        textures.set(d, Texture::fromPixelTransferBuffer("G3D::Texture::zero", imageBuffer, ImageFormat::RGBA8(), d));
    }
    
    return textures[d];
}


const shared_ptr<Texture>& Texture::opaqueBlack(Dimension d) {
    alwaysAssertM(d == DIM_2D || d == DIM_3D || d == DIM_2D_ARRAY, "Dimension must be 2D, 3D, or 2D Array");
    static Table<int, shared_ptr<Texture> > textures;
    if (!textures.containsKey(d)) {
        // Cache is empty                                                                                      
        shared_ptr<CPUPixelTransferBuffer> imageBuffer = CPUPixelTransferBuffer::create(8, 8, ImageFormat::RGBA8());
        for (int i = 0; i < imageBuffer->width() * imageBuffer->height(); ++i) {
            Color4unorm8* pixels = static_cast<Color4unorm8*>(imageBuffer->buffer());
            pixels[i] = Color4unorm8(unorm8::zero(), unorm8::zero(), unorm8::zero(), unorm8::one());
        }
        textures.set(d, Texture::fromPixelTransferBuffer("G3D::Texture::opaqueBlack", imageBuffer, ImageFormat::RGBA8(), d));
        
        //cache = t;
    }
    
    return textures[d];
}


const shared_ptr<Texture>& Texture::opaqueGray() {
    static shared_ptr<Texture> t;
    if (isNull(t)) {
        // Cache is empty                                                                                      
        shared_ptr<CPUPixelTransferBuffer> imageBuffer = CPUPixelTransferBuffer::create(8, 8, ImageFormat::RGBA8());
        const Color4unorm8 c(Color4(0.5f, 0.5f, 0.5f, 1.0f));
        for (int i = 0; i < imageBuffer->width() * imageBuffer->height(); ++i) {
            Color4unorm8* pixels = static_cast<Color4unorm8*>(imageBuffer->buffer());
            pixels[i] = c;
        }
        t = Texture::fromPixelTransferBuffer("Gray", imageBuffer);
    }
    
    return t;
}

void Texture::generateMipMaps(){
    glBindTexture(openGLTextureTarget(), openGLID());
    glGenerateMipmap(openGLTextureTarget());
    m_hasMipMaps = true;
}

static GLenum dimensionToTarget(Texture::Dimension d, int numSamples = 1);

static void createTexture
   (GLenum          target,
    const uint8*    rawBytes,
    GLenum          bytesActualFormat,
    GLenum          bytesFormat,
    int             m_width,
    int             m_height,
    int             depth,
    GLenum          ImageFormat,
    int             bytesPerPixel,
    int             mipLevel,
    bool            compressed,
    float           rescaleFactor,
    GLenum          dataType,
    bool            computeMinMaxMean,
    Color4&         minval, 
    Color4&         maxval, 
    Color4&         meanval,
    AlphaHint&      alphaHint,
	int				numSamples,
    const Texture::Encoding& encoding);

/////////////////////////////////////////////////////////////////////////////

Texture::Texture
   (const String&           name,
    GLuint                  textureID,
    Dimension               dimension,
    const Encoding&         encoding,
    bool                    opaque,
    AlphaHint               alphaHint,
    int                     numSamples) :
#ifdef G3D_ENABLE_CUDA
	m_cudaTextureResource(NULL),
	m_cudaTextureArray(NULL),
	m_cudaUsageFlags(0),
	m_cudaIsMapped(false),
#endif
    m_textureID(textureID),
    m_destroyGLTextureInDestructor(true),
    m_cachedSamplerSettings(Sampler(WrapMode::TILE, InterpolateMode::NEAREST_NO_MIPMAP)),
    m_name(name),
    m_dimension(dimension),
    m_opaque(opaque && (encoding.readMultiplyFirst.a == 1.0f)),
    m_encoding(encoding),
    m_min(Color4::nan()),
    m_max(Color4::nan()),
    m_mean(Color4::nan()),
    m_detectedHint(alphaHint),
    m_numSamples(numSamples),
    m_hasMipMaps(false),
    m_appearsInTextureBrowserWindow(true) {

    debugAssert(m_encoding.format);
    debugAssertGLOk();

    glStatePush();
    {
        GLenum target = dimensionToTarget(m_dimension, m_numSamples);
        glBindTexture(target, m_textureID);
        debugAssertGLOk();
        
        // For cube map, we can't read "cube map" but must choose a face
        GLenum readbackTarget = target;
        if (m_dimension == DIM_CUBE_MAP) {
            readbackTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        }
		debugAssertGLOk();

        glGetTexLevelParameteriv(readbackTarget, 0, GL_TEXTURE_WIDTH, &m_width);
        glGetTexLevelParameteriv(readbackTarget, 0, GL_TEXTURE_HEIGHT, &m_height);

        if ((readbackTarget == GL_TEXTURE_3D) || (readbackTarget == GL_TEXTURE_2D_ARRAY)) {
            glGetTexLevelParameteriv(readbackTarget, 0, GL_TEXTURE_DEPTH, &m_depth);
        } else {
            m_depth = 1;
        }
        
        debugAssertGLOk();
        
        setAllSamplerParameters(target, m_cachedSamplerSettings);
        
        debugAssertGLOk();
    } glStatePop();
    debugAssertGLOk();

    m_sizeOfAllTexturesInMemory += sizeInMemory();
}


Texture::Texture
   (const String&              name,
    const MipsPerCubeFace&     mipsPerCubeFace,
    Dimension                  dimension,
    InterpolateMode            interpolation,
    WrapMode                   wrapping,
    const Encoding&            encoding,
    AlphaHint                  alphaHint,
    int                        numSamples)
    : m_textureID(0)
    , m_destroyGLTextureInDestructor(true)
    , m_name(name)

#ifdef G3D_ENABLE_CUDA
	, m_cudaTextureResource(NULL)
	, m_cudaTextureArray(NULL)
	, m_cudaUsageFlags(0)
	, m_cudaIsMapped(false)
#endif
    , m_dimension(dimension)
    , m_opaque(encoding.format->opaque && (encoding.readMultiplyFirst.a == 1.0f))
    , m_encoding(encoding)
    , m_width(0)
    , m_height(0)
    , m_depth(0)
    , m_min(Color4::nan())
    , m_max(Color4::nan())
    , m_mean(Color4::nan())
    , m_detectedHint(alphaHint)
    , m_numSamples(numSamples)
    , m_hasMipMaps(false)
    , m_appearsInTextureBrowserWindow(true)
{

    // Verify that enough PixelTransferBuffer's were passed in to create a texture
    debugAssert(mipsPerCubeFace.length() > 0);
    debugAssert(mipsPerCubeFace[0].length() > 0);

    if (mipsPerCubeFace.length() == 0 || mipsPerCubeFace[0].length() == 0) {
        debugAssertM(false, "Cannot create Texture without source images");
        return;
    }

    // Generate texture id and configure texture settings
    configureTexture(mipsPerCubeFace);
    uploadImages(mipsPerCubeFace);

    m_sizeOfAllTexturesInMemory += sizeInMemory();
}


void Texture::configureTexture(const MipsPerCubeFace& mipsPerCubeFace) {
    // Get new texture from OpenGL
    m_textureID = newGLTextureID();
    debugAssertGLOk();

    const shared_ptr<PixelTransferBuffer>& fullImage = mipsPerCubeFace[0][0];

    // Get image dimensions
    m_width = fullImage->width();
    m_height = fullImage->height();
    m_depth = fullImage->depth();

    glStatePush();
    {
        // Behind texture to target for configuration
        GLenum target = dimensionToTarget(m_dimension, m_numSamples);
        glBindTexture(target, m_textureID);
        debugAssertGLOk();
        setAllSamplerParameters(target, m_cachedSamplerSettings);
        debugAssertGLOk();
    }
    glStatePop();
    debugAssertGLOk();
}


void Texture::uploadImages(const MipsPerCubeFace& mipsPerCubeFace) {
    glBindTexture(openGLTextureTarget(), m_textureID);
    debugAssertGLOk();

    for (int mipIndex = 0; mipIndex < mipsPerCubeFace[0].length(); ++mipIndex) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        shared_ptr<PixelTransferBuffer>   buffer   = mipsPerCubeFace[0][mipIndex];
        shared_ptr<GLPixelTransferBuffer> glBuffer = dynamic_pointer_cast<GLPixelTransferBuffer>(buffer);

        if (notNull(glBuffer)) {
            // Direct GPU->GPU transfer
            glBuffer->bindRead();
            glTexImage2D(openGLTextureTarget(), mipIndex, format()->openGLFormat, m_width, m_height, 0, mipsPerCubeFace[0][mipIndex]->format()->openGLBaseFormat, mipsPerCubeFace[0][mipIndex]->format()->openGLDataFormat, 0);
            glBuffer->unbindRead();
        } else {
            // Any->GPU transfer
            const void* ptr = buffer->mapRead();
            glTexImage2D(openGLTextureTarget(), mipIndex, format()->openGLFormat, m_width, m_height, 0, mipsPerCubeFace[0][mipIndex]->format()->openGLBaseFormat, mipsPerCubeFace[0][mipIndex]->format()->openGLDataFormat, ptr);
            mipsPerCubeFace[0][mipIndex]->unmap();
        }

        debugAssertGLOk();
    }
}


///// Texture PixelTransferBuffer interface end


shared_ptr<Texture> Texture::fromMemory(
    const String&                   name,
    const void*                     bytes,
    const class ImageFormat*        bytesFormat,
    int                             width,
    int                             height,
    int                             depth,
    int                             numSamples,
    Encoding                        desiredEncoding,
    Dimension                       dimension,
    bool                            generateMipMaps,
    const Preprocess&               preprocess,
    bool                            preferSRGBForAuto) {

    Array< Array<const void*> > data;
    data.resize(1);
    data[0].append(bytes);

    return fromMemory(name, 
                      data,
                      bytesFormat, 
                      width, 
                      height, 
                      depth, 
                      numSamples,
                      desiredEncoding,
                      dimension,
                      generateMipMaps,
                      preprocess,
                      preferSRGBForAuto);
}


shared_ptr<Texture> Texture::fromGLTexture(
    const String&           name,
    GLuint                  textureID,
    Encoding                encoding,
    AlphaHint               alphaHint,
    Dimension               dimension,
    bool                    destroyGLTextureInDestructor,
	int                     numSamples) {

    debugAssert(encoding.format);
    //const int numSamples = 1;
    shared_ptr<Texture> t = shared_ptr<Texture>(new Texture(
        name, 
        textureID, 
        dimension,
        encoding, 
        encoding.format->opaque,
        alphaHint,
        numSamples));

    t->m_destroyGLTextureInDestructor = destroyGLTextureInDestructor;

    return t;
}


static void transform(shared_ptr<Image>& image, const CubeMapConvention::CubeMapInfo::Face& info) {
    // Apply transformations
    if (info.flipX) {
        image->flipHorizontal();
    }
    if (info.flipY) {
        image->flipVertical();
    }
    if (info.rotations > 0) {
        image->rotateCW(toRadians(90.0 * info.rotations));
    }
}


shared_ptr<Texture> Texture::loadTextureFromSpec(const Texture::Specification& s) {
    shared_ptr<Texture> t;

    

    if (s.alphaFilename.empty()) {
        t = Texture::fromFile(s.filename, s.encoding, s.dimension, s.generateMipMaps, s.preprocess, s.assumeSRGBSpaceForAuto);
    } else {
        t = Texture::fromTwoFiles(s.filename, s.alphaFilename, s.encoding, s.dimension, s.generateMipMaps, s.preprocess, s.assumeSRGBSpaceForAuto, false);
    }

    if (s.filename == "<white>" && (! s.encoding.readMultiplyFirst.isOne() || ! s.encoding.readAddSecond.isZero())) {
        t->m_name = String("Color4") + (s.encoding.readMultiplyFirst + s.encoding.readAddSecond).toString();
        t->m_appearsInTextureBrowserWindow = false;
    }

    if (s.name != "") {
        t->m_name = s.name;
    }

    return t;
}

Texture::TexelType Texture::texelType() const {
    const ImageFormat* f = format();
    if (f->numberFormat == ImageFormat::INTEGER_FORMAT) {
        if (f->openGLDataFormat == GL_UNSIGNED_BYTE ||
            f->openGLDataFormat == GL_UNSIGNED_SHORT ||
            f->openGLDataFormat == GL_UNSIGNED_INT) {
            return TexelType::UNSIGNED_INTEGER;
        } else {
            return TexelType::INTEGER;
        }
    }
    return TexelType::FLOAT;
}

shared_ptr<Texture> Texture::create(const Specification& s) {
    if (s.cachable) {
        if ((s.filename == "<white>") && s.alphaFilename.empty() && (s.dimension == DIM_2D) && s.encoding.readMultiplyFirst.isOne() && s.encoding.readAddSecond.isZero()) {
            // Make a single white texture when the other properties don't matter
            return Texture::white();
        } else {
            shared_ptr<Texture> cachedValue = s_cache[s];
            if (isNull(cachedValue)) {
                cachedValue = loadTextureFromSpec(s);
                s_cache.set(s, cachedValue);
            }
            return cachedValue;
        }
    } else {
        return loadTextureFromSpec(s);
    }
}


class ImageLoaderThread : public GThread {
private:
    String         m_filename;
    shared_ptr<Image>&         m_image;
    const ImageFormat*         m_format;
public:
    
    ImageLoaderThread(const String& filename, shared_ptr<Image>& im, const ImageFormat* format = ImageFormat::AUTO()) : 
        GThread(filename), m_filename(filename), m_image(im) { m_format = format; }

    void threadMain() {
        m_image = Image::fromFile(m_filename, m_format);
    }
};


shared_ptr<Texture> Texture::fromFile
   (const String                    filename[6],
    Encoding                        desiredEncoding,
    Dimension                       dimension,
    bool                            generateMipMaps,
    const Preprocess&               preprocess,
    bool                            preferSRGBSpaceForAuto) {
    
    if (endsWith(toLower(filename[0]), ".exr") && (desiredEncoding.format == ImageFormat::AUTO())) {
        desiredEncoding.format = ImageFormat::RGBA32F();
    }
    
    if (dimension == DIM_2D_ARRAY) {
        Array<String> files;
        FileSystem::getFiles(filename[0], files, true);
        files.sort();

        Array < shared_ptr<Image> > images;
        images.resize(files.length());
        ThreadSet threadSet;
        for (int i = 0; i < files.size(); ++i) {
            threadSet.insert(shared_ptr<ImageLoaderThread>(new ImageLoaderThread(files[i], images[i])));
        }
        threadSet.start(GThread::USE_CURRENT_THREAD);
        threadSet.waitForCompletion();

        return Texture::fromPixelTransferBuffer(FilePath::base(filename[0]), Image::arrayToPixelTransferBuffer(images), desiredEncoding.format, dimension);
    }

    String realFilename[6];
    Array< Array< const void* > > byteMipMapFaces;

    const int numFaces = (dimension == DIM_CUBE_MAP) ? 6 : 1;

    // Single mip-map level
    byteMipMapFaces.resize(1);
    
    Array<const void*>& array = byteMipMapFaces[0];
    array.resize(numFaces);

    debugAssertM(filename[1] == "",
        "Can't specify more than one filename");

    realFilename[0] = filename[0];
    CubeMapConvention::CubeMapInfo info;

    // Ensure that this is not "<white>" before splitting names
    if ((numFaces == 6) && ! beginsWith(filename[0], "<")) {
        // Parse the filename into a base name and extension
        generateCubeMapFilenames(filename[0], realFilename, info);
    }

    // The six cube map faces, or the one texture and 5 dummys.
    shared_ptr<Image> image[6];

    if (((numFaces == 1) && (dimension == DIM_2D)) || (dimension == DIM_3D)) {
        if ((toLower(realFilename[0]) == "<white>") || realFilename[0].empty()) {
            const shared_ptr<CPUPixelTransferBuffer>& buffer = CPUPixelTransferBuffer::create(1, 1, ImageFormat::RGBA8());
            image[0] = Image::fromPixelTransferBuffer(buffer);
            image[0]->set(Point2int32(0, 0), Color4unorm8::one());
        } else {
            image[0] = Image::fromFile(realFilename[0]);

            alwaysAssertM(image[0]->width() > 0, 
                G3D::format("Image not found: \"%s\" and GImage failed to throw an exception", realFilename[0].c_str()));
        }
    } else {
        // Load each cube face on a different thread to overlap compute and I/O
        ThreadSet threadSet;

        for (int f = 0; f < numFaces; ++f) {
            if ((toLower(realFilename[f]) == "<white>") || realFilename[f].empty()) {
                const shared_ptr<CPUPixelTransferBuffer>& buffer = CPUPixelTransferBuffer::create(1, 1, ImageFormat::RGBA8());
                image[f] = Image::fromPixelTransferBuffer(buffer);
                image[f]->set(Point2int32(0, 0), Color4unorm8::one());
            } else {
                threadSet.insert(shared_ptr<ImageLoaderThread>(new ImageLoaderThread(realFilename[f], image[f])));
            }
        }
        threadSet.start(GThread::USE_CURRENT_THREAD);
        threadSet.waitForCompletion();
    }

    shared_ptr<PixelTransferBuffer> buffers[6];
    for (int f = 0; f < numFaces; ++f) {
        //debugPrintf("Loading %s\n", realFilename[f].c_str());
        alwaysAssertM(image[f]->width() > 0,  "Image not found");
        alwaysAssertM(image[f]->height() > 0, "Image not found");

        if (numFaces > 1) {
            transform(image[f], info.face[f]);
        }
        
        if (! GLCaps::supportsTexture(image[f]->format())) {
            if (image[f]->format() == ImageFormat::L8()) {
                image[f]->convertToRGB8();
            } else { 
                debugAssertM(false, "Unsupported texture format on this machine");
            }
        }

        const shared_ptr<CPUPixelTransferBuffer>& b = image[f]->toPixelTransferBuffer();
        buffers[f] = b;
        array[f] = b->buffer();
    }

    const shared_ptr<Texture>& t =
        fromMemory(FilePath::base(filename[0]), 
                   byteMipMapFaces, 
                   buffers[0]->format(),
                   buffers[0]->width(), 
                   buffers[0]->height(), 
                   1,
                   1,
                   desiredEncoding, 
                   dimension,
                   generateMipMaps,
                   preprocess,
                   preferSRGBSpaceForAuto);
    
    return t;
}


shared_ptr<Texture> Texture::fromFile
   (const String&           filename,
    Encoding                desiredEncoding,
    Dimension               dimension,
    bool                    generateMipMaps,
    const Preprocess&       preprocess,
    bool                    preferSRGBSpaceForAuto) {

    String f[6];
    f[0] = filename;
    f[1] = "";
    f[2] = "";
    f[3] = "";
    f[4] = "";
    f[5] = "";

    if (filename.find('*') != String::npos) {
        CubeMapConvention::CubeMapInfo info;
        String filenameBase, filenameExt;
        Texture::splitFilenameAtWildCard(filename, filenameBase, filenameExt);

        // Cube map formats:
        if ((FileSystem::exists(filenameBase + "east" + filenameExt)) || (FileSystem::exists(filenameBase + "lf" + filenameExt)) || 
            (FileSystem::exists(filenameBase + "+x" + filenameExt)) || (FileSystem::exists(filenameBase + "+X" + filenameExt)) || 
            (FileSystem::exists(filenameBase + "PX" + filenameExt)) || 
            (FileSystem::exists(filenameBase + "px" + filenameExt))) {

            dimension = DIM_CUBE_MAP;
        } else {
            // Must be a texture array
            dimension = DIM_2D_ARRAY;
        }
    } else if ((dimension == DIM_CUBE_MAP) && (filename != "<white>")) {
        dimension = DIM_2D;
    }

    return fromFile(f, desiredEncoding, dimension, generateMipMaps, preprocess, preferSRGBSpaceForAuto);
}


shared_ptr<Texture> Texture::fromTwoFiles
   (const String&           filename,
    const String&           alphaFilename,
    Encoding                desiredEncoding,
    Dimension               dimension,
    bool                    generateMipMaps,
    const Preprocess&       preprocess,
    const bool              preferSRGBSpaceForAuto,
    const bool              useAlpha) {

    // The six cube map faces, or the one texture and 5 dummys.
    Array< Array<const void*> > mip;
    mip.resize(1);
    Array<const void*>& array = mip[0];

    const int numFaces = (dimension == DIM_CUBE_MAP) ? 6 : 1;

    array.resize(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        array[i] = NULL;
    }

    // Parse the filename into a base name and extension
    String filenameArray[6];
    String alphaFilenameArray[6];
    filenameArray[0] = filename;
    alphaFilenameArray[0] = alphaFilename;

    // Test for DIM_CUBE_MAP
    CubeMapConvention::CubeMapInfo info, alphaInfo;
    if (numFaces == 6) {
        generateCubeMapFilenames(filename, filenameArray, info);
        generateCubeMapFilenames(alphaFilename, alphaFilenameArray, alphaInfo);
    }
    
    shared_ptr<Image> color[6];
    shared_ptr<Image> alpha[6];
    shared_ptr<PixelTransferBuffer> buffers[6];
    shared_ptr<Texture> t;

    try {
        for (int f = 0; f < numFaces; ++f) {
            // Compose the two images to a single RGBA
            alpha[f] = Image::fromFile(alphaFilenameArray[f]);
            if ( !((toLower(filenameArray[f]) == "<white>") || filenameArray[f].empty() )) {
                color[f] = Image::fromFile(filenameArray[f]);
            } 

            const shared_ptr<CPUPixelTransferBuffer>& b    = CPUPixelTransferBuffer::create(alpha[f]->width(), alpha[f]->height(), ImageFormat::RGBA8());
            uint8* newMap                           = reinterpret_cast<uint8*>(b->mapWrite());

            if (notNull(color[f])) {
                if (numFaces > 1) {
                    transform(color[f], info.face[f]);
                    transform(alpha[f], alphaInfo.face[f]);
                }
                shared_ptr<CPUPixelTransferBuffer> cbuf = color[f]->toPixelTransferBuffer();
                const uint8* colorMap                   = reinterpret_cast<const uint8*>(cbuf->mapRead());
                shared_ptr<CPUPixelTransferBuffer> abuf = alpha[f]->toPixelTransferBuffer();
                const uint8* alphaMap                   = reinterpret_cast<const uint8*>(abuf->mapRead());
            

                alwaysAssertM((color[f]->width()  == alpha[f]->width()) && 
                              (color[f]->height() == alpha[f]->height()), "Texture images for RGB + R -> RGBA packing conversion must be the same size");
                /** write into new map byte-by-byte, copying over alpha properly */
                const int N = color[f]->height() * color[f]->width();
                const int colorStride = cbuf->format()->numComponents;
                const int alphaStride = abuf->format()->numComponents;
                for (int i = 0; i < N; ++i) {
                    newMap[i * 4 + 0] = colorMap[i * colorStride + 0];
                    newMap[i * 4 + 1] = colorMap[i * colorStride + 1];
                    newMap[i * 4 + 2] = colorMap[i * colorStride + 2];
                    newMap[i * 4 + 3] = useAlpha ? alphaMap[i * 4 + 3] : alphaMap[i * alphaStride];
                }
                cbuf->unmap();
                abuf->unmap();
            } else { // No color map, use white
                if (numFaces > 1) {
                    transform(alpha[f], alphaInfo.face[f]);
                }
                shared_ptr<CPUPixelTransferBuffer> abuf = alpha[f]->toPixelTransferBuffer();
                const uint8* alphaMap                   = reinterpret_cast<const uint8*>(abuf->mapRead());

                /** write into new map byte-by-byte, copying over alpha properly */
                const int N = alpha[f]->height() * alpha[f]->width();
                const int alphaStride = abuf->format()->numComponents;
                for (int i = 0; i < N; ++i) {
                    newMap[i * 4 + 0] = 255;
                    newMap[i * 4 + 1] = 255;
                    newMap[i * 4 + 2] = 255;
                    newMap[i * 4 + 3] = useAlpha ? alphaMap[i * 4 + 3] : alphaMap[i * alphaStride];
                }
                abuf->unmap();
            }
            
            b->unmap();
            buffers[f] = b;
            array[f] = static_cast<uint8*>(b->buffer());
        }

        t = fromMemory
               (filename, 
                mip, 
                ImageFormat::SRGBA8(),
                buffers[0]->width(), 
                buffers[0]->height(), 
                1,
                1,
                desiredEncoding,
                dimension,
                generateMipMaps,
                preprocess,
                preferSRGBSpaceForAuto);

    } catch (const Image::Error& e) {
        Log::common()->printf("\n**************************\n\n"
            "Loading \"%s\" failed. %s\n", e.filename.c_str(),
            e.reason.c_str());
    }

    return t;
}

/** Create texture from nothing. */
shared_ptr<Texture> Texture::fromNothing
   (const String&                  name,
    const ImageFormat*                  bytesFormat,
    int                                 width,
    int                                 height,
    int                                 depth,
	int									numSamples,
    const ImageFormat*                  desiredFormat,
    Dimension                           dimension,
    bool                                preferSRGBForAuto,
	const Texture::Encoding& encoding) {

    if (dimension != DIM_3D) {
        debugAssertM(depth == 1, "Depth must be 1 for all textures that are not DIM_3D or DIM_3D");
    }

    if (desiredFormat == ImageFormat::AUTO()) {
        if (preferSRGBForAuto) {
            desiredFormat = ImageFormat::getSRGBFormat(bytesFormat);
        } else {
            desiredFormat = bytesFormat;
        }
    }

    /*if (settings.interpolateMode != NEAREST_NO_MIPMAP &&
        settings.interpolateMode != NEAREST_MIPMAP) {
        debugAssertM(
                 (desiredFormat->openGLFormat != GL_RGBA32UI) &&
                 (desiredFormat->openGLFormat != GL_RGBA32I) &&
                 (desiredFormat->openGLFormat != GL_RGBA16UI) &&
                 (desiredFormat->openGLFormat != GL_RGBA16I) &&
                 (desiredFormat->openGLFormat != GL_RGBA8UI) &&
                 (desiredFormat->openGLFormat != GL_RGBA8I) &&
                 
                 (desiredFormat->openGLFormat != GL_RGB32UI) &&
                 (desiredFormat->openGLFormat != GL_RGB32I) &&
                 (desiredFormat->openGLFormat != GL_RGB16UI) &&
                 (desiredFormat->openGLFormat != GL_RGB16I) &&
                 (desiredFormat->openGLFormat != GL_RGB8UI) &&
                 (desiredFormat->openGLFormat != GL_RGB8I) &&
                 
                 (desiredFormat->openGLFormat != GL_RG32UI) &&
                 (desiredFormat->openGLFormat != GL_RG32I) &&
                 (desiredFormat->openGLFormat != GL_RG16UI) &&
                 (desiredFormat->openGLFormat != GL_RG16I) &&
                 (desiredFormat->openGLFormat != GL_RG8UI) &&
                 (desiredFormat->openGLFormat != GL_RG8I) &&
                 
                 (desiredFormat->openGLFormat != GL_R32UI) &&
                 (desiredFormat->openGLFormat != GL_R32I) &&
                 (desiredFormat->openGLFormat != GL_R16UI) &&
                 (desiredFormat->openGLFormat != GL_R16I) &&
                 (desiredFormat->openGLFormat != GL_R8UI) &&
                 (desiredFormat->openGLFormat != GL_R8I),
                 
                 "Integer and unsigned integer formats only support NEAREST interpolation");	//Why don't we use OpenGL errors for this ? -Cyril
    }*/
    debugAssert(bytesFormat);
    (void)depth;
    
    // Check for at least one miplevel on the incoming data
	int maxRes=std::max(width, std::max(height, depth));
    int numMipMaps = int(log2(float(maxRes)))+1;
    debugAssert(numMipMaps > 0);


    // Create the texture
    GLuint textureID = newGLTextureID();
    GLenum target = dimensionToTarget(dimension, numSamples);

    if ((desiredFormat == ImageFormat::AUTO()) || (bytesFormat->compressed)) {
        desiredFormat = bytesFormat;
    }

    /*if (GLCaps::hasBug_redBlueMipmapSwap() && (desiredFormat == ImageFormat::RGB8())) {
        desiredFormat = ImageFormat::RGBA8();
    }*/

    debugAssertM(GLCaps::supportsTexture(desiredFormat), "Unsupported texture format.");

    int mipWidth = width;
    int mipHeight = height;
	int mipDepth = depth;
    Color4 minval = Color4::nan();
    Color4 meanval = Color4::nan();
    Color4 maxval = Color4::nan();
	AlphaHint alphaHint = AlphaHint::DETECT;

    glStatePush(); {

        glBindTexture(target, textureID);
        debugAssertGLOk();

        /*if (isMipMapFormat(settings.interpolateMode) && hasAutoMipMap() && (numMipMaps == 1)) {
            // Enable hardware MIP-map generation.
            // Must enable before setting the level 0 image (we'll set it again
            // in setTexParameters, but that is intended primarily for 
            // the case where that function is called for a pre-existing GL texture ID).
            glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        }*/

        for (int mipLevel = 0; mipLevel < numMipMaps; ++mipLevel) {

            int numFaces = 1;
            if(dimension == DIM_CUBE_MAP)
				numFaces=6;

            for (int f = 0; f < numFaces; ++f) {
                if (numFaces == 6) {
                    // Choose the appropriate face target
                    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + f;
                }

                debugAssertGLOk();
                createTexture(target, 
                                NULL, 
                                bytesFormat->openGLFormat, 
                                bytesFormat->openGLBaseFormat,
                                mipWidth, 
                                mipHeight, 
                                mipDepth,
                                desiredFormat->openGLFormat, 
                                bytesFormat->cpuBitsPerPixel / 8, 
                                mipLevel, 
                                bytesFormat->compressed, 
                                1.0f,
                                bytesFormat->openGLDataFormat,
                                false,
                                minval, maxval, meanval,
								alphaHint,
								numSamples,
								encoding);
                debugAssertGLOk();

#               ifndef G3D_DEBUG
                {
                    GLenum e = glGetError();
                    if (e == GL_OUT_OF_MEMORY) {
                        throw String("The texture map was too large (GL_OUT_OF_MEMORY)");
                    }
                    if (e != GL_NO_ERROR) {
                    String errors;
                    while (e != GL_NO_ERROR) {
                        e = glGetError();
                        if (e == GL_OUT_OF_MEMORY) {
                            throw String("The texture map was too large (GL_OUT_OF_MEMORY)");
                        }
                    }
                    }
                }
#               endif
            }

            mipWidth  = iMax(1, mipWidth / 2);
            mipHeight = iMax(1, mipHeight / 2);
			mipDepth = iMax(1, mipDepth / 2);
			
        }
   
    } glStatePop();

    debugAssertGLOk();
    shared_ptr<Texture> t = fromGLTexture(name, textureID, desiredFormat, alphaHint, dimension);
    debugAssertGLOk();

    t->m_width  = width;
    t->m_height = height;
    t->m_depth  = depth;
    t->m_min    = minval;
    t->m_max    = maxval;
    t->m_mean   = meanval;

    s_allTextures.set((uint64)t.get(),t);
    return t;
}


shared_ptr<Texture> Texture::fromImage
   (const String&                   name,
    const shared_ptr<Image3>&       image,
    const ImageFormat*              desiredFormat,
    Dimension                       dimension,
    bool                            generateMipMaps,
    const Preprocess&               preprocess) {

    return fromMemory(name, image->getCArray(), image->format(),
                        image->width(), image->height(), 1, 1,
                        (desiredFormat == NULL) ? image->format() : desiredFormat, 
                        dimension, generateMipMaps, preprocess);
}


shared_ptr<Texture> Texture::fromMemory
   (const String&                       name,
    const Array< Array<const void*> >&  _bytes,
    const ImageFormat*                  bytesFormat,
    int                                 width,
    int                                 height,
    int                                 depth,
    int                                 numSamples,
    Encoding                            desiredEncoding,
    Dimension                           dimension,
    bool                                generateMipMaps,
    const Preprocess&                   preprocess,
    bool                                preferSRGBForAuto) {

    debugAssertGLOk();
    // For use computing normal maps
    shared_ptr<PixelTransferBuffer> normal;

    typedef Array< Array<const void*> > MipArray;

    float scaleFactor = preprocess.scaleFactor;
    
    // Indirection needed in case we have to reallocate our own
    // data for preprocessing.
    MipArray* bytesPtr = const_cast<MipArray*>(&_bytes);

    if (dimension == DIM_3D) {
        /*debugAssertM((settings.interpolateMode == BILINEAR_NO_MIPMAP) ||
                     (settings.interpolateMode == NEAREST_NO_MIPMAP), 
                     "DIM_3D textures do not support mipmaps");*/
        debugAssertM(_bytes.size() == 1,                    
                     "DIM_3D textures do not support mipmaps");
    } else if (dimension != DIM_3D && dimension != DIM_CUBE_MAP_ARRAY && dimension != DIM_2D_ARRAY) {
        debugAssertM(depth == 1, "Depth must be 1 for all textures that are not DIM_3D, DIM_CUBE_MAP_ARRAY, or DIM_2D_ARRAY");
    }

    if ((preprocess.modulate != Color4::one()) || (preprocess.gammaAdjust != 1.0f) || preprocess.convertToPremultipliedAlpha) {
        debugAssert((bytesFormat->code == ImageFormat::CODE_RGB8) ||
                    (bytesFormat->code == ImageFormat::CODE_RGBA8) ||
                    (bytesFormat->code == ImageFormat::CODE_R8) ||
                    (bytesFormat->code == ImageFormat::CODE_L8));

        // Allow brightening to fail silently in release mode
        if (( bytesFormat->code == ImageFormat::CODE_R8) ||
	    ( bytesFormat->code == ImageFormat::CODE_L8) ||
            ( bytesFormat->code == ImageFormat::CODE_R8) ||
            ( bytesFormat->code == ImageFormat::CODE_RGB8) ||
            ( bytesFormat->code == ImageFormat::CODE_RGBA8)) {

            bytesPtr = new MipArray();
            bytesPtr->resize(_bytes.size());

            for (int m = 0; m < bytesPtr->size(); ++m) {
                Array<const void*>& face = (*bytesPtr)[m]; 
                face.resize(_bytes[m].size());
            
                for (int f = 0; f < face.size(); ++f) {

                    int numBytes = iCeil(width * height * depth * bytesFormat->cpuBitsPerPixel / 8.0f);

                    // Allocate space for the converted image
                    face[f] = System::alignedMalloc(numBytes, 16);

                    // Copy the original data
                    System::memcpy(const_cast<void*>(face[f]), _bytes[m][f], numBytes);

                    // Apply the processing to the copy
                    preprocess.modulateImage(bytesFormat->code, const_cast<void*>(face[f]), numBytes);
                }
            }
        }
    }

    debugAssertM(! (preprocess.computeNormalMap && preprocess.convertToPremultipliedAlpha), "A texture should not be both a bump map and an alpha-masked value");

    if (preprocess.computeNormalMap) {
        debugAssertM(bytesFormat->redBits == 8 || bytesFormat->luminanceBits == 8, "To preprocess a texture with normal maps, 8-bit channels are required");
        debugAssertM(bytesFormat->compressed == false, "Cannot compute normal maps from compressed textures");
        debugAssertM(bytesFormat->floatingPoint == false, "Cannot compute normal maps from floating point textures");
        debugAssertM(bytesFormat->numComponents == 1 || bytesFormat->numComponents == 3 || bytesFormat->numComponents == 4, "1, 3, or 4 channels needed to compute normal maps");
        debugAssertM(bytesPtr->size() == 1, "Cannot specify mipmaps when computing normal maps automatically");

        normal = BumpMap::computeNormalMap(width, height, bytesFormat->numComponents, 
                                 reinterpret_cast<const unorm8*>((*bytesPtr)[0][0]),
                                 preprocess.bumpMapPreprocess);
        
        // Replace the previous array with the data from our normal map
        bytesPtr = new MipArray();
        bytesPtr->resize(1);
        (*bytesPtr)[0].append(normal->mapRead());
        
        bytesFormat = ImageFormat::RGBA8();

        if (desiredEncoding.format == ImageFormat::AUTO()) {
            desiredEncoding.format = ImageFormat::RGBA8();
        }

        debugAssertM(desiredEncoding.format->openGLBaseFormat == GL_RGBA, "Desired format must contain RGBA channels for bump mapping");
    }


    if (desiredEncoding.format == ImageFormat::AUTO()) {
        if (preferSRGBForAuto) {
            desiredEncoding.format = ImageFormat::getSRGBFormat(bytesFormat);
        } else {
            desiredEncoding.format = bytesFormat;
        }
    }

    if (! GLCaps::supportsTexture(desiredEncoding.format)) {
        if (desiredEncoding.format == ImageFormat::L8()) {
            desiredEncoding.format = ImageFormat::RGB8(); 
        } else {
            throw String("Unsupported texture format: ") + desiredEncoding.format->name();
        }
    }

    if (bytesFormat == ImageFormat::L8()) {
        // Force to R8 because L8 is not supported in core
        bytesFormat = ImageFormat::R8();
    }

    debugAssert(bytesFormat);
    (void)depth;
    
    // Check for at least one miplevel on the incoming data
    int numMipMaps = bytesPtr->length();
    debugAssert(numMipMaps > 0);

    // Create the texture
    const GLuint textureID = newGLTextureID();

    // May be overriden below for cube maps
    GLenum target = dimensionToTarget(dimension, numSamples);

    if (bytesFormat->compressed) {
        desiredEncoding.format = bytesFormat;
    }

    debugAssertGLOk();
    debugAssertM(GLCaps::supportsTexture(desiredEncoding.format), "Unsupported texture format: " + desiredEncoding.format->name());

    int mipWidth   = width;
    int mipHeight  = height;
	int mipDepth = depth;
    Color4 minval = Color4::nan();
    Color4 meanval = Color4::nan();
    Color4 maxval  = Color4::nan();
    AlphaHint alphaHint = AlphaHint::DETECT;

    debugAssertGLOk();
    glStatePush(); {
 
        // Set unpacking alignment
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(target, textureID);

        for (int mipLevel = 0; mipLevel < numMipMaps; ++mipLevel) {

            const int numFaces = (*bytesPtr)[mipLevel].length();
            
            debugAssert(((dimension == DIM_CUBE_MAP) ? 6 : 1) == numFaces);
        
            for (int f = 0; f < numFaces; ++f) {
                if (numFaces == 6) {
                    // Choose the appropriate face target
                    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + f;
                }

                debugAssertGLOk();
                createTexture
                    (target, 
                     reinterpret_cast<const uint8*>((*bytesPtr)[mipLevel][f]), 
                     bytesFormat->openGLFormat, 
                     bytesFormat->openGLBaseFormat,
                     mipWidth, 
                     mipHeight, 
                     depth,
                     desiredEncoding.format->openGLFormat, 
                     bytesFormat->cpuBitsPerPixel / 8, 
                     mipLevel, 
                     bytesFormat->compressed, 
                     scaleFactor,
                     bytesFormat->openGLDataFormat,
                     preprocess.computeMinMaxMean,
                     minval, maxval, meanval,
                     alphaHint,
                     numSamples,
                     desiredEncoding);
                debugAssertGLOk();
                
#               ifndef G3D_DEBUG
                {
                    GLenum e = glGetError();
                    if (e == GL_OUT_OF_MEMORY) {
                        throw String("The texture map was too large (GL_OUT_OF_MEMORY)");
                    }
                    if (e != GL_NO_ERROR) {
                    String errors;
                    while (e != GL_NO_ERROR) {
                        e = glGetError();
                        if (e == GL_OUT_OF_MEMORY) {
                            throw String("The texture map was too large (GL_OUT_OF_MEMORY)");
                        }
                    }
                    }
                }
#               endif
            }

            mipWidth  = iMax(1, mipWidth / 2);
            mipHeight = iMax(1, mipHeight / 2);
			mipDepth = iMax(1, mipDepth / 2);
			
        }
   
    } glStatePop();

    debugAssertGLOk();
    const shared_ptr<Texture>& t = fromGLTexture(name, textureID, desiredEncoding, alphaHint, dimension, true, numSamples);
    debugAssertGLOk();

    t->m_width  = width;
    t->m_height = height;
    t->m_depth  = depth;
    t->m_min    = minval;
    t->m_max    = maxval;
    t->m_mean   = meanval;

    if (bytesPtr != &_bytes) {

        // We must free our own data
        if (normal) {
            // The normal PixelTransferBuffer is holding the data; do not free it because 
            // the destructor will do so at the end of the method automatically.
        } else {
            for (int m = 0; m < bytesPtr->size(); ++m) {
                Array<const void*>& face = (*bytesPtr)[m]; 
                for (int f = 0; f < face.size(); ++f) {
                    System::alignedFree(const_cast<void*>(face[f]));
                }
            }
        }
        delete bytesPtr;
        bytesPtr = NULL;
    }

    if (normal) {
        normal->unmap();
    }

    debugAssertGLOk();
    if (generateMipMaps && (numMipMaps == 1)) {
        // Generate mipmaps for textures requiring them
        t->generateMipMaps();
    } else {
        if (numMipMaps > 1) {
            t->m_hasMipMaps = true;
        }
    }

    debugAssertGLOk();

    s_allTextures.set((uint64)t.get(),t);
    return t;
}


shared_ptr<Texture> Texture::fromImage
   (const String&                   name,
    const shared_ptr<Image>&        image,
    const ImageFormat*              desiredFormat,
    Dimension                       dimension,
    bool                            generateMipMaps,
    const Preprocess&               preprocess) {
    return Texture::fromPixelTransferBuffer(name, image->toPixelTransferBuffer(), desiredFormat, dimension, generateMipMaps, preprocess);
}


shared_ptr<Texture> Texture::fromPixelTransferBuffer
   (const String&                   name,
    const shared_ptr<PixelTransferBuffer>& image,
    const ImageFormat*              desiredFormat,
    Dimension                       dimension,
    bool                            generateMipMaps,
    const Preprocess&               preprocess) {

    Array< Array< shared_ptr<PixelTransferBuffer> > > mips;
    mips.append( Array< shared_ptr<PixelTransferBuffer> >() );
    mips[0].append(image);

    if (desiredFormat == ImageFormat::AUTO()) {
        desiredFormat = image->format();
    }
    const int numSamples = 1;
    // TODO: If the image is a GLPixelTransferBuffer, bind it directly instead of extracting the bits
    shared_ptr<Texture> t =
        fromMemory(
            name, 
            image->mapRead(), 
            image->format(),
            image->width(), 
            image->height(), 
            image->depth(),
            numSamples,
            desiredFormat, 
            dimension, 
            generateMipMaps,
            preprocess);

    image->unmap();

    return t;
}

shared_ptr<Texture> Texture::createEmpty
(const String&                    name,
 int                              w,
 int                              h,
 const Encoding&                  encoding,
 Dimension                        dimension,
 bool                             generateMipMaps,
 int                              d,
 int                              numSamples) {

    debugAssertGLOk();
    debugAssertM(encoding.format, "encoding.format may not be ImageFormat::AUTO()");

    if ((dimension != DIM_3D) && (dimension != DIM_2D_ARRAY) && (dimension != DIM_CUBE_MAP_ARRAY)) {
        debugAssertM(d == 1, "Depth must be 1 for DIM_2D textures");
    }

    shared_ptr<Texture> t;

    switch (dimension) {
    case DIM_CUBE_MAP: {
        // Cube map requires six faces
        Array< Array<const void*> > data;
        data.resize(1);
        data[0].resize(6);
        for (int i = 0; i < 6; ++i) {
            data[0][i] = NULL;
        }
        t = fromMemory
            (name, 
             data,
             encoding.format, 
             w, 
             h, 
             d, 
             numSamples,
             encoding,
             dimension,
             generateMipMaps);
    } break;

    case DIM_3D:
    case DIM_2D_ARRAY:
		t = fromNothing
            (name, 
             encoding.format, 
             w, 
             h, 
             d, 
			 numSamples,
             encoding.format, 
             dimension);
        break;

    default:
        t = fromMemory
            (name, 
             NULL, 
             encoding.format, 
             w, 
             h, 
             d, 
             numSamples,
             encoding, 
             dimension, 
             generateMipMaps);
    }

    if (encoding.format->depthBits > 0) {
        t->visualization = Visualization::depthBuffer();
    }

    if (generateMipMaps) {
        // Some GPU drivers will not allocate the MIP levels until
        // this is called explicitly, which can cause framebuffer
        // calls to fail
        t->generateMipMaps();
    }

    //t->clear();

    debugAssertGLOk();
    return t;
}


void Texture::resize(int w, int h) {
    if ((width() == w) && (height() == h)) {
        return;
    }
    m_sizeOfAllTexturesInMemory -= sizeInMemory();
    
    m_min  = Color4::nan();
    m_max  = Color4::nan();
    m_mean = Color4::nan();

    m_width = w;
    m_height = h;
	m_depth = 1;

    alwaysAssertM(m_dimension != DIM_CUBE_MAP , "Cannot resize cube map textures");
    Array<GLenum> targets;
    if (m_dimension == DIM_CUBE_MAP) {
        targets.append(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                        GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB);
        targets.append(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                        GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB);        
    } else {
        targets.append(openGLTextureTarget());
    }
	debugAssertGLOk();

    glStatePush();
    {
        glBindTexture(openGLTextureTarget(), m_textureID);
        for (int t = 0; t < targets.size(); ++t) {
			if(targets[t]==GL_TEXTURE_2D_MULTISAMPLE){
				glTexImage2DMultisample(targets[t], m_numSamples, format()->openGLFormat, w, h, false);	//Not using fixed locations by default
			}else{
				glTexImage2D(targets[t], 0, format()->openGLFormat, w, h,
                                 0, format()->openGLBaseFormat, format()->openGLDataFormat /*GL_UNSIGNED_BYTE*/, NULL);
			}
        }
    }
    glStatePop();

    m_sizeOfAllTexturesInMemory += sizeInMemory();

	debugAssertGLOk();
}

void Texture::resize(int w, int h, int d) {
	if (d == 1) {
     	// 2D case
		resize(w, h);
	} else if (m_width != w || m_height != h || m_depth != d) {
		m_width = w;
		m_height = h;
		m_depth = d;

		alwaysAssertM(m_dimension != DIM_CUBE_MAP, "Cannot resize cube map textures");
		Array<GLenum> targets;
		if (m_dimension == DIM_CUBE_MAP) {
			targets.append(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
							GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
							GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB);
			targets.append(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
							GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
							GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB);        
		} else {
			targets.append(openGLTextureTarget());
		}

		glStatePush();
		{
			glBindTexture(openGLTextureTarget(), m_textureID);
			for (int t = 0; t < targets.size(); ++t) {
				glTexImage3D(targets[t], 0, format()->openGLFormat, w, h, d,
									 0, format()->openGLBaseFormat, GL_UNSIGNED_BYTE, NULL);
			}
		}
		glStatePop();

		m_sizeOfAllTexturesInMemory += sizeInMemory();
	}

	debugAssertGLOk();
}


void Texture::copy
   (shared_ptr<Texture>     src, 
    shared_ptr<Texture>     dst, 
    int                     srcMipLevel, 
    int                     dstMipLevel, 
    float                   scale,
    const Vector2int16&     shift,
    CubeFace                srcCubeFace, 
    CubeFace                dstCubeFace, 
    RenderDevice*           rd,
    bool                    resize,
    int                     srcLayer,
    int                     dstLayer) {

    alwaysAssertM((src->format()->depthBits == 0) || (srcMipLevel == 0 && dstMipLevel == 0), 
            "Texture::copy only defined for mipLevel 0 for depth textures");
    alwaysAssertM((src->format()->depthBits == 0) == (dst->format()->depthBits == 0), "Cannot copy color texture to depth texture or vice-versa");
    alwaysAssertM( ((src->dimension() == DIM_2D) || (src->dimension() == DIM_2D_ARRAY)), "Texture::copy only defined for 2D textures or texture arrays");
    //alwaysAssertM(((dst->dimension() == DIM_2D) || (dst->dimension() == DIM_2D_ARRAY)), "Texture::copy only defined for 2D textures or texture arrays");
    alwaysAssertM((dst->dimension() == DIM_2D_ARRAY) || (dstLayer == 0), "Layer can only be 0 for non-array textures");
    alwaysAssertM((src->dimension() == DIM_2D_ARRAY) || (srcLayer == 0), "Layer can only be 0 for non-array textures");

    alwaysAssertM( src && dst, "Both textures sent to Texture::copy must already exist");
    
    if (resize) {
        if (srcMipLevel != dstMipLevel) {
            alwaysAssertM(dstMipLevel == 0, "If miplevels mismatch, dstMipLevel must be 0 in Texture::copy");
            int mipFactor = 1 << srcMipLevel;
            dst->resize(int(src->width() / (mipFactor * scale)), int(src->height() * scale / mipFactor));
        } else {
            dst->resize(int(src->width() / scale), int(src->height() * scale));
        }
    }

    const shared_ptr<Framebuffer>& fbo = workingFramebuffer();
    if (rd == NULL) {
        rd = RenderDevice::current;
    }

    /** If it isn't an array texture, then don't try to bind a single layer */
    if ((dst->dimension() != DIM_2D_ARRAY) && (dst->dimension() != DIM_CUBE_MAP_ARRAY)) {
        dstLayer = -1;
    }
    fbo->clear();
    if (src->format()->depthBits > 0) {
        fbo->set(Framebuffer::DEPTH, dst, dstCubeFace, dstMipLevel, dstLayer);
    } else {
        fbo->set(Framebuffer::COLOR0, dst, dstCubeFace, dstMipLevel, dstLayer);
    }
    
    rd->push2D(fbo); {
        rd->setSRGBConversion(true);
        if (src->format()->depthBits > 0) {
            rd->setDepthClearValue(1.0f);
            rd->setDepthWrite(true);
        } else {
            rd->setColorClearValue(Color4::zero());
        }
        rd->clear();
        
        Args args;
        args.setUniform("mipLevel", srcMipLevel);
        
        const bool layered = (src->dimension() == Texture::DIM_2D_ARRAY);
        args.setMacro("IS_LAYERED", layered ? 1 : 0);
        args.setUniform("layer",    srcLayer);
        args.setUniform("src", layered ? Texture::zero() : src, Sampler::video());
        args.setUniform("layeredSrc", layered ? src : Texture::zero(Texture::DIM_2D_ARRAY), Sampler::video());
        
        args.setUniform("shift",    Vector2(shift));
        args.setUniform("scale",    scale);
        args.setMacro("DEPTH",      (src->format()->depthBits > 0) ? 1 : 0);
        args.setRect(rd->viewport());

        LAUNCH_SHADER_WITH_HINT("Texture_copy.*", args, src->name() + "->" + dst->name());
    } rd->pop2D();

    fbo->clear();
}


bool Texture::copyInto(shared_ptr<Texture>& dest, CubeFace cf, int mipLevel, RenderDevice* rd) const {
    alwaysAssertM
        (((format()->depthBits == 0) || mipLevel == 0) && ((dimension() == DIM_2D) || (dimension() == DIM_2D)), 
        "copyInto only defined for 2D color textures as input, or mipLevel 0 of a depth texture");

    bool allocated = false;
    if (isNull(dest) || (dest->format() != format())) {
        // Allocate
        dest = Texture::createEmpty(name() + " copy", width(), height(), format(), dimension(), hasMipMaps(), depth());
        allocated = true;
    }

    dest->resize(width(), height());

    const shared_ptr<Framebuffer>& fbo = workingFramebuffer();
    if (rd == NULL) {
        rd = RenderDevice::current;
    }

    fbo->clear();
    if (format()->depthBits > 0) {
        fbo->set(Framebuffer::DEPTH, dest, cf, mipLevel);
    } else {
        fbo->set(Framebuffer::COLOR0, dest, cf, mipLevel);
    }
    
    rd->push2D(fbo); {
        if (format()->depthBits > 0) {
            rd->setDepthClearValue(1.0f);
            rd->setDepthWrite(true);
        } else {
            rd->setColorClearValue(Color4::zero());
        }
        rd->clear();
        rd->setSRGBConversion(true);
        Args args;
        args.setUniform("mipLevel", mipLevel);
        const shared_ptr<Texture>& me = dynamic_pointer_cast<Texture>(const_cast<Texture*>(this)->shared_from_this());
        args.setUniform("src",      me, Sampler::buffer());

        bool layered = (me->dimension() == Texture::DIM_2D_ARRAY);
        args.setMacro("IS_LAYERED", layered ? 1 : 0);
        args.setUniform("layer", 0);
        args.setUniform("src", layered ? Texture::zero() : me, Sampler::video());
        args.setUniform("layeredSrc", layered ? me : Texture::zero(Texture::DIM_2D_ARRAY), Sampler::video());

        args.setUniform("shift",    Vector2(0, 0));
        args.setUniform("scale",    1.0f);
        args.setMacro("DEPTH", (format()->depthBits > 0) ? 1 : 0);
        args.setRect(rd->viewport());

        LAUNCH_SHADER_WITH_HINT("Texture_copy.*", args, name());
    } rd->pop2D();

    fbo->clear();
    return allocated;
}


void Texture::clear(CubeFace cf, int mipLevel, RenderDevice* rd) {
    if (rd == NULL) {
        rd = RenderDevice::current;
    }

    const shared_ptr<Framebuffer>& fbo = workingFramebuffer();

    if (format()->depthBits > 0) {
        fbo->set(Framebuffer::DEPTH, dynamic_pointer_cast<Texture>(shared_from_this()), cf, mipLevel);
    } else {
        fbo->set(Framebuffer::COLOR0, dynamic_pointer_cast<Texture>(shared_from_this()), cf, mipLevel);
    }

    rd->pushState(fbo);    
    rd->clear();
    rd->popState();

    fbo->clear();
}


Rect2D Texture::rect2DBounds() const {
    return Rect2D::xywh(0, 0, float(m_width), float(m_height));
}


void Texture::getTexImage(void* data, const ImageFormat* desiredFormat, CubeFace face, int mipLevel) const {
    const shared_ptr<GLPixelTransferBuffer>& transferBuffer = toPixelTransferBuffer(desiredFormat, mipLevel, face);
    transferBuffer->getData(data);
}


shared_ptr<Image4> Texture::toImage4() const {
    const shared_ptr<Image4>& im = Image4::createEmpty(m_width, m_height, WrapMode::TILE, m_depth); 
    getTexImage(im->getCArray(), ImageFormat::RGBA32F());
    return im;
}


shared_ptr<Image4unorm8> Texture::toImage4unorm8() const {
    const shared_ptr<Image4unorm8>& im = Image4unorm8::createEmpty(m_width, m_height, WrapMode::TILE); 
    getTexImage(im->getCArray(), ImageFormat::RGBA8());
    return im;
}

Image3Ref Texture::toImage3() const {    
    Image3::Ref im = Image3::createEmpty(m_width, m_height, WrapMode::TILE, m_depth); 
    getTexImage(im->getCArray(), ImageFormat::RGB32F());
    return im;
}


Image3unorm8Ref Texture::toImage3unorm8() const {    
    shared_ptr<Image3unorm8> im = Image3unorm8::createEmpty(m_width, m_height, WrapMode::TILE); 
    getTexImage(im->getCArray(), ImageFormat::RGB8());
    return im;
}


shared_ptr<Map2D<float> > Texture::toDepthMap() const {
    const shared_ptr<Map2D<float> >& im = Map2D<float>::create(m_width, m_height, WrapMode::TILE); 
    getTexImage(im->getCArray(), ImageFormat::DEPTH32F());
    return im;
}


shared_ptr<Image1> Texture::toDepthImage1() const {
    shared_ptr<Image1> im = Image1::createEmpty(m_width, m_height, WrapMode::TILE);
    getTexImage(im->getCArray(), ImageFormat::DEPTH32F());
    return im;
}


shared_ptr<Image1unorm8> Texture::toDepthImage1unorm8() const {
    shared_ptr<Image1> src = toDepthImage1();
    shared_ptr<Image1unorm8> dst = Image1unorm8::createEmpty(m_width, m_height, WrapMode::TILE);
    
    const Color1* s = src->getCArray();
    Color1unorm8* d = dst->getCArray();
    
    // Float to int conversion
    for (int i = m_width * m_height - 1; i >= 0; --i) {
        d[i] = Color1unorm8(s[i]);
    }
    
    return dst;
}


shared_ptr<Image1> Texture::toImage1() const {
    shared_ptr<Image1> im = Image1::createEmpty(m_width, m_height, WrapMode::TILE); 
    getTexImage(im->getCArray(), ImageFormat::L32F());
    return im;
}


shared_ptr<Image1unorm8> Texture::toImage1unorm8() const {
    shared_ptr<Image1unorm8> im = Image1unorm8::createEmpty(m_width, m_height, WrapMode::TILE); 
    getTexImage(im->getCArray(), ImageFormat::R8());
    return im;
}


void Texture::splitFilenameAtWildCard
   (const String&  filename,
    String&        filenameBase,
    String&        filenameExt) {

    const String splitter = "*";

    size_t i = filename.rfind(splitter);
    if (i != String::npos) {
        filenameBase = filename.substr(0, i);
        filenameExt  = filename.substr(i + 1, filename.size() - i - splitter.length()); 
    } else {
        throw Image::Error("Cube map filenames must contain \"*\" as a "
            "placeholder for {up,lf,rt,bk,ft,dn} or {up,north,south,east,west,down}", filename);
    }
}


bool Texture::isSupportedImage(const String& filename) {
    // Reminder: this looks in zipfiles as well
    if (! FileSystem::exists(filename)) {
        return false;
    }

    String ext = toLower(filenameExt(filename));

    if ((ext == "jpg") ||    
        (ext == "ico") ||
        (ext == "dds") ||
        (ext == "png") ||
        (ext == "tga") || 
        (ext == "bmp") ||
        (ext == "ppm") ||
        (ext == "pgm") ||
        (ext == "pbm") ||
        (ext == "tiff") ||
        (ext == "exr") ||
        (ext == "cut") ||
        (ext == "psd") ||
        (ext == "jbig") ||
        (ext == "xbm") ||
        (ext == "xpm") ||
        (ext == "gif") ||
        (ext == "hdr") ||
        (ext == "iff") ||
        (ext == "jng") ||
        (ext == "pict") ||
        (ext == "ras") ||
        (ext == "wbmp") ||
        (ext == "sgi") ||
        (ext == "pcd") ||
        (ext == "jp2") ||
        (ext == "jpx") ||
        (ext == "jpf") ||
        (ext == "pcx")) {
        return true;
    } else {
        return false;
    } 
}


Texture::~Texture() {
    if (m_destroyGLTextureInDestructor) {
        m_sizeOfAllTexturesInMemory -= sizeInMemory();
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
}


unsigned int Texture::newGLTextureID() {
    // Clear the OpenGL error flag
#   ifdef G3D_DEBUG
        glGetError();
#   endif 

    unsigned int id;
    glGenTextures(1, &id);

    debugAssertM(glGetError() != GL_INVALID_OPERATION, 
         "GL_INVALID_OPERATION: Probably caused by invoking "
         "glGenTextures between glBegin and glEnd.");

    return id;
}


void Texture::copyFromScreen(const Rect2D& rect, const ImageFormat* fmt) {
    glStatePush();
    debugAssertGLOk();

    m_sizeOfAllTexturesInMemory -= sizeInMemory();

    if (fmt == NULL) {
        fmt = format();
    } else {
        m_encoding = Encoding(fmt);
    }

    // Set up new state
    m_width   = (int)rect.width();
    m_height  = (int)rect.height();
    m_depth   = 1;
    debugAssert(m_dimension == DIM_2D || m_dimension == DIM_2D_RECT || m_dimension == DIM_2D);

    GLenum target = dimensionToTarget(m_dimension, m_numSamples);

    debugAssertGLOk();
    glBindTexture(target, m_textureID);
    debugAssertGLOk();

    glCopyTexImage2D(target, 0, format()->openGLFormat,
                     iRound(rect.x0()), 
                     iRound(rect.y0()), 
                     iRound(rect.width()), 
                     iRound(rect.height()), 
                     0);

    debugAssertGLOk();
    // Reset the original properties
    setAllSamplerParameters(target, m_cachedSamplerSettings);

    debugAssertGLOk();

    glStatePop();

    m_sizeOfAllTexturesInMemory += sizeInMemory();
}



void Texture::copyFromScreen(
    const Rect2D&       rect,
    CubeFace            face) {

    glStatePush();

    // Set up new state
    debugAssertM(m_width == rect.width(), "Cube maps require all six faces to have the same dimensions");
    debugAssertM(m_height == rect.height(), "Cube maps require all six faces to have the same dimensions");
    debugAssert(m_dimension == DIM_CUBE_MAP);

    if (GLCaps::supports_GL_ARB_multitexture()) {
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }
    glDisableAllTextures();

    glEnable(GL_TEXTURE_CUBE_MAP_ARB);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_textureID);

    GLenum target = openGLTextureTarget();
    if (isCubeMap()) {
        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face;
    }

    debugAssertGLOk();

    double viewport[4];
    glGetDoublev(GL_VIEWPORT, viewport);
    double viewportHeight = viewport[3];
    debugAssertGLOk();

    glCopyTexImage2D(target, 0, format()->openGLFormat,
                     iRound(rect.x0()), 
                     iRound(viewportHeight - rect.y1()), 
                     iRound(rect.width()), 
                     iRound(rect.height()), 0);

    debugAssertGLOk();
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);
    glStatePop();
}


void Texture::getCubeMapRotation(CubeFace face, Matrix3& outMatrix) {
    switch (face) {
    case CubeFace::POS_X:
        outMatrix = Matrix3::fromAxisAngle(Vector3::unitY(), (float)-halfPi());
        break;
        
    case CubeFace::NEG_X:
        outMatrix = Matrix3::fromAxisAngle(Vector3::unitY(), (float)halfPi());
        break;
        
    case CubeFace::POS_Y:
       outMatrix = CFrame::fromXYZYPRDegrees(0,0,0,0,90,0).rotation;
        break;
        
    case CubeFace::NEG_Y: 
        outMatrix = CFrame::fromXYZYPRDegrees(0,0,0,0,-90,0).rotation;
        break;
        
    case CubeFace::POS_Z:
        outMatrix = Matrix3::identity();
        break;
        
    case CubeFace::NEG_Z:
        outMatrix = Matrix3::fromAxisAngle(Vector3::unitY(), (float)pi());
        break;

    default:
        alwaysAssertM(false, "");
    }
    
    // GL's cube maps are "inside out" (they are the outside of a box,
    // not the inside), but its textures are also upside down, so
    // these turn into a 180-degree rotation, which fortunately does
    // not affect the winding direction.
    outMatrix = Matrix3::fromAxisAngle(Vector3::unitZ(), toRadians(180)) * -outMatrix;
}


int Texture::sizeInMemory() const {

    int64 base = (m_width * m_height * m_depth * format()->openGLBitsPerPixel) / 8;

    int64 total = 0;

    if (m_hasMipMaps) {
        int w = m_width;
        int h = m_height;

        while ((w > 2) && (h > 2)) {
            total += base;
            base /= 4;
            w /= 2;
            h /= 2;
        }

    } else {
        total = base;
    }

    if (m_dimension == DIM_CUBE_MAP) {
        total *= 6;
    }

    return (int)total;
}


unsigned int Texture::openGLTextureTarget() const {
   return dimensionToTarget(m_dimension, m_numSamples);
}


shared_ptr<Texture> Texture::alphaOnlyVersion() const {
    if (opaque()) {
        return shared_ptr<Texture>();
    }
    debugAssertM(
        m_dimension == DIM_2D ||
        m_dimension == DIM_2D_RECT ||
        m_dimension == DIM_2D,
        "alphaOnlyVersion only supported for 2D textures");
    
    int numFaces = 1;

    Array< Array<const void*> > mip;
    mip.resize(1);
    Array<const void*>& bytes = mip[0];
    bytes.resize(numFaces);
    const ImageFormat* bytesFormat = ImageFormat::A8();

    glStatePush();
    // Setup to later implement cube faces
    for (int f = 0; f < numFaces; ++f) {
        GLenum target = dimensionToTarget(m_dimension, m_numSamples);
        glBindTexture(target, m_textureID);
        bytes[f] = (const void*)System::malloc(m_width * m_height);
        glGetTexImage(target, 0, GL_ALPHA, GL_UNSIGNED_BYTE, const_cast<void*>(bytes[f]));
    }

    glStatePop();
    const int numSamples = 1;
    const shared_ptr<Texture>& ret = 
        fromMemory(
            m_name + " Alpha", 
            mip,
            bytesFormat,
            m_width, 
            m_height, 
            1, 
            numSamples,
            ImageFormat::A8(),
            m_dimension);

    for (int f = 0; f < numFaces; ++f) {
        System::free(const_cast<void*>(bytes[f]));
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////

void Texture::setDepthTexParameters(GLenum target, DepthReadMode depthReadMode) {
    debugAssertGLOk();
    //glTexParameteri(target, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

    if (depthReadMode == DepthReadMode::DEPTH_NORMAL) {
        glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    } else {
        glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, 
                        (depthReadMode == DepthReadMode::DEPTH_LEQUAL) ? GL_LEQUAL : GL_GEQUAL);
    }

    debugAssertGLOk();
}

static void setWrapMode(GLenum target, WrapMode wrapMode) {
    GLenum mode = GL_NONE;
    
    switch (wrapMode) {
    case WrapMode::TILE:
        mode = GL_REPEAT;
        break;

    case WrapMode::CLAMP:  
        if (target != GL_TEXTURE_2D_MULTISAMPLE) {
            mode = GL_CLAMP_TO_EDGE;
        } else {
            mode = GL_CLAMP;
        }
        break;

    case WrapMode::ZERO:
        mode = GL_CLAMP_TO_BORDER;
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, reinterpret_cast<const float*>(&Color4::clear()));
        debugAssertGLOk();
        break;

    default:
        debugAssertM(Texture::supportsWrapMode(wrapMode), "Unsupported wrap mode for Texture");
    }
    debugAssertGLOk();


    if (target != GL_TEXTURE_2D_MULTISAMPLE) {
        glTexParameteri(target, GL_TEXTURE_WRAP_S, mode);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, mode);
        glTexParameteri(target, GL_TEXTURE_WRAP_R, mode);

        debugAssertGLOk();
    }
}


static bool textureHasMipMaps(GLenum target, InterpolateMode interpolateMode) {
    return (target != GL_TEXTURE_RECTANGLE) &&
        (interpolateMode != InterpolateMode::BILINEAR_NO_MIPMAP) &&
        (interpolateMode != InterpolateMode::NEAREST_NO_MIPMAP) &&
		(target != GL_TEXTURE_2D_MULTISAMPLE);}


static void setMinMaxMipMaps(GLenum target, bool hasMipMaps, int minMipMap, int maxMipMap) {
    if (hasMipMaps) {
        glTexParameteri(target, GL_TEXTURE_MAX_LOD_SGIS, maxMipMap);
        glTexParameteri(target, GL_TEXTURE_MIN_LOD_SGIS, minMipMap);

		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxMipMap);
		
    }
}


static void setInterpolateMode(GLenum target, InterpolateMode interpolateMode) {
    if (target != GL_TEXTURE_2D_MULTISAMPLE) {
        switch (interpolateMode) {
        case InterpolateMode::TRILINEAR_MIPMAP:
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            break;

        case InterpolateMode::BILINEAR_MIPMAP:
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            break;
            
        case InterpolateMode::NEAREST_MIPMAP:
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            break;
            
        case InterpolateMode::BILINEAR_NO_MIPMAP:
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
            break;
            
        case InterpolateMode::NEAREST_NO_MIPMAP:
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;
            
        default:
            debugAssert(false);
        }
        debugAssertGLOk();
    }
}
    

static void setMaxAnisotropy(GLenum target, bool hasMipMaps, float maxAnisotropy) {
    static const bool anisotropic = GLCaps::supports("GL_EXT_texture_filter_anisotropic");

    if (anisotropic && hasMipMaps) {
        glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }
}


static void setMipBias(GLenum target, float mipBias) {
    if (mipBias != 0.0f) {
        glTexParameterf(target, GL_TEXTURE_LOD_BIAS, mipBias);
    }
}


void Texture::setDepthReadMode(GLenum target, DepthReadMode depthReadMode) {
    if (target != GL_TEXTURE_2D_MULTISAMPLE) {
        Texture::setDepthTexParameters(target, depthReadMode);
        debugAssertGLOk();
    }
}


void Texture::setAllSamplerParameters
    (GLenum               target,
    const Sampler&        settings) {
    
    debugAssert(
        target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_MULTISAMPLE ||
        target == GL_TEXTURE_RECTANGLE_EXT ||
        target == GL_TEXTURE_CUBE_MAP ||
        target == GL_TEXTURE_2D_ARRAY ||
        target == GL_TEXTURE_3D);

    debugAssertGLOk();
    
    const bool hasMipMaps = textureHasMipMaps(target, settings.interpolateMode);

    setWrapMode(target, settings.xWrapMode);
	debugAssertGLOk();
    setMinMaxMipMaps(target, hasMipMaps, settings.minMipMap, settings.maxMipMap);
	debugAssertGLOk();
    setInterpolateMode(target, settings.interpolateMode);
	debugAssertGLOk();
    setMaxAnisotropy(target, hasMipMaps, settings.maxAnisotropy);
	debugAssertGLOk();
    setMipBias(target, settings.mipBias);
	debugAssertGLOk();
    setDepthReadMode(target, settings.depthReadMode);
    debugAssertGLOk();
}


void Texture::updateSamplerParameters(const Sampler& settings) {
    GLenum target = dimensionToTarget(m_dimension, m_numSamples);
    debugAssert(
        target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_MULTISAMPLE ||
        target == GL_TEXTURE_RECTANGLE_EXT ||
        target == GL_TEXTURE_CUBE_MAP ||
        target == GL_TEXTURE_2D_ARRAY ||
        target == GL_TEXTURE_3D);

    debugAssertGLOk();

    bool hasMipMaps = textureHasMipMaps(target, settings.interpolateMode);
    
    if (settings.xWrapMode != m_cachedSamplerSettings.xWrapMode) {
        //debugAssertM(false, G3D::format("WrapMode: %s != %s", settings.wrapMode.toString(), m_cachedSamplerSettings.wrapMode.toString()));
        setWrapMode(target, settings.xWrapMode);
    }

    if (settings.minMipMap != m_cachedSamplerSettings.minMipMap ||
        settings.maxMipMap != m_cachedSamplerSettings.maxMipMap) {
        //debugAssertM(false, G3D::format("MinMipMap: %d != %d or MaxMipMap %d != %d", settings.minMipMap, m_cachedSamplerSettings.minMipMap, settings.maxMipMap, m_cachedSamplerSettings.maxMipMap));
        setMinMaxMipMaps(target, hasMipMaps, settings.minMipMap, settings.maxMipMap);
    }

    if (settings.interpolateMode != m_cachedSamplerSettings.interpolateMode) {
        //debugAssertM(false, G3D::format("InterpolateMode: %d != %d", settings.interpolateMode, m_cachedSamplerSettings.interpolateMode));
        setInterpolateMode(target, settings.interpolateMode);
    }

    if (settings.maxAnisotropy != m_cachedSamplerSettings.maxAnisotropy) {
        //debugAssertM(false, G3D::format("MaxAnisotropy: %f != %f", settings.maxAnisotropy, m_cachedSamplerSettings.maxAnisotropy));
        setMaxAnisotropy(target, hasMipMaps, settings.maxAnisotropy);
    }

    if (settings.mipBias != m_cachedSamplerSettings.mipBias) {
        //debugAssertM(false, G3D::format("MipBias: %f != %f", settings.mipBias, m_cachedSamplerSettings.mipBias));
        setMipBias(target, settings.mipBias);
    }

    if (settings.depthReadMode != m_cachedSamplerSettings.depthReadMode) {
        //debugAssertM(false, G3D::format("DepthReadMode: %f != %f", settings.depthReadMode, m_cachedSamplerSettings.depthReadMode));
        setDepthReadMode(target, settings.depthReadMode);
    }
    
    m_cachedSamplerSettings = settings;
}


static GLenum dimensionToTarget(Texture::Dimension d, int numSamples) {
    switch (d) {
    case Texture::DIM_CUBE_MAP:
        return GL_TEXTURE_CUBE_MAP;

    case Texture::DIM_CUBE_MAP_ARRAY:
        return GL_TEXTURE_CUBE_MAP_ARRAY;
        
    case Texture::DIM_2D:
        if (numSamples < 2) {
			return GL_TEXTURE_2D;
        } else {
			return GL_TEXTURE_2D_MULTISAMPLE;
        }
    case Texture::DIM_2D_ARRAY:
        return GL_TEXTURE_2D_ARRAY;

    case Texture::DIM_2D_RECT:
        return GL_TEXTURE_RECTANGLE_EXT;

    case Texture::DIM_3D:
        return GL_TEXTURE_3D;

    default:
        debugAssert(false);
        return 0 ;//GL_TEXTURE_2D;
    }
}



void computeStats
(const uint8* rawBytes, 
 GLenum       bytesActualFormat, 
 int          width,
 int          height,
 Color4&      minval,
 Color4&      maxval,
 Color4&      meanval,
 AlphaHint&   alphaHint,
 const Texture::Encoding& encoding) {
    minval  = Color4::nan();
    maxval  = Color4::nan();
    meanval = Color4::nan();
    alphaHint = AlphaHint::DETECT;

    if (rawBytes == NULL) {
        return;
    }

    // For sRGB conversion
    const int UNINITIALIZED = 255;
    static int toRGB[255] = {UNINITIALIZED};
    if (toRGB[0] == UNINITIALIZED) {
        // Initialize
        for (int i = 0; i < 255; ++i) {
            toRGB[i] = iRound(pow(i / 255.0f, 2.15f) * 255.0f);
        }
    }

    float inv255Width = 1.0f / (width * 255);
    switch (bytesActualFormat) {
    case GL_RGB8:
        {
            Color3unorm8 mn = Color3unorm8::one();
            Color3unorm8 mx = Color3unorm8::zero();
            meanval = Color4::zero();
            // Compute mean along rows to avoid overflow
            for (int y = 0; y < height; ++y) {
                const Color3unorm8* ptr = ((const Color3unorm8*)rawBytes) + (y * width);
                uint32 r = 0, g = 0, b = 0;
                for (int x = 0; x < width; ++x) {
                    const Color3unorm8 i = ptr[x];
                    mn = mn.min(i);
                    mx = mx.max(i);
                    r += i.r.bits(); g += i.g.bits(); b += i.b.bits();
                }
                meanval += Color4(float(r) * inv255Width, float(g) * inv255Width, float(b) * inv255Width, 1.0);
            }
            minval  = Color4(Color3(mn), 1.0f);
            maxval  = Color4(Color3(mx), 1.0f);
            meanval /= (float)height;
            alphaHint = AlphaHint::ONE;
        }
        break;

    case GL_RGBA8:
        {
            meanval = Color4::zero();
            Color4unorm8 mn = Color4unorm8::one();
            Color4unorm8 mx = Color4unorm8::zero();
            bool anyFractionalAlpha = false;
            // Compute mean along rows to avoid overflow
            for (int y = 0; y < height; ++y) {
                const Color4unorm8* ptr = ((const Color4unorm8*)rawBytes) + (y * width);
                uint32 r = 0, g = 0, b = 0, a = 0;
                for (int x = 0; x < width; ++x) {
                    const Color4unorm8 i = ptr[x];
                    mn = mn.min(i);
                    mx = mx.max(i);
                    r += i.r.bits(); g += i.g.bits(); b += i.b.bits(); a += i.a.bits();
                    anyFractionalAlpha = anyFractionalAlpha || ((i.a.bits() < 255) && (i.a.bits() > 0));
                }
                meanval += Color4(float(r) * inv255Width, float(g) * inv255Width, float(b) * inv255Width, float(a) * inv255Width);
            }
            minval = mn;
            maxval = mx;
            meanval = meanval / (float)height;
            if (mn.a.bits() * encoding.readMultiplyFirst.a + encoding.readAddSecond.a * 255 == 255) {
                alphaHint = AlphaHint::ONE;
            } else if (anyFractionalAlpha || (encoding.readMultiplyFirst.a != 1.0f) || (encoding.readAddSecond.a != 0.0f)) {
                alphaHint = AlphaHint::BLEND;
            } else {
                alphaHint = AlphaHint::BINARY;
            }
        }
        break;

    case GL_SRGB8:
        {
            Color3unorm8 mn = Color3unorm8::one();
            Color3unorm8 mx = Color3unorm8::zero();
            meanval = Color4::zero();
            // Compute mean along rows to avoid overflow
            for (int y = 0; y < height; ++y) {
                const Color3unorm8* ptr = ((const Color3unorm8*)rawBytes) + (y * width);
                uint32 r = 0, g = 0, b = 0;
                for (int x = 0; x < width; ++x) {
                    Color3unorm8 i = ptr[x];
                    // SRGB_A->RGB_A
                    i.r = unorm8::fromBits(toRGB[i.r.bits()]);
                    i.g = unorm8::fromBits(toRGB[i.r.bits()]);
                    i.b = unorm8::fromBits(toRGB[i.r.bits()]);

                    mn = mn.min(i);
                    mx = mx.max(i);
                    r += i.r.bits(); g += i.g.bits(); b += i.b.bits();
                }
                meanval += Color4(float(r) * inv255Width, float(g) * inv255Width, float(b) * inv255Width, 1.0);
            }
            minval  = Color4(Color3(mn), 1.0f);
            maxval  = Color4(Color3(mx), 1.0f);
            meanval /= (float)height;
            alphaHint = (1 * encoding.readMultiplyFirst.a + encoding.readAddSecond.a == 1) ? AlphaHint::ONE : AlphaHint::BLEND;
        }
        break;

    case GL_SRGB8_ALPHA8:
        {
            meanval = Color4::zero();
            Color4unorm8 mn = Color4unorm8::one();
            Color4unorm8 mx = Color4unorm8::zero();
            bool anyFractionalAlpha = false;
            // Compute mean along rows to avoid overflow
            for (int y = 0; y < height; ++y) {
                const Color4unorm8* ptr = ((const Color4unorm8*)rawBytes) + (y * width);
                uint32 r = 0, g = 0, b = 0, a = 0;
                for (int x = 0; x < width; ++x) {
                    Color4unorm8 i = ptr[x];
                    // SRGB_A->RGB_A
                    i.r = unorm8::fromBits(toRGB[i.r.bits()]);
                    i.g = unorm8::fromBits(toRGB[i.r.bits()]);
                    i.b = unorm8::fromBits(toRGB[i.r.bits()]);
                    mn = mn.min(i);
                    mx = mx.max(i);
                    r += i.r.bits(); g += i.g.bits(); b += i.b.bits(); a += i.a.bits();
                    anyFractionalAlpha = anyFractionalAlpha || ((i.a.bits() < 255) && (i.a.bits() > 0));
                }
                meanval += Color4(float(r) * inv255Width, float(g) * inv255Width, float(b) * inv255Width, float(a) * inv255Width);
            }
            minval = mn;
            maxval = mx;
            meanval = meanval / (float)height;
            if (anyFractionalAlpha) {
                alphaHint = AlphaHint::BLEND;
            } else if (mn.a.bits() == 255) {
                alphaHint = AlphaHint::ONE;
            } else {
                alphaHint = AlphaHint::BINARY;
            }
        }
        break;

    default:
        break;
    }
}

/** 
   @param bytesFormat OpenGL base format.

   @param bytesActualFormat OpenGL true format.  For compressed data,
   distinguishes the format that the data has due to compression.
 
   @param dataType Type of the incoming data from the CPU, e.g. GL_UNSIGNED_BYTES 
*/
static void createTexture
   (GLenum          target,
    const uint8*    rawBytes,
    GLenum          bytesActualFormat,
    GLenum          bytesFormat,
    int             m_width,
    int             m_height,
    int             depth,
    GLenum          ImageFormat,
    int             bytesPerPixel,
    int             mipLevel,
    bool            compressed,
    float           rescaleFactor,
    GLenum          dataType,
    bool            computeMinMaxMean,
    Color4&         minval, 
    Color4&         maxval, 
    Color4&         meanval,
    AlphaHint&      alphaHint,
    int		        numSamples,
    const Texture::Encoding& encoding) {

    uint8* bytes = const_cast<uint8*>(rawBytes);

    // If true, we're supposed to free the byte array at the end of
    // the function.
    bool   freeBytes = false; 
    int maxSize = GLCaps::maxTextureSize();
    if (computeMinMaxMean) {
        computeStats(rawBytes, bytesActualFormat, m_width, m_height, minval, maxval, meanval, alphaHint, encoding);
    }

    switch (target) {
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        maxSize = GLCaps::maxCubeMapSize();
        // Fall through

    case GL_TEXTURE_2D:
    case GL_TEXTURE_2D_MULTISAMPLE:
        if ((rescaleFactor != 1.0) || 
             (m_width > maxSize) || 
             (m_height > maxSize) ) {

            debugAssertM(! compressed,
                "Cannot rescale compressed textures");

            if (rawBytes != NULL) {
                int oldWidth = m_width;
                int oldHeight = m_height;
                m_width  = iMin(maxSize, static_cast<unsigned int>(m_width * rescaleFactor));
                m_height = iMin(maxSize, static_cast<unsigned int>(m_height * rescaleFactor));

                if ((oldWidth > maxSize) || (oldHeight > maxSize)) {
                    logPrintf("WARNING: %d x %d texture exceeded maximum size and was resized to %d x %d\n",
                              oldWidth, oldHeight, m_width, m_height);
                }

                bytes = new uint8[m_width * m_height * bytesPerPixel];
                freeBytes = true;

                gluScaleImage
                   (bytesFormat,
                    oldWidth,
                    oldHeight,
                    dataType,
                    rawBytes,
                    m_width,
                    m_height,
                    dataType,
                    bytes);
                debugAssertGLOk();
            }
        }

        // Intentionally fall through 

    case GL_TEXTURE_RECTANGLE:

        // Note code falling through from above

        if (compressed) {
            
            debugAssertM((target != GL_TEXTURE_RECTANGLE),
                "Compressed textures must be DIM_2D or DIM_2D.");

            glCompressedTexImage2DARB
                (target, mipLevel, bytesActualFormat, m_width, 
                 m_height, 0, (bytesPerPixel * ((m_width + 3) / 4) * ((m_height + 3) / 4)), 
                 rawBytes);

        } else {

            if (bytes != NULL) {
                debugAssert(isValidPointer(bytes));
                debugAssertM(isValidPointer(bytes + (m_width * m_height - 1) * bytesPerPixel), 
                    "Byte array in Texture creation was too small");
            }

            // 2D texture, level of detail 0 (normal), internal
            // format, x size from image, y size from image, border 0
            // (normal), rgb color data, unsigned byte data, and
            // finally the data itself.
            glPixelStorei(GL_PACK_ALIGNMENT, 1);

            if (target == GL_TEXTURE_2D_MULTISAMPLE) {
                glTexImage2DMultisample(target, numSamples, ImageFormat, m_width, m_height, false);
            } else {
                debugAssertGLOk();
                glTexImage2D(target, mipLevel, ImageFormat, m_width, m_height, 0, bytesFormat, dataType, bytes);
                debugAssertGLOk();
            }
        }
        break;

    case GL_TEXTURE_3D:
    case GL_TEXTURE_2D_ARRAY:
        if (bytes != NULL) {
            debugAssert(isValidPointer(bytes));
        }

        glTexImage3D(target, mipLevel, ImageFormat, m_width, m_height, depth, 0, bytesFormat, dataType, bytes);
        break;

    default:
        debugAssertM(false, "Fell through switch");
    }

    if (freeBytes) {
        // Texture was resized; free the temporary.
        delete[] bytes;
    }
}


static bool isSRGBFormat(const ImageFormat* format) {
    return format->colorSpace == ImageFormat::COLOR_SPACE_SRGB;
}


static GLint getPackAlignment(int bufferStride, GLint& oldPackAlignment, bool& alignmentNeedsToChange) {
    oldPackAlignment = 8; // LCM of all possible alignments
    int alignmentOffset = bufferStride % oldPackAlignment;
    if (alignmentOffset != 0) {
        glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment); // find actual alignment
        alignmentOffset = bufferStride % oldPackAlignment;
    }
    alignmentNeedsToChange = alignmentOffset != 0;
    GLint newPackAlignment = oldPackAlignment;
    if (alignmentNeedsToChange) {
        if (alignmentOffset == 4) {
            newPackAlignment = 4;
        } else if (alignmentOffset % 2 == 0) {
            newPackAlignment = 2;
        } else {
            newPackAlignment = 1;
        }
    }
    return newPackAlignment;
}


shared_ptr<GLPixelTransferBuffer> Texture::toPixelTransferBuffer(const ImageFormat* outFormat, int mipLevel, CubeFace face) const {
    if (outFormat == ImageFormat::AUTO()) {
        outFormat = format();
    }
    debugAssertGLOk();
    alwaysAssertM( !isSRGBFormat(outFormat) || isSRGBFormat(format()), "glGetTexImage doesn't do sRGB conversion, so we need to first copy an RGB texture to sRGB on the GPU. However, this functionality is broken as of the time of writing this code");
    if (isSRGBFormat(format()) && !isSRGBFormat(outFormat) ) { // Copy to non-srgb texture and read back.
        const shared_ptr<Texture>& temp = Texture::createEmpty("Temporary copy", m_width, m_height, outFormat, m_dimension, false, m_depth);
        Texture::copy(dynamic_pointer_cast<Texture>(const_cast<Texture*>(this)->shared_from_this()), temp);
        return temp->toPixelTransferBuffer(outFormat, mipLevel, face);
    }

    // OpenGL's sRGB readback is non-intuitive.  If we're reading from sRGB to sRGB, we actually read back using "RGB".
    if (outFormat == format()) {
        if (outFormat == ImageFormat::SRGB8()) {
            outFormat = ImageFormat::RGB8();
        } else if (outFormat == ImageFormat::SRGBA8()) {
            outFormat = ImageFormat::RGBA8();
        }
    }
    int mipDepth = 1;
    if (dimension() == DIM_3D) {
        mipDepth = depth() >> mipLevel;
    } else if (dimension() == DIM_2D_ARRAY) {
        mipDepth = depth();
    }
    const shared_ptr<GLPixelTransferBuffer>& buffer = GLPixelTransferBuffer::create(width() >> mipLevel, height() >> mipLevel, outFormat, NULL, mipDepth, GL_STATIC_READ);
    
    glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer->glBufferID()); {

        glBindTexture(openGLTextureTarget(), openGLID()); {
        
            GLenum target;
            if (isCubeMap()) { 
                target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face;
            } else {
                // Not a cubemap
                target = openGLTextureTarget();
            }

            bool alignmentNeedsToChange;
            GLint oldPackAlignment;
            GLint newPackAlignment = getPackAlignment((int)buffer->stride(), oldPackAlignment, alignmentNeedsToChange);
            if (alignmentNeedsToChange) {
                glPixelStorei(GL_PACK_ALIGNMENT, newPackAlignment);   
            }
            glGetTexImage(target, mipLevel, outFormat->openGLBaseFormat, outFormat->openGLDataFormat, 0);
            if (alignmentNeedsToChange) {
                glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);  
            }

            
        } glBindTexture(openGLTextureTarget(), GL_NONE);
        
    } glBindBuffer(GL_PIXEL_PACK_BUFFER, GL_NONE);
    debugAssertGLOk();
    
    return buffer;
}


shared_ptr<Image> Texture::toImage(const ImageFormat* outFormat, int mipLevel, CubeFace face) const {
    return Image::fromPixelTransferBuffer(toPixelTransferBuffer(outFormat, mipLevel, face));
}


void Texture::update(const shared_ptr<PixelTransferBuffer>& src, int mipLevel, CubeFace face) {
    alwaysAssertM(format()->openGLBaseFormat == src->format()->openGLBaseFormat,
                    "Data must have the same number of channels as the texture: this = " + format()->name() + 
                    "  src = " + src->format()->name());

    const shared_ptr<GLPixelTransferBuffer>& glsrc = dynamic_pointer_cast<GLPixelTransferBuffer>(src);


    {
        glBindTexture(openGLTextureTarget(), openGLID());
debugAssertGLOk();

        GLint previousPackAlignment;
        glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
debugAssertGLOk();            
        const GLint xoffset = 0;
        const GLint yoffset = 0;
        const GLint zoffset = 0;
            
        GLenum target = openGLTextureTarget();
        if (isCubeMap()) {
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face;
        }

        const void* ptr = NULL;

        if (notNull(glsrc)) {
debugAssertGLOk();
            // Bind directly instead of invoking bindRead(); see below for discussion
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, glsrc->glBufferID());
debugAssertGLOk();
            // The pointer is an offset in this case
            ptr = 0;
        } else {
            // Regular path
            ptr = src->mapRead();
        }
        
        if (dimension() == DIM_2D || dimension() == DIM_CUBE_MAP) {
            debugAssertGLOk();
            glTexSubImage2D
                (target,
                    mipLevel,
                    xoffset,
                    yoffset,
                    src->width(),
                    src->height(),
                    src->format()->openGLBaseFormat,
                    src->format()->openGLDataFormat,
                    ptr);
            debugAssertGLOk();
        } else {
            alwaysAssertM(dimension() == DIM_3D || dimension() == DIM_2D_ARRAY, 
                "Texture::update only works with 2D, 3D, cubemap, and 2D array textures");
            debugAssertGLOk();
            glTexSubImage3D
                (target,
                    mipLevel,
                    xoffset,
                    yoffset,
                    zoffset,
                    src->width(),
                    src->height(),
                    src->depth(),
                    src->format()->openGLBaseFormat,
                    src->format()->openGLDataFormat,
                    ptr);
            debugAssertGLOk();
        }

        if (notNull(glsrc)) {
            // Creating the fence for this operation is VERY expensive because it causes a pipeline stall [on NVIDIA GPUs],
            // so we directly unbind the buffer instead of creating a fence.
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GL_NONE);
debugAssertGLOk();
        } else {
            // We mapped the non-GL PTB, so unmap it 
            src->unmap();
        }
	glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
	debugAssertGLOk();
        glBindTexture(openGLTextureTarget(), 0);
        debugAssertGLOk();

    }
}


void Texture::setShaderArgs(UniformTable& args, const String& prefix, const Sampler& sampler) {
    const bool structStyle = ! prefix.empty() && (prefix[prefix.size() - 1] == '.');

    debugAssert(this != NULL);
    if (prefix.find('.') == std::string::npos) {
        args.setMacro(prefix + "notNull", true);
    } else if (structStyle) {
        args.setUniform(prefix + "notNull", true);
    }

    if (structStyle) {
        args.setUniform(prefix + "sampler", dynamic_pointer_cast<Texture>(shared_from_this()), sampler);
    } else {
        // Backwards compatibility
        args.setUniform(prefix + "buffer", dynamic_pointer_cast<Texture>(shared_from_this()), sampler);
    }

    args.setUniform(prefix + "readMultiplyFirst", m_encoding.readMultiplyFirst, true);
    args.setUniform(prefix + "readAddSecond", m_encoding.readAddSecond, true);

    if (structStyle && (m_dimension != DIM_2D_ARRAY) && (m_dimension != DIM_3D) && (m_dimension != DIM_CUBE_MAP_ARRAY)) {
        const Vector2 size((float)width(), (float)height());
        args.setUniform(prefix + "size", size);
        args.setUniform(prefix + "invSize", Vector2(1.0f, 1.0f) / size);
    } else {
        const Vector3 size((float)width(), (float)height(), (float)depth());
        args.setUniform(prefix + "size", size);
        args.setUniform(prefix + "invSize", Vector3(1.0f, 1.0f, 1.0f) / size);
    }
}


/////////////////////////////////////////////////////


Texture::Dimension Texture::toDimension(const String& s) {
    if (s == "DIM_2D") {
        return DIM_2D;
    } else if (s == "DIM_2D_ARRAY") {
        return DIM_2D_ARRAY;
    } else if (s == "DIM_2D_RECT") {
        return DIM_2D_RECT;
    } else if (s == "DIM_3D") {
        return DIM_3D;
    } else if (s == "DIM_CUBE_MAP") {
        return DIM_CUBE_MAP;
    } else if (s == "DIM_CUBE_MAP_ARRAY") {
        return DIM_CUBE_MAP_ARRAY;
    } else {
        debugAssertM(false, "Unrecognized dimension");
        return DIM_2D;
    }
}


const char* Texture::toString(Dimension d) {
    switch (d) {
    case DIM_2D: return "DIM_2D";
    case DIM_2D_ARRAY: return "DIM_2D_ARRAY";
    case DIM_3D: return "DIM_3D";
    case DIM_2D_RECT: return "DIM_2D_RECT";
    case DIM_CUBE_MAP: return "DIM_CUBE_MAP";
    case DIM_CUBE_MAP_ARRAY: return "DIM_CUBE_MAP_ARRAY";
    default:
        return "ERROR";
    }
}

#ifdef G3D_ENABLE_CUDA

CUarray &Texture::cudaMap(unsigned int	usageflags){ //default should be: CU_GRAPHICS_REGISTER_FLAGS_NONE
	//CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST
	//TODO: Unregister ressource in destructor.

	if( m_cudaTextureResource != NULL && usageflags!=m_cudaUsageFlags){
		cuGraphicsUnregisterResource(m_cudaTextureResource);
	}

	if( m_cudaTextureResource == NULL || usageflags!=m_cudaUsageFlags){
		cuGraphicsGLRegisterImage(&m_cudaTextureResource, this->openGLID(), GL_TEXTURE_2D, usageflags );

		m_cudaUsageFlags = usageflags;
	}
		
	debugAssert(m_cudaIsMapped==false);

	cuGraphicsMapResources (1, &m_cudaTextureResource, 0);
	cuGraphicsSubResourceGetMappedArray ( &m_cudaTextureArray, m_cudaTextureResource, 0, 0);

	m_cudaIsMapped = true;

	return m_cudaTextureArray;
}

void Texture::cudaUnmap(){
	debugAssert(m_cudaIsMapped);

	cuGraphicsUnmapResources(1, &m_cudaTextureResource, 0);

	m_cudaIsMapped = false;
}

#endif

} // G3D
