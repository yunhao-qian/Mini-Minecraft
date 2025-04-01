#ifndef MINI_MINECRAFT_PLAYER_H
#define MINI_MINECRAFT_PLAYER_H

#include "camera.h"
#include "entity.h"

namespace minecraft {

class Player : public Entity
{
public:
    Player(const Pose &pose, const glm::vec3 &velocity);

    auto setCameraViewportSize(const int width, const int height) -> void;
    auto getSyncedCamera() -> const Camera &;

private:
    static auto computeCameraPose(const Pose &playerPose) -> Pose;

    Camera _camera;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_H
