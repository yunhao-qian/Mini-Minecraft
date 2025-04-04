#ifndef MINI_MINECRAFT_TERRAIN_CHUNK_H
#define MINI_MINECRAFT_TERRAIN_CHUNK_H

#include "block_type.h"
#include "direction.h"
#include "gl_context.h"
#include "vertex.h"
#include "vertex_array_helper.h"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace minecraft {

class TerrainChunk;

class TerrainChunkDrawDelegateBase
{
public:
    TerrainChunkDrawDelegateBase(const TerrainChunk *const chunk);
    virtual ~TerrainChunkDrawDelegateBase() = default;

    auto markDirty() -> void;

    virtual auto vertexType() const -> VertexType = 0;

    virtual auto prepareDraw() -> void = 0;

    virtual auto draw() -> void = 0;

protected:
    const TerrainChunk *_chunk;
    bool _dirty;
};

template<typename Vertex>
class TerrainChunkDrawDelegate : public TerrainChunkDrawDelegateBase
{
public:
    TerrainChunkDrawDelegate(const TerrainChunk *const chunk, GLContext *const context);

    auto vertexType() const -> VertexType override;

    auto prepareDraw() -> void override;

    auto draw() -> void override;

private:
    static constexpr std::array<std::array<glm::ivec3, 4>, 6> VertexPositions{{
        {{{1, 1, 1}, {1, 0, 1}, {1, 0, 0}, {1, 1, 0}}},
        {{{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1}}},
        {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}},
        {{{1, 0, 0}, {1, 0, 1}, {0, 0, 1}, {0, 0, 0}}},
        {{{0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}}},
        {{{1, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}}},
    }};

    static constexpr std::array<glm::ivec3, 6> FaceNormals{
        {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
    static constexpr std::array<glm::ivec3, 6> FaceTangents{
        {{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, 1}, {0, -1, 0}, {0, -1, 0}}};

    static constexpr std::array<glm::ivec2, 4> GrassTopTextureCoords{
        {{8, 2}, {8, 3}, {9, 3}, {9, 2}}};
    static constexpr std::array<glm::ivec2, 4> GrassSideTextureCoords{
        {{3, 0}, {3, 1}, {4, 1}, {4, 0}}};
    static constexpr std::array<glm::ivec2, 4> DirtTextureCoords{{{2, 0}, {2, 1}, {3, 1}, {3, 0}}};
    static constexpr std::array<glm::ivec2, 4> StoneTextureCoords{{{1, 0}, {1, 1}, {2, 1}, {2, 0}}};
    static constexpr std::array<glm::ivec2, 4> WaterTextureCoords{
        {{13, 12}, {13, 13}, {14, 13}, {14, 12}}};
    static constexpr std::array<glm::ivec2, 4> SnowTextureCoords{{{2, 4}, {2, 5}, {3, 5}, {3, 4}}};
    static constexpr std::array<glm::ivec2, 4> UnknownTextureCoords{
        {{8, 11}, {8, 12}, {9, 12}, {9, 11}}};

    VertexArrayHelper<Vertex> _vertexArrayHelper;
};

class TerrainChunk
{
public:
    TerrainChunk(GLContext *const context, const int minX, const int minZ);

    auto minX() const -> int;
    auto minZ() const -> int;

    auto getBlockLocal(const int x, const int y, const int z) const -> BlockType;
    auto setBlockLocal(const int x, const int y, const int z, const BlockType block) -> void;

    auto getNeighbor(const Direction direction) const -> const TerrainChunk *;
    auto getNeighbor(const Direction direction) -> TerrainChunk *;
    auto setNeighbor(const Direction direction, TerrainChunk *const chunk) -> void;

    auto getNeighborBlockLocal(const int x, const int y, const int z, const Direction direction) const
        -> BlockType;

    auto isVisible() const -> bool;
    auto setVisible(const bool visible) -> void;

    auto markDirty() -> void;
    auto markSelfAndNeighborsDirty() -> void;

    template<typename Vertex>
    auto prepareDraw() -> void;

    auto draw() -> void;

    auto releaseDrawDelegate() -> void;

    static auto alignToChunkOrigin(const int x, const int z) -> std::pair<int, int>;

    static constexpr auto SizeX{16};
    static constexpr auto SizeY{256};
    static constexpr auto SizeZ{16};

private:
    template<typename Self>
    static auto getNeighborPointer(Self &self, const Direction direction);

    GLContext *_context;
    int _minX;
    int _minZ;
    std::array<std::array<std::array<BlockType, SizeZ>, SizeY>, SizeX> _blocks;
    std::array<TerrainChunk *, 4> _neighbors;
    bool _visible;
    std::unique_ptr<TerrainChunkDrawDelegateBase> _drawDelegate;
};

} // namespace minecraft

