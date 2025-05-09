#ifndef MINECRAFT_PLAYER_INFO_DISPLAY_DATA_H
#define MINECRAFT_PLAYER_INFO_DISPLAY_DATA_H

#include <glm/glm.hpp>

namespace minecraft {

struct PlayerInfoDisplayData
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 lookVector;
    glm::ivec2 chunk;
    int terrainZone;
};

} // namespace minecraft

#endif // MINECRAFT_PLAYER_INFO_DISPLAY_DATA_H
