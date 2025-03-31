#ifndef MINI_MINECRAFT_VERTEX_ARRAY_HELPER_H
#define MINI_MINECRAFT_VERTEX_ARRAY_HELPER_H

#include "gl_context.h"
#include "vertex.h"

#include <vector>

namespace minecraft {

template<typename Vertex>
class VertexArrayHelper
{
public:
    VertexArrayHelper(GLContext *const context, const GLuint vao = 0u);
    ~VertexArrayHelper();

    auto setVertices(const std::vector<Vertex> &vertices, const GLenum usage) -> void;
    auto setIndices(const std::vector<GLuint> &indices, const GLenum usage) -> void;

    auto drawElements(const GLenum mode) const -> void;

private:
    auto enableVertexAttributes() const -> void;

    GLContext *_context;
    bool _ownsVAO;
    GLuint _vao;
    GLuint _vbo;
    GLuint _ebo;
    GLsizei _elementCount;
};

} // namespace minecraft

template<typename Vertex>
minecraft::VertexArrayHelper<Vertex>::VertexArrayHelper(GLContext *const context, const GLuint vao)
    : _context{context}
    , _ownsVAO{vao == 0u}
    , _vao{vao}
    , _vbo{0u}
    , _ebo{0u}
    , _elementCount{0}
{
    if (_ownsVAO) {
        _context->glGenVertexArrays(1, &_vao);
        _context->debugGLError();
    }
    _context->glGenBuffers(1, &_vbo);
    _context->debugGLError();
    _context->glGenBuffers(1, &_ebo);
    _context->debugGLError();
}

template<typename Vertex>
minecraft::VertexArrayHelper<Vertex>::~VertexArrayHelper<Vertex>()
{
    if (_ownsVAO) {
        _context->glDeleteVertexArrays(1, &_vao);
        _context->debugGLError();
    }
    _context->glDeleteBuffers(1, &_vbo);
    _context->debugGLError();
    _context->glDeleteBuffers(1, &_ebo);
    _context->debugGLError();
}

template<typename Vertex>
auto minecraft::VertexArrayHelper<Vertex>::setVertices(const std::vector<Vertex> &vertices,
                                                       const GLenum usage) -> void
{
    _context->glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    _context->debugGLError();
    _context->glBufferData(GL_ARRAY_BUFFER,
                           static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                           vertices.data(),
                           usage);
    _context->debugGLError();
    if (_ownsVAO) {
        _context->glBindVertexArray(_vao);
        _context->debugGLError();
        enableVertexAttributes();
    }
}

template<typename Vertex>
auto minecraft::VertexArrayHelper<Vertex>::setIndices(const std::vector<GLuint> &indices,
                                                      const GLenum usage) -> void
{
    _context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    _context->debugGLError();
    _context->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                           static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                           indices.data(),
                           usage);
    _context->debugGLError();
    _elementCount = static_cast<GLsizei>(indices.size());
}

template<typename Vertex>
auto minecraft::VertexArrayHelper<Vertex>::drawElements(const GLenum mode) const -> void
{
    _context->glBindVertexArray(_vao);
    _context->debugGLError();
    if (!_ownsVAO) {
        _context->glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        _context->debugGLError();
        enableVertexAttributes();
        _context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        _context->debugGLError();
    }
    _context->glDrawElements(mode, _elementCount, GL_UNSIGNED_INT, static_cast<const GLvoid *>(0));
    _context->debugGLError();
    if (!_ownsVAO) {
        for (const auto &attribute : VertexTraits<Vertex>::Attributes) {
            _context->glDisableVertexAttribArray(attribute.index);
            _context->debugGLError();
        }
    }
}

template<typename Vertex>
auto minecraft::VertexArrayHelper<Vertex>::enableVertexAttributes() const -> void
{
    for (const auto &attribute : VertexTraits<Vertex>::Attributes) {
        _context->glVertexAttribPointer(attribute.index,
                                        attribute.size,
                                        attribute.type,
                                        attribute.normalized,
                                        static_cast<GLsizei>(VertexTraits<Vertex>::Stride),
                                        static_cast<const GLvoid *>(attribute.offset));
        _context->debugGLError();
        _context->glEnableVertexAttribArray(attribute.index);
        _context->debugGLError();
    }
}

#endif // MINI_MINECRAFT_VERTEX_ARRAY_HELPER_H
