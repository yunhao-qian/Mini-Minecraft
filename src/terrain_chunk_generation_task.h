#ifndef MINI_MINECRAFT_TERRAIN_CHUNK_GENERATION_TASK_H
#define MINI_MINECRAFT_TERRAIN_CHUNK_GENERATION_TASK_H

#include "terrain_chunk.h"
#include "terrain_streamer.h"

#include <glm/glm.hpp>

#include <QRunnable>

#include <memory>
#include <utility>

namespace minecraft {

class TerrainChunkGenerationTask : public QRunnable
{
public:
    TerrainChunkGenerationTask(TerrainStreamer *const streamer, std::unique_ptr<TerrainChunk> chunk);

    void run() override;

private:
    void generateColumn(const glm::ivec2 localXZ);

    TerrainStreamer *_streamer;
    std::unique_ptr<TerrainChunk> _chunk;
};

inline TerrainChunkGenerationTask::TerrainChunkGenerationTask(TerrainStreamer *const streamer,
                                                              std::unique_ptr<TerrainChunk> chunk)
    : _streamer{streamer}
    , _chunk{std::move(chunk)}
{}

} // namespace minecraft

#endif // MINI_MINECRAFT_TERRAIN_CHUNK_GENERATION_TASK_H
