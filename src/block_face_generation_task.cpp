#include "block_face_generation_task.h"

#include "aligned_box_3d.h"
#include "block_type.h"
#include "constants.h"
#include "direction.h"

#include <limits>
#include <mutex>
#include <ranges>
#include <utility>

namespace minecraft {

BlockFaceGenerationTask::BlockFaceGenerationTask(TerrainChunk *const chunk)
    : _chunk{chunk}
    , _blocks{}
    , _blockFaces{}
    , _blockFaceMinPoints{}
    , _blockFaceMaxPoints{}
{
    // Because we cannot access the block data safely from worker threads, we make a local copy of
    // them in the task constructor.
    for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto y : std::views::iota(0, TerrainChunk::SizeY)) {
            for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
                _blocks[x + 1][y + 1][z + 1] = _chunk->_blocks[x][y][z];
            }
        }
    }
    // Add paddings of 1 block on each side to store blocks from neighboring chunks.
    // X axis
    if (const auto neighbor{_chunk->getNeighbor(Direction::PositiveX)}; neighbor != nullptr) {
        for (const auto y : std::views::iota(0, TerrainChunk::SizeY)) {
            for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
                _blocks.back()[y + 1][z + 1] = neighbor->_blocks.front()[y][z];
            }
        }
    }
    if (const auto neighbor{_chunk->getNeighbor(Direction::NegativeX)}; neighbor != nullptr) {
        for (const auto y : std::views::iota(0, TerrainChunk::SizeY)) {
            for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
                _blocks.front()[y + 1][z + 1] = neighbor->_blocks.back()[y][z];
            }
        }
    }
    // No neighbor chunks along the Y axis
    // Z axis
    if (const auto neighbor{_chunk->getNeighbor(Direction::PositiveZ)}; neighbor != nullptr) {
        for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
            for (const auto y : std::views::iota(0, TerrainChunk::SizeY)) {
                _blocks[x + 1][y + 1].back() = neighbor->_blocks[x][y].front();
            }
        }
    }
    if (const auto neighbor{_chunk->getNeighbor(Direction::NegativeZ)}; neighbor != nullptr) {
        for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
            for (const auto y : std::views::iota(0, TerrainChunk::SizeY)) {
                _blocks[x + 1][y + 1].front() = neighbor->_blocks[x][y].back();
            }
        }
    }
}

void BlockFaceGenerationTask::run()
{
    _blockFaceMinPoints.fill(glm::ivec3{std::numeric_limits<int>::max()});
    _blockFaceMaxPoints.fill(glm::ivec3{std::numeric_limits<int>::min()});

    for (const auto x : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto y : std::views::iota(0, TerrainChunk::SizeY)) {
            for (const auto z : std::views::iota(0, TerrainChunk::SizeZ)) {
                generateBlock(glm::ivec3{x, y, z});
            }
        }
    }

    const std::lock_guard lock{_chunk->_blockFaceMutex};
    _chunk->_isBlockFaceReady = true;
    for (const auto i : std::views::iota(0, 4)) {
        _chunk->_blockFaces[i] = std::move(_blockFaces[i]);
        _chunk->_blockFaceBoundingBoxes[i] = AlignedBox3D{
            glm::vec3{_blockFaceMinPoints[i]},
            glm::vec3{_blockFaceMaxPoints[i]},
        };
    }
}

