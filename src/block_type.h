#ifndef MINI_MINECRAFT_BLOCK_TYPE_H
#define MINI_MINECRAFT_BLOCK_TYPE_H

#include <cstdint>

namespace minecraft {

enum class BlockType : std::uint8_t {
    Air = 0,
    Bedrock = 1,
    Dirt = 2,
    Grass = 3,
    Lava = 4,
    Snow = 5,
    Stone = 6,
    Water = 7,
};

} // namespace minecraft

#endif // MINI_MINECRAFT_BLOCK_TYPE_H
