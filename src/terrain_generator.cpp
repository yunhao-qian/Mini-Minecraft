#include "terrain_generator.h"

#include "block_type.h"
#include "terrain_chunk.h"

#include <ranges>

namespace {

auto generateChunk(minecraft::TerrainChunk *const chunk)
{
    using minecraft::BlockType;
    using minecraft::TerrainChunk;

    for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
            for (const auto y : std::views::iota(0, 128)) {
                chunk->setBlockLocal(x, y, z, BlockType::Stone);
            }
        }
    }
}

} // namespace

minecraft::TerrainGenerator::TerrainGenerator(Terrain *const terrain)
    : _terrain{terrain}
{}

auto minecraft::TerrainGenerator::generateTerrainAround(const glm::vec3 &center, const float radius)
    -> void
{
    const glm::vec2 center2D{center.x, center.z};
    const glm::ivec2 minPosition{glm::floor(center2D - radius)};
    const glm::ivec2 maxPosition{glm::floor(center2D + radius)};
    const auto minOrigin{TerrainChunk::alignToChunkOrigin(minPosition[0], minPosition[1])};
    const auto maxOrigin{TerrainChunk::alignToChunkOrigin(maxPosition[0], maxPosition[1])};
    for (auto minX{minOrigin.first}; minX <= maxOrigin.first; minX += TerrainChunk::SizeX) {
        for (auto minZ{minOrigin.second}; minZ <= maxOrigin.second; minZ += TerrainChunk::SizeZ) {
            if (_terrain->getChunk(minX, minZ) != nullptr) {
                continue;
            }
            generateChunk(_terrain->getOrCreateChunk(minX, minZ));
        }
    }
}
