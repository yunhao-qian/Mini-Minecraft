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

    auto createPlayerInfoDisplayData() const -> PlayerInfoDisplayData;

private:
    Camera _camera;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_H
