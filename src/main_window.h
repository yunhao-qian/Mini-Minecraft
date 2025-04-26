#ifndef MINECRAFT_MAIN_WINDOW_H
#define MINECRAFT_MAIN_WINDOW_H

#include "opengl_widget.h"

#include <QMainWindow>

namespace minecraft {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *const parent = nullptr);
};

} // namespace minecraft

#endif // MINECRAFT_MAIN_WINDOW_H
