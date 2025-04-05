#include "block_type.h"

auto minecraft::isLiquidBlock(const BlockType block) -> bool
{
    return block == BlockType::Water || block == BlockType::Lava;
}
