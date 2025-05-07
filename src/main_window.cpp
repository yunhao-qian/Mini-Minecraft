#include "main_window.h"

#include "camera_controls_window.h"
#include "player_info_window.h"

#include <QApplication>
#include <QMenuBar>

minecraft::MainWindow::MainWindow(QWidget *const parent)
    : QMainWindow{parent}
    , _glWidget{nullptr}
{
    setWindowTitle("Mini Minecraft");
    resize(1280, 960);

    _glWidget = new GLWidget{};
    setCentralWidget(_glWidget);

    const auto playerInfoWindow{new PlayerInfoWindow{this}};
    playerInfoWindow->setWindowFlag(Qt::Dialog);

    const auto cameraControlsWindow{new CameraControlsWindow{this}};
    cameraControlsWindow->setWindowFlag(Qt::Dialog);

    const auto actionQuit{new QAction{"Quit", this}};
    {
        const auto menuFile{menuBar()->addMenu("File")};
        menuFile->addAction(actionQuit);
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

    connect(actionCameraControls, &QAction::triggered, cameraControlsWindow, &QWidget::show);
}
