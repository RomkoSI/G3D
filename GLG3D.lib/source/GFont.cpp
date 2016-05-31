/**
 \file GFont.cpp
 
 \maintainer Morgan McGuire, http://graphics.cs.williams.edu

 \created 2002-11-02
 \edited  2016-06-25

 Copyright 2000-2016, Morgan McGuire.
 All rights reserved.
*/

#include "G3D/ImageFormat.h"
#include "G3D/Vector2.h"
#include "G3D/System.h"
#include "G3D/Array.h"
#include "G3D/fileutils.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"
#include "G3D/WeakCache.h"
#include "G3D/TextInput.h"
#include "G3D/Log.h"
#include "G3D/FileSystem.h"
#include "GLG3D/GFont.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/GLCaps.h"
#include "G3D/CPUPixelTransferBuffer.h"
#include "GLG3D/Shader.h"


namespace G3D {

namespace _internal {

WeakCache< String, shared_ptr<GFont> >& fontCache() {
    static WeakCache<String, shared_ptr<GFont> > cache;
    return cache;
}

}


// Copies blocks of src so that they are centered in the corresponding squares of dst.  
// Assumes each is a 16x16 grid.
void GFont::recenterGlyphs(const shared_ptr<Image>& src, shared_ptr<Image>& dst) {
    // Set dst to all black
    debugAssert(src->format() == ImageFormat::RGB8());
    debugAssert(dst->format() == ImageFormat::RGB8());
    debugAssert(dst->width()  >= src->width());
    debugAssert(dst->height() >= src->height());

    // Block sizes
    const int srcWidth  = src->width()  / 16;
    const int srcHeight = src->height() / 16;
    const int dstWidth  = dst->width()  / 16;
    const int dstHeight = dst->height() / 16;

    const int dstOffsetX = (dstWidth  - srcWidth) / 2;
    const int dstOffsetY = (dstHeight - srcHeight) / 2;

    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            shared_ptr<PixelTransferBuffer> copy = src->toPixelTransferBuffer(Rect2D::xywh(float(x * srcWidth), float(y * srcHeight), (float)srcWidth, (float)srcHeight));
            dst->set(copy, x * dstWidth + dstOffsetX, y * dstHeight + dstOffsetY);
        }
    }
}


void GFont::adjustINIWidths(const String& srcFile, const String& dstFile, float scale) {
    TextInput in(srcFile);
    TextOutput out(dstFile);
    in.readSymbols("[", "Char", "Widths", "]");
    out.printf("[Char Widths]\n");
    for (int i = 0; i <= 255; ++i) {
        in.readNumber();
        in.readSymbol("=");

        int w = (int)in.readNumber();
        w = iRound(w * scale);
        out.printf("%d=%d\n", i, w);
    }
    out.commit();
}


shared_ptr<GFont> GFont::fromFile(const String& filename) {

    if (! FileSystem::exists(filename)) {
        debugAssertM(false, format("Could not load font: %s", filename.c_str()));
        return shared_ptr<GFont>();
    }

    String key = filenameBaseExt(filename);
    shared_ptr<GFont> font = _internal::fontCache()[key];
    if (isNull(font)) {
        BinaryInput b(filename, G3D_LITTLE_ENDIAN, true);
        font.reset(new GFont(filename, b));
        _internal::fontCache().set(key, font);
    }

    return font;
}


shared_ptr<GFont> GFont::fromMemory(const String& name, const uint8* bytes, const int size) {
    // Note that we do not need to copy the memory since GFont will be done with it
    // by the time this method returns.
    BinaryInput b(bytes, size, G3D_LITTLE_ENDIAN, true, false); 
    return shared_ptr<GFont>(new GFont(name, b));
} 


