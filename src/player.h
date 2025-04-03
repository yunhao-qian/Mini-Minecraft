#ifndef MINI_MINECRAFT_PLAYER_H
#define MINI_MINECRAFT_PLAYER_H

#include "aligned_box.h"
#include "camera.h"
#include "entity.h"
#include "movement_mode.h"
#include "player_info_display_data.h"
#include "terrain.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace minecraft {

class Player : public Entity
{
public:
    Player(const Pose &pose, const glm::vec3 &velocity, const glm::vec3 &acceleration);

    auto setCameraViewportSize(const int width, const int height) -> void;
    auto getSyncedCamera() -> const Camera &;

    auto movementMode() const -> MovementMode;
    auto setMovementMode(const MovementMode mode) -> void;

    auto desiredOrientation() const -> const glm::quat &;
    auto setDesiredOrientation(const glm::quat &orientation) -> void;

    auto updatePhysics(const float dT, const Terrain &terrain) -> void;

    auto createPlayerInfoDisplayData() const -> PlayerInfoDisplayData;

private:
    auto boxCollider() const -> AlignedBox;

    auto simulateWithTerrainCollisions(const float dT, const Terrain &terrain) -> void;

    auto isCloseToGround(const Terrain &terrain) const -> bool;

    Camera _camera;
    MovementMode _movementMode;
    glm::quat _desiredOrientation;
    glm::vec3 _previousAcceleration;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_H
