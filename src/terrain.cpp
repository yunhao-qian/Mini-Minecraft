#include "terrain.h"

#include <utility>

auto minecraft::Terrain::getChunk(const int x, const int z) const -> const TerrainChunk *
{
    return getChunk(*this, x, z);
}

auto minecraft::Terrain::getChunk(const int x, const int z) -> TerrainChunk *
{
    return getChunk(*this, x, z);
}

auto minecraft::Terrain::setChunk(std::unique_ptr<TerrainChunk> chunk) -> void
{
    const auto minX{chunk->minX()};
    const auto minZ{chunk->minZ()};
    const auto chunkPointer{chunk.get()};
    _chunks[{minX, minZ}] = std::move(chunk);
    if (const auto neighbor{getChunk(minX + TerrainChunk::SizeX, minZ)}; neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::PositiveX, neighbor);
        neighbor->setNeighbor(Direction::NegativeX, chunkPointer);
    }
    if (const auto neighbor{getChunk(minX - TerrainChunk::SizeX, minZ)}; neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::NegativeX, neighbor);
        neighbor->setNeighbor(Direction::PositiveX, chunkPointer);
    }
    if (const auto neighbor{getChunk(minX, minZ + TerrainChunk::SizeZ)}; neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::PositiveZ, neighbor);
        neighbor->setNeighbor(Direction::NegativeZ, chunkPointer);
    }
    if (const auto neighbor{getChunk(minX, minZ - TerrainChunk::SizeZ)}; neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::NegativeZ, neighbor);
        neighbor->setNeighbor(Direction::PositiveZ, chunkPointer);
    }
}

auto minecraft::Terrain::getBlockGlobal(const int x, const int y, const int z) const -> BlockType
{
    if (y < 0 || y >= TerrainChunk::SizeY) {
        return BlockType::Empty;
    }
    const auto chunk{getChunk(x, z)};
    if (chunk == nullptr) {
        return BlockType::Empty;
    }
    return chunk->getBlockLocal(x - chunk->minX(), y, z - chunk->minZ());
}

auto minecraft::Terrain::setBlockGlobal(const int x, const int y, const int z, const BlockType block)
    -> void
{
    if (y < 0 || y >= TerrainChunk::SizeY) {
        return;
    }
    const auto chunk{getChunk(x, z)};
    if (chunk == nullptr) {
        return;
    }
    chunk->setBlockLocal(x - chunk->minX(), y, z - chunk->minZ(), block);
}

auto minecraft::Terrain::draw() -> void
{
    for (const auto &[_, chunk] : _chunks) {
        chunk->draw();
    }
}
