#ifndef MINECRAFT_VERTEX_ATTRIBUTE_H
#define MINECRAFT_VERTEX_ATTRIBUTE_H

#include <glm/glm.hpp>

#include <QOpenGLFunctions_4_1_Core>

#include <array>
#include <cstddef>

namespace minecraft {

struct VertexAttribute
{
    bool isInteger;
    GLuint index;
    GLint size;
    GLenum type;
    std::size_t offset;
};

template<typename Vertex>
struct VertexAttributeTrait;

struct BlockFace
{
    glm::ivec3 faceOrigin;
    GLubyte faceIndex;
    GLubyte textureIndex;
    GLubyte blockType;
    GLubyte mediumType;
};

template<>
struct VertexAttributeTrait<BlockFace>
{
    static constexpr auto Attributes{std::to_array<VertexAttribute>({
        {true, 0u, 3, GL_INT, offsetof(BlockFace, faceOrigin)},
        {true, 1u, 1, GL_UNSIGNED_BYTE, offsetof(BlockFace, faceIndex)},
        {true, 2u, 1, GL_UNSIGNED_BYTE, offsetof(BlockFace, textureIndex)},
        {true, 3u, 1, GL_UNSIGNED_BYTE, offsetof(BlockFace, blockType)},
        {true, 4u, 1, GL_UNSIGNED_BYTE, offsetof(BlockFace, mediumType)},
    })};
};

} // namespace minecraft

#endif // MINECRAFT_VERTEX_ATTRIBUTE_H
