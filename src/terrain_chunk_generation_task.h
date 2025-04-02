#ifndef MINI_MINECRAFT_TERRAIN_CHUNK_GENERATION_TASK_H
#define MINI_MINECRAFT_TERRAIN_CHUNK_GENERATION_TASK_H

#include "int_pair_hash.h"
#include "terrain_chunk.h"

#include <QRunnable>

#include <memory>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vector>

namespace minecraft {

class TerrainChunkGenerationTask : public QRunnable
{
public:
    TerrainChunkGenerationTask(
        std::unique_ptr<TerrainChunk> chunk,
        std::mutex *const mutex,
        std::unordered_set<std::pair<int, int>, IntPairHash> *const pendingChunks,
        std::vector<std::unique_ptr<TerrainChunk>> *const finishedChunks);

    auto run() -> void override;

private:
    auto generateColumn(const int localX, const int localZ) -> void;

    std::unique_ptr<TerrainChunk> _chunk;
    std::mutex *_mutex;
    std::unordered_set<std::pair<int, int>, IntPairHash> *_pendingChunks;
    std::vector<std::unique_ptr<TerrainChunk>> *_finishedChunks;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_TERRAIN_CHUNK_GENERATION_TASK_H
