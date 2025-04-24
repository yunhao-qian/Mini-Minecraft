#ifndef MINECRAFT_IVEC2_HASH_H
#define MINECRAFT_IVEC2_HASH_H

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>

namespace minecraft {

struct IVec2Hash
{
    std::size_t operator()(const glm::ivec2 v) const
    {
        // Assume that all int values fall within the int32_t range.
        // int64_t guarantees the 2's complement representation:
        // https://en.cppreference.com/w/cpp/types/integer
        return std::hash<std::int64_t>{}((static_cast<std::int64_t>(v[0]) << 32)
                                         | (static_cast<std::int64_t>(v[1]) & 0xFFFFFFFF));
    }
};

} // namespace minecraft

#endif // MINECRAFT_IVEC2_HASH_H
