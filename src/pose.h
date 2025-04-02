#ifndef MINI_MINECRAFT_POSE_H
#define MINI_MINECRAFT_POSE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace minecraft {

class Pose
{
public:
    Pose();
    Pose(const glm::vec3 &position);

    auto position() const -> const glm::vec3 &;
    auto setPosition(const glm::vec3 &position) -> void;

    auto orientation() const -> const glm::quat &;
    auto setOrientation(const glm::quat &orientation) -> void;

    auto right() const -> glm::vec3;
    auto up() const -> glm::vec3;
    auto forward() const -> glm::vec3;

    auto rotateAround(const glm::vec3 &axis, const float degrees) -> void;

    auto rotateAroundLocalRight(const float degrees) -> void;
    auto rotateAroundLocalUp(const float degrees) -> void;
    auto rotateAroundLocalForward(const float degrees) -> void;

    auto rotateAroundGlobalRight(const float degrees) -> void;
    auto rotateAroundGlobalUp(const float degrees) -> void;
    auto rotateAroundGlobalForward(const float degrees) -> void;

    auto viewMatrix() const -> glm::mat4;

private:
    glm::vec3 _position;
    glm::quat _orientation;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_POSE_H
