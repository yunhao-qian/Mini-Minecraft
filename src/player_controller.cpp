#include "player_controller.h"

#include "pose.h"

minecraft::PlayerController::PlayerController(Player *const player)
    : _player{player}
{}

auto minecraft::PlayerController::keyPressEvent(const QKeyEvent *const event) -> void
{
    const auto shiftPressed{(event->modifiers() & Qt::ShiftModifier) != 0};
    const auto deltaSpeed{shiftPressed ? 5.0f : 1.0f};
    const auto deltaAngle{shiftPressed ? 20.0f : 4.0f};

    const auto pose{_player->pose()};
    auto desiredVelocity{_player->desiredVelocity()};
    Pose desiredPose;
    desiredPose.setRotationMatrix(_player->desiredOrientation());

    switch (event->key()) {
    case Qt::Key_W:
        desiredVelocity += pose.forward() * deltaSpeed;
        break;
    case Qt::Key_S:
        desiredVelocity -= pose.forward() * deltaSpeed;
        break;
    case Qt::Key_A:
        desiredVelocity -= pose.right() * deltaSpeed;
        break;
    case Qt::Key_D:
        desiredVelocity += pose.right() * deltaSpeed;
        break;
    case Qt::Key_Q:
        desiredVelocity -= pose.up() * deltaSpeed;
        break;
    case Qt::Key_E:
        desiredVelocity += pose.up() * deltaSpeed;
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

    _player->setDesiredVelocity(desiredVelocity);
    _player->setDesiredOrientation(desiredPose.rotationMatrix());
}
