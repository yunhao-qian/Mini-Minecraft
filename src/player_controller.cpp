#include "player_controller.h"

#include "block_type.h"
#include "movement_mode.h"
#include "pose.h"

#include <initializer_list>
#include <ranges>

namespace minecraft {

namespace {

bool rayMarch(const Terrain &terrain,
              const glm::vec3 &origin,
              const glm::vec3 &direction,
              const float minDistance,
              const float maxDistance,
              glm::ivec3 &hitPosition)
{
    auto blockPosition{glm::ivec3{glm::floor(origin + direction * minDistance)}};
    auto distance{minDistance};

    do {
        if (const auto block{terrain.getBlockAtGlobal(blockPosition)};
            block != BlockType::Air && block != BlockType::Water && block != BlockType::Lava) {
            hitPosition = blockPosition;
            return true;
        }

        const auto rayPosition{origin + direction * distance};

        auto boundaryDistance{maxDistance};
        auto nextBlockPosition{blockPosition};

        for (const auto i : std::views::iota(0, 3)) {
            if (direction[i] > 0.0f) {
                const auto boundary{static_cast<float>(blockPosition[i] + 1)};
                const auto axisDistance{(boundary - rayPosition[i]) / direction[i]};
                if (axisDistance < boundaryDistance) {
                    boundaryDistance = axisDistance;
                    nextBlockPosition = blockPosition;
                    ++nextBlockPosition[i];
                }
            } else if (direction[i] < 0.0f) {
                const auto boundary{static_cast<float>(blockPosition[i])};
                const auto axisDistance{(boundary - rayPosition[i]) / direction[i]};
                if (axisDistance < boundaryDistance) {
                    boundaryDistance = axisDistance;
                    nextBlockPosition = blockPosition;
                    --nextBlockPosition[i];
                }
            }
        }

        blockPosition = nextBlockPosition;
        distance += boundaryDistance;
    } while (distance <= maxDistance);

    return false;
}

BlockType determineNewBlockType(const Terrain &terrain, const glm::ivec3 &position)
{
    if (terrain.getBlockAtGlobal(position) == BlockType::Bedrock) {
        // Bedrock cannot be modified.
        return BlockType::Bedrock;
    }
    // If the block is surrounded by water or lava, return that type. Otherwise, return air.
    auto hasWater{false};
    auto hasLava{false};
    for (const auto &offset : {
             glm::ivec3{1, 0, 0},
             glm::ivec3{-1, 0, 0},
             glm::ivec3{0, 1, 0},
             glm::ivec3{0, 0, 1},
             glm::ivec3{0, 0, -1},
         }) {
        const auto neighborBlock{terrain.getBlockAtGlobal(position + offset)};
        if (neighborBlock == BlockType::Water) {
            hasWater = true;
        } else if (neighborBlock == BlockType::Lava) {
            hasLava = true;
        }
    }
    if (hasWater) {
        return BlockType::Water;
    }
    if (hasLava) {
        return BlockType::Lava;
    }
    return BlockType::Air;
}

} // namespace

void PlayerController::keyPressEvent(const QKeyEvent *const event) const
{
    const auto isShiftPressed{(event->modifiers() & Qt::ShiftModifier) != 0};

    switch (event->key()) {
    case Qt::Key_Space: {
        // Jump.
        auto velocity{_player->velocity()};
        switch (_player->movementMode()) {
        case MovementMode::Walk:
            velocity.y += isShiftPressed ? 12.0f : 6.0f;
            break;
        case MovementMode::Swim:
            velocity.y += isShiftPressed ? 4.0f : 2.0f;
            break;
        default:
            return;
        }
        _player->setVelocity(velocity);
        return;
    }
    case Qt::Key_F: {
        // Toggle on/off the flight mode.
        if (_player->movementMode() == MovementMode::Fly) {
            _player->setMovementMode(MovementMode::Fall);
        } else {
            _player->setMovementMode(MovementMode::Fly);
        }
        return;
    }
    case Qt::Key_W:
    case Qt::Key_S:
    case Qt::Key_A:
    case Qt::Key_D:
    case Qt::Key_Q:
    case Qt::Key_E: {
        // Change acceleration.
        Pose pose;
        pose.setOrientation(_player->orientation());
        glm::vec3 direction;
        switch (event->key()) {
        case Qt::Key_W:
            direction = pose.forward();
            break;
        case Qt::Key_S:
            direction = -pose.forward();
            break;
        case Qt::Key_A:
            direction = -pose.right();
            break;
        case Qt::Key_D:
            direction = pose.right();
            break;
        case Qt::Key_Q:
            direction = -pose.up();
            break;
        case Qt::Key_E:
            direction = pose.up();
            break;
        default:
            return;
        }
        const auto magnitude{isShiftPressed ? 60.0f : 12.0f};
        _player->setAcceleration(_player->acceleration() + direction * magnitude);
        return;
    }
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right: {
        // Change orientation.
        Pose desiredPose;
        desiredPose.setOrientation(_player->desiredOrientation());
        const auto magnitude{isShiftPressed ? 30.0f : 6.0f};
        switch (event->key()) {
        case Qt::Key_Up:
            desiredPose.rotateAroundLocalRight(-magnitude);
            break;
        case Qt::Key_Down:
            desiredPose.rotateAroundLocalRight(magnitude);
            break;
        case Qt::Key_Left:
            desiredPose.rotateAroundGlobalUp(magnitude);
            break;
        case Qt::Key_Right:
            desiredPose.rotateAroundGlobalUp(-magnitude);
            break;
        default:
            return;
        }
        _player->setDesiredOrientation(desiredPose.orientation());
        return;
    }
    default:
        return;
    }
}

void PlayerController::mousePressEvent(const QMouseEvent *const event, Terrain &terrain) const
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const auto &cameraPose{_player->getSyncedCamera().pose()};
    glm::ivec3 hitPosition;
    if (!rayMarch(terrain, cameraPose.position(), cameraPose.forward(), 0.1f, 3.0f, hitPosition)) {
        return;
    }
    terrain.setBlockAtGlobal(hitPosition, determineNewBlockType(terrain, hitPosition));
    terrain.getChunk(glm::ivec2{hitPosition.x, hitPosition.z})->markSelfAndNeighborsDirty();
}

} // namespace minecraft
