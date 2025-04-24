#include "geometry_framebuffer.h"

namespace minecraft {

void GeometryFramebuffer::resize(const int width, const int height)
{
    if (_width == width && _height == height) {
        return;
    }
    _width = width;
    _height = height;
    _depthTexture.reset();
    _normalTexture.reset();
    _albedoTexture.reset();
    _depthRenderbuffer.reset();

    const auto context{OpenGLContext::instance()};

    // Create the FBO if it does not exist yet.
    if (!_fbo) {
        GLuint fbo{0u};
        context->glGenFramebuffers(1, &fbo);
        context->checkError();
        _fbo = OpenGLObject{
            fbo,
            [](OpenGLContext *const context, const GLuint fbo) {
                context->glDeleteFramebuffers(1, &fbo);
            },
        };
    }
    context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo.get());
    context->checkError();

    {
        const auto textureDeleter{[](OpenGLContext *const context, const GLuint texture) {
            context->glDeleteTextures(1, &texture);
        }};
        // Use higher precisions for the depth and normal textures.
        // Use float32 for the depth texture because we may need to recover accurate world-space
        // positions from the depth values. This saves the need for a separate position texture.
        _depthTexture = OpenGLObject{
            generateAndAttachTexture(GL_R32F, GL_RED, GL_FLOAT, GL_COLOR_ATTACHMENT0),
            textureDeleter,
        };
        _normalTexture = OpenGLObject{
            generateAndAttachTexture(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, GL_COLOR_ATTACHMENT1),
            textureDeleter,
        };
        _albedoTexture = OpenGLObject{
            generateAndAttachTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT2),
            textureDeleter,
        };
    }

    {
        const GLenum drawBuffers[3]{GL_COLOR_ATTACHMENT0,
                                    GL_COLOR_ATTACHMENT1,
                                    GL_COLOR_ATTACHMENT2};
        context->glDrawBuffers(3, drawBuffers);
        context->checkError();
    }

    {
        GLuint renderbuffer{0u};
        context->glGenRenderbuffers(1, &renderbuffer);
        context->checkError();
        _depthRenderbuffer = OpenGLObject{
            renderbuffer,
            [](OpenGLContext *const context, const GLuint renderbuffer) {
                context->glDeleteRenderbuffers(1, &renderbuffer);
            },
        };
    }
    context->glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderbuffer.get());
    context->checkError();
    context->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);
    context->checkError();
    context->glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                       GL_DEPTH_ATTACHMENT,
                                       GL_RENDERBUFFER,
                                       _depthRenderbuffer.get());
    context->checkError();

    {
        const auto status{context->glCheckFramebufferStatus(GL_FRAMEBUFFER)};
        context->checkError();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            qFatal() << "Failed to initialize framebuffer";
        }
    }

    // The GeometryFramebuffer class does not know the default framebuffer's ID, as it is not zero in Qt.
    // Users are responsible for recovering the framebuffer if needed.
}

GLuint GeometryFramebuffer::generateAndAttachTexture(const GLint internalFormat,
                                                     const GLenum format,
                                                     const GLenum type,
                                                     const GLenum attachment)
{
    const auto context{OpenGLContext::instance()};

    GLuint texture{0u};
    context->glGenTextures(1, &texture);
    context->checkError();
    context->glBindTexture(GL_TEXTURE_2D, texture);
    context->checkError();
    context
        ->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _width, _height, 0, format, type, nullptr);
    context->checkError();

    // Use GL_NEAREST because interpolating attributes of neighboring pixels belonging to different
    // objects is not meaningful and may cause artifacts.
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    context->checkError();
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    context->checkError();

    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    context->checkError();
    context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    context->checkError();

    context->glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
    context->checkError();

    return texture;
}

} // namespace minecraft
