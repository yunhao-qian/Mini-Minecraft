#ifndef MINECRAFT_PLAYER_H
#define MINECRAFT_PLAYER_H

#include "aligned_box_3d.h"
#include "camera.h"
#include "entity.h"
#include "movement_mode.h"
#include "player_info_display_data.h"
#include "pose.h"
#include "terrain.h"
#include "terrain_chunk.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

namespace minecraft {

class Player : public Entity
{
public:
    Player(const Pose &pose)
        : Entity{pose.position(), glm::vec3{0.0f}, glm::vec3{0.0f}, MovementMode::Fly}
        , _camera{Pose{}, 1280, 960}
        , _orientation{pose.orientation()}
        , _desiredOrientation{pose.orientation()}
    {}

    void resizeCameraViewport(const int width, const int height)
    {
        _camera.resizeViewport(width, height);
    }

    const Camera &getSyncedCamera()
    {
        Pose cameraPose{position() + glm::vec3{0.0f, 1.5f, 0.0f}};
        cameraPose.setOrientation(_orientation);
        _camera.setPose(cameraPose);
        return _camera;
    }

    const glm::quat &orientation() const { return _orientation; }

    const glm::quat &desiredOrientation() const { return _desiredOrientation; }

    void setDesiredOrientation(const glm::quat &orientation) { _desiredOrientation = orientation; }

    AlignedBox3D boxCollider() const override
    {
        return {
            position() - glm::vec3{0.5f, 0.0f, 0.5f},
            position() + glm::vec3{0.5f, 2.0f, 0.5f},
        };
    }

    void updatePhysics(const float dT, const Terrain &terrain) override
    {
        Entity::updatePhysics(dT, terrain);
        const auto interpolation{1.0f - std::exp(-5.0f * dT)};
        _orientation = glm::slerp(_orientation, _desiredOrientation, interpolation);
    }

    PlayerInfoDisplayData createPlayerInfoDisplayData() const
    {
        return {
            .position{position()},
            .velocity{velocity()},
            .acceleration{previousAcceleration()},
            .lookVector{_camera.pose().forward()},
            .chunk{glm::ivec2{
                glm::floor(glm::vec2{position().x, position().z}
                           / glm::vec2{glm::ivec2{TerrainChunk::SizeX, TerrainChunk::SizeZ}})}},
            .terrainZone = static_cast<int>(std::floor(position().y / 64.f)),
        };
    }

private:
    Camera _camera;
    glm::quat _orientation;
    glm::quat _desiredOrientation;
};

} // namespace minecraft

#endif // MINECRAFT_PLAYER_H
