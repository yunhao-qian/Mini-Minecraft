#include "framebuffer.h"

minecraft::Framebuffer::Framebuffer(GLContext *const context)
    : _context{context}
    , _width{0}
    , _height{0}
    , _fbo{0u}
    , _colorTexture{0u}
    , _depthTexture{0u}
{}

minecraft::Framebuffer::~Framebuffer()
{
    destroy();
}

auto minecraft::Framebuffer::width() const -> int
{
    return _width;
}

auto minecraft::Framebuffer::height() const -> int
{
    return _height;
}

auto minecraft::Framebuffer::colorTexture() const -> GLuint
{
    return _colorTexture;
}

auto minecraft::Framebuffer::depthTexture() const -> GLuint
{
    return _depthTexture;
}

auto minecraft::Framebuffer::setSize(const int width, const int height) -> void
{
    if (_width == width && _height == height) {
        return;
    }
    destroy();
    _width = width;
    _height = height;

    _context->glGenFramebuffers(1, &_fbo);
    _context->debugGLError();
    _context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _context->debugGLError();

    _context->glGenTextures(1, &_colorTexture);
    _context->debugGLError();
    _context->glBindTexture(GL_TEXTURE_2D, _colorTexture);
    _context->debugGLError();
    _context->glTexImage2D(GL_TEXTURE_2D,
                           0,
                           GL_RGBA8,
                           width,
                           height,
                           0,
                           GL_RGBA,
                           GL_UNSIGNED_BYTE,
                           nullptr);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _context->debugGLError();
    _context->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                     GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D,
                                     _colorTexture,
                                     0);
    _context->debugGLError();
    {
        GLenum drawBuffers[1]{GL_COLOR_ATTACHMENT0};
        _context->glDrawBuffers(1, drawBuffers);
        _context->debugGLError();
    }

    _context->glGenTextures(1, &_depthTexture);
    _context->debugGLError();
    _context->glBindTexture(GL_TEXTURE_2D, _depthTexture);
    _context->debugGLError();
    _context->glTexImage2D(GL_TEXTURE_2D,
                           0,
                           GL_DEPTH_COMPONENT24,
                           width,
                           height,
                           0,
                           GL_DEPTH_COMPONENT,
                           GL_FLOAT,
                           nullptr);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _context->debugGLError();
    _context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _context->debugGLError();
    _context->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                     GL_DEPTH_ATTACHMENT,
                                     GL_TEXTURE_2D,
                                     _depthTexture,
                                     0);
    _context->debugGLError();

    const auto status{_context->glCheckFramebufferStatus(GL_FRAMEBUFFER)};
    _context->debugGLError();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        qFatal() << "Failed to initialize framebuffer";
    }

    _context->glBindFramebuffer(GL_FRAMEBUFFER, 0u);
    _context->debugGLError();
}

auto minecraft::Framebuffer::fbo() const -> GLuint
{
    return _fbo;
}

auto minecraft::Framebuffer::bind() -> void
{
    _context->glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    _context->debugGLError();
}

auto minecraft::Framebuffer::destroy() -> void
{
    if (_colorTexture != 0u) {
        _context->glDeleteTextures(1, &_colorTexture);
        _colorTexture = 0u;
    }
    if (_depthTexture != 0u) {
        _context->glDeleteTextures(1, &_depthTexture);
        _depthTexture = 0u;
    }
    if (_fbo != 0u) {
        _context->glDeleteFramebuffers(1, &_fbo);
        _fbo = 0u;
    }
}
