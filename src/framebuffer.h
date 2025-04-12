#ifndef MINI_MINECRAFT_FRAMEBUFFER_H
#define MINI_MINECRAFT_FRAMEBUFFER_H

#include "opengl_context.h"

namespace minecraft {

class Framebuffer
{
public:
    Framebuffer(OpenGLContext *const context);
    Framebuffer(const Framebuffer &) = delete;
    Framebuffer(Framebuffer &&) = delete;

    ~Framebuffer();

    Framebuffer &operator=(const Framebuffer &) = delete;
    Framebuffer &operator=(Framebuffer &&) = delete;

    int width() const;
    int height() const;
    void resizeViewport(const int width, const int height);

    GLuint depthTexture() const;
    GLuint normalTexture() const;
    GLuint albedoTexture() const;

    void bind();

private:
    void releaseResources();

    GLuint generateAndAttachTexture(const GLint internalFormat,
                                    const GLenum format,
                                    const GLenum type,
                                    const GLenum attachment);

    OpenGLContext *_context;
    int _width;
    int _height;
    GLuint _fbo;
    GLuint _depthTexture;
    GLuint _normalTexture;
    GLuint _albedoTexture;
    GLuint _depthRenderbuffer;
};

inline Framebuffer::Framebuffer(OpenGLContext *const context)
    : _context{context}
    , _width{0}
    , _height{0}
    , _fbo{0u}
    , _depthTexture{0u}
    , _normalTexture{0u}
    , _albedoTexture{0u}
    , _depthRenderbuffer{0u}
{}

inline Framebuffer::~Framebuffer()
{
    releaseResources();
}

inline int Framebuffer::width() const
{
    return _width;
}

inline int Framebuffer::height() const
{
    return _height;
}

inline GLuint Framebuffer::depthTexture() const
{
    return _depthTexture;
}

inline GLuint Framebuffer::normalTexture() const
{
    return _normalTexture;
}

inline GLuint Framebuffer::albedoTexture() const
{
    return _albedoTexture;
}

inline void Framebuffer::bind()
{
    _context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _context->debugError();
    _context->glViewport(0, 0, _width, _height);
    _context->debugError();
}

} // namespace minecraft

#endif // MINI_MINECRAFT_FRAMEBUFFER_H
