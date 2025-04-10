#ifndef MINI_MINECRAFT_MAIN_WINDOW_H
#define MINI_MINECRAFT_MAIN_WINDOW_H

#include "opengl_widget.h"

#include <QMainWindow>

namespace minecraft {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *const parent = nullptr);

private:
    OpenGLWidget *_openGLWidget;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_MAIN_WINDOW_H
