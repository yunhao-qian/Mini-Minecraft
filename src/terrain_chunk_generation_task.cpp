#include "terrain_chunk_generation_task.h"

#include <ranges>

// TODO: Remove this.
#include <QThread>

minecraft::TerrainChunkGenerationTask::TerrainChunkGenerationTask(
    std::unique_ptr<TerrainChunk> chunk,
    std::mutex *const mutex,
    std::unordered_set<std::pair<int, int>, IntPairHash> *const pendingChunks,
    std::vector<std::unique_ptr<TerrainChunk>> *const finishedChunks)
    : _chunk{std::move(chunk)}
    , _mutex{mutex}
    , _pendingChunks{pendingChunks}
    , _finishedChunks{finishedChunks}
{}

auto minecraft::TerrainChunkGenerationTask::run() -> void
{
    for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
            for (const auto y : std::views::iota(0, 128)) {
                _chunk->setBlockLocal(x, y, z, BlockType::Stone);
            }
        }
    }
    // TODO: Remove this.
    QThread::msleep(500u);
    {
        std::lock_guard lock{*_mutex};
        _pendingChunks->erase({_chunk->minX(), _chunk->minZ()});
        _finishedChunks->push_back(std::move(_chunk));
    }
}
