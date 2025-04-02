#include "terrain_streamer.h"

#include <algorithm>
#include <cmath>
#include <ranges>
#include <utility>

namespace {

constexpr auto VisibleDistance{256.0f};
constexpr auto GenerateDistance{320.0f};
constexpr auto ReleaseDistance{384.0f};

auto getChunkDistance(const glm::vec3 &position, const int minX, const int minZ) -> float
{
    using minecraft::TerrainChunk;

    const auto minY{0};
    const auto maxX{minX + TerrainChunk::SizeX};
    const auto maxY{minY + TerrainChunk::SizeY};
    const auto maxZ{minZ + TerrainChunk::SizeZ};

    const auto distanceX{std::max(std::max(minX - position.x, position.x - maxX), 0.0f)};
    const auto distanceY{std::max(std::max(minY - position.y, position.y - maxY), 0.0f)};
    const auto distanceZ{std::max(std::max(minZ - position.z, position.z - maxZ), 0.0f)};

    return std::sqrt(distanceX * distanceX + distanceY * distanceY + distanceZ * distanceZ);
}

} // namespace

minecraft::TerrainStreamer::TerrainStreamer(GLContext *const context, Terrain *const terrain)
    : _context{context}
    , _terrain{terrain}
{}

auto minecraft::TerrainStreamer::update(const glm::vec3 &cameraPosition) -> void
{
    for (auto &chunk : _chunksToGenerate) {
        _terrain->setChunk(std::move(chunk));
    }
    _chunksToGenerate.clear();

    _terrain->forEachChunk([&cameraPosition](TerrainChunk *const chunk) {
        const auto distance{getChunkDistance(cameraPosition, chunk->minX(), chunk->minZ())};
        if (distance <= VisibleDistance) {
            if (!chunk->isVisible()) {
                chunk->setVisible(true);
                chunk->markSelfAndNeighborsDirty();
            }
        } else if (distance > GenerateDistance) {
            if (chunk->isVisible()) {
                chunk->setVisible(false);
                chunk->markSelfAndNeighborsDirty();
            }
            if (distance > ReleaseDistance) {
                chunk->releaseDrawDelegate();
            }
        }
    });

    const glm::vec2 center2D{cameraPosition.x, cameraPosition.z};
    const glm::ivec2 minPosition{glm::floor(center2D - GenerateDistance)};
    const glm::ivec2 maxPosition{glm::floor(center2D + GenerateDistance)};
    const auto minOrigin{TerrainChunk::alignToChunkOrigin(minPosition[0], minPosition[1])};
    const auto maxOrigin{TerrainChunk::alignToChunkOrigin(maxPosition[0], maxPosition[1])};
    for (auto minX{minOrigin.first}; minX <= maxOrigin.first; minX += TerrainChunk::SizeX) {
        for (auto minZ{minOrigin.second}; minZ <= maxOrigin.second; minZ += TerrainChunk::SizeZ) {
            if (!(getChunkDistance(cameraPosition, minX, minZ) <= GenerateDistance)
                || _terrain->getChunk(minX, minZ) != nullptr) {
                continue;
            }
            _chunksToGenerate.push_back(std::make_unique<TerrainChunk>(_context, minX, minZ));
        }
    }
    for (const auto &chunk : _chunksToGenerate) {
        for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
            for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
                for (const auto y : std::views::iota(0, 128)) {
                    chunk->setBlockLocal(x, y, z, BlockType::Stone);
                }
            }
        }
    }
}
