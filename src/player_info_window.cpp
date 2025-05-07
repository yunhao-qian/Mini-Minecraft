#include "player_info_window.h"

#include <QFormLayout>
#include <QString>

minecraft::PlayerInfoWindow::PlayerInfoWindow(QWidget *const parent)
    : QWidget{parent}
    , _positionLabel{nullptr}
    , _velocityLabel{nullptr}
    , _accelerationLabel{nullptr}
    , _lookVectorLabel{nullptr}
    , _chunkLabel{nullptr}
    , _terrainZoneLabel{nullptr}
{
    setWindowTitle("Player Information");

    {
        const QString noInfoString{"--"};
        _positionLabel = new QLabel{noInfoString};
        _velocityLabel = new QLabel{noInfoString};
        _accelerationLabel = new QLabel{noInfoString};
        _lookVectorLabel = new QLabel{noInfoString};
        _chunkLabel = new QLabel{noInfoString};
        _terrainZoneLabel = new QLabel{noInfoString};
    }

    const auto layout{new QFormLayout{this}};
    layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addRow("Position:", _positionLabel);
    layout->addRow("Velocity:", _velocityLabel);
    layout->addRow("Acceleration:", _accelerationLabel);
    layout->addRow("Look direction:", _lookVectorLabel);
    layout->addRow("Chunk:", _chunkLabel);
    layout->addRow("Terrain zone:", _terrainZoneLabel);

    adjustSize();
    resize(480, height());
}

auto minecraft::PlayerInfoWindow::playerInfoChanged(const PlayerInfoDisplayData &data) -> void
{
    _positionLabel->setText(data.position);
    _velocityLabel->setText(data.velocity);
    _accelerationLabel->setText(data.acceleration);
    _lookVectorLabel->setText(data.lookVector);
    _chunkLabel->setText(data.chunk);
    _terrainZoneLabel->setText(data.terrainZone);
}

auto minecraft::PlayerInfoWindow::showEvent(QShowEvent *const event) -> void
{
    QWidget::showEvent(event);
    emit visibleChanged(true);
}

auto minecraft::PlayerInfoWindow::closeEvent(QCloseEvent *const event) -> void
{
    QWidget::closeEvent(event);
    emit visibleChanged(false);
}
