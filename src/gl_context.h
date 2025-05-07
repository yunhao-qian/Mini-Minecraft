#ifndef MINI_MINECRAFT_GL_CONTEXT_H
#define MINI_MINECRAFT_GL_CONTEXT_H

#include <QOpenGLFunctions_4_1_Core>

#include <source_location>

namespace minecraft {

class GLContext : public QOpenGLFunctions_4_1_Core
{
public:
    auto debugGLError(const std::source_location location = std::source_location::current())
        -> void;

    auto debugShaderInfoLog(const GLuint shader) -> void;
    auto debugProgramInfoLog(const GLuint program) -> void;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_GL_CONTEXT_H
