#include "int_pair_hash.h"

#include <functional>

auto minecraft::IntPairHash::operator()(const std::pair<int, int> &pair) const -> std::size_t
{
    return std::hash<std::int64_t>{}((static_cast<std::int64_t>(pair.first) << 32)
                                     | (static_cast<std::int64_t>(pair.second) & 0xFFFFFFFF));
}
