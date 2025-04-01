#ifndef MINI_MINECRAFT_TERRAIN_GENERATOR_H
#define MINI_MINECRAFT_TERRAIN_GENERATOR_H

#include "terrain.h"

#include <glm/glm.hpp>

namespace minecraft {

class TerrainGenerator
{
public:
    TerrainGenerator(Terrain *const terrain);

    auto generateTerrainAround(const glm::vec3 &center, const float radius) -> void;

private:
    Terrain *_terrain;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_TERRAIN_GENERATOR_H
