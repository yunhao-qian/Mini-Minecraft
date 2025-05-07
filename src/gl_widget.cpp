#include "gl_widget.h"

minecraft::GLWidget::GLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _programFlat{this}
{
    connect(&_timer, &QTimer::timeout, this, &GLWidget::tick);
    _timer.start(16); // ~60 frames per second
}

auto minecraft::GLWidget::initializeGL() -> void
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    debugGLError();
    glDepthFunc(GL_LEQUAL);
    debugGLError();
    glClearColor(0.37f, 0.74f, 1.0f, 1.0f);
    debugGLError();

    if (!_programFlat.create(":/shaders/flat.vert.glsl",
                             ":/shaders/flat.frag.glsl",
                             {"u_viewProjectionMatrix", "u_modelMatrix"})) {
        qFatal("Failed to create one or more shader programs");
    }
}

auto minecraft::GLWidget::resizeGL(const int width, const int height) -> void {}

auto minecraft::GLWidget::paintGL() -> void
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    debugGLError();
}

auto minecraft::GLWidget::tick() -> void
{
    update();
}