GFont::GFont(const String& filename, BinaryInput& b) {    

    const int ver = b.readInt32();
    debugAssertM((ver == 1) || (ver == 2), "Can't read font files other than version 1");
    (void)ver;

    if (ver == 1) {
        charsetSize = 128;
    } else {
        charsetSize = b.readInt32();
    }

    // Read the widths
    subWidth.resize(charsetSize);
    for (int c = 0; c < charsetSize; ++c) {
        subWidth[c] = b.readUInt16();
    }

    baseline = b.readUInt16();
    int texWidth = b.readUInt16();
    charWidth  = texWidth / 16;
    charHeight = texWidth / 16;

    // The input may not be a power of 2
    const int width  = ceilPow2(unsigned(charWidth * 16));
    const int height = ceilPow2(unsigned(charHeight * (charsetSize / 16)));
  
    // Create a texture
    const uint8* ptr = ((uint8*)b.getCArray()) + b.getPosition();
    debugAssertM((b.getLength() - b.getPosition()) >= width * height, 
        "File does not contain enough data for this size texture");

    Texture::Preprocess preprocess;
    preprocess.computeMinMaxMean = false;
    const bool generateMipMaps = true;

    
    m_texture = 
        Texture::fromMemory
        (   "G3D::GFont " + filename, 
            ptr,
            ImageFormat::R8(),
            width, 
            height,
            1,
            1,
            ImageFormat::R8(), 
            Texture::DIM_2D,
            generateMipMaps,
            preprocess);
       
    m_textureMatrix[0] = 1.0f / m_texture->width();
    m_textureMatrix[1] = 0;
    m_textureMatrix[2] = 0;
    m_textureMatrix[3] = 0;
    m_textureMatrix[4] = 0;
    m_textureMatrix[5] = 1.0f / m_texture->height();
    m_textureMatrix[6] = 0;
    m_textureMatrix[7] = 0;    
    m_textureMatrix[8] = 0;
    m_textureMatrix[9] = 0;
    m_textureMatrix[10] = 1;
    m_textureMatrix[11] = 0;
    m_textureMatrix[12] = 0;
    m_textureMatrix[13] = 0;
    m_textureMatrix[14] = 0;
    m_textureMatrix[15] = 1;

    m_name = filename;
}


Vector2 GFont::texelSize() const {
    return Vector2((float)charWidth, (float)charHeight);
}


Vector2 GFont::appendToPackedArray
   (const String&           s,
    float                   x,
    float                   y,
    float                   w,
    float                   h,
    Spacing                 spacing,
    const Color4&           color,
    const Color4&           borderColor,
    Array<CPUCharVertex>&   vertexArray,
    Array<int>&             indexArray) const {
        
    const float propW = w / charWidth;

    // Worst case number of characters to add
    const int n = int(s.length());
    int oldVertexCount = vertexArray.size();
    int newVertexCount = vertexArray.size() + n * 4;
    int nextIndexCount = indexArray.size()   + n * 6;
    if (vertexArray.capacity() < newVertexCount) {
        vertexArray.reserve(newVertexCount);
    }
    if (indexArray.capacity() < nextIndexCount) {
        indexArray.reserve(nextIndexCount);
    }

    // Shrink the vertical texture coordinates by 1 texel to avoid
    // bilinear interpolation interactions with mipmapping.
    float sy = h / charHeight;

    float x0 = x;

    const float mwidth = subWidth[(int)'M'] * 0.85f * propW;

    for (int i = 0; i < n; ++i) {
        const unsigned char c = s[i] & (charsetSize - 1); // s[i] % charsetSize; avoid using illegal chars

        if (c != ' ') {
            const int row   = c >> 4; // fast version of c / 16
            const int col   = c & 15; // fast version of c % 16

            // Fixed width
            const float sx = (spacing == PROPORTIONAL_SPACING) ?
                (charWidth - subWidth[(int)c]) * propW * 0.5f : 0.0f;

            {
                const int v = vertexArray.size();
                indexArray.append
                    (v + 0, v + 1, v + 2,
                     v + 0, v + 2, v + 3);
            }
            int baseV = vertexArray.size();
            vertexArray.resize(baseV + 4, false);
            float xx = x - sx;
            {
                CPUCharVertex& CV = vertexArray[baseV+0];
                CV.texCoord.x   = float(col * charWidth);
                CV.texCoord.y   = float(row * charHeight + 1);
                CV.position.x   = xx;
                CV.position.y   = y + sy;
                CV.color        = color;
                CV.borderColor  = borderColor;
            }

            {
                CPUCharVertex& CV = vertexArray[baseV+1];
                CV.texCoord.x   = float(col * charWidth);
                CV.texCoord.y   = float((row + 1) * charHeight - 2);
                CV.position.x   = xx;
                CV.position.y   = y + h - sy;
                CV.color        = color;
                CV.borderColor  = borderColor;
            }
            xx += w;

            {
                CPUCharVertex& CV = vertexArray[baseV+2];
                CV.texCoord.x   = float((col + 1) * charWidth - 1);
                CV.texCoord.y   = float((row + 1) * charHeight - 2);
                CV.position.x   = xx;
                CV.position.y   = y + h - sy;
                CV.color        = color;
                CV.borderColor  = borderColor;
            }

            {
                CPUCharVertex& CV = vertexArray[baseV+3];
                CV.texCoord.x   = float((col + 1) * charWidth - 1);
                CV.texCoord.y   = float(row * charHeight + 1);
                CV.position.x   = xx;
                CV.position.y   = y + sy;
                CV.color        = color;
                CV.borderColor  = borderColor;
            }            
        }
        x += (spacing == PROPORTIONAL_SPACING) ? propW * subWidth[(int)c] : mwidth;
    }


    return Vector2(x - x0, h);
}


