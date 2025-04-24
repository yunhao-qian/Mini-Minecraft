#ifndef MINECRAFT_ENTITY_H
#define MINECRAFT_ENTITY_H

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
           const MovementMode movementMode)
        : _position{position}
        , _velocity{velocity}
        , _acceleration{acceleration}
        , _movementMode{movementMode}
        , _previousAcceleration{0.0f}
    {}

    Entity(const Entity &) = delete;
    Entity(Entity &&) = delete;

    virtual ~Entity() = default;

    Entity &operator=(const Entity &) = delete;
    Entity &operator=(Entity &&) = delete;

    const glm::vec3 &position() const { return _position; }

    void setPosition(const glm::vec3 &position) { _position = position; }

    const glm::vec3 &velocity() const { return _velocity; }

    void setVelocity(const glm::vec3 &velocity) { _velocity = velocity; }

    const glm::vec3 &acceleration() const { return _acceleration; }

    void setAcceleration(const glm::vec3 &acceleration) { _acceleration = acceleration; }

    MovementMode movementMode() const { return _movementMode; }

    void setMovementMode(const MovementMode movementMode) { _movementMode = movementMode; }

    const glm::vec3 &previousAcceleration() const { return _previousAcceleration; }

    virtual AlignedBox3D boxCollider() const = 0;

    virtual void updatePhysics(const float dT, const Terrain &terrain);

private:
    BlockType getBlockAtCurrentPosition(const Terrain &terrain) const
    {
        const glm::ivec3 blockPosition{glm::floor(_position)};
        return terrain.getBlockAtGlobal(blockPosition);
    }

    void simulateWithTerrainCollisions(const float dT, const Terrain &terrain);

    bool isCloseToGround(const Terrain &terrain) const;

    glm::vec3 _position;
    glm::vec3 _velocity;
    glm::vec3 _acceleration;
    MovementMode _movementMode;
    glm::vec3 _previousAcceleration;
};

} // namespace minecraft

#endif // MINECRAFT_ENTITY_H
