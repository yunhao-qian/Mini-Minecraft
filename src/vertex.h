#ifndef MINI_MINECRAFT_VERTEX_H
#define MINI_MINECRAFT_VERTEX_H

#include <glm/glm.hpp>

#include <QOpenGLFunctions_4_1_Core>

#include <array>
#include <cstddef>

namespace minecraft {

enum class VertexType {
    Lambert,
};

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
    glm::vec2 textureCoords;
    glm::vec3 normal;
    glm::vec3 tangent;
    GLbyte isWater;
    GLbyte isLava;
};

template<>
struct VertexTraits<LambertVertex>
{
    static constexpr auto Type{VertexType::Lambert};
    static constexpr auto Stride{sizeof(LambertVertex)};
    static constexpr auto Attributes{std::to_array<VertexAttribute>({
        {0u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, position)},
        {1u, 2, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, textureCoords)},
        {2u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, normal)},
        {3u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, tangent)},
        {4u, 1, GL_BYTE, GL_FALSE, offsetof(LambertVertex, isWater)},
        {5u, 1, GL_BYTE, GL_FALSE, offsetof(LambertVertex, isLava)},
    })};
};

} // namespace minecraft

#endif // MINI_MINECRAFT_VERTEX_H
