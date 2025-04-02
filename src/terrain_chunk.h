#ifndef MINI_MINECRAFT_TERRAIN_CHUNK_H
#define MINI_MINECRAFT_TERRAIN_CHUNK_H

#include "block_type.h"
#include "direction.h"
#include "gl_context.h"
#include "vertex.h"
#include "vertex_array_helper.h"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <ranges>
#include <type_traits>
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
    static constexpr std::array<std::array<std::array<std::int8_t, 3>, 4>, 6> CubeVertices{{
        {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}},
        {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}},
        {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}},
        {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},
        {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
        {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}},
    }};

    static constexpr std::array<std::array<std::int8_t, 3>, 6> CubeNormals{{
        {1, 0, 0},
        {-1, 0, 0},
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1},
    }};

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

                const auto globalX{_chunk->minX() + localX};
                const auto globalY{localY};
                const auto globalZ{_chunk->minZ() + localZ};

                glm::vec3 color;
                switch (block) {
                case BlockType::Grass:
                    color = {0.37f, 0.62f, 0.21f};
                    break;
                case BlockType::Dirt:
                    color = {0.47f, 0.33f, 0.23f};
                    break;
                case BlockType::Stone:
                    color = {0.50f, 0.50f, 0.50f};
                    break;
                case BlockType::Water:
                    color = {0.00f, 0.00f, 0.75f};
                    break;
                case BlockType::Snow:
                    color = {1.00f, 1.00f, 1.00f};
                    break;
                default:
                    color = {1.00f, 0.00f, 1.00f};
                }

                for (const auto &[direction, i] : {
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
                    const auto normal{CubeNormals[i]};
                    for (const auto &position : CubeVertices[i]) {
                        Vertex vertex{
                            .position{static_cast<float>(globalX + position[0]),
                                      static_cast<float>(globalY + position[1]),
                                      static_cast<float>(globalZ + position[2])},
                            .color{color},
                        };
                        if constexpr (std::is_same_v<Vertex, LambertVertex>) {
                            vertex.normal = {
                                static_cast<float>(normal[0]),
                                static_cast<float>(normal[1]),
                                static_cast<float>(normal[2]),
                            };
                        }
                        vertices.push_back(vertex);
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
