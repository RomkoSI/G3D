#include "GLG3D/GLPixelTransferBuffer.h"
#include "GLG3D/BufferTexture.h"
#include "G3D/ImageFormat.h"
#include "GLG3D/glcalls.h"
#include "GLG3D/Texture.h"
#include "GLG3D/GLCaps.h"
#include "G3D/g3dmath.h"

namespace G3D {


static bool isValidBufferTextureImageFormat(const ImageFormat* f) {
    // From http://www.opengl.org/sdk/docs/man/xhtml/glTexBuffer.xml
    return 
        // Valid 1-component formats
        f == ImageFormat::R8() ||
        f == ImageFormat::R16() ||
        f == ImageFormat::R16F() ||
        f == ImageFormat::R32F() ||
        f == ImageFormat::R8I() ||
        f == ImageFormat::R16I() ||
        f == ImageFormat::R32I() ||
        f == ImageFormat::R8UI() ||
        f == ImageFormat::R16UI() ||
        f == ImageFormat::R32UI() ||

        // Valid 2-component formats
        f == ImageFormat::RG8() ||
        f == ImageFormat::RG16() ||
        f == ImageFormat::RG16F() ||
        f == ImageFormat::RG32F() ||
        f == ImageFormat::RG8I() ||
        f == ImageFormat::RG16I() ||
        f == ImageFormat::RG32I() ||
        f == ImageFormat::RG8UI() ||
        f == ImageFormat::RG16UI() ||
        f == ImageFormat::RG32UI() ||

        // Valid 4-component formats
        f == ImageFormat::RGBA8() ||
        f == ImageFormat::RGBA16() ||
        f == ImageFormat::RGBA16F() ||
        f == ImageFormat::RGBA32F() ||
        f == ImageFormat::RGBA8I() ||
        f == ImageFormat::RGBA16I() ||
        f == ImageFormat::RGBA32I() ||
        f == ImageFormat::RGBA8UI() ||
        f == ImageFormat::RGBA16UI() ||
        f == ImageFormat::RGBA32UI() ||

        // Valid 3 component formats
        f == ImageFormat::RGB32F() ||
        f == ImageFormat::RGB32I() ||
        f == ImageFormat::RGB32UI();
}


GLenum BufferTexture::glslSamplerType() const {
    const ImageFormat* f = m_buffer->format();
    if (f->isIntegerFormat()) {
        if (f->openGLDataFormat == GL_UNSIGNED_BYTE ||
            f->openGLDataFormat == GL_UNSIGNED_SHORT ||
            f->openGLDataFormat == GL_UNSIGNED_INT) {
            return GL_UNSIGNED_INT_SAMPLER_BUFFER;
        } else {
            return GL_INT_SAMPLER_BUFFER;
        }
    } else {
        return GL_SAMPLER_BUFFER;
    }
}

BufferTexture::~BufferTexture() {
    glBindTexture(GL_TEXTURE_BUFFER, m_textureID);
    glTexBuffer(GL_TEXTURE_BUFFER, m_buffer->format()->openGLFormat, 0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glDeleteTextures(1, &m_textureID);
    m_textureID = 0;
}

BufferTexture::BufferTexture(const String& name, const shared_ptr<GLPixelTransferBuffer>& buffer, GLuint textureID) :
                m_textureID(textureID), m_buffer(buffer), m_name(name) {}


unsigned int BufferTexture::openGLTextureTarget() const {
    return GL_TEXTURE_BUFFER;
}

int BufferTexture::size() const {
    return iMin(m_buffer->pixelCount(), GLCaps::maxTextureBufferSize());
}

bool BufferTexture::someTexelsInaccessible() const {
    return (m_buffer->pixelCount() > GLCaps::maxTextureBufferSize());
}

const ImageFormat* BufferTexture::format() const {
    return m_buffer->format();
}

shared_ptr<BufferTexture> BufferTexture::create(const String& name, const shared_ptr<GLPixelTransferBuffer>& buffer) {
    debugAssertM(GLCaps::maxTextureBufferSize() > 0, "Buffer Textures not supported by your driver.");
    alwaysAssertM(isValidBufferTextureImageFormat(buffer->format()), G3D::format("Invalidly formatted buffer passed to BufferTexture::create(), format %s is unsupported.", buffer->format()->name().c_str()) );
    GLuint textureID = Texture::newGLTextureID();

    /** Load the data */
    glBindTexture(GL_TEXTURE_BUFFER, textureID);
    glTexBuffer(GL_TEXTURE_BUFFER, buffer->format()->openGLFormat, buffer->glBufferID());
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    return shared_ptr<BufferTexture>(new BufferTexture(name, buffer, textureID));
}

}
