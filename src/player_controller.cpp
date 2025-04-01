#include "player_controller.h"

minecraft::PlayerController::PlayerController(Player *const player)
    : _player{player}
{}

auto minecraft::PlayerController::keyPressEvent(const QKeyEvent *const event) -> void
{
    // TODO: Temporary logic for player movement
    auto pose{_player->pose()};
    const auto distance{(event->modifiers() | Qt::ShiftModifier) == 0 ? 2.0f : 10.0f};
    switch (event->key()) {
    case Qt::Key_W:
        pose.moveLocalForward(distance);
        break;
    case Qt::Key_S:
        pose.moveLocalForward(-distance);
        break;
    case Qt::Key_A:
        pose.moveLocalRight(-distance);
        break;
    case Qt::Key_D:
        pose.moveLocalRight(distance);
        break;
    case Qt::Key_Q:
        pose.moveGlobalUp(-distance);
        break;
    case Qt::Key_E:
        pose.moveGlobalUp(distance);
        break;
    case Qt::Key_Up:
        pose.rotateAroundLocalRight(-distance);
        break;
    case Qt::Key_Down:
        pose.rotateAroundLocalRight(distance);
        break;
    case Qt::Key_Left:
        pose.rotateAroundLocalUp(distance);
        break;
    case Qt::Key_Right:
        pose.rotateAroundLocalUp(-distance);
        break;
    default:
        return;
    }
    _player->setPose(pose);
}
