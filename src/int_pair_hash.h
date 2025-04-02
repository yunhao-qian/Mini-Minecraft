#ifndef MINI_MINECRAFT_INT_PAIR_HASH_H
#define MINI_MINECRAFT_INT_PAIR_HASH_H

#include <cstddef>
#include <utility>

namespace minecraft {

struct IntPairHash
{
    auto operator()(const std::pair<int, int> &pair) const -> std::size_t;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_INT_PAIR_HASH_H
