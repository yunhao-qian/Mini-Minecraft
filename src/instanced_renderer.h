#ifndef MINECRAFT_INSTANCED_RENDERER_H
#define MINECRAFT_INSTANCED_RENDERER_H

#include "opengl_context.h"
#include "opengl_object.h"
#include "vertex_attribute.h"

#include <cstddef>
#include <vector>

namespace minecraft {

class InstancedRenderer
{
public:
    InstancedRenderer()
        : _vao{}
        , _instanceVBO{}
        , _instanceCount{0}
    {}

    InstancedRenderer(const InstancedRenderer &) = delete;
    InstancedRenderer(InstancedRenderer &&) = delete;

    ~InstancedRenderer() { releaseResources(); }

    InstancedRenderer &operator=(const InstancedRenderer &) = delete;
    InstancedRenderer &operator=(InstancedRenderer &&) = delete;

    GLsizei instanceCount() const { return _instanceCount; }

    template<typename T>
    void uploadInstances(const std::vector<T> &instances, const GLenum usage = GL_STATIC_DRAW)
    {
        if (instances.empty()) {
            _instanceCount = 0;
            return;
        }

        const auto context{OpenGLContext::instance()};

        if (!_instanceVBO) {
            GLuint vbo{0u};
            context->glGenBuffers(1, &vbo);
            context->checkError();
            _instanceVBO = OpenGLObject{
                vbo,
                [](OpenGLContext *const context, const GLuint vbo) {
                    context->glDeleteBuffers(1, &vbo);
                },
            };
        }
        context->glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO.get());
        context->checkError();

        // Upload the instance attributes to the GPU.
        context->glBufferData(GL_ARRAY_BUFFER,
                              static_cast<GLsizeiptr>(instances.size() * sizeof(T)),
                              instances.data(),
                              usage);
        context->checkError();

        _instanceCount = static_cast<GLsizei>(instances.size());

        // Assuming that the attribute type does not change across function calls, we only need to set
        // the vertex attribute pointers once.
        if (_vao) {
            return;
        }

        {
            GLuint vao{0u};
            context->glGenVertexArrays(1, &vao);
            context->checkError();
            _vao = OpenGLObject{
                vao,
                [](OpenGLContext *const context, const GLuint vao) {
                    context->glDeleteVertexArrays(1, &vao);
                },
            };
        }
        // There is no problem with binding the VAO after the VBO has been bound.
        // https://community.khronos.org/t/understanding-why-we-bind-a-vao-before-a-vbo/75304/3
        context->glBindVertexArray(_vao.get());
        context->checkError();

        for (const auto &attribute : VertexAttributeTrait<T>::Attributes) {
            if (attribute.isInteger) {
                context->glVertexAttribIPointer(attribute.index,
                                                attribute.size,
                                                attribute.type,
                                                sizeof(T),
                                                reinterpret_cast<const GLvoid *>(attribute.offset));
            } else {
                context->glVertexAttribPointer(attribute.index,
                                               attribute.size,
                                               attribute.type,
                                               GL_FALSE,
                                               sizeof(T),
                                               reinterpret_cast<const GLvoid *>(attribute.offset));
            }
            context->checkError();
            context->glEnableVertexAttribArray(attribute.index);
            context->checkError();
            // The attribute advances once per instance.
            context->glVertexAttribDivisor(attribute.index, 1);
            context->checkError();
        }
    }

    void draw(const GLsizei elementCount, const GLenum mode) const
    {
        if (_instanceCount <= 0) {
            return;
        }

        const auto context{OpenGLContext::instance()};
        context->glBindVertexArray(_vao.get());
        context->checkError();
        context->glDrawArraysInstanced(mode, 0, elementCount, _instanceCount);
        context->checkError();
    }

    void releaseResources()
    {
        _vao.reset();
        _instanceVBO.reset();
        _instanceCount = 0;
    }

private:
    OpenGLObject _vao;
    OpenGLObject _instanceVBO;
    GLsizei _instanceCount;
};

} // namespace minecraft

#endif // MINECRAFT_INSTANCED_RENDERER_H
