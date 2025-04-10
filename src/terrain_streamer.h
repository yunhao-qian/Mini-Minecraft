#ifndef MINI_MINECRAFT_TERRAIN_STREAMER_H
#define MINI_MINECRAFT_TERRAIN_STREAMER_H

#include "ivec2_hash.h"
#include "opengl_context.h"
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

    TerrainStreamer(OpenGLContext *const context, Terrain *const terrain);

    UpdateResult update(const glm::vec3 &cameraPosition);

private:
    friend class TerrainChunkGenerationTask;

    OpenGLContext *_context;
    Terrain *_terrain;

    std::mutex _mutex;
    std::unordered_set<glm::ivec2, IVec2Hash> _pendingChunks;
    std::vector<std::unique_ptr<TerrainChunk>> _readyChunks;
};

inline TerrainStreamer::TerrainStreamer(OpenGLContext *const context, Terrain *const terrain)
    : _context{context}
    , _terrain{terrain}
{}

} // namespace minecraft

#endif // MINI_MINECRAFT_TERRAIN_STREAMER_H
