#include "terrain_chunk.h"

minecraft::TerrainChunk::TerrainChunk(GLContext *const context, const int minX, const int minZ)
    : _context{context}
    , _minX{minX}
    , _minZ{minZ}
    , _blocks{}
    , _neighbors{}
    , _visible{false}
    , _drawDelegate{nullptr}
{}

auto minecraft::TerrainChunk::minX() const -> int
{
    return _minX;
}

auto minecraft::TerrainChunk::minZ() const -> int
{
    return _minZ;
}

auto minecraft::TerrainChunk::getBlockLocal(const int x, const int y, const int z) const
    -> BlockType
{
    return _blocks[x][y][z];
}

auto minecraft::TerrainChunk::setBlockLocal(const int x,
                                            const int y,
                                            const int z,
                                            const BlockType block) -> void
{
    _blocks[x][y][z] = block;
}

auto minecraft::TerrainChunk::getNeighbor(const Direction direction) const -> const TerrainChunk *
{
    return *getNeighborPointer(*this, direction);
}

auto minecraft::TerrainChunk::getNeighbor(const Direction direction) -> TerrainChunk *
{
    return *getNeighborPointer(*this, direction);
}

auto minecraft::TerrainChunk::setNeighbor(const Direction direction, TerrainChunk *const chunk)
    -> void
{
    *getNeighborPointer(*this, direction) = chunk;
}

auto minecraft::TerrainChunk::getNeighborBlockLocal(const int x,
                                                    const int y,
                                                    const int z,
                                                    const Direction direction) const -> BlockType
{
    auto localX{x};
    auto localY{y};
    auto localZ{z};
    auto chunk{this};

    switch (direction) {
    case Direction::PositiveX:
        if (++localX >= SizeX) {
            localX -= SizeX;
            chunk = getNeighbor(Direction::PositiveX);
        }
        break;
    case Direction::NegativeX:
        if (--localX < 0) {
            localX += SizeX;
            chunk = getNeighbor(Direction::NegativeX);
        }
        break;
    case Direction::PositiveY:
        if (++localY >= SizeY) {
            chunk = nullptr;
        }
        break;
    case Direction::NegativeY:
        if (--localY < 0) {
            chunk = nullptr;
        }
        break;
    case Direction::PositiveZ:
        if (++localZ >= SizeZ) {
            localZ -= SizeZ;
            chunk = getNeighbor(Direction::PositiveZ);
        }
        break;
    case Direction::NegativeZ:
        if (--localZ < 0) {
            localZ += SizeZ;
            chunk = getNeighbor(Direction::NegativeZ);
        }
        break;
    }

    if (chunk == nullptr) {
        return BlockType::Empty;
    }
    return chunk->getBlockLocal(localX, localY, localZ);
}

auto minecraft::TerrainChunk::isVisible() const -> bool
{
    return _visible;
}

auto minecraft::TerrainChunk::setVisible(const bool visible) -> void
{
    _visible = visible;
}

auto minecraft::TerrainChunk::markDirty() -> void
{
    if (_drawDelegate != nullptr) {
        _drawDelegate->markDirty();
    }
}

auto minecraft::TerrainChunk::markSelfAndNeighborsDirty() -> void
{
    markDirty();
    for (const auto &neighbor : _neighbors) {
        if (neighbor != nullptr) {
            neighbor->markDirty();
        }
    }
}

auto minecraft::TerrainChunk::prepareDraw() -> void
{
    if (!_visible) {
        return;
    }
    if (_drawDelegate == nullptr) {
        _drawDelegate = std::make_unique<TerrainChunkDrawDelegate>(_context, this);
    }
    _drawDelegate->prepareDraw();
}

auto minecraft::TerrainChunk::drawOpaqueBlocks() -> void
{
    if (_visible && _drawDelegate != nullptr) {
        _drawDelegate->drawOpaqueBlocks();
    }
}

auto minecraft::TerrainChunk::drawNonOpaqueBlocks() -> void
{
    if (_visible && _drawDelegate != nullptr) {
        _drawDelegate->drawNonOpaqueBlocks();
    }
}

auto minecraft::TerrainChunk::releaseDrawDelegate() -> void
{
    _drawDelegate.reset();
}

auto minecraft::TerrainChunk::alignToChunkOrigin(const int x, const int z) -> std::pair<int, int>
{
    return {
        (x >= 0 ? x : x - (SizeX - 1)) / SizeX * SizeX,
        (z >= 0 ? z : z - (SizeZ - 1)) / SizeZ * SizeZ,
    };
}
