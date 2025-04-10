#ifndef MINI_MINECRAFT_POSE_H
#define MINI_MINECRAFT_POSE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace minecraft {

class Pose
{
public:
    Pose(const glm::vec3 &position = {0.0f, 0.0f, 0.0f});

    const glm::vec3 &position() const;
    void setPosition(const glm::vec3 &position);

    const glm::quat &orientation() const;
    void setOrientation(const glm::quat &orientation);

    glm::vec3 right() const;
    glm::vec3 up() const;
    glm::vec3 forward() const;

    void rotateAround(const glm::vec3 &axis, const float degrees);

    void rotateAroundLocalRight(const float degrees);
    void rotateAroundLocalUp(const float degrees);
    void rotateAroundLocalForward(const float degrees);

    void rotateAroundGlobalRight(const float degrees);
    void rotateAroundGlobalUp(const float degrees);
    void rotateAroundGlobalForward(const float degrees);

    glm::mat4 viewMatrix() const;

private:
    glm::vec3 _position;
    glm::quat _orientation;
};

inline Pose::Pose(const glm::vec3 &position)
    : _position{position}
    , _orientation{1.0f, 0.0f, 0.0f, 0.0f}
{}

inline const glm::vec3 &Pose::position() const
{
    return _position;
}

inline void Pose::setPosition(const glm::vec3 &position)
{
    _position = position;
}

inline const glm::quat &Pose::orientation() const
{
    return _orientation;
}

inline void Pose::setOrientation(const glm::quat &orientation)
{
    _orientation = glm::normalize(orientation);
}

inline glm::vec3 Pose::right() const
{
    return _orientation * glm::vec3{1.0f, 0.0f, 0.0f};
}

inline glm::vec3 Pose::up() const
{
    return _orientation * glm::vec3{0.0f, 1.0f, 0.0f};
}

inline glm::vec3 Pose::forward() const
{
    // The forward vector is the negative Z axis instead of the positive Z axis.
    return _orientation * glm::vec3{0.0f, 0.0f, -1.0f};
}

inline void Pose::rotateAround(const glm::vec3 &axis, const float degrees)
{
    _orientation = glm::normalize(glm::angleAxis(glm::radians(degrees), axis) * _orientation);
}

inline void Pose::rotateAroundLocalRight(const float degrees)
{
    rotateAround(right(), degrees);
}

inline void Pose::rotateAroundLocalUp(const float degrees)
{
    rotateAround(up(), degrees);
}

inline void Pose::rotateAroundLocalForward(const float degrees)
{
    rotateAround(forward(), degrees);
}

inline void Pose::rotateAroundGlobalRight(const float degrees)
{
    rotateAround(glm::vec3{1.0f, 0.0f, 0.0f}, degrees);
}

inline void Pose::rotateAroundGlobalUp(const float degrees)
{
    rotateAround(glm::vec3{0.0f, 1.0f, 0.0f}, degrees);
}

inline void Pose::rotateAroundGlobalForward(const float degrees)
{
    rotateAround(glm::vec3{0.0f, 0.0f, -1.0f}, degrees);
}

inline glm::mat4 Pose::viewMatrix() const
{
    // Since orientation is normalized, conjugate() is equivalent to inverse().
    return glm::translate(glm::mat4_cast(glm::conjugate(_orientation)), -_position);
}

} // namespace minecraft

#endif // MINI_MINECRAFT_POSE_H
