#include "player_controller.h"

#include "movement_mode.h"
#include "pose.h"

#include <ranges>

minecraft::PlayerController::PlayerController(Player *const player)
    : _player{player}
{}

auto minecraft::PlayerController::keyPressEvent(const QKeyEvent *const event) -> void
{
    const auto shiftPressed{(event->modifiers() & Qt::ShiftModifier) != 0};
    const auto acceleration{shiftPressed ? 60.0f : 12.0f};
    const auto deltaAngle{shiftPressed ? 30.0f : 6.0f};

    Pose desiredPose;
    desiredPose.setOrientation(_player->desiredOrientation());

    switch (event->key()) {
    case Qt::Key_Space:
        if (_player->movementMode() == MovementMode::Walk) {
            _player->setVelocity(_player->velocity()
                                 + glm::vec3{0.0f, shiftPressed ? 12.0f : 6.0f, 0.0f});
        }
        return;
    case Qt::Key_F:
        // Toggle the flight mode.
        if (_player->movementMode() == MovementMode::Fly) {
            _player->setMovementMode(MovementMode::Fall);
        } else {
            _player->setMovementMode(MovementMode::Fly);
        }
        return;
    case Qt::Key_W:
        _player->setAcceleration(_player->acceleration() + _player->pose().forward() * acceleration);
        break;
    case Qt::Key_S:
        _player->setAcceleration(_player->acceleration() - _player->pose().forward() * acceleration);
        break;
    case Qt::Key_A:
        _player->setAcceleration(_player->acceleration() - _player->pose().right() * acceleration);
        break;
    case Qt::Key_D:
        _player->setAcceleration(_player->acceleration() + _player->pose().right() * acceleration);
        break;
    case Qt::Key_Q:
        if (_player->movementMode() == MovementMode::Fly) {
            _player->setAcceleration(_player->acceleration() - _player->pose().up() * acceleration);
        }
        break;
    case Qt::Key_E:
        if (_player->movementMode() == MovementMode::Fly) {
            _player->setAcceleration(_player->acceleration() + _player->pose().up() * acceleration);
        }
        break;
    case Qt::Key_Up:
        desiredPose.rotateAroundLocalRight(-deltaAngle);
        break;
    case Qt::Key_Down:
        desiredPose.rotateAroundLocalRight(deltaAngle);
        break;
    case Qt::Key_Left:
        desiredPose.rotateAroundGlobalUp(deltaAngle);
        break;
    case Qt::Key_Right:
        desiredPose.rotateAroundGlobalUp(-deltaAngle);
        break;
    default:
        return;
    }

    _player->setDesiredOrientation(desiredPose.orientation());
}

auto minecraft::PlayerController::mousePressEvent([[maybe_unused]] const QMouseEvent *const event,
                                                  Terrain &terrain) -> void
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const auto &cameraPose{_player->getSyncedCamera().pose()};
    const auto blockPosition{
        rayMarch(terrain, cameraPose.position(), cameraPose.forward(), 0.1f, 3.0f)};
    if (!blockPosition.has_value()) {
        return;
    }
    terrain.setBlockGlobal(blockPosition->x, blockPosition->y, blockPosition->z, BlockType::Empty);
    terrain.getChunk(blockPosition->x, blockPosition->z)->markSelfAndNeighborsDirty();
}

auto minecraft::PlayerController::rayMarch(const Terrain &terrain,
                                           const glm::vec3 &origin,
                                           const glm::vec3 &direction,
                                           const float minDistance,
                                           const float maxDistance) const
    -> std::optional<glm::ivec3>
{
    auto blockPosition{glm::ivec3{glm::floor(origin + direction * minDistance)}};
    auto distance{minDistance};

    do {
        if (terrain.getBlockGlobal(blockPosition.x, blockPosition.y, blockPosition.z)
            != BlockType::Empty) {
            return blockPosition;
        }

        const auto position{origin + direction * distance};

        auto boundaryDistance{maxDistance};
        auto nextBlockPosition{blockPosition};

        for (const auto i : std::views::iota(0, 3)) {
            if (direction[i] > 0.0f) {
                const auto boundary{static_cast<float>(blockPosition[i] + 1)};
                const auto axisDistance{(boundary - position[i]) / direction[i]};
                if (axisDistance < boundaryDistance) {
                    boundaryDistance = axisDistance;
                    nextBlockPosition = blockPosition;
                    ++nextBlockPosition[i];
                }
            } else if (direction[i] < 0.0f) {
                const auto boundary{static_cast<float>(blockPosition[i])};
                const auto axisDistance{(boundary - position[i]) / direction[i]};
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

    return std::nullopt;
}
