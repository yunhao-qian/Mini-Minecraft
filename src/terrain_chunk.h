#ifndef MINI_MINECRAFT_TERRAIN_CHUNK_H
#define MINI_MINECRAFT_TERRAIN_CHUNK_H

#include "block_type.h"
#include "direction.h"
#include "gl_context.h"
#include "terrain_chunk_draw_delegate.h"

#include <array>
#include <memory>
#include <utility>

namespace minecraft {

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

    auto prepareDraw() -> void;

    auto drawSolidBlocks() -> void;
    auto drawLiquidBlocks() -> void;

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
    std::unique_ptr<TerrainChunkDrawDelegate> _drawDelegate;
};

} // namespace minecraft

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
