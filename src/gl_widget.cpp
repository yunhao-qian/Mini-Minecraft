#include "gl_widget.h"

#include "vertex.h"

#include <glm/glm.hpp>

#include <QDateTime>
#include <QString>

#include <mutex>

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
    glEnable(GL_BLEND);
    debugGLError();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    debugGLError();

    if (!_programFlat.create(":/shaders/flat.vert.glsl",
                             ":/shaders/flat.frag.glsl",
                             {"u_viewMatrix", "u_projectionMatrix"})
        || !_programLambert.create(":/shaders/lambert.vert.glsl",
                                   ":/shaders/lambert.frag.glsl",
                                   {"u_viewMatrix", "u_projectionMatrix"})) {
        qFatal("Failed to create one or more shader programs");
    }
}

auto minecraft::GLWidget::resizeGL(const int width, const int height) -> void
{
    std::lock_guard lock{_scene.playerMutex()};
    _scene.player().setCameraViewportSize(width, height);
}

auto minecraft::GLWidget::paintGL() -> void
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    debugGLError();

    glm::vec3 cameraPosition;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    {
        std::lock_guard lock{_scene.playerMutex()};
        const auto &camera{_scene.player().getSyncedCamera()};
        cameraPosition = camera.pose().position();
        viewMatrix = camera.pose().viewMatrix();
        projectionMatrix = camera.projectionMatrix();
    }
    _programLambert.setUniform("u_viewMatrix", viewMatrix);
    _programLambert.setUniform("u_projectionMatrix", projectionMatrix);
    _programLambert.useProgram();

    {
        std::lock_guard lock{_scene.terrainMutex()};
        _terrainStreamer.update(cameraPosition);
        _scene.terrain().prepareDraw<LambertVertex>();
        _scene.terrain().draw();
    }
}

auto minecraft::GLWidget::keyPressEvent(QKeyEvent *const event) -> void
{
    std::lock_guard lock{_scene.playerMutex()};
    _playerController.keyPressEvent(event);
}

auto minecraft::GLWidget::mousePressEvent(QMouseEvent *const event) -> void
{
    std::lock_guard playerLock{_scene.playerMutex()};
    std::lock_guard terrainLock{_scene.terrainMutex()};
    _playerController.mousePressEvent(event, _scene.terrain());
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
    {
        std::lock_guard playerLock{_scene.playerMutex()};
        emit playerInfoChanged(_scene.player().createPlayerInfoDisplayData());
        {
            std::lock_guard terrainLock{_scene.terrainMutex()};
            _scene.player().updatePhysics(dT, _scene.terrain());
        }
    }
    update();
}
