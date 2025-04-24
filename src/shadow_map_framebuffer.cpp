#include "shadow_map_framebuffer.h"

#include "shadow_map_camera.h"

namespace minecraft {

void ShadowMapFramebuffer::resizeViewport(const int width, const int height)
{
    if (_width == width && _height == height) {
        return;
    }
    _width = width;
    _height = height;
    _depthTexture.reset();
    _depthRenderbuffer.reset();

    const auto &context{OpenGLContext::instance()};

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
        GLuint texture{0u};
        context->glGenTextures(1, &texture);
        context->checkError();
        _depthTexture = OpenGLObject{
            texture,
            [](OpenGLContext *const context, const GLuint texture) {
                context->glDeleteTextures(1, &texture);
            },
        };
    }
    context->glBindTexture(GL_TEXTURE_2D_ARRAY, _depthTexture.get());
    context->checkError();
    context->glTexImage3D(GL_TEXTURE_2D_ARRAY,
                          0,
                          GL_RG32F,
                          width,
                          height,
                          ShadowMapCamera::CascadeCount,
                          0,
                          GL_RG,
                          GL_FLOAT,
                          nullptr);
    context->checkError();

    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    context->checkError();
    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    context->checkError();

    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    context->checkError();
    context->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    context->checkError();

    setTextureLayer(0);

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
    context->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
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
}

void ShadowMapFramebuffer::setTextureLayer(const int cascadeIndex) const
{
    const auto &context{OpenGLContext::instance()};

    context->glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                       GL_COLOR_ATTACHMENT0,
                                       _depthTexture.get(),
                                       0,
                                       cascadeIndex);
    context->checkError();
    {
        const GLenum drawBuffers[1]{GL_COLOR_ATTACHMENT0};
        context->glDrawBuffers(1, drawBuffers);
        context->checkError();
    }
}

} // namespace minecraft
