#include "terrain_streamer.h"

#include "terrain_chunk_generation_task.h"

#include <QThreadPool>

#include <algorithm>
#include <cmath>
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

minecraft::TerrainStreamer::~TerrainStreamer()
{
    // TODO: This works only when TerrainStreamer is the only class using QThreadPool.
    QThreadPool::globalInstance()->waitForDone();
}

auto minecraft::TerrainStreamer::update(const glm::vec3 &cameraPosition) -> void
{
    const glm::vec2 center2D{cameraPosition.x, cameraPosition.z};
    const glm::ivec2 minPosition{glm::floor(center2D - GenerateDistance)};
    const glm::ivec2 maxPosition{glm::floor(center2D + GenerateDistance)};
    const auto minOrigin{TerrainChunk::alignToChunkOrigin(minPosition[0], minPosition[1])};
    const auto maxOrigin{TerrainChunk::alignToChunkOrigin(maxPosition[0], maxPosition[1])};

    std::vector<std::pair<float, std::pair<int, int>>> chunksToProcess;
    {
        std::lock_guard lock{_mutex};

        for (auto &chunk : _finishedChunks) {
            _terrain->setChunk(std::move(chunk));
        }
        _finishedChunks.clear();

        for (auto minX{minOrigin.first}; minX <= maxOrigin.first; minX += TerrainChunk::SizeX) {
            for (auto minZ{minOrigin.second}; minZ <= maxOrigin.second;
                 minZ += TerrainChunk::SizeZ) {
                const auto distance{getChunkDistance(cameraPosition, minX, minZ)};
                if (distance > GenerateDistance || _terrain->getChunk(minX, minZ) != nullptr
                    || _pendingChunks.contains({minX, minZ})) {
                    continue;
                }
                _pendingChunks.insert({minX, minZ});
                chunksToProcess.push_back({distance, {minX, minZ}});
            }
        }
    }

    std::ranges::sort(chunksToProcess,
                      [](const auto &a, const auto &b) { return a.first < b.first; });
    for (const auto &[_, position] : chunksToProcess) {
        QThreadPool::globalInstance()->start(new TerrainChunkGenerationTask{
            std::make_unique<TerrainChunk>(_context, position.first, position.second),
            &_mutex,
            &_pendingChunks,
            &_finishedChunks,
        });
    }

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
}
