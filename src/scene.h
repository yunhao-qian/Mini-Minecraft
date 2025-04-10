#ifndef MINI_MINECRAFT_SCENE_H
#define MINI_MINECRAFT_SCENE_H

#include "player.h"
#include "pose.h"
#include "terrain.h"

#include <glm/glm.hpp>

#include <mutex>

namespace minecraft {

class Scene
{
public:
    Scene();

    Terrain &terrain();
    Player &player();

    std::mutex &terrainMutex();
    std::mutex &playerMutex();

private:
    Terrain _terrain;
    Player _player;

    std::mutex _terrainMutex;
    std::mutex _playerMutex;
};

inline Scene::Scene()
    : _terrain{}
    , _player{Pose{glm::vec3{0.0f, 160.0f, 0.0f}}}
{}

inline Terrain &Scene::terrain()
{
    return _terrain;
}

inline Player &Scene::player()
{
    return _player;
}

inline std::mutex &Scene::terrainMutex()
{
    return _terrainMutex;
}

inline std::mutex &Scene::playerMutex()
{
    return _playerMutex;
}

} // namespace minecraft

#endif // MINI_MINECRAFT_SCENE_H
