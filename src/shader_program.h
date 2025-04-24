#ifndef MINECRAFT_SHADER_PROGRAM_H
#define MINECRAFT_SHADER_PROGRAM_H

#include "opengl_context.h"
#include "opengl_object.h"

#include <glm/glm.hpp>

#include <QString>

#include <type_traits>
#include <unordered_map>

namespace minecraft {

class ShaderProgram
{
public:
    ShaderProgram()
        : _program{}
        , _uniformLocations{}
    {}

    ShaderProgram(const ShaderProgram &) = delete;
    ShaderProgram(ShaderProgram &&) = delete;

    ShaderProgram &operator=(const ShaderProgram &) = delete;
    ShaderProgram &operator=(ShaderProgram &&) = delete;

    void create(const QString &vertexShaderFileName, const QString &fragmentShaderFileName);

    void use() const
    {
        const auto context{OpenGLContext::instance()};
        context->glUseProgram(_program.get());
        context->checkError();
    }

    template<typename T>
    void setUniform(const QString &name, const T &value)
    {
        const auto context{OpenGLContext::instance()};

        const auto location{getUniformLocation(name)};
        if constexpr (std::is_same_v<T, GLint>) {
            context->glUniform1i(location, value);
        } else if constexpr (std::is_same_v<T, GLfloat>) {
            context->glUniform1f(location, value);
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            context->glUniform3f(location, value[0], value[1], value[2]);
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            context->glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
        } else {
            // Workaround for static_assert(false):
            // https://en.cppreference.com/w/cpp/language/static_assert
            static_assert(DependentFalse<T>, "Unsupported uniform type");
        }
        context->checkError();
    }

    template<typename T>
    void setUniforms(const QString &name, const GLsizei count, const T *values)
    {
        const auto context{OpenGLContext::instance()};

        const auto location{getUniformLocation(name)};
        if constexpr (std::is_same_v<T, glm::vec2>) {
            context->glUniform2fv(location, count, &values[0][0]);
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            context->glUniformMatrix4fv(location, count, GL_FALSE, &values[0][0][0]);
        } else {
            static_assert(DependentFalse<T>, "Unsupported uniform type");
        }
        context->checkError();
    }

private:
    template<typename T>
    static constexpr auto DependentFalse{false};

    void compileShader(const GLuint shader, const QString &fileName) const;

    GLuint getUniformLocation(const QString &name);

    OpenGLObject _program;
    std::unordered_map<QString, GLuint> _uniformLocations;
};

} // namespace minecraft

#endif // MINECRAFT_SHADER_PROGRAM_H
