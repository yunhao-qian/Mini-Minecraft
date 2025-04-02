#include "gl_widget.h"

#include "vertex.h"

#include <QDateTime>
#include <QString>

minecraft::GLWidget::GLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _lastTickMilliseconds{-1}
    , _scene{}
    , _terrainStreamer{this, &_scene.terrain()}
    , _playerController{&_scene.player()}
    , _programFlat{this}
    , _programLambert{this}
{
    setFocusPolicy(Qt::StrongFocus);
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
                             {"u_viewProjectionMatrix"})
        || !_programLambert.create(":/shaders/lambert.vert.glsl",
                                   ":/shaders/lambert.frag.glsl",
                                   {"u_viewProjectionMatrix"})) {
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
    _programLambert.setUniform("u_viewProjectionMatrix", camera.viewProjectionMatrix());
    _programLambert.useProgram();
    _scene.terrain().draw();
}

auto minecraft::GLWidget::keyPressEvent(QKeyEvent *const event) -> void
{
    _playerController.keyPressEvent(event);
}

auto minecraft::GLWidget::tick() -> void
{
    const auto milliseconds{QDateTime::currentMSecsSinceEpoch()};
    if (_lastTickMilliseconds < 0) {
        // First tick
        _lastTickMilliseconds = milliseconds;
        return;
    }
    const auto dT{static_cast<float>(milliseconds - _lastTickMilliseconds) * 0.001f};
    _lastTickMilliseconds = milliseconds;

    _scene.player().updatePhysics(dT);

    _terrainStreamer.update(_scene.player().pose().position());
    _scene.terrain().prepareDraw<LambertVertex>();
    update();
    emit playerInfoChanged(_scene.player().createPlayerInfoDisplayData());
}
