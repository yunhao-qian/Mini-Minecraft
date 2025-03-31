#include "pose.h"

#include <glm/gtc/matrix_transform.hpp>

minecraft::Pose::Pose()
    : Pose{{0.0f, 0.0f, 0.0f}}
{}

minecraft::Pose::Pose(const glm::vec3 &position)
    : _position{position}
    , _right{1.0f, 0.0f, 0.0f}
    , _up{0.0f, 1.0f, 0.0f}
    , _forward{0.0f, 0.0f, -1.0f}
{}

auto minecraft::Pose::position() const -> const glm::vec3 &
{
    return _position;
}

auto minecraft::Pose::right() const -> const glm::vec3 &
{
    return _right;
}

auto minecraft::Pose::up() const -> const glm::vec3 &
{
    return _up;
}

auto minecraft::Pose::forward() const -> const glm::vec3 &
{
    return _forward;
}

auto minecraft::Pose::move(const glm::vec3 &displacement) -> void
{
    _position += displacement;
}

auto minecraft::Pose::moveLocalRight(const float distance) -> void
{
    move(_right * distance);
}

auto minecraft::Pose::moveLocalUp(const float distance) -> void
{
    move(_up * distance);
}

auto minecraft::Pose::moveLocalForward(const float distance) -> void
{
    move(_forward * distance);
}

auto minecraft::Pose::moveGlobalRight(const float distance) -> void
{
    _position.x += distance;
}

auto minecraft::Pose::moveGlobalUp(const float distance) -> void
{
    _position.y += distance;
}

auto minecraft::Pose::moveGlobalForward(const float distance) -> void
{
    _position.z -= distance;
}

auto minecraft::Pose::rotateAround(const glm::vec3 &axis, const float degrees) -> void
{
    const glm::mat3 rotation{glm::rotate({}, glm::radians(degrees), axis)};
    _right = glm::normalize(rotation * _right);
    _up = glm::normalize(rotation * _up);
    _forward = glm::normalize(rotation * _forward);
}

auto minecraft::Pose::rotateAroundLocalRight(const float degrees) -> void
{
    const glm::mat3 rotation{glm::rotate({}, glm::radians(degrees), _right)};
    _up = glm::normalize(rotation * _up);
    _forward = glm::normalize(rotation * _forward);
}

auto minecraft::Pose::rotateAroundLocalUp(const float degrees) -> void
{
    const glm::mat3 rotation{glm::rotate({}, glm::radians(degrees), _up)};
    _right = glm::normalize(rotation * _right);
    _forward = glm::normalize(rotation * _forward);
}

auto minecraft::Pose::rotateAroundLocalForward(const float degrees) -> void
{
    const glm::mat3 rotation{glm::rotate({}, glm::radians(degrees), _forward)};
    _right = glm::normalize(rotation * _right);
    _up = glm::normalize(rotation * _up);
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
    return glm::lookAt(_position, _position + _forward, _up);
}
