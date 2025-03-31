#ifndef MINI_MINECRAFT_VERTEX_H
#define MINI_MINECRAFT_VERTEX_H

#include <glm/glm.hpp>

#include <QOpenGLFunctions_4_1_Core>

#include <array>
#include <cstddef>

namespace minecraft {

enum class VertexType {
    Flat,
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

struct FlatVertex
{
    glm::vec3 position;
    glm::vec3 color;
};

template<>
struct VertexTraits<FlatVertex>
{
    static constexpr auto Type{VertexType::Flat};
    static constexpr auto Stride{sizeof(FlatVertex)};
    static constexpr auto Attributes{std::to_array<VertexAttribute>({
        {0u, 3, GL_FLOAT, GL_FALSE, offsetof(FlatVertex, position)},
        {1u, 3, GL_FLOAT, GL_FALSE, offsetof(FlatVertex, color)},
    })};
};

struct LambertVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

template<>
struct VertexTraits<LambertVertex>
{
    static constexpr auto Type{VertexType::Lambert};
    static constexpr auto Stride{sizeof(LambertVertex)};
    static constexpr auto Attributes{std::to_array<VertexAttribute>({
        {0u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, position)},
        {1u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, normal)},
        {2u, 3, GL_FLOAT, GL_FALSE, offsetof(LambertVertex, color)},
    })};
};

} // namespace minecraft

#endif // MINI_MINECRAFT_VERTEX_H
