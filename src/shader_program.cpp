#include "shader_program.h"

#include "scope_guard.h"

#include <QFile>

#include <vector>

namespace minecraft {

void ShaderProgram::create(const QString &vertexShaderFileName,
                           const QString &fragmentShaderFileName)
{
    const auto context{OpenGLContext::instance()};

    OpenGLObject vertexShader;
    OpenGLObject fragmentShader;
    {
        const auto shaderDeleter{[](OpenGLContext *const context, const GLuint shader) {
            context->glDeleteShader(shader);
        }};

        vertexShader = OpenGLObject{context->glCreateShader(GL_VERTEX_SHADER), shaderDeleter};
        context->checkError();
        if (!vertexShader) {
            qFatal() << "Failed to create vertex shader";
        }

        fragmentShader = OpenGLObject{context->glCreateShader(GL_FRAGMENT_SHADER), shaderDeleter};
        context->checkError();
        if (!fragmentShader) {
            qFatal() << "Failed to create fragment shader";
        }
    }

    _program = OpenGLObject{
        context->glCreateProgram(),
        [](OpenGLContext *const context, const GLuint program) {
            context->glDeleteProgram(program);
        },
    };
    context->checkError();
    if (!_program) {
        qFatal() << "Failed to create shader program";
    }

    compileShader(vertexShader.get(), vertexShaderFileName);
    compileShader(fragmentShader.get(), fragmentShaderFileName);

    context->glAttachShader(_program.get(), vertexShader.get());
    context->checkError();
    context->glAttachShader(_program.get(), fragmentShader.get());
    context->checkError();
    context->glLinkProgram(_program.get());
    context->checkError();

    const ScopeGuard guard{[context,
                            program{_program.get()},
                            vertexShader{vertexShader.get()},
                            fragmentShader{fragmentShader.get()}]() {
        context->glDetachShader(program, vertexShader);
        context->checkError();
        context->glDetachShader(program, fragmentShader);
        context->checkError();
    }};

    GLint linkStatus{GL_FALSE};
    context->glGetProgramiv(_program.get(), GL_LINK_STATUS, &linkStatus);
    context->checkError();
    if (linkStatus == GL_FALSE) {
        // Linking failed. Print the information log if available.

        GLint infoLogLength{0};
        context->glGetProgramiv(_program.get(), GL_INFO_LOG_LENGTH, &infoLogLength);
        context->checkError();
        if (infoLogLength <= 0) {
            qFatal() << "Failed to link shader program, but no information log is available";
        }

        std::vector<GLchar> infoLog(infoLogLength);
        context->glGetProgramInfoLog(_program.get(), infoLogLength, nullptr, infoLog.data());
        context->checkError();
        qFatal().noquote().nospace() << "Failed to link shader program:\n" << infoLog.data();
    }
}

void ShaderProgram::compileShader(const GLuint shader, const QString &fileName) const
{
    // The source code are in multiple strings, starting with the version string, followed by one
    // string per .glsl file.

    QFile file{fileName};
    if (!file.open(QFile::ReadOnly)) {
        qFatal() << "Failed to open shader file" << fileName;
    }
    const auto fileData{file.readAll()};
    if (fileData.isEmpty()) {
        qFatal() << "Shader file" << fileName << "is empty or could not be read";
    }

    const auto context{OpenGLContext::instance()};

    const auto sourceData{fileData.constData()};
    context->glShaderSource(shader, 1, &sourceData, nullptr);
    context->checkError();
    context->glCompileShader(shader);
    context->checkError();

    GLint compileStatus{GL_FALSE};
    context->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    context->checkError();
    if (compileStatus == GL_TRUE) {
        return;
    }

    // Compilation failed. Print the information log if available.

    GLint infoLogLength{0};
    context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    context->checkError();
    if (infoLogLength <= 0) {
        qFatal() << "Failed to compile shader, but no information log is available";
    }

    std::vector<GLchar> infoLog(infoLogLength);
    context->glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
    context->checkError();
    qFatal().noquote().nospace() << "Failed to compile shader:\n" << infoLog.data();
}

GLuint ShaderProgram::getUniformLocation(const QString &name)
{
    const auto it{_uniformLocations.find(name)};
    if (it != _uniformLocations.end()) {
        return it->second;
    }

    const auto context{OpenGLContext::instance()};

    const auto nameBytes{name.toUtf8()};
    const auto location{context->glGetUniformLocation(_program.get(), nameBytes.data())};
    context->checkError();
    if (location < 0) {
        qFatal() << "Failed to get location for uniform" << name;
        return 0u;
    }
    const auto locationUInt{static_cast<GLuint>(location)};
    _uniformLocations.emplace(name, locationUInt);
    return locationUInt;
}

} // namespace minecraft
