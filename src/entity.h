#ifndef MINI_MINECRAFT_ENTITY_H
#define MINI_MINECRAFT_ENTITY_H

#include "pose.h"

namespace minecraft {

class Entity
{
public:
    Entity(const Pose &pose, const glm::vec3 &velocity);
    virtual ~Entity() = default;

    auto pose() const -> const Pose &;
    auto setPose(const Pose &pose) -> void;

    auto velocity() const -> const glm::vec3 &;
    auto setVelocity(const glm::vec3 &velocity) -> void;

protected:
    Pose _pose;
    glm::vec3 _velocity;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_ENTITY_H
