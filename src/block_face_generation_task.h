#ifndef MINECRAFT_BLOCK_FACE_GENERATION_TASK_H
#define MINECRAFT_BLOCK_FACE_GENERATION_TASK_H

#include "block_type.h"
#include "terrain_chunk.h"
#include "vertex_attribute.h"

#include <glm/glm.hpp>

#include <QRunnable>

#include <array>
#include <vector>

namespace minecraft {

class BlockFaceGenerationTask : public QRunnable
{
public:
    BlockFaceGenerationTask(TerrainChunk *const chunk);

    void run() override;

private:
    void generateBlock(const glm::ivec3 &position);

    TerrainChunk *_chunk;
    std::array<std::array<std::array<BlockType, TerrainChunk::SizeZ + 2>, TerrainChunk::SizeY + 2>,
               TerrainChunk::SizeX + 2>
        _blocks;
    std::array<std::vector<BlockFace>, 4> _blockFaces;
    std::array<glm::ivec3, 4> _blockFaceMinPoints;
    std::array<glm::ivec3, 4> _blockFaceMaxPoints;
};

} // namespace minecraft

#endif // MINECRAFT_BLOCK_FACE_GENERATION_TASK_H
