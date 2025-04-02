#ifndef MINI_MINECRAFT_TERRAIN_H
#define MINI_MINECRAFT_TERRAIN_H

#include "int_pair_hash.h"
#include "terrain_chunk.h"

#include <memory>
#include <unordered_map>
#include <utility>

namespace minecraft {

class Terrain
{
public:
    auto getChunk(const int x, const int z) const -> const TerrainChunk *;
    auto getChunk(const int x, const int z) -> TerrainChunk *;
    auto setChunk(std::unique_ptr<TerrainChunk> chunk) -> void;

    auto getBlockGlobal(const int x, const int y, const int z) const -> BlockType;
    auto setBlockGlobal(const int x, const int y, const int z, const BlockType block) -> void;

    template<typename Function>
    auto forEachChunk(Function function) -> void;

    template<typename Vertex>
    auto prepareDraw() -> void;

    auto draw() -> void;

private:
    template<typename Self>
    static auto getChunk(Self &self, const int x, const int z);

    std::unordered_map<std::pair<int, int>, std::unique_ptr<TerrainChunk>, IntPairHash> _chunks;
};

} // namespace minecraft

template<typename Function>
auto minecraft::Terrain::forEachChunk(Function function) -> void
{
    for (const auto &[_, chunk] : _chunks) {
        function(chunk.get());
    }
}

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
