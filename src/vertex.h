#ifndef MINI_MINECRAFT_VERTEX_H
#define MINI_MINECRAFT_VERTEX_H

#include <glm/glm.hpp>

#include <QOpenGLFunctions_4_1_Core>

#include <array>
#include <cstddef>

namespace minecraft {

struct VertexAttribute
{
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    std::size_t offset;
};

template<typename Vertex>
struct VertexTraits;

struct LambertVertex
{
    glm::vec3 position;
    GLubyte textureIndex;
    glm::vec2 textureCoords;
    glm::vec3 normal;
    glm::vec3 tangent;
    GLubyte isWater;
    GLubyte isLava;
    GLubyte isAdjacentToWater;
    GLubyte isAdjacentToLava;
};

template<>
struct VertexTraits<LambertVertex>
{
    static constexpr auto Stride{sizeof(LambertVertex)};
    static constexpr auto Attributes{std::to_array<VertexAttribute>({
        {0u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, position)},
        {1u, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(LambertVertex, textureIndex)},
        {2u, 2, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, textureCoords)},
        {3u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, normal)},
        {4u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, tangent)},
        {5u, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(LambertVertex, isWater)},
        {6u, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(LambertVertex, isLava)},
        {7u, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(LambertVertex, isAdjacentToWater)},
        {8u, 1, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(LambertVertex, isAdjacentToLava)},
    })};
};

struct EmptyVertex
{};

template<>
struct VertexTraits<EmptyVertex>
{
    static constexpr auto Stride{sizeof(EmptyVertex)};
    static constexpr std::array<VertexAttribute, 0> Attributes{};
};

} // namespace minecraft

#endif // MINI_MINECRAFT_VERTEX_H
