#include "player_controller.h"

#include "pose.h"

#include <glm/glm.hpp>

#include <algorithm>

minecraft::PlayerController::PlayerController(Player *const player)
    : _player{player}
{}

auto minecraft::PlayerController::keyPressEvent(const QKeyEvent *const event) -> void
{
    const auto shiftPressed{(event->modifiers() & Qt::ShiftModifier) != 0};
    const auto deltaSpeed{shiftPressed ? 5.0f : 1.0f};
    const auto deltaAngle{shiftPressed ? 20.0f : 4.0f};

    auto desiredVelocity{_player->desiredVelocity()};
    Pose desiredPose;
    desiredPose.setOrientation(_player->desiredOrientation());

    switch (event->key()) {
    case Qt::Key_W:
        desiredVelocity += _player->pose().forward() * deltaSpeed;
        break;
    case Qt::Key_S:
        desiredVelocity -= _player->pose().forward() * deltaSpeed;
        break;
    case Qt::Key_A:
        desiredVelocity -= _player->pose().right() * deltaSpeed;
        break;
    case Qt::Key_D:
        desiredVelocity += _player->pose().right() * deltaSpeed;
        break;
    case Qt::Key_Q:
        desiredVelocity -= _player->pose().up() * deltaSpeed;
        break;
    case Qt::Key_E:
        desiredVelocity += _player->pose().up() * deltaSpeed;
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

    if (auto desiredSpeed{glm::length(desiredVelocity)}; desiredSpeed != 0.0f) {
        desiredSpeed = std::clamp(desiredSpeed, -32.0f, 32.0f);
        desiredVelocity = glm::normalize(desiredVelocity) * desiredSpeed;
    }
    _player->setDesiredVelocity(desiredVelocity);
    _player->setDesiredOrientation(desiredPose.orientation());
}
