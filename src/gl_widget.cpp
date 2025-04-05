#include "gl_widget.h"

#include <glm/glm.hpp>

#include <QDateTime>
#include <QImage>
#include <QString>

#include <mutex>

minecraft::GLWidget::GLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _startTimeMilliseconds(QDateTime::currentMSecsSinceEpoch())
    , _lastTickMilliseconds{-1}
    , _scene{}
    , _terrainStreamer{this, &_scene.terrain()}
    , _playerController{&_scene.player()}
    , _program{this}
    , _colorTexture{0u}
    , _normalTexture{0u}
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

    if (!_program.create(":/shaders/lambert.vert.glsl",
                         ":/shaders/lambert.frag.glsl",
                         {"u_viewMatrix",
                          "u_projectionMatrix",
                          "u_time",
                          "u_colorTexture",
                          "u_normalTexture"})) {
        qFatal() << "Failed to create one or more shader programs";
    }

    _colorTexture = loadTexture(":/textures/minecraft_textures_all.png");
    _normalTexture = loadTexture(":/textures/minecraft_normals_all.png");
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
    _program.useProgram();

    glActiveTexture(GL_TEXTURE0);
    debugGLError();
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    debugGLError();
    glActiveTexture(GL_TEXTURE1);
    debugGLError();
    glBindTexture(GL_TEXTURE_2D, _normalTexture);
    debugGLError();

    _program.setUniform("u_viewMatrix", viewMatrix);
    _program.setUniform("u_projectionMatrix", projectionMatrix);
    _program.setUniform("u_time",
                        (QDateTime::currentMSecsSinceEpoch() - _startTimeMilliseconds) / 1000.0f);
    _program.setUniform("u_colorTexture", 0);
    _program.setUniform("u_normalTexture", 1);

    {
        std::lock_guard lock{_scene.terrainMutex()};
        _terrainStreamer.update(cameraPosition);
        _scene.terrain().prepareDraw();
        _scene.terrain().drawOpaqueBlocks();
        _scene.terrain().drawNonOpaqueBlocks();
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

auto minecraft::GLWidget::loadTexture(const QString &fileName) -> GLuint
{
    QImage originalImage{fileName};
    if (originalImage.isNull()) {
        qFatal() << "Failed to load texture image" << fileName;
    }
    originalImage.mirror();
    const auto convertedImage{originalImage.convertToFormat(QImage::Format_RGBA8888)};
    GLuint textureId{0u};
    glGenTextures(1, &textureId);
    debugGLError();
    glBindTexture(GL_TEXTURE_2D, textureId);
    debugGLError();
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 convertedImage.width(),
                 convertedImage.height(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 convertedImage.constBits());
    debugGLError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    debugGLError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    debugGLError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    debugGLError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    debugGLError();

    glGenerateMipmap(GL_TEXTURE_2D);
    debugGLError();

    return textureId;
}
