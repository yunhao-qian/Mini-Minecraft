#ifndef TERRAIN_CHUNK_DRAW_DELEGATE_H
#define TERRAIN_CHUNK_DRAW_DELEGATE_H

#include "gl_context.h"
#include "vertex.h"
#include "vertex_array_helper.h"

namespace minecraft {

class TerrainChunk;

class TerrainChunkDrawDelegate
{
public:
    TerrainChunkDrawDelegate(GLContext *const context, const TerrainChunk *const chunk);

    auto markDirty() -> void;

    auto prepareDraw() -> void;

    auto drawSolidBlocks() const -> void;
    auto drawLiquidBlocks() const -> void;

private:
    const TerrainChunk *_chunk;
    bool _dirty;
    VertexArrayHelper<LambertVertex> _solidBlocksHelper;
    VertexArrayHelper<LambertVertex> _liquidBlocksHelper;
};

} // namespace minecraft

#endif // TERRAIN_CHUNK_DRAW_DELEGATE_H
