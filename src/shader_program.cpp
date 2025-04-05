#include "shader_program.h"

#include <QFile>
#include <QTextStream>

#include <algorithm>

minecraft::ShaderProgram::ShaderProgram(GLContext *const context)
    : _context{context}
    , _vertexShader{0u}
    , _fragmentShader{0u}
    , _program{0u}
    , _uniformLocations{}
{}

minecraft::ShaderProgram::~ShaderProgram()
{
    _context->glDeleteProgram(_program);
    _context->debugGLError();
    _context->glDeleteShader(_vertexShader);
    _context->debugGLError();
    _context->glDeleteShader(_fragmentShader);
    _context->debugGLError();
}

auto minecraft::ShaderProgram::create(const QString &vertexShaderPath,
                                      const QString &fragmentShaderPath,
                                      const std::vector<QString> &uniformNames) -> bool
{
    _vertexShader = _context->glCreateShader(GL_VERTEX_SHADER);
    _context->debugGLError();
    if (_vertexShader == 0u) {
        qWarning() << "Failed to create vertex shader";
        return false;
    }

    _fragmentShader = _context->glCreateShader(GL_FRAGMENT_SHADER);
    _context->debugGLError();
    if (_fragmentShader == 0u) {
        qWarning() << "Failed to create fragment shader";
        return false;
    }

    _program = _context->glCreateProgram();
    _context->debugGLError();
    if (_program == 0u) {
        qWarning() << "Failed to create shader program";
        return false;
    }

    if (!compileShader(_vertexShader, vertexShaderPath)
        || !compileShader(_fragmentShader, fragmentShaderPath)) {
        return false;
    }

    _context->glAttachShader(_program, _vertexShader);
    _context->debugGLError();
    _context->glAttachShader(_program, _fragmentShader);
    _context->debugGLError();
    _context->glLinkProgram(_program);
    _context->debugGLError();
    {
        GLint programLinked{GL_FALSE};
        _context->glGetProgramiv(_program, GL_LINK_STATUS, &programLinked);
        _context->debugGLError();
        if (programLinked == GL_FALSE) {
            qWarning() << "Failed to link shader program";
            _context->debugProgramInfoLog(_program);
            return false;
        }
    }

    _uniformLocations.reserve(uniformNames.size());
    for (const auto &name : uniformNames) {
        const auto nameBytes{name.toUtf8()};
        const auto location{_context->glGetUniformLocation(_program, nameBytes.data())};
        _context->debugGLError();
        if (location < 0) {
            qWarning() << "Failed to get location for uniform" << name;
            return false;
        }
        _uniformLocations.emplace_back(name, static_cast<GLuint>(location));
    }

    return true;
}

auto minecraft::ShaderProgram::useProgram() const -> void
{
    _context->glUseProgram(_program);
    _context->debugGLError();
}

auto minecraft::ShaderProgram::setUniform(const QString &name, const GLint value) const -> void
{
    useProgram();
    const auto location{findUniformLocation(name)};
    if (!location.has_value()) {
        return;
    }
    _context->glUniform1i(*location, value);
    _context->debugGLError();
}

auto minecraft::ShaderProgram::setUniform(const QString &name, const GLfloat value) const -> void
{
    useProgram();
    const auto location{findUniformLocation(name)};
    if (!location.has_value()) {
        return;
    }
    _context->glUniform1f(*location, value);
    _context->debugGLError();
}

auto minecraft::ShaderProgram::setUniform(const QString &name, const glm::mat4 &value) const -> void
{
    useProgram();
    const auto location{findUniformLocation(name)};
    if (!location.has_value()) {
        return;
    }
    _context->glUniformMatrix4fv(*location, 1, GL_FALSE, &value[0][0]);
    _context->debugGLError();
}

auto minecraft::ShaderProgram::compileShader(const GLuint shader, const QString &filePath) const
    -> bool
{
    {
        QFile file{filePath};
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << "Failed to open shader file" << filePath;
            return false;
        }
        QTextStream stream{&file};
        const auto shaderSourceBytes{stream.readAll().toUtf8()};
        const auto shaderSourceData{shaderSourceBytes.data()};
        _context->glShaderSource(shader, 1, &shaderSourceData, nullptr);
        _context->debugGLError();
    }
    _context->glCompileShader(shader);
    {
        GLint shaderCompiled{GL_FALSE};
        _context->glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
        _context->debugGLError();
        if (shaderCompiled == GL_FALSE) {
            qWarning() << "Failed to compile shader file" << filePath;
            _context->debugShaderInfoLog(shader);
            return false;
        }
    }
    return true;
}

auto minecraft::ShaderProgram::findUniformLocation(const QString &name) const
    -> std::optional<GLuint>
{
    const auto it{std::ranges::find_if(_uniformLocations,
                                       [&name](const auto &pair) { return pair.first == name; })};
    if (it == _uniformLocations.end()) {
        qWarning() << "Uniform" << name << "not found in shader program";
        return std::nullopt;
    }
    return it->second;
}
