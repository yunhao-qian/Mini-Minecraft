#ifndef MINI_MINECRAFT_SCENE_H
#define MINI_MINECRAFT_SCENE_H

#include "player.h"
#include "terrain.h"

namespace minecraft {

class Scene
{
public:
    Scene();

    auto terrain() -> Terrain &;
    auto player() -> Player &;

private:
    Terrain _terrain;
    Player _player;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_SCENE_H
