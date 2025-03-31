#ifndef MINI_MINECRAFT_TERRAIN_H
#define MINI_MINECRAFT_TERRAIN_H

#include "gl_context.h"
#include "terrain_chunk.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>

namespace minecraft {

class Terrain
{
public:
    Terrain(GLContext *const context);

    auto getChunk(const int x, const int z) const -> const TerrainChunk *;
    auto getChunk(const int x, const int z) -> TerrainChunk *;
    auto getOrCreateChunk(const int x, const int z) -> TerrainChunk *;

    auto getBlockGlobal(const int x, const int y, const int z) const -> BlockType;
    auto setBlockGlobal(const int x, const int y, const int z, const BlockType block) -> void;

    template<typename Vertex>
    auto prepareDraw() -> void;

    auto draw() -> void;

private:
    struct ChunkKeyHash
    {
        auto operator()(const std::pair<int, int> &key) const -> std::size_t;
    };

    template<typename Self>
    static auto getChunk(Self &self, const int x, const int z);

    GLContext *_context;
    std::unordered_map<std::pair<int, int>, std::unique_ptr<TerrainChunk>, ChunkKeyHash> _chunks;
};

} // namespace minecraft

template<typename Vertex>
auto minecraft::Terrain::prepareDraw() -> void
{
    for (const auto &[_, chunk] : _chunks) {
        chunk->prepareDraw<Vertex>();
    }
}

template<typename Self>
auto minecraft::Terrain::getChunk(Self &self, const int x, const int z)
{
    const auto it{self._chunks.find(TerrainChunk::alignToChunkOrigin(x, z))};
    if (it == self._chunks.end()) {
        return static_cast<decltype(it->second.get())>(nullptr);
    }
    return it->second.get();
}

#endif // MINI_MINECRAFT_TERRAIN_H
