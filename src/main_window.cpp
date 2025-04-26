#include "main_window.h"

#include "camera_controls_window.h"
#include "player_info_window.h"
#include "scene_settings_window.h"

#include <QAction>
#include <QApplication>
#include <QMenuBar>

namespace minecraft {

MainWindow::MainWindow(QWidget *const parent)
    : QMainWindow{parent}
{
    setWindowTitle("Mini Minecraft");
    resize(1280, 960);

    const auto openGLWidget{new OpenGLWidget{}};
    setCentralWidget(openGLWidget);
    // Give the keyboard input focus to this widget.
    openGLWidget->setFocus();

    const auto playerInfoWindow{new PlayerInfoWindow{this}};
    playerInfoWindow->setWindowFlag(Qt::Dialog);

    const auto sceneSettingsWindow{new SceneSettingsWindow{&openGLWidget->sceneSettings(), this}};
    sceneSettingsWindow->setWindowFlag(Qt::Dialog);

    const auto cameraControlsWindow{new CameraControlsWindow{this}};
    cameraControlsWindow->setWindowFlag(Qt::Dialog);

    const auto actionQuit{new QAction{"Quit", this}};
    {
        const auto menuFile{menuBar()->addMenu("File")};
        menuFile->addAction(actionQuit);
    }

    const auto actionSceneSettings{new QAction{"Scene Settings", this}};
    {
        const auto menuEdit{menuBar()->addMenu("Edit")};
        menuEdit->addAction(actionSceneSettings);
    }

    const auto actionPlayerInfo{new QAction{"Player Information", this}};
    actionPlayerInfo->setCheckable(true);
    {
        const auto menuView{menuBar()->addMenu("View")};
        menuView->addAction(actionPlayerInfo);
    }

    const auto actionCameraControls{new QAction{"Camera Controls", this}};
    {
        const auto menuHelp{menuBar()->addMenu("Help")};
        menuHelp->addAction(actionCameraControls);
    }

    connect(actionQuit, &QAction::triggered, [] { QApplication::exit(); });

    connect(actionSceneSettings, &QAction::triggered, sceneSettingsWindow, &QWidget::show);

    connect(actionPlayerInfo, &QAction::toggled, [playerInfoWindow](const bool checked) {
        if (checked) {
            playerInfoWindow->show();
        } else {
            playerInfoWindow->hide();
        }
    });
    connect(playerInfoWindow,
            &PlayerInfoWindow::visibleChanged,
            actionPlayerInfo,
            &QAction::setChecked);
    // Use signals and slots instead of direct function calls because OpenGLWidget::tick() may not
    // run on the GUI thread.
    connect(openGLWidget,
            &OpenGLWidget::playerInfoChanged,
            playerInfoWindow,
            &PlayerInfoWindow::setPlayerInfo);

    connect(actionCameraControls, &QAction::triggered, cameraControlsWindow, &QWidget::show);
}

} // namespace minecraft