void BlockFaceGenerationTask::generateBlock(const glm::ivec3 &position)
{
    constexpr auto FaceDirections{std::to_array<glm::ivec3>({
        {1, 0, 0},
        {-1, 0, 0},
        {0, 1, 0},
        {0, -1, 0},
        {0, 0, 1},
        {0, 0, -1},
    })};

    constexpr auto FaceOrigins{std::to_array<glm::ivec3>({
        {1, 0, 1},
        {0, 0, 0},
        {0, 1, 1},
        {0, 0, 0},
        {0, 0, 1},
        {1, 0, 0},
    })};

    const auto block{_blocks[position.x + 1][position.y + 1][position.z + 1]};
    if (block == BlockType::Air) {
        return;
    }

    // Use integer coordinates to avoid floating-point rounding errors, e.g.,
    // float(i) + 1.0f != float(i + 1).
    const glm::ivec3 blockMinPoint{
        _chunk->_originXZ[0] + position.x,
        position.y,
        _chunk->_originXZ[1] + position.z,
    };
    const auto blockMaxPoint{blockMinPoint + 1};

    for (const auto faceIndex : std::views::iota(0, 6)) {
        const auto neighborPosition{position + FaceDirections[faceIndex]};
        const auto neighborBlock{
            _blocks[neighborPosition.x + 1][neighborPosition.y + 1][neighborPosition.z + 1]};

        std::array<bool, 4> blockFaceGroups{false, false, false, false};
        if (block == BlockType::Water) {
            if (neighborBlock != BlockType::Air) {
                continue;
            }
            if (Direction{faceIndex} != Direction::PositiveY) {
                // Because the water wave algorithm in the shader only works for the positive Y
                // face, we skip the other faces to avoid rendering artifacts.
                continue;
            }
            blockFaceGroups[static_cast<int>(BlockFaceGroup::Translucent)] = true;
        } else {
            if (block == BlockType::Lava) {
                if (neighborBlock != BlockType::Air && neighborBlock != BlockType::Water) {
                    continue;
                }
                blockFaceGroups[static_cast<int>(BlockFaceGroup::Opaque)] = true;
            } else {
                if (neighborBlock != BlockType::Air && neighborBlock != BlockType::Water
                    && neighborBlock != BlockType::Lava) {
                    continue;
                }
                blockFaceGroups[static_cast<int>(BlockFaceGroup::Opaque)] = true;
            }
            if (blockMinPoint.y < WaterLevel && neighborBlock == BlockType::Water) {
                blockFaceGroups[static_cast<int>(BlockFaceGroup::UnderWater)] = true;
            }
            if (blockMinPoint.y >= WaterLevel - 1) {
                blockFaceGroups[static_cast<int>(BlockFaceGroup::AboveWater)] = true;
            }
        }

        glm::ivec2 textureRowColumn;
        switch (block) {
        case BlockType::Dirt:
            textureRowColumn = {0, 2};
            break;
        case BlockType::Bedrock:
            textureRowColumn = {1, 1};
            break;
        case BlockType::Grass:
            if (Direction{faceIndex} == Direction::PositiveY) {
                textureRowColumn = {2, 8};
            } else if (Direction{faceIndex} == Direction::NegativeY) {
                textureRowColumn = {0, 2};
            } else {
                textureRowColumn = {0, 3};
            }
            break;
        case BlockType::Lava:
            textureRowColumn = {14, 13};
            break;
        case BlockType::Snow:
            textureRowColumn = {4, 2};
            break;
        case BlockType::Stone:
            textureRowColumn = {0, 1};
            break;
        case BlockType::Water:
            textureRowColumn = {12, 13};
            break;
        default:
            textureRowColumn = {10, 8};
        }

        const BlockFace blockFace{
            .faceOrigin{blockMinPoint + FaceOrigins[faceIndex]},
            .faceIndex = static_cast<GLubyte>(faceIndex),
            .textureIndex = static_cast<GLubyte>(textureRowColumn[0] * 16 + textureRowColumn[1]),
            .blockType = static_cast<std::underlying_type_t<BlockType>>(block),
            .mediumType = static_cast<std::underlying_type_t<BlockType>>(neighborBlock),
        };
        for (const auto i : std::views::iota(0, 4)) {
            if (!blockFaceGroups[i]) {
                continue;
            }
            _blockFaces[i].push_back(blockFace);
            // This is the simplified logic. We can constrain the bounding box to include only the
            // face instead of the whole block, but this is not necessary for now.
            _blockFaceMinPoints[i] = glm::min(_blockFaceMinPoints[i], blockMinPoint);
            _blockFaceMaxPoints[i] = glm::max(_blockFaceMaxPoints[i], blockMaxPoint);
        }
    }
}

} // namespace minecraft
