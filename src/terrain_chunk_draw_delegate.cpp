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
constexpr std::array<glm::ivec2, 4> TextureCoords{{{0, 0}, {1, 0}, {1, 1}, {0, 1}}};

} // namespace

minecraft::TerrainChunkDrawDelegate::TerrainChunkDrawDelegate(GLContext *const context,
                                                              const TerrainChunk *const chunk)
    : _chunk{chunk}
    , _dirty{true}
    , _opaqueBlocksHelper{context}
    , _nonOpaqueBlocksHelper{context}
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

    std::vector<LambertVertex> opaqueVertices;
    std::vector<GLuint> opaqueIndices;
    std::vector<LambertVertex> nonOpaqueVertices;
    std::vector<GLuint> nonOpaqueIndices;

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

                    const auto neighborBlock{
                        _chunk->getNeighborBlockLocal(localX, localY, localZ, direction)};
                    if (isOpaqueBlock(block)) {
                        if (isOpaqueBlock(neighborBlock)) {
                            continue;
                        }
                        vertices = &opaqueVertices;
                        indices = &opaqueIndices;
                    } else {
                        if (neighborBlock != BlockType::Empty) {
                            continue;
                        }
                        vertices = &nonOpaqueVertices;
                        indices = &nonOpaqueIndices;
                    }
                    {
                        const auto indexOffset{static_cast<GLuint>(vertices->size())};
                        for (const auto index : {0u, 1u, 2u, 0u, 2u, 3u}) {
                            indices->push_back(indexOffset + index);
                        }
                    }

                    const auto &vertexPositions{VertexPositions[faceIndex]};

                    glm::ivec2 textureRowColumn;
                    switch (block) {
                    case BlockType::Grass:
                        if (direction == Direction::PositiveY) {
                            textureRowColumn = {2, 8};
                        } else if (direction == Direction::NegativeY) {
                            textureRowColumn = {0, 2};
                        } else {
                            textureRowColumn = {0, 3};
                        }
                        break;
                    case BlockType::Dirt:
                        textureRowColumn = {0, 2};
                        break;
                    case BlockType::Stone:
                        textureRowColumn = {0, 1};
                        break;
                    case BlockType::Water:
                        textureRowColumn = {12, 13};
                        break;
                    case BlockType::Snow:
                        textureRowColumn = {4, 2};
                        break;
                    case BlockType::Lava:
                        textureRowColumn = {14, 13};
                        break;
                    default:
                        textureRowColumn = {10, 8};
                    }
                    const auto textureIndex{
                        static_cast<GLubyte>(textureRowColumn[0] * 16 + textureRowColumn[1])};

                    const auto normal{FaceNormals[faceIndex]};
                    const auto tangent{FaceTangents[faceIndex]};
                    const auto isWater{static_cast<GLubyte>(block == BlockType::Water)};
                    const auto isLava{static_cast<GLubyte>(block == BlockType::Lava)};

                    for (const auto vertexIndex : std::views::iota(0, 4)) {
                        vertices->push_back({
                            .position{blockPosition + vertexPositions[vertexIndex]},
                            .textureIndex = textureIndex,
                            .textureCoords{TextureCoords[vertexIndex]},
                            .normal{normal},
                            .tangent{tangent},
                            .isWater = isWater,
                            .isLava = isLava,
                        });
                    }
                }
            }
        }
    }

    _opaqueBlocksHelper.setVertices(opaqueVertices, GL_STATIC_DRAW);
    _opaqueBlocksHelper.setIndices(opaqueIndices, GL_STATIC_DRAW);
    _nonOpaqueBlocksHelper.setVertices(nonOpaqueVertices, GL_STATIC_DRAW);
    _nonOpaqueBlocksHelper.setIndices(nonOpaqueIndices, GL_STATIC_DRAW);
    _dirty = false;
}

auto minecraft::TerrainChunkDrawDelegate::drawOpaqueBlocks() const -> void
{
    _opaqueBlocksHelper.drawElements(GL_TRIANGLES);
}

auto minecraft::TerrainChunkDrawDelegate::drawNonOpaqueBlocks() const -> void
{
    _nonOpaqueBlocksHelper.drawElements(GL_TRIANGLES);
}
