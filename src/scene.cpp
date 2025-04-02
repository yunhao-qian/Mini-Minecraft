#include "scene.h"

minecraft::Scene::Scene()
    : _terrain{}
    , _player{{{0.0f, 128.0f, 0.0f}}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}
{}

auto minecraft::Scene::terrain() -> Terrain &
{
    return _terrain;
}

auto minecraft::Scene::player() -> Player &
{
    return _player;
}
