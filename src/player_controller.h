#ifndef MINI_MINECRAFT_PLAYER_CONTROLLER_H
#define MINI_MINECRAFT_PLAYER_CONTROLLER_H

#include "player.h"
#include "terrain.h"

#include <glm/glm.hpp>

#include <QKeyEvent>
#include <QMouseEvent>

namespace minecraft {

class PlayerController
{
public:
    PlayerController(Player *const player);

    void keyPressEvent(const QKeyEvent *const event) const;

    void mousePressEvent(const QMouseEvent *const event, Terrain &terrain) const;

private:
    Player *_player;
};

inline PlayerController::PlayerController(Player *const player)
    : _player{player}
{}

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_CONTROLLER_H
