#include "terrain.h"

#include <utility>

namespace minecraft {

void Terrain::setChunk(std::unique_ptr<TerrainChunk> chunk)
{
    const auto originXZ{chunk->originXZ()};
    const auto chunkPointer{chunk.get()};
    _chunks[originXZ] = std::move(chunk);
    if (const auto neighbor{getChunk(originXZ + glm::ivec2{TerrainChunk::SizeX, 0})};
        neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::PositiveX, neighbor);
        neighbor->setNeighbor(Direction::NegativeX, chunkPointer);
    }
    if (const auto neighbor{getChunk(originXZ - glm::ivec2{TerrainChunk::SizeX, 0})};
        neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::NegativeX, neighbor);
        neighbor->setNeighbor(Direction::PositiveX, chunkPointer);
    }
    if (const auto neighbor{getChunk(originXZ + glm::ivec2{0, TerrainChunk::SizeZ})};
        neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::PositiveZ, neighbor);
        neighbor->setNeighbor(Direction::NegativeZ, chunkPointer);
    }
    if (const auto neighbor{getChunk(originXZ - glm::ivec2{0, TerrainChunk::SizeZ})};
        neighbor != nullptr) {
        chunkPointer->setNeighbor(Direction::NegativeZ, neighbor);
        neighbor->setNeighbor(Direction::PositiveZ, chunkPointer);
    }
}

} // namespace minecraft
