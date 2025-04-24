#ifndef MINECRAFT_TERRAIN_STREAMER_H
#define MINECRAFT_TERRAIN_STREAMER_H

#include "ivec2_hash.h"
#include "terrain.h"
#include "terrain_chunk.h"

#include <glm/glm.hpp>

#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace minecraft {

class TerrainStreamer
{
public:
    struct UpdateResult
    {
        std::vector<TerrainChunk *> chunksWithOpaqueFaces;
        std::vector<TerrainChunk *> chunksWithTranslucentFaces;
    };

    TerrainStreamer(Terrain *const terrain)
        : _terrain{terrain}
    {}

    UpdateResult update(const glm::vec3 &cameraPosition);

private:
    friend class TerrainChunkGenerationTask;

    Terrain *_terrain;

    std::mutex _mutex;
    std::unordered_set<glm::ivec2, IVec2Hash> _pendingChunks;
    std::vector<std::unique_ptr<TerrainChunk>> _readyChunks;
};

} // namespace minecraft

#endif // MINECRAFT_TERRAIN_STREAMER_H
