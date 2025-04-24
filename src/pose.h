#ifndef MINECRAFT_POSE_H
#define MINECRAFT_POSE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace minecraft {

class Pose
{
public:
    Pose(const glm::vec3 &position = glm::vec3{0.0f})
        : _position{position}
        , _orientation{1.0f, 0.0f, 0.0f, 0.0f}
    {}

    const glm::vec3 &position() const { return _position; }

    void setPosition(const glm::vec3 &position) { _position = position; }

    const glm::quat &orientation() const { return _orientation; }

    void setOrientation(const glm::quat &orientation)
    {
        _orientation = glm::normalize(orientation);
    }

    glm::vec3 right() const { return _orientation * glm::vec3{1.0f, 0.0f, 0.0f}; }

    glm::vec3 up() const { return _orientation * glm::vec3{0.0f, 1.0f, 0.0f}; }

    glm::vec3 forward() const
    {
        // The forward vector is the negative Z axis instead of the positive Z axis.
        return _orientation * glm::vec3{0.0f, 0.0f, -1.0f};
    }

    void rotateAround(const glm::vec3 &axis, const float degrees)
    {
        _orientation = glm::normalize(glm::angleAxis(glm::radians(degrees), axis) * _orientation);
    }

    void rotateAroundLocalRight(const float degrees) { rotateAround(right(), degrees); }

    void rotateAroundLocalUp(const float degrees) { rotateAround(up(), degrees); }

    void rotateAroundLocalForward(const float degrees) { rotateAround(forward(), degrees); }

    void rotateAroundGlobalRight(const float degrees)
    {
        rotateAround(glm::vec3{1.0f, 0.0f, 0.0f}, degrees);
    }

    void rotateAroundGlobalUp(const float degrees)
    {
        rotateAround(glm::vec3{0.0f, 1.0f, 0.0f}, degrees);
    }

    void rotateAroundGlobalForward(const float degrees)
    {
        rotateAround(glm::vec3{0.0f, 0.0f, -1.0f}, degrees);
    }

    glm::mat4 viewMatrix() const
    {
        // Since orientation is normalized, conjugate() is equivalent to inverse().
        return glm::translate(glm::mat4_cast(glm::conjugate(_orientation)), -_position);
    }

private:
    glm::vec3 _position;
    glm::quat _orientation;
};

} // namespace minecraft

#endif // MINECRAFT_POSE_H
