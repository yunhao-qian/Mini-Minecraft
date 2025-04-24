#ifndef MINECRAFT_TERRAIN_H
#define MINECRAFT_TERRAIN_H

#include "block_type.h"
#include "ivec2_hash.h"
#include "terrain_chunk.h"

#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>

namespace minecraft {

class Terrain
{
public:
    const TerrainChunk *getChunk(const glm::ivec2 xz) const;
    TerrainChunk *getChunk(const glm::ivec2 xz);

    void setChunk(std::unique_ptr<TerrainChunk> chunk);

    BlockType getBlockAtGlobal(const glm::ivec3 &position) const
    {
        if (position.y < 0 || position.y >= TerrainChunk::SizeY) {
            return BlockType::Air;
        }
        const auto chunk{getChunk(glm::ivec2{position.x, position.z})};
        if (chunk == nullptr) {
            return BlockType::Air;
        }
        return chunk->getBlockAtLocal(glm::ivec3{
            position.x - chunk->originXZ()[0],
            position.y,
            position.z - chunk->originXZ()[1],
        });
    }

    void setBlockAtGlobal(const glm::ivec3 &position, const BlockType block)
    {
        if (position.y < 0 || position.y >= TerrainChunk::SizeY) {
            return;
        }
        const auto chunk{getChunk(glm::ivec2{position.x, position.z})};
        if (chunk == nullptr) {
            return;
        }
        chunk->setBlockAtLocal(
            glm::ivec3{
                position.x - chunk->originXZ()[0],
                position.y,
                position.z - chunk->originXZ()[1],
            },
            block);
    }

    template<typename Callable>
    void forEachChunk(Callable callable)
    {
        for (const auto &[_, chunk] : _chunks) {
            callable(chunk.get());
        }
    }

private:
    template<typename Self>
    static auto getTrunkImpl(Self &self, const glm::ivec2 xz)
    {
        const auto it{self._chunks.find(TerrainChunk::alignToChunkOrigin(xz))};
        if (it == self._chunks.end()) {
            return static_cast<decltype(it->second.get())>(nullptr);
        }
        return it->second.get();
    }

    std::unordered_map<glm::ivec2, std::unique_ptr<TerrainChunk>, IVec2Hash> _chunks;
};

inline const TerrainChunk *Terrain::getChunk(const glm::ivec2 xz) const
{
    return getTrunkImpl(*this, xz);
}

inline TerrainChunk *Terrain::getChunk(const glm::ivec2 xz)
{
    return getTrunkImpl(*this, xz);
}

} // namespace minecraft

#endif // MINECRAFT_TERRAIN_H
