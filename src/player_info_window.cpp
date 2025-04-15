#include "player_info_window.h"

#include <QFontDatabase>
#include <QFormLayout>
#include <QString>

#include <array>
#include <ranges>

namespace minecraft {

namespace {

QString vec3ToString(const glm::vec3 &vector)
{
    QString result{"( %1, %2, %3 )"};
    for (const auto i : std::views::iota(0, 3)) {
        result = result.arg(vector[i], 10, 'f', 3);
    }
    return result;
}

QString ivec2ToString(const glm::ivec2 vector)
{
    QString result{"( %1, %2 )"};
    for (const auto i : std::views::iota(0, 2)) {
        result = result.arg(vector[i], 5);
    }
    return result;
}

QString intToString(const int value)
{
    return QString{"( %1 )"}.arg(value, 5);
}

} // namespace

PlayerInfoWindow::PlayerInfoWindow(QWidget *const parent)
    : QWidget{parent}
    , _positionLabel{nullptr}
    , _velocityLabel{nullptr}
    , _accelerationLabel{nullptr}
    , _lookVectorLabel{nullptr}
    , _chunkLabel{nullptr}
    , _terrainZoneLabel{nullptr}
{
    setWindowTitle("Player Information");

    const auto labelPointers{std::to_array<QLabel **>({
        &_positionLabel,
        &_velocityLabel,
        &_accelerationLabel,
        &_lookVectorLabel,
        &_chunkLabel,
        &_terrainZoneLabel,
    })};

    {
        // Use a fixed-width font for better alignment.
        const auto font{QFontDatabase::systemFont(QFontDatabase::FixedFont)};
        for (const auto labelPointer : labelPointers) {
            *labelPointer = new QLabel{};
            (*labelPointer)->setFont(font);
        }
    }

    const auto layout{new QFormLayout{this}};
    layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addRow("Position:", _positionLabel);
    layout->addRow("Velocity:", _velocityLabel);
    layout->addRow("Acceleration:", _accelerationLabel);
    layout->addRow("Look direction:", _lookVectorLabel);
    layout->addRow("Chunk:", _chunkLabel);
    layout->addRow("Terrain zone:", _terrainZoneLabel);

    // Fill in dummy data to adjust the window size.
    setPlayerInfo({
        .position{glm::vec3{0.0f}},
        .velocity{glm::vec3{0.0f}},
        .acceleration{glm::vec3{0.0f}},
        .lookVector{0.0f, 0.0f, -1.0f},
        .chunk{glm::ivec2{0}},
        .terrainZone = 0,
    });
    adjustSize();
    {
        const QString noInfoString{"--"};
        for (const auto labelPointer : labelPointers) {
            (*labelPointer)->setText(noInfoString);
        }
    }
}

void PlayerInfoWindow::setPlayerInfo(const PlayerInfoDisplayData &data)
{
    _positionLabel->setText(vec3ToString(data.position));
    _velocityLabel->setText(vec3ToString(data.velocity));
    _accelerationLabel->setText(vec3ToString(data.acceleration));
    _lookVectorLabel->setText(vec3ToString(data.lookVector));
    _chunkLabel->setText(ivec2ToString(data.chunk));
    _terrainZoneLabel->setText(intToString(data.terrainZone));
}

void PlayerInfoWindow::showEvent(QShowEvent *const event)
{
    QWidget::showEvent(event);
    emit visibleChanged(true);
}

void PlayerInfoWindow::closeEvent(QCloseEvent *const event)
{
    QWidget::closeEvent(event);
    emit visibleChanged(false);
}

} // namespace minecraft
