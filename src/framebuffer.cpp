#include "framebuffer.h"

#include <initializer_list>

namespace minecraft {

void Framebuffer::resizeViewport(const int width, const int height)
{
    if (width == _width && height == _height) {
        return;
    }
    releaseResources();
    _width = width;
    _height = height;

    _context->glGenFramebuffers(1, &_fbo);
    _context->debugError();
    _context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _context->debugError();

    // Use higher precisions for the normal and depth textures.
    if (!_depthOnly) {
        _normalTexture = generateAndAttachTexture(GL_RGBA16F,
                                                  GL_RGBA,
                                                  GL_HALF_FLOAT,
                                                  GL_COLOR_ATTACHMENT0);
        _albedoTexture = generateAndAttachTexture(GL_RGBA8,
                                                  GL_RGBA,
                                                  GL_UNSIGNED_BYTE,
                                                  GL_COLOR_ATTACHMENT1);
        const GLenum drawBuffers[2]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        _context->glDrawBuffers(2, drawBuffers);
        _context->debugError();
    }

    // Use float32 for the depth texture because we may need to recover accurate world-space
    // positions from the depth values. This saves the need for a separate position texture.
    _depthTexture = generateAndAttachTexture(GL_DEPTH_COMPONENT32F,
                                             GL_DEPTH_COMPONENT,
                                             GL_FLOAT,
                                             GL_DEPTH_ATTACHMENT);

    {
        const auto status{_context->glCheckFramebufferStatus(GL_FRAMEBUFFER)};
        _context->debugError();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            qFatal() << "Failed to initialize framebuffer";
        }
    }

    // The Framebuffer class does not know the default framebuffer's ID, as it is not zero in Qt.
    // Users are responsible for recovering the framebuffer if needed.
}

void Framebuffer::releaseResources()
{
    _width = 0;
    _height = 0;

    _context->glDeleteFramebuffers(1, &_fbo);
    _context->debugError();
    _fbo = 0u;

    for (const auto texture : {&_normalTexture, &_albedoTexture, &_depthTexture}) {
        _context->glDeleteTextures(1, texture);
        _context->debugError();
        *texture = 0u;
    }
}

GLuint Framebuffer::generateAndAttachTexture(const GLint internalFormat,
                                             const GLenum format,
                                             const GLenum type,
                                             const GLenum attachment)
{
    GLuint texture;
    _context->glGenTextures(1, &texture);
    _context->debugError();
    _context->glBindTexture(GL_TEXTURE_2D, texture);
    _context->debugError();
    _context
        ->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _width, _height, 0, format, type, nullptr);
    _context->debugError();

    // All passes are pixel-to-pixel aligned, so there is no aliasing issue.
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _context->debugError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _context->debugError();

    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _context->debugError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _context->debugError();

    _context->glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
    _context->debugError();

    return texture;
}

} // namespace minecraft
