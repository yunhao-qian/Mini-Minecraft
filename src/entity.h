#ifndef MINI_MINECRAFT_ENTITY_H
#define MINI_MINECRAFT_ENTITY_H

#include "aligned_box_3d.h"
#include "block_type.h"
#include "movement_mode.h"
#include "terrain.h"

#include <glm/glm.hpp>

namespace minecraft {

class Entity
{
public:
    Entity(const glm::vec3 &position,
           const glm::vec3 &velocity,
           const glm::vec3 &acceleration,
           const MovementMode);
    Entity(const Entity &) = delete;
    Entity(Entity &&) = delete;

    virtual ~Entity() = default;

    Entity &operator=(const Entity &) = delete;
    Entity &operator=(Entity &&) = delete;

    const glm::vec3 &position() const;
    void setPosition(const glm::vec3 &position);

    const glm::vec3 &velocity() const;
    void setVelocity(const glm::vec3 &velocity);

    const glm::vec3 &acceleration() const;
    void setAcceleration(const glm::vec3 &acceleration);

    MovementMode movementMode() const;
    void setMovementMode(const MovementMode movementMode);

    const glm::vec3 &previousAcceleration() const;

    virtual AlignedBox3D boxCollider() const = 0;

    virtual void updatePhysics(const float dT, const Terrain &terrain);

private:
    BlockType getBlockAtCurrentPosition(const Terrain &terrain) const;

    void simulateWithTerrainCollisions(const float dT, const Terrain &terrain);

    bool isCloseToGround(const Terrain &terrain) const;

    glm::vec3 _position;
    glm::vec3 _velocity;
    glm::vec3 _acceleration;
    MovementMode _movementMode;
    glm::vec3 _previousAcceleration;
};

inline Entity::Entity(const glm::vec3 &position,
                      const glm::vec3 &velocity,
                      const glm::vec3 &acceleration,
                      const MovementMode movementMode)
    : _position{position}
    , _velocity{velocity}
    , _acceleration{acceleration}
    , _movementMode{movementMode}
    , _previousAcceleration{0.0f, 0.0f, 0.0f}
{}

inline const glm::vec3 &Entity::position() const
{
    return _position;
}

inline void Entity::setPosition(const glm::vec3 &position)
{
    _position = position;
}

inline const glm::vec3 &Entity::velocity() const
{
    return _velocity;
}

inline void Entity::setVelocity(const glm::vec3 &velocity)
{
    _velocity = velocity;
}

inline const glm::vec3 &Entity::acceleration() const
{
    return _acceleration;
}

inline void Entity::setAcceleration(const glm::vec3 &acceleration)
{
    _acceleration = acceleration;
}

inline MovementMode Entity::movementMode() const
{
    return _movementMode;
}

inline void Entity::setMovementMode(const MovementMode movementMode)
{
    _movementMode = movementMode;
}

inline const glm::vec3 &Entity::previousAcceleration() const
{
    return _previousAcceleration;
}

inline BlockType Entity::getBlockAtCurrentPosition(const Terrain &terrain) const
{
    const glm::ivec3 blockPosition{glm::floor(_position)};
    return terrain.getBlockAtGlobal(blockPosition);
}

} // namespace minecraft

#endif // MINI_MINECRAFT_ENTITY_H
