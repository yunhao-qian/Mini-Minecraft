#ifndef MINI_MINECRAFT_PLAYER_CONTROLLER_H
#define MINI_MINECRAFT_PLAYER_CONTROLLER_H

#include "player.h"

#include "terrain.h"

#include <glm/glm.hpp>

#include <QKeyEvent>
#include <QMouseEvent>

#include <optional>

namespace minecraft {

class PlayerController
{
public:
    PlayerController(Player *const player);

    auto keyPressEvent(const QKeyEvent *const event) -> void;

    auto mousePressEvent(const QMouseEvent *const event, Terrain &terrain) -> void;

private:
    auto rayMarch(const Terrain &terrain,
                  const glm::vec3 &origin,
                  const glm::vec3 &direction,
                  const float minDistance,
                  const float maxDistance) const -> std::optional<glm::ivec3>;

    Player *_player;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_CONTROLLER_H