void GFont::renderCharVertexArray(RenderDevice* renderDevice, const Array<CPUCharVertex>& cpuCharArray, Array<int>& indexArray) const {

    // Avoid the overhead of Profiler events by not calling LAUNCH_SHADER...
    static const shared_ptr<Shader> fontShader = Shader::getShaderFromPattern("GFont_render.*");

    renderDevice->setBlendFunc(RenderDevice::BLEND_SRC_ALPHA, RenderDevice::BLEND_ONE_MINUS_SRC_ALPHA);
    renderDevice->setCullFace(CullFace::NONE);

    const shared_ptr<VertexBuffer>& vb = VertexBuffer::create((cpuCharArray.size() * sizeof(CPUCharVertex)) + (indexArray.size() * sizeof(int)) + 8, VertexBuffer::WRITE_EVERY_FRAME);

    IndexStream indexStream(indexArray, vb);
    AttributeArray interleavedArray(cpuCharArray.size() * sizeof(CPUCharVertex), vb);

    CPUCharVertex dummy;
#   define OFFSET(field) ((size_t)(&dummy.field) - (size_t)&dummy)
        AttributeArray  texCoordArray   (dummy.texCoord,    cpuCharArray.size(), interleavedArray, OFFSET(texCoord),    sizeof(CPUCharVertex));
        AttributeArray  positionArray   (dummy.position,    cpuCharArray.size(), interleavedArray, OFFSET(position),    sizeof(CPUCharVertex));
        AttributeArray  colorArray      (dummy.color,       cpuCharArray.size(), interleavedArray, OFFSET(color),       sizeof(CPUCharVertex));
        AttributeArray  borderColorArray(dummy.borderColor, cpuCharArray.size(), interleavedArray, OFFSET(borderColor), sizeof(CPUCharVertex));
#   undef OFFSET
    
    CPUCharVertex* dst = (CPUCharVertex*)interleavedArray.mapBuffer(GL_WRITE_ONLY);
    System::memcpy(dst, cpuCharArray.getCArray(), cpuCharArray.size() * sizeof(CPUCharVertex));
    interleavedArray.unmapBuffer();

    Args args;
    args.enableG3DArgs(false); // Lower CPU overhead for uniform bind

    args.setAttributeArray("g3d_Vertex",    positionArray);
    args.setAttributeArray("g3d_TexCoord0", texCoordArray);
    args.setAttributeArray("borderColor",   borderColorArray);    
    args.setAttributeArray("color",         colorArray);
    args.setIndexStream(indexStream);
    args.setPrimitiveType(PrimitiveType::TRIANGLES);

    args.setUniform("g3d_ObjectToScreenMatrixTranspose", RenderDevice::current->objectToScreenMatrix().transpose()); 
    args.setUniform("textureMatrix",        Matrix4(m_textureMatrix));
    args.setUniform("borderWidth",          Vector2(m_textureMatrix[0], m_textureMatrix[5]));
    args.setUniform("textureLODBias",       -0.6f);
    args.setUniform("fontTexture",          m_texture, Sampler::defaults());
    args.setUniform("alphaThreshold",       1.0f / 255.0f);
    args.setUniform("translation",          Vector2(0, 0));
    
    RenderDevice::current->apply(fontShader, args);
}


