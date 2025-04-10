#ifndef MINI_MINECRAFT_SHADER_PROGRAM_H
#define MINI_MINECRAFT_SHADER_PROGRAM_H

#include "opengl_context.h"

#include <glm/glm.hpp>

#include <QString>

#include <type_traits>
#include <unordered_map>
#include <vector>

namespace minecraft {

class ShaderProgram
{
public:
    ShaderProgram(OpenGLContext *const context);
    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram(ShaderProgram &&) = delete;

    ~ShaderProgram();

    ShaderProgram &operator=(const ShaderProgram &) = delete;
    ShaderProgram &operator=(ShaderProgram &&) = delete;

    void create(const std::vector<QString> &vertexShaderFileNames,
                const std::vector<QString> &fragmentShaderFileNames,
                const std::vector<QString> &uniformNames);

    void use() const;

    template<typename T>
    void setUniform(const QString &name, const T &value) const;

private:
    template<typename T>
    static constexpr auto DependentFalse{false};

    void compileShader(const GLuint shader, const std::vector<QString> &fileNames) const;

    OpenGLContext *_context;
    GLuint _program;
    std::unordered_map<QString, GLuint> _uniformLocations;
};

inline ShaderProgram::ShaderProgram(OpenGLContext *const context)
    : _context{context}
    , _program{0u}
    , _uniformLocations{}
{}

inline ShaderProgram::~ShaderProgram()
{
    _context->glDeleteProgram(_program);
    _context->debugError();
}

inline void ShaderProgram::use() const
{
    _context->glUseProgram(_program);
    _context->debugError();
}

template<typename T>
void ShaderProgram::setUniform(const QString &name, const T &value) const
{
    const auto it{_uniformLocations.find(name)};
    if (it == _uniformLocations.end()) {
        qWarning() << "Uniform" << name << "not found in shader program";
        return;
    }
    const auto location{it->second};
    if constexpr (std::is_same_v<T, GLint>) {
        _context->glUniform1i(location, value);
    } else if constexpr (std::is_same_v<T, GLfloat>) {
        _context->glUniform1f(location, value);
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
        _context->glUniform2f(location, value[0], value[1]);
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
        _context->glUniform3f(location, value[0], value[1], value[2]);
    } else if constexpr (std::is_same_v<T, glm::mat4>) {
        _context->glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    } else {
        // Workaround for static_assert(false):
        // https://en.cppreference.com/w/cpp/language/static_assert
        static_assert(DependentFalse<T>, "Unsupported uniform type");
    }
    _context->debugError();
}

} // namespace minecraft

#endif // MINI_MINECRAFT_SHADER_PROGRAM_H
