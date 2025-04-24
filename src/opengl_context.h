#ifndef MINECRAFT_OPENGL_CONTEXT_H
#define MINECRAFT_OPENGL_CONTEXT_H

#include <QOpenGLFunctions_4_1_Core>
#include <QString>

#include <source_location>

namespace minecraft {

class OpenGLContext : public QOpenGLFunctions_4_1_Core
{
public:
    void checkError(const std::source_location location = std::source_location::current());

    static OpenGLContext *instance() { return _instance; }

private:
    friend class OpenGLWidget;

    static OpenGLContext *_instance;
};

#ifdef MINECRAFT_NO_GL_ERROR_CHECK

inline void OpenGLContext::checkError([[maybe_unused]] std::source_location location) {}

#else

inline void OpenGLContext::checkError(const std::source_location location)
{
    const auto error{glGetError()};
    if (error == GL_NO_ERROR) {
        return;
    }

    QString errorString;

#define MINECRAFT_HANDLE_ERROR_CASE(x) \
    case x: \
        errorString = #x; \
        break

    // List of possible error flags:
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetError.xhtml
    switch (error) {
        MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_ENUM);
        MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_VALUE);
        MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_OPERATION);
        MINECRAFT_HANDLE_ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
        MINECRAFT_HANDLE_ERROR_CASE(GL_OUT_OF_MEMORY);
        // GL_STACK_UNDERFLOW and GL_STACK_OVERFLOW no longer exist in OpenGL 4.1.
    default:
        errorString = QString{"Unknown error (%1)"}.arg(error);
    }

#undef MINECRAFT_HANDLE_ERROR_CASE

    qFatal().noquote().nospace() << "OpenGL error at " << location.file_name() << ":"
                                 << location.line() << ": " << errorString;
}

#endif // MINECRAFT_NO_GL_ERROR_CHECK

} // namespace minecraft

#endif // MINECRAFT_OPENGL_CONTEXT_H
