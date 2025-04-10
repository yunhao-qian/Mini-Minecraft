#include "shader_program.h"

#include <QByteArray>
#include <QFile>

#include <utility>

namespace minecraft {

void ShaderProgram::create(const std::vector<QString> &vertexShaderFileNames,
                           const std::vector<QString> &fragmentShaderFileNames,
                           const std::vector<QString> &uniformNames)
{
    const auto vertexShader{_context->glCreateShader(GL_VERTEX_SHADER)};
    _context->debugError();
    if (vertexShader == 0u) {
        qFatal() << "Failed to create vertex shader";
    }

    const auto fragmentShader{_context->glCreateShader(GL_FRAGMENT_SHADER)};
    _context->debugError();
    if (fragmentShader == 0u) {
        qFatal() << "Failed to create fragment shader";
    }

    _program = _context->glCreateProgram();
    _context->debugError();
    if (_program == 0u) {
        qFatal() << "Failed to create shader program";
    }

    compileShader(vertexShader, vertexShaderFileNames);
    compileShader(fragmentShader, fragmentShaderFileNames);

    _context->glAttachShader(_program, vertexShader);
    _context->debugError();
    _context->glAttachShader(_program, fragmentShader);
    _context->debugError();
    _context->glLinkProgram(_program);
    _context->debugError();

    GLint linkStatus{GL_FALSE};
    _context->glGetProgramiv(_program, GL_LINK_STATUS, &linkStatus);
    _context->debugError();
    if (linkStatus == GL_FALSE) {
        // Linking failed. Print the information log if available.

        GLint infoLogLength{0};
        _context->glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infoLogLength);
        _context->debugError();
        if (infoLogLength <= 0) {
            qFatal() << "Failed to link shader program, but no information log is available";
        }

        std::vector<GLchar> infoLog(infoLogLength);
        _context->glGetProgramInfoLog(_program, infoLogLength, nullptr, infoLog.data());
        _context->debugError();
        qFatal().noquote().nospace() << "Failed to link shader program:\n" << infoLog.data();
    }

    // Delete the shaders as they are no longer needed.
    _context->glDeleteShader(vertexShader);
    _context->debugError();
    _context->glDeleteShader(fragmentShader);
    _context->debugError();

    // Get uniform locations.
    _uniformLocations.reserve(uniformNames.size());
    for (const auto &name : uniformNames) {
        const auto nameBytes{name.toUtf8()};
        const auto location{_context->glGetUniformLocation(_program, nameBytes.data())};
        _context->debugError();
        if (location < 0) {
            qWarning() << "Failed to get location for uniform" << name;
            continue;
        }
        _uniformLocations.emplace(name, static_cast<GLuint>(location));
    }
}

void ShaderProgram::compileShader(const GLuint shader, const std::vector<QString> &fileNames) const
{
    // The source code are in multiple strings, starting with the version string, followed by one
    // string per .glsl file.

    std::vector<const GLchar *> strings;
    strings.reserve(fileNames.size() + 1u);
    // Add the version string first.
    strings.push_back("#version 330 core\n");

    std::vector<QByteArray> stringData;
    stringData.reserve(fileNames.size());

    for (const auto &fileName : fileNames) {
        QFile file{fileName};
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qFatal() << "Failed to open shader file" << fileName;
        }
        auto fileData{file.readAll()};
        if (fileData.isEmpty()) {
            qFatal() << "Shader file" << fileName << "is empty or could not be read";
        }
        const auto &movedFileData{stringData.emplace_back(std::move(fileData))};
        strings.push_back(movedFileData.constData());
    }

    _context->glShaderSource(shader, static_cast<GLsizei>(strings.size()), strings.data(), nullptr);
    _context->debugError();
    _context->glCompileShader(shader);
    _context->debugError();

    GLint compileStatus{GL_FALSE};
    _context->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    _context->debugError();
    if (compileStatus == GL_TRUE) {
        return;
    }

    // Compilation failed. Print the information log if available.

    GLint infoLogLength{0};
    _context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    _context->debugError();
    if (infoLogLength <= 0) {
        qFatal() << "Failed to compile shader, but no information log is available";
    }

    std::vector<GLchar> infoLog(infoLogLength);
    _context->glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog.data());
    _context->debugError();
    qFatal().noquote().nospace() << "Failed to compile shader:\n" << infoLog.data();
}

} // namespace minecraft
