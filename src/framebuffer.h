#ifndef MINI_MINECRAFT_FRAMEBUFFER_H
#define MINI_MINECRAFT_FRAMEBUFFER_H

#include "gl_context.h"

namespace minecraft {

class Framebuffer
{
public:
    Framebuffer(GLContext *const context);
    ~Framebuffer();

    auto width() const -> int;
    auto height() const -> int;
    auto setSize(const int width, const int height) -> void;

    auto fbo() const -> GLuint;
    auto colorTexture() const -> GLuint;
    auto depthTexture() const -> GLuint;

    auto bind() -> void;

private:
    auto destroy() -> void;

    GLContext *_context;
    int _width;
    int _height;
    GLuint _fbo;
    GLuint _colorTexture;
    GLuint _depthTexture;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_FRAMEBUFFER_H
