#ifndef MINECRAFT_SCENE_H
#define MINECRAFT_SCENE_H

#include "player.h"
#include "pose.h"
#include "terrain.h"

#include <glm/glm.hpp>

#include <mutex>

namespace minecraft {

class Scene
{
public:
    Scene()
        : _terrain{}
        , _player{Pose{glm::vec3{0.0f, 160.0f, 0.0f}}}
    {}

    Terrain &terrain() { return _terrain; }

    Player &player() { return _player; }

    std::mutex &terrainMutex() { return _terrainMutex; }

    std::mutex &playerMutex() { return _playerMutex; }

private:
    Terrain _terrain;
    Player _player;

    std::mutex _terrainMutex;
    std::mutex _playerMutex;
};

} // namespace minecraft

#endif // MINECRAFT_SCENE_H
