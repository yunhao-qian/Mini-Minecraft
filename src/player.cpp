#include "player.h"

#include "terrain_chunk.h"

#include <glm/gtc/quaternion.hpp>

#include <QString>

#include <cmath>

namespace {

auto vec3ToString(const glm::vec3 &vector) -> QString
{
    return QString{"( %1, %2, %3 )"}.arg(vector.x).arg(vector.y).arg(vector.z);
}

} // namespace

minecraft::Player::Player(const Pose &pose, const glm::vec3 &velocity, const glm::vec3 &acceleration)
    : Entity{pose, velocity, acceleration}
    , _camera{{}, 1280, 960}
    , _desiredVelocity{}
    , _desiredOrientation{}
{
    _desiredVelocity = _velocity;
    _desiredOrientation = _pose.rotationMatrix();
}

auto minecraft::Player::setCameraViewportSize(const int width, const int height) -> void
{
    _camera.setViewportSize(width, height);
}

auto minecraft::Player::getSyncedCamera() -> const Camera &
{
    auto pose{_pose};
    pose.moveGlobalUp(1.5f);
    _camera.setPose(pose);
    return _camera;
}

auto minecraft::Player::desiredVelocity() const -> const glm::vec3 &
{
    return _desiredVelocity;
}

auto minecraft::Player::setDesiredVelocity(const glm::vec3 &desiredVelocity) -> void
{
    _desiredVelocity = desiredVelocity;
}

auto minecraft::Player::desiredOrientation() const -> const glm::mat3 &
{
    return _desiredOrientation;
}

auto minecraft::Player::setDesiredOrientation(const glm::mat3 &desiredOrientation) -> void
{
    _desiredOrientation = desiredOrientation;
}

auto minecraft::Player::updatePhysics(const float dT) -> void
{
    const auto interpolation{1.0f - std::exp(-5.0f * dT)};
    {
        const auto quaternion{glm::quat_cast(_pose.rotationMatrix())};
        const auto desiredQuaternion{glm::quat_cast(_desiredOrientation)};
        const auto interpolatedQuaternion{glm::slerp(quaternion, desiredQuaternion, interpolation)};
        _pose.setRotationMatrix(glm::mat3_cast(interpolatedQuaternion));
    }
    {
        const auto interpolatedVelocity{glm::mix(_velocity, _desiredVelocity, interpolation)};
        _pose.move(_velocity * dT);
        _acceleration = (interpolatedVelocity - _velocity) / dT;
        _velocity = interpolatedVelocity;
    }
}

auto minecraft::Player::createPlayerInfoDisplayData() const -> PlayerInfoDisplayData
{
    return {
        .position{vec3ToString(_pose.position())},
        .velocity{vec3ToString(_velocity)},
        .acceleration{vec3ToString(_acceleration)},
        .lookVector{vec3ToString(_pose.forward())},
        .chunk{QString("( %1, %2 )")
                   .arg(static_cast<int>(
                       std::floor(_pose.position().x / static_cast<float>(TerrainChunk::SizeX))))
                   .arg(static_cast<int>(
                       std::floor(_pose.position().z / static_cast<float>(TerrainChunk::SizeZ))))},
        .terrainZone{QString("( %1 )").arg(static_cast<int>(std::floor(_pose.position().y / 64.f)))},
    };
}