Vector2 GFont::appendToCharVertexArrayWordWrap
(Array<CPUCharVertex>&       cpuCharArray,
 Array<int>&                 indexArray,
 RenderDevice*               renderDevice,
 float                       maxWidth,
 const String&               s,
 const Vector2&              pos2D,
 float                       size,
 const Color4&               color,
 const Color4&               border,
 XAlign                      xalign,
 YAlign                      yalign,
 Spacing                     spacing) const {

    if (maxWidth == finf()) {
        return appendToCharVertexArray(cpuCharArray, indexArray, renderDevice, s, pos2D, size, color, border, xalign, yalign, spacing);
    }

    Vector2 bounds = Vector2::zero();
    Point2 p = pos2D;
    String rest = s;
    String first = "";

    while (! rest.empty()) {
        wordWrapCut(maxWidth, rest, first, size, spacing);
        Vector2 extent = appendToCharVertexArray(cpuCharArray, indexArray, renderDevice, first, p, size, color, border, xalign, yalign, spacing);
        bounds.x = max(bounds.x, extent.x);
        bounds.y += extent.y;
        p.y += iCeil(extent.y * 0.8f);
    }

    return bounds;
}


Vector2 GFont::appendToCharVertexArray
   (Array<CPUCharVertex>&       cpuCharArray,
    Array<int>&                 indexArray,
    RenderDevice*               renderDevice,
    const String&               s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    float x = pos2D.x;
    float y = pos2D.y;

    float h = size * 1.5f;
    float w = h * charWidth / charHeight;

    int numChars = 0;
    for (unsigned int i = 0; i < s.length(); ++i) {
        // Spaces don't count as characters
        numChars += ((s[i] & (charsetSize - 1)) != ' ') ? 1 : 0;
    }

    if (numChars == 0) {
        return Vector2(0, h);
    }

    switch (xalign) {
    case XALIGN_RIGHT:
        x -= bounds(s, size, spacing).x;
        break;

    case XALIGN_CENTER:
        x -= bounds(s, size, spacing).x / 2.0f;
        break;
    
    default:
        break;
    }

    switch (yalign) {
    case YALIGN_CENTER:
        y -= h / 2.0f;
        break;

    case YALIGN_BASELINE:
        y -= baseline * h / (float)charHeight;
        break;

    case YALIGN_BOTTOM:
        y -= h;
        break;

    default:
        break;
    }

    // Packed vertex array; tex coord and vertex are interlaced
    // For each character we need 4 vertices.
    const Vector2& bounds = appendToPackedArray(s, x, y, w, h, spacing, color, border, cpuCharArray, indexArray);
    return bounds;
}


Vector2 GFont::draw2D
   (RenderDevice*               renderDevice,
    const String&               s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    debugAssert(renderDevice != NULL);

    Vector2 bounds;
    renderDevice->pushState(); {
        static Array<CPUCharVertex> charVertexArray;
        static Array<int> indexArray;
        indexArray.fastClear();
        charVertexArray.fastClear();
        bounds = appendToCharVertexArray(charVertexArray, indexArray, renderDevice, s, pos2D, size, color, border, xalign, yalign, spacing);
        renderCharVertexArray(renderDevice, charVertexArray, indexArray);
    } renderDevice->popState();

    return bounds;
}


