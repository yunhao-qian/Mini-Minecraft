#ifndef MINI_MINECRAFT_BLOCK_TYPE_H
#define MINI_MINECRAFT_BLOCK_TYPE_H

#include <cstdint>

namespace minecraft {

enum class BlockType : std::uint8_t {
    Air = 0,
    Dirt = 1,
    Grass = 2,
    Lava = 3,
    Snow = 4,
    Stone = 5,
    Water = 6,
};

} // namespace minecraft

#endif // MINI_MINECRAFT_BLOCK_TYPE_H
