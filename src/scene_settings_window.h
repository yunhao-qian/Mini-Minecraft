#ifndef MINECRAFT_SCENE_SETTINGS_WINDOW_H
#define MINECRAFT_SCENE_SETTINGS_WINDOW_H

#include "scene_settings.h"

#include <QDoubleSpinBox>
#include <QWidget>

namespace minecraft {

class SceneSettingsWindow : public QWidget
{
    Q_OBJECT

public:
    SceneSettingsWindow(SceneSettings *const settings, QWidget *const parent = nullptr);

private slots:
    void updateSettings();

    void resetSettings();

private:
    SceneSettings *_settings;

    QDoubleSpinBox *_sunAltitudeSpinBox;
    QDoubleSpinBox *_sunAzimuthSpinBox;
    QDoubleSpinBox *_waterRefractiveIndexSpinBox;
    QDoubleSpinBox *_waterWaveAmplitudeScaleSpinBox;
};

} // namespace minecraft

#endif // MINECRAFT_SCENE_SETTINGS_WINDOW_H
