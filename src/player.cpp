#include "player.h"

#include "terrain_chunk.h"

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
