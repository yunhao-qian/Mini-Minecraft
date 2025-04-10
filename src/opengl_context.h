#ifndef MINI_MINECRAFT_OPENGL_CONTEXT_H
#define MINI_MINECRAFT_OPENGL_CONTEXT_H

#include <QOpenGLFunctions_4_1_Core>
#include <QString>

#include <source_location>

namespace minecraft {

class OpenGLContext : public QOpenGLFunctions_4_1_Core
{
public:
    void debugError(const std::source_location location = std::source_location::current());
};

#ifdef MINI_MINECRAFT_OPENGL_NO_DEBUG_ERROR

inline void OpenGLContext::debugError([[maybe_unused]] std::source_location location) {}

#else

inline void OpenGLContext::debugError(const std::source_location location)
{
    const auto error{glGetError()};
    if (error == GL_NO_ERROR) {
        return;
    }

    QString errorString;

#define MINI_MINECRAFT_HANDLE_ERROR_CASE(x) \
    case x: \
        errorString = #x; \
        break

    // List of possible error flags:
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetError.xhtml
    switch (error) {
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_ENUM);
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_VALUE);
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_OPERATION);
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_OUT_OF_MEMORY);
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_STACK_UNDERFLOW);
        MINI_MINECRAFT_HANDLE_ERROR_CASE(GL_STACK_OVERFLOW);
    default:
        errorString = QString{"Unknown error (%1)"}.arg(error);
    }

#undef MINI_MINECRAFT_HANDLE_ERROR_CASE

    qDebug().noquote().nospace() << "OpenGL error at " << location.file_name() << ":"
                                 << location.line() << ": " << errorString;
}

#endif // MINI_MINECRAFT_OPENGL_NO_DEBUG_ERROR

} // namespace minecraft

#endif // MINI_MINECRAFT_OPENGL_CONTEXT_H
