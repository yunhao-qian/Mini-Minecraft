#ifndef MINI_MINECRAFT_CAMERA_CONTROLS_WINDOW_H
#define MINI_MINECRAFT_CAMERA_CONTROLS_WINDOW_H

#include <QWidget>

namespace minecraft {

class CameraControlsWindow : public QWidget
{
public:
    CameraControlsWindow(QWidget *const parent = nullptr);
};

} // namespace minecraft

#endif // MINI_MINECRAFT_CAMERA_CONTROLS_WINDOW_H
