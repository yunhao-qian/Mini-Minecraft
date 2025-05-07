#ifndef MINI_MINECRAFT_PLAYER_INFO_DISPLAY_DATA_H
#define MINI_MINECRAFT_PLAYER_INFO_DISPLAY_DATA_H

#include <QString>

namespace minecraft {

struct PlayerInfoDisplayData
{
    QString position;
    QString velocity;
    QString acceleration;
    QString lookVector;
    QString chunk;
    QString terrainZone;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_INFO_DISPLAY_DATA_H
