#ifndef MINECRAFT_CAMERA_CONTROLS_WINDOW_H
#define MINECRAFT_CAMERA_CONTROLS_WINDOW_H

#include <QWidget>

namespace minecraft {

class CameraControlsWindow : public QWidget
{
    Q_OBJECT

public:
    CameraControlsWindow(QWidget *const parent = nullptr);
};

} // namespace minecraft

#endif // MINECRAFT_CAMERA_CONTROLS_WINDOW_H
