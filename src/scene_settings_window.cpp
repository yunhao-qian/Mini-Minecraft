#include "scene_settings_window.h"

#include <QFormLayout>
#include <QIcon>
#include <QPushButton>
#include <QVBoxLayout>

#include <initializer_list>

namespace minecraft {

SceneSettingsWindow::SceneSettingsWindow(SceneSettings *const settings, QWidget *const parent)
    : QWidget{parent}
    , _settings{settings}
    , _sunAltitudeSpinBox{nullptr}
    , _sunAzimuthSpinBox{nullptr}
    , _waterRefractiveIndexSpinBox{nullptr}
    , _waterWaveAmplitudeScaleSpinBox{nullptr}
{
    setWindowTitle("Scene Settings");
    setWindowIcon(QIcon{":/icons/settings.ico"});

    const auto settingsData{_settings->get().second};

    _sunAltitudeSpinBox = new QDoubleSpinBox{};
    _sunAltitudeSpinBox->setDecimals(0);
    _sunAltitudeSpinBox->setRange(0.0, 90.0);
    _sunAltitudeSpinBox->setSuffix("°");
    _sunAltitudeSpinBox->setValue(settingsData.sunAltitude);

    _sunAzimuthSpinBox = new QDoubleSpinBox{};
    _sunAzimuthSpinBox->setDecimals(0);
    _sunAzimuthSpinBox->setRange(0.0, 360.0);
    _sunAzimuthSpinBox->setSuffix("°");
    _sunAzimuthSpinBox->setValue(settingsData.sunAzimuth);

    _waterRefractiveIndexSpinBox = new QDoubleSpinBox{};
    _waterRefractiveIndexSpinBox->setDecimals(2);
    _waterRefractiveIndexSpinBox->setRange(1.0, 1.5);
    _waterRefractiveIndexSpinBox->setSingleStep(0.01);
    _waterRefractiveIndexSpinBox->setValue(settingsData.waterRefractiveIndex);

    _waterWaveAmplitudeScaleSpinBox = new QDoubleSpinBox{};
    _waterWaveAmplitudeScaleSpinBox->setDecimals(1);
    _waterWaveAmplitudeScaleSpinBox->setRange(0.0, 2.0);
    _waterWaveAmplitudeScaleSpinBox->setSuffix("x");
    _waterWaveAmplitudeScaleSpinBox->setSingleStep(0.1);
    _waterWaveAmplitudeScaleSpinBox->setValue(settingsData.waterWaveAmplitudeScale);

    const auto mainLayout{new QVBoxLayout{this}};

    {
        const auto formLayout{new QFormLayout{}};
        formLayout->addRow("Sun Altitude", _sunAltitudeSpinBox);
        formLayout->addRow("Sun Azimuth", _sunAzimuthSpinBox);
        formLayout->addRow("Water Refractive Index", _waterRefractiveIndexSpinBox);
        formLayout->addRow("Water Wave Amplitude Scale", _waterWaveAmplitudeScaleSpinBox);
        mainLayout->addLayout(formLayout);
    }

    const auto resetButton{new QPushButton{"Reset"}};
    mainLayout->addWidget(resetButton);

    for (const auto spinBox : {
             _sunAltitudeSpinBox,
             _sunAzimuthSpinBox,
             _waterRefractiveIndexSpinBox,
             _waterWaveAmplitudeScaleSpinBox,
         }) {
        connect(spinBox, &QDoubleSpinBox::valueChanged, this, &SceneSettingsWindow::updateSettings);
    };

    connect(resetButton, &QPushButton::clicked, this, &SceneSettingsWindow::resetSettings);
}

void SceneSettingsWindow::updateSettings()
{
    SceneSettingsData updatedData;
    updatedData.sunAltitude = _sunAltitudeSpinBox->value();
    updatedData.sunAzimuth = _sunAzimuthSpinBox->value();
    updatedData.waterRefractiveIndex = _waterRefractiveIndexSpinBox->value();
    updatedData.waterWaveAmplitudeScale = _waterWaveAmplitudeScaleSpinBox->value();
    _settings->set(updatedData);
}

void SceneSettingsWindow::resetSettings()
{
    const SceneSettingsData defaultData;
    _sunAltitudeSpinBox->setValue(defaultData.sunAltitude);
    _sunAzimuthSpinBox->setValue(defaultData.sunAzimuth);
    _waterRefractiveIndexSpinBox->setValue(defaultData.waterRefractiveIndex);
    _waterWaveAmplitudeScaleSpinBox->setValue(defaultData.waterWaveAmplitudeScale);
    _settings->set(defaultData);
}

} // namespace minecraft
