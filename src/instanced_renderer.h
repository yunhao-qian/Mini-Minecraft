#ifndef MINI_MINECRAFT_INSTANCED_RENDERER_H
#define MINI_MINECRAFT_INSTANCED_RENDERER_H

#include "opengl_context.h"
#include "vertex_attribute.h"

#include <cstddef>
#include <vector>

namespace minecraft {

class InstancedRenderer
{
public:
    InstancedRenderer(OpenGLContext *const context);
    InstancedRenderer(const InstancedRenderer &) = delete;
    InstancedRenderer(InstancedRenderer &&) = delete;

    ~InstancedRenderer();

    InstancedRenderer &operator=(const InstancedRenderer &) = delete;
    InstancedRenderer &operator=(InstancedRenderer &&) = delete;

    GLsizei instanceCount() const;

    template<typename T>
    void uploadInstances(const std::vector<T> &instances, const GLenum usage = GL_STATIC_DRAW);

    void draw(const GLsizei elementCount, const GLenum mode) const;

    void releaseResources();

private:
    OpenGLContext *_context;
    GLuint _vao;
    GLuint _instanceVBO;
    GLsizei _instanceCount;
};

inline InstancedRenderer::InstancedRenderer(OpenGLContext *const context)
    : _context{context}
    , _vao{0u}
    , _instanceVBO{0u}
    , _instanceCount{0}
{}

inline InstancedRenderer::~InstancedRenderer()
{
    releaseResources();
}

inline GLsizei InstancedRenderer::instanceCount() const
{
    return _instanceCount;
}

template<typename T>
void InstancedRenderer::uploadInstances(const std::vector<T> &instances, const GLenum usage)
{
    if (instances.empty()) {
        _instanceCount = 0;
        return;
    }

    if (_instanceVBO == 0u) {
        _context->glGenBuffers(1, &_instanceVBO);
        _context->debugError();
    }
    _context->glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
    _context->debugError();

    // Upload the instance attributes to the GPU.
    _context->glBufferData(GL_ARRAY_BUFFER,
                           static_cast<GLsizeiptr>(instances.size() * sizeof(T)),
                           instances.data(),
                           usage);
    _context->debugError();

    _instanceCount = static_cast<GLsizei>(instances.size());

    // Assuming that the attribute type does not change across function calls, we only need to set
    // the vertex attribute pointers once.
    if (_vao != 0u) {
        return;
    }

    _context->glGenVertexArrays(1, &_vao);
    _context->debugError();
    // There is no problem with binding the VAO after the VBO has been bound.
    // https://community.khronos.org/t/understanding-why-we-bind-a-vao-before-a-vbo/75304/3
    _context->glBindVertexArray(_vao);
    _context->debugError();

    for (const auto &attribute : VertexAttributeTrait<T>::Attributes) {
        if (attribute.isInteger) {
            _context->glVertexAttribIPointer(attribute.index,
                                             attribute.size,
                                             attribute.type,
                                             sizeof(T),
                                             reinterpret_cast<const GLvoid *>(attribute.offset));
        } else {
            _context->glVertexAttribPointer(attribute.index,
                                            attribute.size,
                                            attribute.type,
                                            GL_FALSE,
                                            sizeof(T),
                                            reinterpret_cast<const GLvoid *>(attribute.offset));
        }
        _context->debugError();
        _context->glEnableVertexAttribArray(attribute.index);
        _context->debugError();
        // The attribute advances once per instance.
        _context->glVertexAttribDivisor(attribute.index, 1);
        _context->debugError();
    }
}

inline void InstancedRenderer::draw(const GLsizei elementCount, const GLenum mode) const
{
    if (_instanceCount <= 0) {
        return;
    }

    _context->glBindVertexArray(_vao);
    _context->debugError();
    _context->glDrawArraysInstanced(mode, 0, elementCount, _instanceCount);
    _context->debugError();
}

inline void InstancedRenderer::releaseResources()
{
    if (_vao != 0u) {
        _context->glDeleteVertexArrays(1, &_vao);
        _context->debugError();
        _vao = 0u;
    }
    if (_instanceVBO != 0u) {
        _context->glDeleteBuffers(1, &_instanceVBO);
        _context->debugError();
        _instanceVBO = 0u;
    }
    _instanceCount = 0;
}

} // namespace minecraft

#endif // MINI_MINECRAFT_INSTANCED_RENDERER_H
