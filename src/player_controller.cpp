#include "player_controller.h"

#include "movement_mode.h"
#include "pose.h"

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
