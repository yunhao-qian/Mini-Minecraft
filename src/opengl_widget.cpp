#include "opengl_widget.h"

#include "shadow_map_camera.h"

#include <glm/glm.hpp>

#include <QDateTime>
#include <QThreadPool>
#include <QtCore/qthreadpool.h>

#include <mutex>
#include <ranges>

namespace minecraft {

OpenGLWidget::OpenGLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _startingMSecs{QDateTime::currentMSecsSinceEpoch()}
    , _lastTickMSecs{-1}
    , _scene{}
    , _terrainStreamer{this, &_scene.terrain()}
    , _playerController{&_scene.player()}
    , _shadowDepthProgram{this}
    , _geometryProgram{this}
    , _lightingProgram{this}
    , _colorTexture{this}
    , _normalTexture{this}
    , _shadowMapFramebuffer{this}
    , _opaqueGeometryFramebuffer{this}
    , _translucentGeometryFramebuffer{this}
    , _lightingVAO{0u}
{
    // Allows the widget to accept focus for keyboard input.
    setFocusPolicy(Qt::StrongFocus);

    connect(&_timer, &QTimer::timeout, this, &OpenGLWidget::tick);
    _timer.start(33); // ~30 frames per second
}

OpenGLWidget::~OpenGLWidget()
{
    glDeleteVertexArrays(1, &_lightingVAO);
    debugError();

    // Worker threads may be still writing to members of this class. Wait them to finish to avoid
    // corrupting the memory.
    const auto threadPool{QThreadPool::globalInstance()};
    threadPool->clear();
    threadPool->waitForDone();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    debugError();
    glDepthFunc(GL_LEQUAL);
    debugError();
    // Blending is disabled by default. We do not need it because we will composite the opaque and
    // translucent contents manually.

    _shadowDepthProgram.create({":/shaders/block_face.glsl", ":/shaders/shadow_depth.vert.glsl"},
                               {":/shaders/shadow_depth.frag.glsl"},
                               {"u_shadowViewMatrix", "u_shadowProjectionMatrix"});

    _geometryProgram.create({":/shaders/block_type.glsl",
                             ":/shaders/block_face.glsl",
                             ":/shaders/geometry.vert.glsl"},
                            {":/shaders/block_type.glsl", ":/shaders/geometry.frag.glsl"},
                            {"u_viewProjectionMatrix",
                             "u_time",
                             "u_cameraPosition",
                             "u_colorTexture",
                             "u_normalTexture"});

    _lightingProgram.create({":/shaders/lighting.vert.glsl"},
                            {":/shaders/block_type.glsl", ":/shaders/lighting.frag.glsl"},
                            {"u_viewMatrixInverse",
                             "u_projectionMatrixInverse",
                             "u_shadowViewMatrices",
                             "u_shadowProjectionMatrices",
                             "u_shadowDepthTexture",
                             "u_opaqueNormalTexture",
                             "u_opaqueAlbedoTexture",
                             "u_opaqueDepthTexture",
                             "u_translucentNormalTexture",
                             "u_translucentAlbedoTexture",
                             "u_translucentDepthTexture"});

    _colorTexture.generate(":/textures/minecraft_textures_all.png", 16, 16);
    _normalTexture.generate(":/textures/minecraft_normals_all.png", 16, 16);

    // The shadow map framebuffer has a fixed size and does not resize with the viewport.
    _shadowMapFramebuffer.resizeViewport(4096, 4096);

    glActiveTexture(GL_TEXTURE0);
    debugError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _colorTexture.texture());
    debugError();
    glActiveTexture(GL_TEXTURE1);
    debugError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _normalTexture.texture());
    debugError();

    _geometryProgram.use();
    _geometryProgram.setUniform("u_colorTexture", 0);
    _geometryProgram.setUniform("u_normalTexture", 1);

    _lightingProgram.use();
    _lightingProgram.setUniform("u_shadowDepthTexture", 2);
    _lightingProgram.setUniform("u_opaqueNormalTexture", 3);
    _lightingProgram.setUniform("u_opaqueAlbedoTexture", 4);
    _lightingProgram.setUniform("u_opaqueDepthTexture", 5);
    _lightingProgram.setUniform("u_translucentNormalTexture", 6);
    _lightingProgram.setUniform("u_translucentAlbedoTexture", 7);
    _lightingProgram.setUniform("u_translucentDepthTexture", 8);

    // The lighting pass does not need any vertex, index, or instance data, but we need a dummy VAO
    // for it.
    glGenVertexArrays(1, &_lightingVAO);
    debugError();
}

