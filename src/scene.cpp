#include "scene.h"

minecraft::Scene::Scene(GLContext *const context)
    : _terrain{context}
    , _player{{}, {0.0f, 0.0f, 0.0f}}
{}

auto minecraft::Scene::terrain() -> Terrain &
{
    return _terrain;
}

auto minecraft::Scene::player() -> Player &
{
    return _player;
}
