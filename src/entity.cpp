#include "entity.h"

minecraft::Entity::Entity(const Pose &pose, const glm::vec3 &velocity, const glm::vec3 &acceleration)
    : _pose{pose}
    , _velocity{velocity}
    , _acceleration{acceleration}
{}

auto minecraft::Entity::pose() const -> const Pose &
{
    return _pose;
}

auto minecraft::Entity::setPose(const Pose &pose) -> void
{
    _pose = pose;
}

auto minecraft::Entity::velocity() const -> const glm::vec3 &
{
    return _velocity;
}

auto minecraft::Entity::setVelocity(const glm::vec3 &velocity) -> void
{
    _velocity = velocity;
}

auto minecraft::Entity::acceleration() const -> const glm::vec3 &
{
    return _acceleration;
}

auto minecraft::Entity::setAcceleration(const glm::vec3 &acceleration) -> void
{
    _acceleration = acceleration;
}
