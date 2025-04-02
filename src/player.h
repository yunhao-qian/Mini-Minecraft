#ifndef MINI_MINECRAFT_PLAYER_H
#define MINI_MINECRAFT_PLAYER_H

#include "camera.h"
#include "entity.h"
#include "player_info_display_data.h"

#include <glm/glm.hpp>

namespace minecraft {

class Player : public Entity
{
public:
    Player(const Pose &pose, const glm::vec3 &velocity, const glm::vec3 &acceleration);

    auto setCameraViewportSize(const int width, const int height) -> void;
    auto getSyncedCamera() -> const Camera &;

    auto desiredVelocity() const -> const glm::vec3 &;
    auto setDesiredVelocity(const glm::vec3 &desiredVelocity) -> void;

    auto desiredOrientation() const -> const glm::mat3 &;
    auto setDesiredOrientation(const glm::mat3 &desiredOrientation) -> void;

    auto updatePhysics(const float dT) -> void;

    auto createPlayerInfoDisplayData() const -> PlayerInfoDisplayData;

private:
    Camera _camera;
    glm::vec3 _desiredVelocity;
    glm::mat3 _desiredOrientation;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_H
