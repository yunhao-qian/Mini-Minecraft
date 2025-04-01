#include "player.h"

minecraft::Player::Player(const Pose &pose, const glm::vec3 &velocity)
    : Entity{pose, velocity}
    , _camera{{}, 1280, 960}
{}

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
