#ifndef MINI_MINECRAFT_ENTITY_H
#define MINI_MINECRAFT_ENTITY_H

#include "pose.h"

#include <glm/glm.hpp>

namespace minecraft {

class Entity
{
public:
    Entity(const Pose &pose, const glm::vec3 &velocity, const glm::vec3 &acceleration);
    virtual ~Entity() = default;

    auto pose() const -> const Pose &;
    auto setPose(const Pose &pose) -> void;

    auto velocity() const -> const glm::vec3 &;
    auto setVelocity(const glm::vec3 &velocity) -> void;

    auto acceleration() const -> const glm::vec3 &;
    auto setAcceleration(const glm::vec3 &acceleration) -> void;

protected:
    Pose _pose;
    glm::vec3 _velocity;
    glm::vec3 _acceleration;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_ENTITY_H
