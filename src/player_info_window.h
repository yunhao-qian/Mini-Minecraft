#ifndef MINI_MINECRAFT_PLAYER_INFO_WINDOW_H
#define MINI_MINECRAFT_PLAYER_INFO_WINDOW_H

#include "player_info_display_data.h"

#include <QLabel>
#include <QWidget>

namespace minecraft {

class PlayerInfoWindow : public QWidget
{
    Q_OBJECT

public:
    PlayerInfoWindow(QWidget *const parent = nullptr);

public slots:
    auto playerInfoChanged(const PlayerInfoDisplayData &data) -> void;

signals:
    auto visibleChanged(const bool visible) -> void;

protected:
    auto showEvent(QShowEvent *const event) -> void override;
    auto closeEvent(QCloseEvent *const event) -> void override;

private:
    QLabel *_positionLabel;
    QLabel *_velocityLabel;
    QLabel *_accelerationLabel;
    QLabel *_lookVectorLabel;
    QLabel *_chunkLabel;
    QLabel *_terrainZoneLabel;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_PLAYER_INFO_WINDOW_H
