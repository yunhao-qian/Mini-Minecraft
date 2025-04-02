#ifndef MINI_MINECRAFT_TERRAIN_STREAMER_H
#define MINI_MINECRAFT_TERRAIN_STREAMER_H

#include "gl_context.h"
#include "int_pair_hash.h"
#include "terrain.h"
#include "terrain_chunk.h"

#include <glm/glm.hpp>

#include <memory>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vector>

namespace minecraft {

class TerrainStreamer
{
public:
    TerrainStreamer(GLContext *const context, Terrain *const terrain);
    ~TerrainStreamer();

    auto update(const glm::vec3 &cameraPosition) -> void;

private:
    GLContext *_context;
    Terrain *_terrain;

    std::mutex _mutex;
    std::unordered_set<std::pair<int, int>, IntPairHash> _pendingChunks;
    std::vector<std::unique_ptr<TerrainChunk>> _finishedChunks;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_TERRAIN_STREAMER_H
