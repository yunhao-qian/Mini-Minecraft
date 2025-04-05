#include "gl_widget.h"

#include <OpenGL/gltypes.h>
#include <glm/glm.hpp>

#include <QDateTime>
#include <QImage>
#include <QString>
#include <QtGui/qopengl.h>

#include <mutex>
#include <ranges>

minecraft::GLWidget::GLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _startingMSecs(QDateTime::currentMSecsSinceEpoch())
    , _lastTickMSecs{-1}
    , _scene{}
    , _terrainStreamer{this, &_scene.terrain()}
    , _playerController{&_scene.player()}
    , _program{this}
    , _colorTextureArray{0u}
    , _normalTextureArray{0u}
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
                          "u_colorTextureArray",
                          "u_normalTextureArray"})) {
        qFatal() << "Failed to create one or more shader programs";
    }

    _colorTextureArray = loadTextureArray(":/textures/minecraft_textures_all.png");
    _normalTextureArray = loadTextureArray(":/textures/minecraft_normals_all.png");
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
    glBindTexture(GL_TEXTURE_2D_ARRAY, _colorTextureArray);
    debugGLError();
    glActiveTexture(GL_TEXTURE1);
    debugGLError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _normalTextureArray);
    debugGLError();

    _program.setUniform("u_viewMatrix", viewMatrix);
    _program.setUniform("u_projectionMatrix", projectionMatrix);
    _program.setUniform("u_time", (QDateTime::currentMSecsSinceEpoch() - _startingMSecs) / 1000.0f);
    _program.setUniform("u_colorTextureArray", 0);
    _program.setUniform("u_normalTextureArray", 1);

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
    const auto currentMSecs{QDateTime::currentMSecsSinceEpoch()};
    if (_lastTickMSecs < 0) {
        // First tick
        _lastTickMSecs = currentMSecs;
        return;
    }
    const auto dT{static_cast<float>(currentMSecs - _lastTickMSecs) * 0.001f};
    _lastTickMSecs = currentMSecs;
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

auto minecraft::GLWidget::loadTextureArray(const QString &filename) -> GLuint
{
    QImage image{filename};
    if (image.isNull()) {
        qFatal() << "Failed to load texture array image" << filename;
    }
    image = image.convertToFormat(QImage::Format_RGBA8888);

    GLuint textureArray{0u};
    glGenTextures(1, &textureArray);
    debugGLError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    debugGLError();

    const auto tileSize{image.width() / 16};
    for (auto level{0}, scaledTileSize{tileSize}; scaledTileSize > 0; ++level, scaledTileSize /= 2) {
        glTexImage3D(GL_TEXTURE_2D_ARRAY,
                     level,
                     GL_RGBA8,
                     scaledTileSize,
                     scaledTileSize,
                     256,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     nullptr);
        debugGLError();
    }

    for (const auto tileY : std::views::iota(0, 16)) {
        for (const auto tileX : std::views::iota(0, 16)) {
            const auto tileIndex{tileY * 16 + tileX};
            auto tile{image.copy(tileX * tileSize, tileY * tileSize, tileSize, tileSize)};
            tile.mirror();
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                            0,
                            0,
                            0,
                            tileIndex,
                            tileSize,
                            tileSize,
                            1,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            tile.constBits());
            debugGLError();
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    debugGLError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    debugGLError();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    debugGLError();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    debugGLError();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    debugGLError();

    return textureArray;
}