template<typename Vertex>
minecraft::TerrainChunkDrawDelegate<Vertex>::TerrainChunkDrawDelegate(
    const TerrainChunk *const chunk, GLContext *const context)
    : TerrainChunkDrawDelegateBase{chunk}
    , _vertexArrayHelper{context}
{}

template<typename Vertex>
auto minecraft::TerrainChunkDrawDelegate<Vertex>::vertexType() const -> VertexType
{
    return VertexTraits<Vertex>::Type;
}

template<typename Vertex>
auto minecraft::TerrainChunkDrawDelegate<Vertex>::prepareDraw() -> void
{
    if (!_dirty) {
        return;
    }

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    for (const auto localX : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto localY : std::views::iota(0, TerrainChunk::SizeY)) {
            for (const auto localZ : std::views::iota(0, TerrainChunk::SizeZ)) {
                const auto block{_chunk->getBlockLocal(localX, localY, localZ)};
                if (block == BlockType::Empty) {
                    continue;
                }

                const glm::ivec3 blockPosition{_chunk->minX() + localX,
                                               localY,
                                               _chunk->minZ() + localZ};

                for (const auto &[direction, faceIndex] : {
                         std::pair{Direction::PositiveX, 0},
                         std::pair{Direction::NegativeX, 1},
                         std::pair{Direction::PositiveY, 2},
                         std::pair{Direction::NegativeY, 3},
                         std::pair{Direction::PositiveZ, 4},
                         std::pair{Direction::NegativeZ, 5},
                     }) {
                    if (_chunk->getNeighborBlockLocal(localX, localY, localZ, direction)
                        != BlockType::Empty) {
                        continue;
                    }
                    {
                        const auto indexOffset{static_cast<GLuint>(vertices.size())};
                        for (const auto index : {0u, 1u, 2u, 0u, 2u, 3u}) {
                            indices.push_back(indexOffset + index);
                        }
                    }

                    const auto &vertexPositions{VertexPositions[faceIndex]};

                    const std::array<glm::ivec2, 4> *textureCoords{nullptr};
                    switch (block) {
                    case BlockType::Grass:
                        if (direction == Direction::PositiveY) {
                            textureCoords = &GrassTopTextureCoords;
                        } else if (direction == Direction::NegativeY) {
                            textureCoords = &DirtTextureCoords;
                        } else {
                            textureCoords = &GrassSideTextureCoords;
                        }
                        break;
                    case BlockType::Dirt:
                        textureCoords = &DirtTextureCoords;
                        break;
                    case BlockType::Stone:
                        textureCoords = &StoneTextureCoords;
                        break;
                    case BlockType::Water:
                        textureCoords = &WaterTextureCoords;
                        break;
                    case BlockType::Snow:
                        textureCoords = &SnowTextureCoords;
                        break;
                    default:
                        textureCoords = &UnknownTextureCoords;
                        break;
                    }

                    const auto normal{FaceNormals[faceIndex]};
                    const auto tangent{FaceTangents[faceIndex]};

                    for (const auto vertexIndex : std::views::iota(0, 4)) {
                        vertices.push_back({
                            .position{blockPosition + vertexPositions[vertexIndex]},
                            .textureCoords{glm::vec2{(*textureCoords)[vertexIndex]} / 16.0f},
                            .normal{normal},
                            .tangent{tangent},
                        });
                    }
                }
            }
        }
    }

    _vertexArrayHelper.setVertices(vertices, GL_STATIC_DRAW);
    _vertexArrayHelper.setIndices(indices, GL_STATIC_DRAW);
    _dirty = false;
}

template<typename Vertex>
auto minecraft::TerrainChunkDrawDelegate<Vertex>::draw() -> void
{
    _vertexArrayHelper.drawElements(GL_TRIANGLES);
}

template<typename Vertex>
auto minecraft::TerrainChunk::prepareDraw() -> void
{
    if (!_visible) {
        return;
    }
    if (_drawDelegate == nullptr || _drawDelegate->vertexType() != VertexTraits<Vertex>::Type) {
        _drawDelegate = std::make_unique<TerrainChunkDrawDelegate<Vertex>>(this, _context);
    }
    _drawDelegate->prepareDraw();
}

template<typename Self>
auto minecraft::TerrainChunk::getNeighborPointer(Self &self, const Direction direction)
{
    switch (direction) {
    case Direction::PositiveX:
        return &self._neighbors[0];
    case Direction::NegativeX:
        return &self._neighbors[1];
    case Direction::PositiveZ:
        return &self._neighbors[2];
    case Direction::NegativeZ:
        return &self._neighbors[3];
    default:
        return static_cast<decltype(&self._neighbors[0])>(nullptr);
    }
}

#endif // MINI_MINECRAFT_TERRAIN_CHUNK_H
