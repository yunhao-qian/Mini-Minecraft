#include "terrain_chunk_draw_delegate.h"

#include "direction.h"
#include "terrain_chunk.h"

#include <glm/glm.hpp>

#include <array>
#include <ranges>
#include <utility>

namespace {

constexpr std::array<std::array<glm::ivec3, 4>, 6> VertexPositions{{
    {{{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}},
    {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}},
    {{{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}},
    {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},
    {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
    {{{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}},
}};

constexpr std::array<glm::ivec3, 6> FaceNormals{
    {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
constexpr std::array<glm::ivec3, 6> FaceTangents{
    {{0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}}};

constexpr std::array<glm::ivec2, 4> GrassTopTextureCoords{{{8, 13}, {9, 13}, {9, 14}, {8, 14}}};
constexpr std::array<glm::ivec2, 4> GrassSideTextureCoords{{{3, 15}, {4, 15}, {4, 16}, {3, 16}}};

constexpr std::array<glm::ivec2, 4> DirtTextureCoords{{{2, 15}, {3, 15}, {3, 16}, {2, 16}}};
constexpr std::array<glm::ivec2, 4> StoneTextureCoords{{{1, 15}, {2, 15}, {2, 16}, {1, 16}}};
constexpr std::array<glm::ivec2, 4> WaterTextureCoords{{{13, 3}, {14, 3}, {14, 4}, {13, 4}}};
constexpr std::array<glm::ivec2, 4> SnowTextureCoords{{{2, 11}, {3, 11}, {3, 12}, {2, 12}}};
constexpr std::array<glm::ivec2, 4> UnknownTextureCoords{{{8, 5}, {9, 5}, {9, 6}, {8, 6}}};

auto isLiquidBlock(const minecraft::BlockType block) -> bool
{
    using minecraft::BlockType;

    return block == BlockType::Water;
}

} // namespace

minecraft::TerrainChunkDrawDelegate::TerrainChunkDrawDelegate(GLContext *const context,
                                                              const TerrainChunk *const chunk)
    : _chunk{chunk}
    , _dirty{true}
    , _solidBlocksHelper{context}
    , _liquidBlocksHelper{context}
{}

auto minecraft::TerrainChunkDrawDelegate::markDirty() -> void
{
    _dirty = true;
}

auto minecraft::TerrainChunkDrawDelegate::prepareDraw() -> void
{
    if (!_dirty) {
        return;
    }

    std::vector<LambertVertex> solidVertices;
    std::vector<GLuint> solidIndices;
    std::vector<LambertVertex> liquidVertices;
    std::vector<GLuint> liquidIndices;

    for (const auto localX : std::views::iota(0, TerrainChunk::SizeX)) {
        for (const auto localY : std::views::iota(0, TerrainChunk::SizeY)) {
            for (const auto localZ : std::views::iota(0, TerrainChunk::SizeZ)) {
                const auto block{_chunk->getBlockLocal(localX, localY, localZ)};
                if (block == BlockType::Empty) {
                    continue;
                }

                const glm::ivec3 blockPosition{_chunk->minX() + localX,
                                               localY,
                                               _chunk->minZ() + localZ};

                for (const auto &[direction, faceIndex] : {
                         std::pair{Direction::PositiveX, 0},
                         std::pair{Direction::NegativeX, 1},
                         std::pair{Direction::PositiveY, 2},
                         std::pair{Direction::NegativeY, 3},
                         std::pair{Direction::PositiveZ, 4},
                         std::pair{Direction::NegativeZ, 5},
                     }) {
                    std::vector<LambertVertex> *vertices;
                    std::vector<GLuint> *indices;

                    if (const auto neighborBlock{
                            _chunk->getNeighborBlockLocal(localX, localY, localZ, direction)};
                        isLiquidBlock(block)) {
                        vertices = &liquidVertices;
                        indices = &liquidIndices;
                        if (neighborBlock != BlockType::Empty) {
                            continue;
                        }
                    } else {
                        vertices = &solidVertices;
                        indices = &solidIndices;
                        if (neighborBlock != BlockType::Empty && !isLiquidBlock(neighborBlock)) {
                            continue;
                        }
                    }
                    {
                        const auto indexOffset{static_cast<GLuint>(vertices->size())};
                        for (const auto index : {0u, 1u, 2u, 0u, 2u, 3u}) {
                            indices->push_back(indexOffset + index);
                        }
                    }

                    const auto &vertexPositions{VertexPositions[faceIndex]};

                    const std::array<glm::ivec2, 4> *textureCoords{nullptr};
                    switch (block) {
                    case BlockType::Grass:
                        if (direction == Direction::PositiveY) {
                            textureCoords = &GrassTopTextureCoords;
                        } else if (direction == Direction::NegativeY) {
                            textureCoords = &DirtTextureCoords;
                        } else {
                            textureCoords = &GrassSideTextureCoords;
                        }
                        break;
                    case BlockType::Dirt:
                        textureCoords = &DirtTextureCoords;
                        break;
                    case BlockType::Stone:
                        textureCoords = &StoneTextureCoords;
                        break;
                    case BlockType::Water:
                        textureCoords = &WaterTextureCoords;
                        break;
                    case BlockType::Snow:
                        textureCoords = &SnowTextureCoords;
                        break;
                    default:
                        textureCoords = &UnknownTextureCoords;
                    }

                    const auto normal{FaceNormals[faceIndex]};
                    const auto tangent{FaceTangents[faceIndex]};

                    for (const auto vertexIndex : std::views::iota(0, 4)) {
                        vertices->push_back({
                            .position{blockPosition + vertexPositions[vertexIndex]},
                            .textureCoords{glm::vec2{(*textureCoords)[vertexIndex]} / 16.0f},
                            .normal{normal},
                            .tangent{tangent},
                        });
                    }
                }
            }
        }
    }

    _solidBlocksHelper.setVertices(solidVertices, GL_STATIC_DRAW);
    _solidBlocksHelper.setIndices(solidIndices, GL_STATIC_DRAW);
    _liquidBlocksHelper.setVertices(liquidVertices, GL_STATIC_DRAW);
    _liquidBlocksHelper.setIndices(liquidIndices, GL_STATIC_DRAW);
    _dirty = false;
}

auto minecraft::TerrainChunkDrawDelegate::drawSolidBlocks() const -> void
{
    _solidBlocksHelper.drawElements(GL_TRIANGLES);
}

auto minecraft::TerrainChunkDrawDelegate::drawLiquidBlocks() const -> void
{
    _liquidBlocksHelper.drawElements(GL_TRIANGLES);
}
