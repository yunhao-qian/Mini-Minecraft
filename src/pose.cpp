#include "pose.h"
#include "glm/fwd.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

minecraft::Pose::Pose()
    : Pose{{0.0f, 0.0f, 0.0f}}
{}

minecraft::Pose::Pose(const glm::vec3 &position)
    : _position{position}
    , _orientation{1.0f, 0.0f, 0.0f, 0.0f}
{}

auto minecraft::Pose::position() const -> const glm::vec3 &
{
    return _position;
}

auto minecraft::Pose::setPosition(const glm::vec3 &position) -> void
{
    _position = position;
}

auto minecraft::Pose::orientation() const -> const glm::quat &
{
    return _orientation;
}

auto minecraft::Pose::setOrientation(const glm::quat &orientation) -> void
{
    _orientation = glm::normalize(orientation);
}

auto minecraft::Pose::right() const -> glm::vec3
{
    return _orientation * glm::vec3{1.0f, 0.0f, 0.0f};
}

auto minecraft::Pose::up() const -> glm::vec3
{
    return _orientation * glm::vec3{0.0f, 1.0f, 0.0f};
}

auto minecraft::Pose::forward() const -> glm::vec3
{
    return _orientation * glm::vec3{0.0f, 0.0f, -1.0f};
}

auto minecraft::Pose::rotateAround(const glm::vec3 &axis, const float degrees) -> void
{
    _orientation = glm::normalize(glm::angleAxis(glm::radians(degrees), axis) * _orientation);
}

auto minecraft::Pose::rotateAroundLocalRight(const float degrees) -> void
{
    rotateAround(right(), degrees);
}

auto minecraft::Pose::rotateAroundLocalUp(const float degrees) -> void
{
    rotateAround(up(), degrees);
}

auto minecraft::Pose::rotateAroundLocalForward(const float degrees) -> void
{
    rotateAround(forward(), degrees);
}

auto minecraft::Pose::rotateAroundGlobalRight(const float degrees) -> void
{
    rotateAround({1.0f, 0.0f, 0.0f}, degrees);
}

auto minecraft::Pose::rotateAroundGlobalUp(const float degrees) -> void
{
    rotateAround({0.0f, 1.0f, 0.0f}, degrees);
}

auto minecraft::Pose::rotateAroundGlobalForward(const float degrees) -> void
{
    rotateAround({0.0f, 0.0f, -1.0f}, degrees);
}

auto minecraft::Pose::viewMatrix() const -> glm::mat4
{
    const auto inverseRotation{glm::toMat4(glm::conjugate(_orientation))};
    const auto inverseTranslation{glm::translate({1.0f}, -_position)};
    return inverseRotation * inverseTranslation;
}
