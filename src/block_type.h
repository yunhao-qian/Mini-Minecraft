#ifndef MINI_MINECRAFT_BLOCK_TYPE_H
#define MINI_MINECRAFT_BLOCK_TYPE_H

#include <cstdint>

namespace minecraft {

enum class BlockType : std::uint8_t {
    Empty,
    Grass,
    Dirt,
    Stone,
    Water,
};

} // namespace minecraft

#endif // MINI_MINECRAFT_BLOCK_TYPE_H
