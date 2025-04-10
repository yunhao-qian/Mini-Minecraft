#include "camera_controls_window.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace minecraft {

CameraControlsWindow::CameraControlsWindow(QWidget *const parent)
    : QWidget{parent}
{
    setWindowTitle("Camera Controls");

    const auto mainLayout{new QHBoxLayout{this}};
    {
        const auto leftLayout{new QVBoxLayout{}};
        for (const auto text : {
                 "W: Move forward",
                 "S: Move backward",
                 "A: Move left",
                 "D: Move right",
                 "Q: Move down",
                 "E: Move up",
                 "1: Increase FOV",
                 "2: Decrease FOV",
             }) {
            leftLayout->addWidget(new QLabel{text});
        }
        mainLayout->addLayout(leftLayout);
    }
    {
        const auto line{new QFrame{}};
        line->setFrameShape(QFrame::Shape::VLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        mainLayout->addWidget(line);
    }
    {
        const auto rightLayout{new QVBoxLayout{}};
        for (const auto text : {
                 "Up arrow: Rotate down",
                 "Down arrow: Rotate up",
                 "Left arrow: Rotate left",
                 "Right arrow: Rotate right",
                 "R: Reset camera to default position and orientation",
             }) {
            rightLayout->addWidget(new QLabel{text});
        }
        mainLayout->addLayout(rightLayout);
    }
}

} // namespace minecraft
