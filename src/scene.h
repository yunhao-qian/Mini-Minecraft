#ifndef MINI_MINECRAFT_SCENE_H
#define MINI_MINECRAFT_SCENE_H

#include "player.h"
#include "terrain.h"

#include <mutex>

namespace minecraft {

class Scene
{
public:
    Scene();

    auto terrain() -> Terrain &;
    auto player() -> Player &;

    auto terrainMutex() -> std::mutex &;
    auto playerMutex() -> std::mutex &;

private:
    Terrain _terrain;
    Player _player;

    // TODO: Slots of the GLWidget class seem to be called from different threads. Use mutexes to
    // synchronize access for now.
    std::mutex _terrainMutex;
    std::mutex _playerMutex;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_SCENE_H
