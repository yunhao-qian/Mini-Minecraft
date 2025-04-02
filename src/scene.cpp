#include "scene.h"

minecraft::Scene::Scene()
    : _terrain{}
    , _player{{{0.0f, 160.0f, 0.0f}}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}
{}

auto minecraft::Scene::terrain() -> Terrain &
{
    return _terrain;
}

auto minecraft::Scene::player() -> Player &
{
    return _player;
}

auto minecraft::Scene::terrainMutex() -> std::mutex &
{
    return _terrainMutex;
}

auto minecraft::Scene::playerMutex() -> std::mutex &
{
    return _playerMutex;
}
