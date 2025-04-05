#ifndef MINI_MINECRAFT_SHADER_PROGRAM_H
#define MINI_MINECRAFT_SHADER_PROGRAM_H

#include "gl_context.h"

#include <glm/glm.hpp>

#include <QString>

#include <optional>
#include <utility>
#include <vector>

namespace minecraft {

class ShaderProgram
{
public:
    ShaderProgram(GLContext *const context);
    ~ShaderProgram();

    auto create(const QString &vertexShaderPath,
                const QString &fragmentShaderPath,
                const std::vector<QString> &uniformNames) -> bool;

    auto useProgram() const -> void;

    auto setUniform(const QString &name, const GLint value) const -> void;
    auto setUniform(const QString &name, const GLfloat value) const -> void;
    auto setUniform(const QString &name, const glm::vec2 &value) const -> void;
    auto setUniform(const QString &name, const glm::vec3 &value) const -> void;
    auto setUniform(const QString &name, const glm::mat4 &value) const -> void;

private:
    auto compileShader(const GLuint shader, const QString &filePath) const -> bool;

    auto findUniformLocation(const QString &name) const -> std::optional<GLuint>;

    GLContext *_context;
    GLuint _vertexShader;
    GLuint _fragmentShader;
    GLuint _program;
    std::vector<std::pair<QString, GLuint>> _uniformLocations;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_SHADER_PROGRAM_H
