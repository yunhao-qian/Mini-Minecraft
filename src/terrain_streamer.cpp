#include "terrain_streamer.h"

#include "terrain_chunk_generation_task.h"

#include <QThreadPool>

#include <algorithm>
#include <utility>

namespace minecraft {

namespace {

constexpr auto VisibleDistance{256.0f};
constexpr auto GenerateDistance{288.0f};
constexpr auto ReleaseDistance{1024.0f};

float getChunkDistance(const glm::vec3 &position, const glm::ivec2 originXZ)
{
    const glm::vec2 positionXZ{position.x, position.z};
    const glm::vec2 minXZ{originXZ};
    const glm::vec2 maxXZ{originXZ + glm::ivec2{TerrainChunk::SizeX, TerrainChunk::SizeZ}};
    const auto distancesPerAxis{glm::max(glm::max(minXZ - positionXZ, positionXZ - maxXZ), 0.0f)};
    return glm::length(distancesPerAxis);
}

} // namespace

std::vector<TerrainChunk *> TerrainStreamer::update(const glm::vec3 &cameraPosition)
{
    const glm::vec2 cameraXZ{cameraPosition.x, cameraPosition.z};
    const auto minOrigin{
        TerrainChunk::alignToChunkOrigin(glm::ivec2{glm::floor(cameraXZ - GenerateDistance)})};
    const auto maxOrigin{
        TerrainChunk::alignToChunkOrigin(glm::ivec2{glm::floor(cameraXZ + GenerateDistance)})};

    std::vector<std::pair<glm::ivec2, float>> chunksWithDistances;
    {
        const std::lock_guard lock{_mutex};

        // Set the chunks that have been generated.
        for (auto &chunk : _readyChunks) {
            _terrain->setChunk(std::move(chunk));
            // New chunks are invisible by default, so there is no need to mark them and their
            // neighbors as dirty.
        }
        _readyChunks.clear();

        for (auto originX{minOrigin[0]}; originX <= maxOrigin[0]; originX += TerrainChunk::SizeX) {
            for (auto originZ{minOrigin[1]}; originZ <= maxOrigin[1];
                 originZ += TerrainChunk::SizeZ) {
                const glm::ivec2 originXZ{originX, originZ};

                const auto distance{getChunkDistance(cameraPosition, originXZ)};
                if (distance > GenerateDistance || _terrain->getChunk(originXZ) != nullptr
                    || _pendingChunks.contains(originXZ)) {
                    // Skip if:
                    // - The chunk is too far away.
                    // - The chunk has already been generated.
                    // - The chunk is being worked on.
                    continue;
                }

                _pendingChunks.insert(originXZ);
                chunksWithDistances.emplace_back(originXZ, distance);
            }
        }
    }

    // Sort the chunks by distance so that the closest chunks get queued first.
    std::ranges::sort(chunksWithDistances,
                      [](const auto &a, const auto &b) { return a.second < b.second; });

    // Generate the chunks in worker threads.
    for (const auto &[originXZ, _] : chunksWithDistances) {
        QThreadPool::globalInstance()->start(new TerrainChunkGenerationTask{
            this,
            std::make_unique<TerrainChunk>(originXZ),
        });
    }

    std::vector<TerrainChunk *> result;

    _terrain->forEachChunk([&cameraPosition, &result](TerrainChunk *const chunk) {
        const auto distance{getChunkDistance(cameraPosition, chunk->originXZ())};
        if (distance <= VisibleDistance) {
            // All chunks closer than VisibleDistance are visible.
            if (!chunk->isVisible()) {
                chunk->setVisible(true);
                chunk->markSelfAndNeighborsDirty();
            }
            chunk->prepareDraw();
            result.push_back(chunk);
        } else if (distance <= GenerateDistance) {
            // Do nothing for chunks between VisibleDistance and GenerateDistance. This introduces
            // hysteresis to prevent flickering.
            if (chunk->isVisible()) {
                chunk->prepareDraw();
                result.push_back(chunk);
            }
        } else {
            // All chunks farther than GenerateDistance are invisible.
            if (chunk->isVisible()) {
                chunk->setVisible(false);
                chunk->markSelfAndNeighborsDirty();
            }
            // Release the renderer resources for chunks farther than ReleaseDistance.
            if (distance > ReleaseDistance) {
                chunk->releaseRendererResources();
            }
        }
    });

    return result;
}

} // namespace minecraft
