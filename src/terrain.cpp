#include "terrain.h"

#include <cstdint>

minecraft::Terrain::Terrain(GLContext *const context)
    : _context{context}
    , _chunks{}
{}

auto minecraft::Terrain::getChunk(const int x, const int z) const -> const TerrainChunk *
{
    return getChunk(*this, x, z);
}

auto minecraft::Terrain::getChunk(const int x, const int z) -> TerrainChunk *
{
    return getChunk(*this, x, z);
}

auto minecraft::Terrain::getOrCreateChunk(const int x, const int z) -> TerrainChunk *
{
    const auto key{TerrainChunk::alignToChunkOrigin(x, z)};
    if (const auto it{_chunks.find(key)}; it != _chunks.end()) {
        return it->second.get();
    }
    const auto chunk{
        _chunks.emplace(key, std::make_unique<TerrainChunk>(_context, key.first, key.second))
            .first->second.get()};
    if (const auto neighbor{getChunk(key.first + TerrainChunk::SizeX, key.second)};
        neighbor != nullptr) {
        chunk->setNeighbor(Direction::PositiveX, neighbor);
        neighbor->setNeighbor(Direction::NegativeX, chunk);
    }
    if (const auto neighbor{getChunk(key.first - TerrainChunk::SizeX, key.second)};
        neighbor != nullptr) {
        chunk->setNeighbor(Direction::NegativeX, neighbor);
        neighbor->setNeighbor(Direction::PositiveX, chunk);
    }
    if (const auto neighbor{getChunk(key.first, key.second + TerrainChunk::SizeZ)};
        neighbor != nullptr) {
        chunk->setNeighbor(Direction::PositiveZ, neighbor);
        neighbor->setNeighbor(Direction::NegativeZ, chunk);
    }
    if (const auto neighbor{getChunk(key.first, key.second - TerrainChunk::SizeZ)};
        neighbor != nullptr) {
        chunk->setNeighbor(Direction::NegativeZ, neighbor);
        neighbor->setNeighbor(Direction::PositiveZ, chunk);
    }
    return chunk;
}

auto minecraft::Terrain::getBlockGlobal(const int x, const int y, const int z) const -> BlockType
{
    const auto chunk{getChunk(x, z)};
    if (chunk == nullptr) {
        return BlockType::Empty;
    }
    return chunk->getBlockLocal(x - chunk->minX(), y, z - chunk->minZ());
}

auto minecraft::Terrain::setBlockGlobal(const int x, const int y, const int z, const BlockType block)
    -> void
{
    const auto chunk{getOrCreateChunk(x, z)};
    chunk->setBlockLocal(x - chunk->minX(), y, z - chunk->minZ(), block);
}

auto minecraft::Terrain::draw() -> void
{
    for (const auto &[_, chunk] : _chunks) {
        chunk->draw();
    }
}

auto minecraft::Terrain::ChunkKeyHash::operator()(const std::pair<int, int> &key) const
    -> std::size_t
{
    return std::hash<std::int64_t>()((static_cast<std::int64_t>(key.first) << 32)
                                     | (static_cast<std::int64_t>(key.second) & 0xFFFFFFFF));
}
