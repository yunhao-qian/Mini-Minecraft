#ifndef MINECRAFT_TERRAIN_CHUNK_H
#define MINECRAFT_TERRAIN_CHUNK_H

#include "aligned_box_3d.h"
#include "block_type.h"
#include "direction.h"
#include "instanced_renderer.h"
#include "vertex_attribute.h"

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

namespace minecraft {

enum class BlockFaceGroup : int {
    Opaque = 0,
    Translucent = 1,
    AboveWater = 2,
    UnderWater = 3,
};

class TerrainChunk
{
public:
    TerrainChunk(const glm::ivec2 originXZ)
        : _originXZ{originXZ}
        , _neighbors{}
        , _blocks{}
        , _blockVersion{0}
        , _isVisible{false}
        , _blockFaceMutex{}
        , _isBlockFaceReady{false}
        , _blockFaces{}
        , _blockFaceBoundingBoxes{}
        , _blockFaceVersion{-1}
        , _renderers{}
        , _rendererBoundingBoxes{}
        , _rendererVersion{-1}
    {}

    glm::ivec2 originXZ() const { return _originXZ; }

    const TerrainChunk *getNeighbor(const Direction direction) const;
    TerrainChunk *getNeighbor(const Direction direction);

    void setNeighbor(const Direction direction, TerrainChunk *const chunk);

    BlockType getBlockAtLocal(const glm::ivec3 &position) const
    {
        return _blocks[position.x][position.y][position.z];
    }

    void setBlockAtLocal(const glm::ivec3 &position, const BlockType block)
    {
        // We do not increment the block version here because this makes terrain generation very
        // inefficient. Users are responsible for calling markSelfDirty() or
        // markSelfAndNeighborsDirty() after modifications.
        _blocks[position.x][position.y][position.z] = block;
    }

    bool isVisible() const { return _isVisible; }

    void setVisible(const bool visible) { _isVisible = visible; }

    void markSelfDirty() { ++_blockVersion; }

    void markSelfAndNeighborsDirty()
    {
        markSelfDirty();
        for (const auto neighbor : _neighbors) {
            if (neighbor != nullptr) {
                neighbor->markSelfDirty();
            }
        }
    }

    void prepareDraw();

    const AlignedBox3D &rendererBoundingBox(const BlockFaceGroup group) const
    {
        return _rendererBoundingBoxes[static_cast<int>(group)];
    }

    void draw(const BlockFaceGroup group)
    {
        // This gives 2 triangles per quad: (0, 1, 2) and (0, 2, 3).
        _renderers[static_cast<int>(group)].draw(4, GL_TRIANGLE_FAN);
    }

    void releaseRendererResources()
    {
        for (auto &renderer : _renderers) {
            renderer.releaseResources();
        }
        _rendererVersion = -1;
    }

    AlignedBox3D boundingBox() const
    {
        return {
            glm::vec3{_originXZ[0], 0.0f, _originXZ[1]},
            glm::vec3{_originXZ[0] + SizeX, SizeY, _originXZ[1] + SizeZ},
        };
    }

    static glm::ivec2 alignToChunkOrigin(const glm::ivec2 xz)
    {
        const auto x{xz[0]};
        const auto z{xz[1]};
        const auto alignedX{(x >= 0 ? x : x - (SizeX - 1)) / SizeX * SizeX};
        const auto alignedZ{(z >= 0 ? z : z - (SizeZ - 1)) / SizeZ * SizeZ};
        return {alignedX, alignedZ};
    }

    static constexpr int SizeX{64};
    static constexpr int SizeY{256};
    static constexpr int SizeZ{64};

private:
    friend class BlockFaceGenerationTask;

    template<typename Self>
    static auto getNeighborPointer(Self &self, const Direction direction)
    {
        switch (direction) {
        case Direction::PositiveX:
            return &self._neighbors[0];
        case Direction::NegativeX:
            return &self._neighbors[1];
        case Direction::PositiveZ:
            return &self._neighbors[2];
        case Direction::NegativeZ:
            return &self._neighbors[3];
        default:
            return static_cast<decltype(&self._neighbors[0])>(nullptr);
        }
    }

    glm::ivec2 _originXZ;
    std::array<TerrainChunk *, 4> _neighbors;

    std::array<std::array<std::array<BlockType, SizeZ>, SizeY>, SizeX> _blocks;
    std::int32_t _blockVersion;

    bool _isVisible;

    std::mutex _blockFaceMutex;
    bool _isBlockFaceReady;
    std::array<std::vector<BlockFace>, 4> _blockFaces;
    std::array<AlignedBox3D, 4> _blockFaceBoundingBoxes;
    std::int32_t _blockFaceVersion;

    std::array<InstancedRenderer, 4> _renderers;
    std::array<AlignedBox3D, 4> _rendererBoundingBoxes;
    std::int32_t _rendererVersion;
};

inline const TerrainChunk *TerrainChunk::getNeighbor(const Direction direction) const
{
    return *getNeighborPointer(*this, direction);
}

inline TerrainChunk *TerrainChunk::getNeighbor(const Direction direction)
{
    return *getNeighborPointer(*this, direction);
}

inline void TerrainChunk::setNeighbor(const Direction direction, TerrainChunk *const chunk)
{
    *getNeighborPointer(*this, direction) = chunk;
}

} // namespace minecraft

#endif // MINECRAFT_TERRAIN_CHUNK_H
