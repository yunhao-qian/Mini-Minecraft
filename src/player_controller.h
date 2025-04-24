#ifndef MINECRAFT_PLAYER_CONTROLLER_H
#define MINECRAFT_PLAYER_CONTROLLER_H

#include "player.h"
#include "terrain.h"

#include <glm/glm.hpp>

#include <QKeyEvent>
#include <QMouseEvent>

namespace minecraft {

class PlayerController
{
public:
    PlayerController(Player *const player)
        : _player{player}
    {}

    void keyPressEvent(const QKeyEvent *const event) const;

    void mousePressEvent(const QMouseEvent *const event, Terrain &terrain) const;

private:
    Player *_player;
};

} // namespace minecraft

#endif // MINECRAFT_PLAYER_CONTROLLER_H
