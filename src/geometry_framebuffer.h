#ifndef MINECRAFT_GEOMETRY_FRAMEBUFFER_H
#define MINECRAFT_GEOMETRY_FRAMEBUFFER_H

#include "opengl_context.h"
#include "opengl_object.h"

namespace minecraft {

class GeometryFramebuffer
{
public:
    GeometryFramebuffer()
        : _width{0}
        , _height{0}
        , _fbo{}
        , _depthTexture{}
        , _normalTexture{}
        , _albedoTexture{}
        , _depthRenderbuffer{}
    {}

    GeometryFramebuffer(const GeometryFramebuffer &) = delete;
    GeometryFramebuffer(GeometryFramebuffer &&) = delete;

    GeometryFramebuffer &operator=(const GeometryFramebuffer &) = delete;
    GeometryFramebuffer &operator=(GeometryFramebuffer &&) = delete;

    int width() const { return _width; }

    int height() const { return _height; }

    void resize(const int width, const int height);

    GLuint depthTexture() const { return _depthTexture.get(); }

    GLuint normalTexture() const { return _normalTexture.get(); }

    GLuint albedoTexture() const { return _albedoTexture.get(); }

    void bind()
    {
        const auto context{OpenGLContext::instance()};
        context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo.get());
        context->checkError();
        context->glViewport(0, 0, _width, _height);
        context->checkError();
    }

private:
    GLuint generateAndAttachTexture(const GLint internalFormat,
                                    const GLenum format,
                                    const GLenum type,
                                    const GLenum attachment);

    int _width;
    int _height;
    OpenGLObject _fbo;
    OpenGLObject _depthTexture;
    OpenGLObject _normalTexture;
    OpenGLObject _albedoTexture;
    OpenGLObject _depthRenderbuffer;
};

} // namespace minecraft

#endif // MINECRAFT_GEOMETRY_FRAMEBUFFER_H
