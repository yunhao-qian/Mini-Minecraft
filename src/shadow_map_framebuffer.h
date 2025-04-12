#ifndef SHADOW_MAP_FRAMEBUFFER_H
#define SHADOW_MAP_FRAMEBUFFER_H

#include "opengl_context.h"
#include <OpenGL/glext.h>

namespace minecraft {

class ShadowMapFramebuffer
{
public:
    ShadowMapFramebuffer(OpenGLContext *const context);
    ShadowMapFramebuffer(const ShadowMapFramebuffer &) = delete;
    ShadowMapFramebuffer(ShadowMapFramebuffer &&) = delete;

    ~ShadowMapFramebuffer();

    ShadowMapFramebuffer &operator=(const ShadowMapFramebuffer &) = delete;
    ShadowMapFramebuffer &operator=(ShadowMapFramebuffer &&) = delete;

    int width() const;
    int height() const;
    void resizeViewport(const int width, const int height);

    GLuint depthTexture() const;

    void bind(const int cascadeIndex);

private:
    void releaseResources();

    void setTextureLayer(const int cascadeIndex);

    OpenGLContext *_context;
    int _width;
    int _height;
    GLuint _fbo;
    GLuint _depthTexture;
    GLuint _depthRenderbuffer;
};

inline ShadowMapFramebuffer::ShadowMapFramebuffer(OpenGLContext *const context)
    : _context{context}
    , _width{0}
    , _height{0}
    , _fbo{0u}
    , _depthTexture{0u}
    , _depthRenderbuffer{0u}
{}

inline ShadowMapFramebuffer::~ShadowMapFramebuffer()
{
    releaseResources();
}

inline int ShadowMapFramebuffer::width() const
{
    return _width;
}

inline int ShadowMapFramebuffer::height() const
{
    return _height;
}

inline GLuint ShadowMapFramebuffer::depthTexture() const
{
    return _depthTexture;
}

inline void ShadowMapFramebuffer::bind(const int cascadeIndex)
{
    _context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _context->debugError();
    setTextureLayer(cascadeIndex);
    _context->glViewport(0, 0, _width, _height);
    _context->debugError();
}

} // namespace minecraft

#endif // SHADOW_MAP_FRAMEBUFFER_H
