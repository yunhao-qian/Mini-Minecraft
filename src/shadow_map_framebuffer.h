#ifndef SHADOW_MAP_FRAMEBUFFER_H
#define SHADOW_MAP_FRAMEBUFFER_H

#include "opengl_context.h"
#include "opengl_object.h"

namespace minecraft {

class ShadowMapFramebuffer
{
public:
    ShadowMapFramebuffer()
        : _width{0}
        , _height{0}
        , _fbo{}
        , _depthTexture{}
        , _depthRenderbuffer{}
    {}

    ShadowMapFramebuffer(const ShadowMapFramebuffer &) = delete;
    ShadowMapFramebuffer(ShadowMapFramebuffer &&) = delete;

    ShadowMapFramebuffer &operator=(const ShadowMapFramebuffer &) = delete;
    ShadowMapFramebuffer &operator=(ShadowMapFramebuffer &&) = delete;

    int width() const { return _width; }

    int height() const { return _height; }

    void resize(const int width, const int height);

    GLuint depthTexture() const { return _depthTexture.get(); }

    void bind(const int cascadeIndex) const
    {
        const auto &context{OpenGLContext::instance()};

        context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo.get());
        context->checkError();
        setTextureLayer(cascadeIndex);
        context->glViewport(0, 0, _width, _height);
        context->checkError();
    }

private:
    void setTextureLayer(const int cascadeIndex) const;

    int _width;
    int _height;
    OpenGLObject _fbo;
    OpenGLObject _depthTexture;
    OpenGLObject _depthRenderbuffer;
};

} // namespace minecraft

#endif // SHADOW_MAP_FRAMEBUFFER_H
