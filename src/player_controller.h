#ifndef MINI_MINECRAFT_PLAYER_CONTROLLER_H
#define MINI_MINECRAFT_PLAYER_CONTROLLER_H

#include "player.h"

#include <QKeyEvent>

namespace minecraft {

class PlayerController
{
public:
    PlayerController(Player *const player);

    auto keyPressEvent(const QKeyEvent *const event) -> void;

private:
    Player *_player;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_CONTROLLER_H