void OpenGLWidget::paintGL()
{
    ShadowMapCamera shadowMapCamera;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPosition;
    {
        const std::lock_guard lock{_scene.playerMutex()};
        const auto &camera{_scene.player().getSyncedCamera()};
        shadowMapCamera.update(glm::vec3{1.5f, 1.0f, 2.0f}, camera);
        viewMatrix = camera.pose().viewMatrix();
        projectionMatrix = camera.projectionMatrix();
        cameraPosition = camera.pose().position();
    }

    const auto time{static_cast<float>(QDateTime::currentMSecsSinceEpoch() - _startingMSecs)
                    / 1000.0f};

    {
        const std::lock_guard lock{_scene.terrainMutex()};
        const auto updateResult{_terrainStreamer.update(cameraPosition)};

        _shadowDepthProgram.use();
        for (const auto cascadeIndex : std::views::iota(0, ShadowMapCamera::NumCascades)) {
            _shadowDepthProgram.setUniform("u_shadowViewMatrix",
                                           shadowMapCamera.viewMatrix(cascadeIndex));
            _shadowDepthProgram.setUniform("u_shadowProjectionMatrix",
                                           shadowMapCamera.projectionMatrix(cascadeIndex));

            _shadowMapFramebuffer.bind(cascadeIndex);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            debugError();
            for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
                chunk->drawOpaque();
            }
        }

        _geometryProgram.use();
        _geometryProgram.setUniform("u_viewProjectionMatrix", projectionMatrix * viewMatrix);
        _geometryProgram.setUniform("u_time", time);
        _geometryProgram.setUniform("u_cameraPosition", cameraPosition);

        _opaqueGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        debugError();
        for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
            chunk->drawOpaque();
        }

        _translucentGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        debugError();
        for (const auto chunk : updateResult.chunksWithTranslucentFaces) {
            chunk->drawTranslucent();
        }
    }

    glActiveTexture(GL_TEXTURE2);
    debugError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _shadowMapFramebuffer.depthTexture());
    debugError();
    bindTextures({
        {GL_TEXTURE3, _opaqueGeometryFramebuffer.normalTexture()},
        {GL_TEXTURE4, _opaqueGeometryFramebuffer.albedoTexture()},
        {GL_TEXTURE5, _opaqueGeometryFramebuffer.depthTexture()},
        {GL_TEXTURE6, _translucentGeometryFramebuffer.normalTexture()},
        {GL_TEXTURE7, _translucentGeometryFramebuffer.albedoTexture()},
        {GL_TEXTURE8, _translucentGeometryFramebuffer.depthTexture()},
    });

    _lightingProgram.use();
    _lightingProgram.setUniform("u_viewMatrixInverse", glm::inverse(viewMatrix));
    _lightingProgram.setUniform("u_projectionMatrixInverse", glm::inverse(projectionMatrix));
    {
        glm::mat4 shadowViewMatrices[ShadowMapCamera::NumCascades];
        glm::mat4 shadowProjectionMatrices[ShadowMapCamera::NumCascades];
        for (const auto cascadeIndex : std::views::iota(0, ShadowMapCamera::NumCascades)) {
            shadowViewMatrices[cascadeIndex] = shadowMapCamera.viewMatrix(cascadeIndex);
            shadowProjectionMatrices[cascadeIndex] = shadowMapCamera.projectionMatrix(cascadeIndex);
        }
        _lightingProgram.setUniforms("u_shadowViewMatrices",
                                     ShadowMapCamera::NumCascades,
                                     shadowViewMatrices);
        _lightingProgram.setUniforms("u_shadowProjectionMatrices",
                                     ShadowMapCamera::NumCascades,
                                     shadowProjectionMatrices);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    debugError();
    // Unnecessary, but set the viewport size for clarity.
    glViewport(0, 0, _opaqueGeometryFramebuffer.width(), _opaqueGeometryFramebuffer.height());
    debugError();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    debugError();
    glBindVertexArray(_lightingVAO);
    debugError();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    debugError();

    // These textures are written to in the shadow depth and geometry passes, so they should be
    // unbound after the lighting pass to avoid potential conflicts.
    bindTextures({
        {GL_TEXTURE2, 0u},
        {GL_TEXTURE3, 0u},
        {GL_TEXTURE4, 0u},
        {GL_TEXTURE5, 0u},
        {GL_TEXTURE6, 0u},
        {GL_TEXTURE7, 0u},
        {GL_TEXTURE8, 0u},
    });
}

void OpenGLWidget::resizeGL([[maybe_unused]] const int width, [[maybe_unused]] const int height)
{
    // This is how QOpenGLWidget::recreateFbos() computes the frame buffer size in its source code.
    const auto deviceSize{size() * devicePixelRatio()};

    _opaqueGeometryFramebuffer.resizeViewport(deviceSize.width(), deviceSize.height());
    _translucentGeometryFramebuffer.resizeViewport(deviceSize.width(), deviceSize.height());
    const std::lock_guard lock{_scene.playerMutex()};
    _scene.player().resizeCameraViewport(deviceSize.width(), deviceSize.height());
}

void OpenGLWidget::keyPressEvent(QKeyEvent *const event)
{
    const std::lock_guard lock{_scene.playerMutex()};
    _playerController.keyPressEvent(event);
}

void OpenGLWidget::mousePressEvent(QMouseEvent *const event)
{
    const std::lock_guard playerLock{_scene.playerMutex()};
    const std::lock_guard terrainLock{_scene.terrainMutex()};
    _playerController.mousePressEvent(event, _scene.terrain());
}

void OpenGLWidget::tick()
{
    // Note the tick() slot is called by the timer and may not run in the OpenGL thread, so we
    // call any OpenGL functions from here.

    const auto currentMSecs{QDateTime::currentMSecsSinceEpoch()};
    if (_lastTickMSecs < 0) {
        // There is no way to compute dT on the first tick, so we just store the current time.
        _lastTickMSecs = currentMSecs;
        return;
    }

    const auto dT{static_cast<float>(currentMSecs - _lastTickMSecs) * 0.001f};
    _lastTickMSecs = currentMSecs;
    {
        const std::lock_guard playerLock{_scene.playerMutex()};
        emit playerInfoChanged(_scene.player().createPlayerInfoDisplayData());
        {
            const std::lock_guard terrainLock{_scene.terrainMutex()};
            _scene.player().updatePhysics(dT, _scene.terrain());
        }
    }

    update();
}

void OpenGLWidget::bindTextures(const std::vector<std::pair<GLenum, GLuint>> &textures)
{
    for (const auto &[textureUnit, textureID] : textures) {
        glActiveTexture(textureUnit);
        debugError();
        glBindTexture(GL_TEXTURE_2D, textureID);
        debugError();
    }
}

} // namespace minecraft
