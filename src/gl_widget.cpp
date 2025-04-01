#include "gl_widget.h"

#include "vertex.h"

minecraft::GLWidget::GLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _scene{this}
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

auto minecraft::GLWidget::resizeGL(const int width, const int height) -> void
{
    _scene.player().setCameraViewportSize(width, height);
}

auto minecraft::GLWidget::paintGL() -> void
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    debugGLError();

    auto &camera{_scene.player().getSyncedCamera()};
    _programFlat.setUniform("u_viewProjectionMatrix", camera.viewProjectionMatrix());
    _programFlat.setUniform("u_modelMatrix", glm::mat4{1.0f});
    _programFlat.useProgram();
    _scene.terrain().draw();
}

auto minecraft::GLWidget::tick() -> void
{
    _scene.terrain().prepareDraw<FlatVertex>();
    update();
}
