#include "player.h"

#include "block_type.h"
#include "movement_mode.h"
#include "terrain_chunk.h"

#include <QString>

#include <cmath>
#include <ranges>

namespace {

auto vec3ToString(const glm::vec3 &vector) -> QString
{
    return QString{"( %1, %2, %3 )"}.arg(vector.x).arg(vector.y).arg(vector.z);
}

} // namespace

minecraft::Player::Player(const Pose &pose, const glm::vec3 &velocity, const glm::vec3 &acceleration)
    : Entity{pose, velocity, acceleration}
    , _camera{{}, 1280, 960}
    , _desiredOrientation{}
    , _previousAcceleration{0.0f, 0.0f, 0.0f}
{
    _desiredOrientation = _pose.orientation();
}

auto minecraft::Player::setCameraViewportSize(const int width, const int height) -> void
{
    _camera.setViewportSize(width, height);
}

auto minecraft::Player::getSyncedCamera() -> const Camera &
{
    auto pose{_pose};
    pose.setPosition(_pose.position() + glm::vec3{0.0f, 1.5f, 0.0f});
    _camera.setPose(pose);
    return _camera;
}

auto minecraft::Player::movementMode() const -> MovementMode
{
    return _movementMode;
}

auto minecraft::Player::setMovementMode(const MovementMode mode) -> void
{
    _movementMode = mode;
}

auto minecraft::Player::desiredOrientation() const -> const glm::quat &
{
    return _desiredOrientation;
}

auto minecraft::Player::setDesiredOrientation(const glm::quat &orientation) -> void
{
    _desiredOrientation = orientation;
}

auto minecraft::Player::updatePhysics(const float dT, const Terrain &terrain) -> void
{
    {
        const auto interpolation{1.0f - std::exp(-5.0f * dT)};
        const auto interpolatedOrientation{
            glm::slerp(_pose.orientation(), _desiredOrientation, interpolation)};
        _pose.setOrientation(interpolatedOrientation);
    }
    {
        auto decayedVelocity{_velocity * std::exp(-0.1f * dT)};
        if (_movementMode == MovementMode::Walk) {
            const auto decay{std::exp(-1.0f * dT)};
            decayedVelocity.x *= decay;
            decayedVelocity.z *= decay;
        }
        _acceleration += (decayedVelocity - _velocity) / dT;
    }
    if (_movementMode != MovementMode::Fly && _movementMode != MovementMode::Walk) {
        _acceleration.y += -9.81f;
    }
    _velocity += _acceleration * dT;
    if (_movementMode == MovementMode::Walk) {
        _velocity.y = std::max(_velocity.y, 0.0f);
    }

    if (_movementMode == MovementMode::Fly) {
        _pose.setPosition(_pose.position() + _velocity * dT);
    } else {
        simulateWithTerrainCollisions(dT, terrain);
    }
    _previousAcceleration = _acceleration;
    _acceleration = {0.0f, 0.0f, 0.0f};
}

auto minecraft::Player::createPlayerInfoDisplayData() const -> PlayerInfoDisplayData
{
    return {
        .position{vec3ToString(_pose.position())},
        .velocity{vec3ToString(_velocity)},
        .acceleration{vec3ToString(_previousAcceleration)},
        .lookVector{vec3ToString(_pose.forward())},
        .chunk{QString("( %1, %2 )")
                   .arg(static_cast<int>(
                       std::floor(_pose.position().x / static_cast<float>(TerrainChunk::SizeX))))
                   .arg(static_cast<int>(
                       std::floor(_pose.position().z / static_cast<float>(TerrainChunk::SizeZ))))},
        .terrainZone{QString("( %1 )").arg(static_cast<int>(std::floor(_pose.position().y / 64.f)))},
    };
}

auto minecraft::Player::boxCollider() const -> AlignedBox
{
    return {_pose.position() - glm::vec3{0.5f, 0.0f, 0.5f},
            _pose.position() + glm::vec3{0.5f, 2.0f, 0.5f}};
}

auto minecraft::Player::simulateWithTerrainCollisions(const float dT, const Terrain &terrain)
    -> void
{
    auto remainingTime{dT};

    // Default to the fall mode unless proven otherwise.
    _movementMode = MovementMode::Fall;

    for ([[maybe_unused]] const auto _ : std::views::iota(0, 6)) {
        auto hasCollision{false};
        auto hitTime{remainingTime};
        glm::vec3 hitNormal{0.0f, 0.0f, 0.0f};

        const auto playerBox{boxCollider()};
        const auto floatMinP{
            glm::min(playerBox.minP(), playerBox.minP() + _velocity * remainingTime)};
        const auto floatMaxP{
            glm::max(playerBox.maxP(), playerBox.maxP() + _velocity * remainingTime)};
        const glm::ivec3 intMinP{glm::floor(floatMinP)};
        const glm::ivec3 intMaxP{glm::floor(floatMaxP) + 1.0f};

        for (const auto x : std::views::iota(intMinP.x, intMaxP.x)) {
            for (const auto y : std::views::iota(intMinP.y, intMaxP.y)) {
                for (const auto z : std::views::iota(intMinP.z, intMaxP.z)) {
                    if (terrain.getBlockGlobal(x, y, z) == BlockType::Empty) {
                        continue;
                    }
                    const glm::vec3 blockMinP{glm::ivec3{x, y, z}};
                    const AlignedBox blockBox{blockMinP, blockMinP + 1.0f};
                    // Returns true and updates the arguments only if the new hitTime is less than
                    // the current hitTime.
                    hasCollision |= playerBox.sweep(_velocity, blockBox, hitTime, hitNormal);
                }
            }
        }

        if (!hasCollision) {
            _pose.setPosition(_pose.position() + _velocity * remainingTime);
            break;
        }
        // Move back from the hit position by a small amount to avoid tunneling.
        _pose.setPosition(_pose.position() + _velocity * hitTime - hitNormal * 0.001f);
        _velocity -= hitNormal * glm::dot(_velocity, hitNormal);

        remainingTime -= hitTime;
        if (remainingTime <= 0.001f) {
            break;
        }

        if (hitNormal.y < -0.5f) {
            _movementMode = MovementMode::Walk;
        }
    };

    if (_movementMode != MovementMode::Walk && isCloseToGround(terrain)) {
        _movementMode = MovementMode::Walk;
    }
}

auto minecraft::Player::isCloseToGround(const Terrain &terrain) const -> bool
{
    const auto playerBox{boxCollider()};

    const auto roundedY{std::round(playerBox.minP().y)};
    if (std::abs(_pose.position().y - roundedY) > 0.002f) {
        return false;
    }

    const glm::ivec2 minP{glm::floor(glm::vec2{playerBox.minP().x, playerBox.minP().z})};
    const glm::ivec2 maxP{glm::floor(glm::vec2{playerBox.maxP().x, playerBox.maxP().z}) + 1.0f};
    const auto groundY{static_cast<int>(roundedY) - 1};
    for (const auto x : std::views::iota(minP[0], maxP[0])) {
        for (const auto z : std::views::iota(minP[1], maxP[1])) {
            if (terrain.getBlockGlobal(x, groundY, z) != BlockType::Empty) {
                return true;
            }
        }
    }

    return false;
}
