#include "shadow_map_framebuffer.h"

#include "shadow_map_camera.h"

namespace minecraft {

void ShadowMapFramebuffer::resizeViewport(const int width, const int height)
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

    _context->glGenTextures(1, &_depthTexture);
    _context->debugError();
    _context->glBindTexture(GL_TEXTURE_2D_ARRAY, _depthTexture);
    _context->debugError();
    _context->glTexImage3D(GL_TEXTURE_2D_ARRAY,
                           0,
                           GL_R32F,
                           width,
                           height,
                           ShadowMapCamera::CascadeCount,
                           0,
                           GL_RED,
                           GL_FLOAT,
                           nullptr);
    _context->debugError();

    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _context->debugError();
    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    _context->debugError();

    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _context->debugError();
    _context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _context->debugError();

    setTextureLayer(0);

    _context->glGenRenderbuffers(1, &_depthRenderbuffer);
    _context->debugError();
    _context->glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderbuffer);
    _context->debugError();
    _context->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    _context->debugError();
    _context->glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                        GL_DEPTH_ATTACHMENT,
                                        GL_RENDERBUFFER,
                                        _depthRenderbuffer);
    _context->debugError();

    {
        const auto status{_context->glCheckFramebufferStatus(GL_FRAMEBUFFER)};
        _context->debugError();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            qFatal() << "Failed to initialize framebuffer";
        }
    }
}

void ShadowMapFramebuffer::releaseResources()
{
    _width = 0;
    _height = 0;

    _context->glDeleteFramebuffers(1, &_fbo);
    _context->debugError();
    _fbo = 0;

    _context->glDeleteTextures(1, &_depthTexture);
    _context->debugError();
    _depthTexture = 0;

    _context->glDeleteRenderbuffers(1, &_depthRenderbuffer);
    _context->debugError();
    _depthRenderbuffer = 0;
}

void ShadowMapFramebuffer::setTextureLayer(const int cascadeIndex) const
{
    _context->glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                        GL_COLOR_ATTACHMENT0,
                                        _depthTexture,
                                        0,
                                        cascadeIndex);
    _context->debugError();
    {
        const GLenum drawBuffers[1]{GL_COLOR_ATTACHMENT0};
        _context->glDrawBuffers(1, drawBuffers);
        _context->debugError();
    }
}

} // namespace minecraft
