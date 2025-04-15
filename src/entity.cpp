#include "entity.h"
#include "terrain.h"

#include <cmath>
#include <ranges>

namespace minecraft {

void Entity::updatePhysics(const float dT, const Terrain &terrain)
{
    const auto block{getBlockAtCurrentPosition(terrain)};
    if (_movementMode != MovementMode::Fly) {
        // Controllers are free to modify the acceleration to apply custom forces, but unless in
        // the fly mode, vertical forces should be disabled.
        _acceleration.y = 0.0f;
        if (_movementMode != MovementMode::Walk) {
            auto gravity{9.81f};
            // Reduce gravity to simulate buoyancy in water and lava.
            if (block == BlockType::Water) {
                gravity *= 0.2f;
            } else if (block == BlockType::Lava) {
                gravity *= 0.1f;
            }
            _acceleration.y -= gravity;
        }
    }
    {
        // If the drag force is proportional to the speed, the speed decays exponentially, i.e.
        // multiplied by a constant factor for each time step.
        float decayFactor;
        if (_movementMode == MovementMode::Fly) {
            decayFactor = 0.1f; // Air drag only
        } else {
            if (block == BlockType::Water) {
                decayFactor = 2.0f; // Water drag
            } else if (block == BlockType::Lava) {
                decayFactor = 4.0f; // Lava drag
            } else {
                decayFactor = 0.1f; // Air drag
            }
        }
        auto decayedVelocity{_velocity * std::exp(-decayFactor * dT)};
        if (_movementMode == MovementMode::Walk) {
            // Additional drag when the entity touches the ground, whether in air, water, or lava.
            const auto decay{std::exp(-1.0f * dT)};
            // The drag applies only to the horizontal components.
            decayedVelocity.x *= decay;
            decayedVelocity.z *= decay;
        }
        // Use the expected decayed velocity to update the acceleration.
        _acceleration += (decayedVelocity - _velocity) / dT;
    }
    // Now, the acceleration reflects the net effect of custom forces, gravity, and drag forces.
    _velocity += _acceleration * dT;
    if (_movementMode == MovementMode::Walk) {
        // Prevent the entity from tunneling through the ground.
        _velocity.y = std::max(_velocity.y, 0.0f);
    }
    if (_movementMode == MovementMode::Fly) {
        _position += _velocity * dT;
    } else {
        simulateWithTerrainCollisions(dT, terrain);
    }

    // Keep the previous acceleration so that it can be displayed in the UI.
    _previousAcceleration = _acceleration;
    _acceleration = glm::vec3{0.0f};
}

void Entity::simulateWithTerrainCollisions(const float dT, const Terrain &terrain)
{
    auto remainingTime{dT};

    // Default to the fall mode unless proven otherwise.
    _movementMode = MovementMode::Fall;

    // In extreme cases infinite collisions may occur, but we limit the number of iterations to 6.
    for ([[maybe_unused]] const auto _ : std::views::iota(0, 6)) {
        auto hasCollision{false};
        auto hitTime{remainingTime};
        glm::vec3 hitNormal{0.0f};

        const auto entityBox{boxCollider()};
        const glm::ivec3 entityMinPoint{glm::floor(
            glm::min(entityBox.minPoint(), entityBox.minPoint() + _velocity * remainingTime))};
        const auto entityMaxPoint{
            glm::ivec3{glm::floor(
                glm::max(entityBox.maxPoint(), entityBox.maxPoint() + _velocity * remainingTime))}
            + 1};

        for (const auto x : std::views::iota(entityMinPoint.x, entityMaxPoint.x)) {
            for (const auto y : std::views::iota(entityMinPoint.y, entityMaxPoint.y)) {
                for (const auto z : std::views::iota(entityMinPoint.z, entityMaxPoint.z)) {
                    const glm::ivec3 blockMinPoint{x, y, z};
                    const auto blockMaxPoint{blockMinPoint + 1};

                    const auto block{terrain.getBlockAtGlobal(blockMinPoint)};
                    if (block == BlockType::Air || block == BlockType::Water
                        || block == BlockType::Lava) {
                        continue;
                    }

                    const AlignedBox3D blockBox{glm::vec3{blockMinPoint}, glm::vec3{blockMaxPoint}};
                    // Returns true and updates the arguments only if the new hitTime is less than
                    // the current hitTime.
                    hasCollision |= entityBox.sweep(_velocity, blockBox, hitTime, hitNormal);
                }
            }
        }

        if (!hasCollision) {
            _position += _velocity * remainingTime;
            break;
        }

        // Move back from the hit position by a small amount to avoid tunneling.
        _position += _velocity * hitTime - hitNormal * 0.001f;
        // 100% damping along the hit normal
        _velocity -= hitNormal * glm::dot(_velocity, hitNormal);

        remainingTime -= hitTime;
        if (remainingTime <= 0.001f) {
            break;
        }

        if (hitNormal.y < -0.5f) {
            // As long as the entity is touching the ground, switch to the walk mode.
            _movementMode = MovementMode::Walk;
        }
    };

    if (_movementMode != MovementMode::Walk && isCloseToGround(terrain)) {
        _movementMode = MovementMode::Walk;
    }
    if (_movementMode == MovementMode::Fall) {
        const auto block{getBlockAtCurrentPosition(terrain)};
        if (block == BlockType::Water || block == BlockType::Lava) {
            _movementMode = MovementMode::Swim;
        }
    }
}

bool Entity::isCloseToGround(const Terrain &terrain) const
{
    const auto entityBox{boxCollider()};

    const auto roundedY{std::round(entityBox.minPoint().y)};
    if (std::abs(entityBox.minPoint().y - roundedY) > 0.002f) {
        // The bottom face is not close enough to the ground. Skip further checks.
        return false;
    }

    const glm::ivec2 minXZ{glm::floor(glm::vec2{entityBox.minPoint().x, entityBox.minPoint().z})};
    const auto maxXZ{
        glm::ivec2{glm::floor(glm::vec2{entityBox.maxPoint().x, entityBox.maxPoint().z})} + 1};
    const auto groundY{static_cast<int>(roundedY) - 1};

    // Entities are rigid-body boxes that do not rotate, so they are considered to be touching the
    // ground if any of the blocks below them is solid.
    for (const auto x : std::views::iota(minXZ[0], maxXZ[0])) {
        for (const auto z : std::views::iota(minXZ[1], maxXZ[1])) {
            const auto block{terrain.getBlockAtGlobal(glm::ivec3{x, groundY, z})};
            if (block != BlockType::Air && block != BlockType::Water && block != BlockType::Lava) {
                return true;
            }
        }
    }

    return false;
}

} // namespace minecraft
