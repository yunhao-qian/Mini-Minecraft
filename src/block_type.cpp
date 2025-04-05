#include "block_type.h"

auto minecraft::isOpaqueBlock(const BlockType block) -> bool
{
    return block != BlockType::Empty && block != BlockType::Water;
}

auto minecraft::isLiquidBlock(const BlockType block) -> bool
{
    return block == BlockType::Water;
}
