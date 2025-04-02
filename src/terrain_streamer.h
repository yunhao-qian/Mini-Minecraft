#ifndef MINI_MINECRAFT_TERRAIN_STREAMER_H
#define MINI_MINECRAFT_TERRAIN_STREAMER_H

#include "gl_context.h"
#include "terrain.h"
#include "terrain_chunk.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace minecraft {

class TerrainStreamer
{
public:
    TerrainStreamer(GLContext *const context, Terrain *const terrain);

    auto update(const glm::vec3 &cameraPosition) -> void;

private:
    GLContext *_context;
    Terrain *_terrain;

    std::vector<std::unique_ptr<TerrainChunk>> _chunksToGenerate;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_TERRAIN_STREAMER_H
