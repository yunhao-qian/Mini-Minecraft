#ifndef MINI_MINECRAFT_TERRAIN_CHUNK_H
#define MINI_MINECRAFT_TERRAIN_CHUNK_H

#include "block_type.h"
#include "direction.h"

#include <array>
#include <utility>

namespace minecraft {

class TerrainChunk
{
public:
    constexpr static auto SizeX{16};
    constexpr static auto SizeY{256};
    constexpr static auto SizeZ{16};

    TerrainChunk(const int minX, const int minZ);

    auto minX() const -> int;
    auto minZ() const -> int;

    auto getBlockLocal(const int x, const int y, const int z) const -> BlockType;
    auto setBlockLocal(const int x, const int y, const int z, const BlockType block) -> void;

    auto getNeighbor(const Direction direction) const -> const TerrainChunk *;
    auto getNeighbor(const Direction direction) -> TerrainChunk *;
    auto setNeighbor(const Direction direction, TerrainChunk *const chunk) -> void;

    auto getNeighborBlockLocal(const int x, const int y, const int z, const Direction direction) const
        -> BlockType;

    auto prepareDraw() -> void;
    auto draw() -> void;

    static auto alignToChunkOrigin(const int x, const int z) -> std::pair<int, int>;

private:
    template<typename Self>
    static auto getNeighborPointer(Self &self, const Direction direction);

    int _minX;
    int _minZ;
    std::array<std::array<std::array<BlockType, SizeZ>, SizeY>, SizeX> _blocks;
    std::array<TerrainChunk *, 4> _neighbors;
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
