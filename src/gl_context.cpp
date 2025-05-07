#include "gl_context.h"

#include <QString>

#include <vector>

auto minecraft::GLContext::debugGLError(const std::source_location location) -> void
{
    const auto error{glGetError()};
    if (error == GL_NO_ERROR) {
        return;
    }
    QString errorString;

#define HANDLE_ERROR_CASE(x) \
    case x: \
        errorString = #x; \
        break

    switch (error) {
        HANDLE_ERROR_CASE(GL_INVALID_ENUM);
        HANDLE_ERROR_CASE(GL_INVALID_VALUE);
        HANDLE_ERROR_CASE(GL_INVALID_OPERATION);
        HANDLE_ERROR_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
        HANDLE_ERROR_CASE(GL_OUT_OF_MEMORY);
        HANDLE_ERROR_CASE(GL_STACK_UNDERFLOW);
        HANDLE_ERROR_CASE(GL_STACK_OVERFLOW);
    default:
        errorString = QString{"Unknown error (%1)"}.arg(error);
    }

#undef HANDLE_ERROR_CASE

    qDebug().noquote().nospace() << "OpenGL error at " << location.file_name() << ":"
                                 << location.line() << ": " << errorString;
}

auto minecraft::GLContext::debugShaderInfoLog(const GLuint shader) -> void
{
    GLint infoLogLength{0};
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    debugGLError();
    if (infoLogLength <= 0) {
        return;
    }
    std::vector<GLchar> infoLog(infoLogLength);
    glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
    debugGLError();
    qDebug().noquote().nospace() << "Shader info log:\n" << infoLog.data();
}

auto minecraft::GLContext::debugProgramInfoLog(const GLuint program) -> void
{
    GLint infoLogLength{0};
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    debugGLError();
    if (infoLogLength <= 0) {
        return;
    }
    std::vector<GLchar> infoLog(infoLogLength);
    glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
    debugGLError();
    qDebug().noquote().nospace() << "Program info log:\n" << infoLog.data();
}