Vector2 GFont::draw3DBillboard
   (RenderDevice*               renderDevice,
    const String&               s,
    const Vector3&              pos3D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    const CFrame& camera = renderDevice->cameraToWorldMatrix();
    const CFrame& object = renderDevice->objectToWorldMatrix();
    
    const CFrame cframe(camera.rotation * object.rotation.transpose(), pos3D);
    return draw3D(renderDevice, s, cframe, size, color, border, xalign, yalign, spacing);
}


Vector2 GFont::draw3D
   (RenderDevice*               renderDevice,
    const String&               s,
    const CoordinateFrame&      pos3D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {
    
    debugAssert(renderDevice != NULL);
    Vector2 bounds;
    renderDevice->pushState(); {
        CoordinateFrame flipY;
        flipY.rotation[1][1] = -1;
        renderDevice->setObjectToWorldMatrix(pos3D * flipY);
        bounds = draw2D(renderDevice, s, Vector2(0, 0), size, color, border, xalign, yalign, spacing);
    } renderDevice->popState();
    return bounds;
}
 

Vector2 GFont::draw2DWordWrap
   (RenderDevice*               renderDevice,
    float                       maxWidth,
    const String&               s,
    const Vector2&              pos2D,
    float                       size,
    const Color4&               color,
    const Color4&               border,
    XAlign                      xalign,
    YAlign                      yalign,
    Spacing                     spacing) const {

    debugAssert(renderDevice != NULL);

    Vector2 bounds;
    renderDevice->pushState();
    {
        static Array<CPUCharVertex> charVertexArray;
        static Array<int> indexArray;
        indexArray.fastClear();
        charVertexArray.fastClear();
        bounds = appendToCharVertexArrayWordWrap(charVertexArray, indexArray, renderDevice, maxWidth, s, pos2D, size, color, border, xalign, yalign, spacing);
        renderCharVertexArray(renderDevice, charVertexArray, indexArray);
    }
    renderDevice->popState();
    
    return bounds;
}


int GFont::wordSplitByWidth
    (const float maxWidth,
    const String& s,
    float size,
    Spacing spacing) const {
    
    int n = (int)s.length();

    const float h = size * 1.5f;
    const float w = h * charWidth / charHeight;
    const float propW = w / charWidth;
    float x = 0;

    // Current character number
    int i = 0;

    // Walk forward until we hit the end of the string or the maximum width
    while ((x <= maxWidth) && (i < n)) {
        const unsigned char c = s[i] & (charsetSize - 1);

        if (spacing == PROPORTIONAL_SPACING) {
            x += propW * subWidth[(int)c];
        } else {
            x += propW * subWidth[(int)'M'] * 0.85f;
        }

        if (c == '\n') {
            // Hit a new line; force us past the line end
            break;
        }
        ++i;
    }

    return i - 1;

}
    
int GFont::wordWrapCut
   (float               maxWidth,
    String&             s,
    String&             firstPart,
    float               size,
    Spacing             spacing) const {

    debugAssert(maxWidth > 0);
    int n = (int)s.length();

    const float h = size * 1.5f;
    const float w = h * charWidth / charHeight;
    const float propW = w / charWidth;
    float x = 0;

    // Current character number
    int i = 0;

    // Walk forward until we hit the end of the string or the maximum width
    while ((x <= maxWidth) && (i < n)) {
        const unsigned char c = s[i] & (charsetSize - 1);

        if (spacing == PROPORTIONAL_SPACING) {
            x += propW * subWidth[(int)c];
        } else {
            x += propW * subWidth[(int)'M'] * 0.85f;
        }

        if (c == '\n') {
            // Hit a new line; force us past the line end
            break;
        }
        ++i;
    }

    if (i == n) {
        // We made it to the end
        firstPart = s;
        s = "";
        return n;
    }

    --i;

    // Search backwards for a space.  Only look up to 25% of maxWidth
    while ((i > 1) && ! isWhiteSpace(s[i]) && (x > maxWidth * 0.25f)) {
        const unsigned char c = s[i] & (charsetSize - 1);
        if (spacing == PROPORTIONAL_SPACING) {
            x -= propW * subWidth[(int)c];
        } else {
            x -= propW * subWidth[(int)'M'] * 0.85f;
        }
        --i;
    }

    firstPart = s.substr(0, i);
    s = trimWhitespace(s.substr(i));

    return i;
}


Vector2 GFont::bounds
   (const String&  s,
    float               size,
    Spacing             spacing) const {

    int n = (int)s.length();

    float h = size * 1.5f;
    float w = h * charWidth / charHeight;
    float propW = w / charWidth;
    float x = 0;
    float y = h;

    if (spacing == PROPORTIONAL_SPACING) {
        for (int i = 0; i < n; ++i) {
            unsigned char c = s[i] & (charsetSize - 1);
            x += propW * subWidth[(int)c];
        }
    } else {
        x = subWidth[(int)'M'] * n * 0.85f * propW;
    }

    return Vector2(x, y);
}


Vector2 GFont::boundsWordWrap
    (float               maxWidth,
     const String&  s,
     float               size,
     Spacing             spacing) const {

     String rest = s;
     String first;

     Vector2 wrapBounds;

     while (! rest.empty()) {
         GFont::wordWrapCut(maxWidth, rest, first, size, spacing);
         Vector2 b = bounds(first, size, spacing);
         wrapBounds.x = max(b.x, wrapBounds.x);
         wrapBounds.y += b.y;
     }

     return wrapBounds;
}


void GFont::makeFont(int charsetSize, const String& infileBase, String outfile) {

    debugAssert(FileSystem::exists(infileBase + ".tga"));
    debugAssert(FileSystem::exists(infileBase + ".ini"));
    debugAssert(charsetSize == 128 || charsetSize == 256);

    if (outfile == "") {
        outfile = infileBase + ".fnt";
    }

    TextInput    ini(infileBase + ".ini");
    BinaryOutput out(outfile, G3D_LITTLE_ENDIAN);

    ini.readSymbol("[");
    ini.readSymbol("Char");
    ini.readSymbol("Widths");
    ini.readSymbol("]");

    // Version
    out.writeInt32(2);

    // Number of characters
    out.writeInt32(charsetSize);

    // Character widths
    for (int i = 0; i < charsetSize; ++i) {
        int n = (int)ini.readNumber();
        (void)n;
        debugAssert(n == i);
        ini.readSymbol("=");
        int cw = (int)ini.readNumber();
        out.writeInt16(cw);
    }

    // Load provided source image
    shared_ptr<Image> image = Image::fromFile(infileBase + ".tga");
    debugAssert(isPow2(image->width()));
    debugAssert(isPow2(image->height()));
    image->convertToR8();

    // Convert to image buffer so can access bytes directly
    shared_ptr<PixelTransferBuffer> imageBuffer = image->toPixelTransferBuffer();
    debugAssert(imageBuffer->format() == ImageFormat::R8());

    // Autodetect baseline from capital E
    const uint8* p        = static_cast<const uint8*>(imageBuffer->mapRead());
    {
        // Size of a character, in texels
        int          w        = imageBuffer->width() / 16;

        int          x0       = ('E' % 16) * w;
        int          y0       = ('E' / 16) * w;
        
        int          baseline = w * 2 / 3;

        bool         done     = false;
    
        // Search up from the bottom for the first pixel
        for (int y = y0 + w - 1; (y >= y0) && ! done; --y) {
            for (int x = x0; (x < x0 + w) && ! done; ++x) {
                if (p[x + y * w * 16] != 0) {
                    baseline = y - y0 + 1;
                    done = true;
                }
            }
        }

        out.writeUInt16(baseline);
    }

    // Texture width
    out.writeUInt16(imageBuffer->width());
    out.writeBytes(p, square(imageBuffer->width()) * 256 / charsetSize);
    imageBuffer->unmap();
 
    out.compress();
    out.commit(false);
}

} // G3D
