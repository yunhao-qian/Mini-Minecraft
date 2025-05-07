#ifndef MINI_MINECRAFT_MAIN_WINDOW_H
#define MINI_MINECRAFT_MAIN_WINDOW_H

#include "gl_widget.h"

#include <QMainWindow>

namespace minecraft {

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *const parent = nullptr);

private:
    GLWidget *_glWidget;
};

} // namespace minecraft

#endif // MAINWINDOW_H
