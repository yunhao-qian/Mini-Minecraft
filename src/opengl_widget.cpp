#include "opengl_widget.h"

#include "shadow_map_camera.h"

#include <glm/glm.hpp>

#include <QDateTime>
#include <QThreadPool>

#include <initializer_list>
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
    , _quadVAO{0u}
{
    // Allows the widget to accept focus for keyboard input.
    setFocusPolicy(Qt::StrongFocus);

    connect(&_timer, &QTimer::timeout, this, &OpenGLWidget::tick);
    _timer.start(33); // ~30 frames per second
}

OpenGLWidget::~OpenGLWidget()
{
    glDeleteVertexArrays(1, &_quadVAO);
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

    // The only place that uses the clear color is the depth textures of the shadow map and geometry
    // passes, where only the R channel is used to store the depth value.
    glClearColor(1e5f, 0.0f, 0.0f, 0.0f);
    debugError();

    _shadowDepthProgram.create(
        {
            ":/shaders/block_face.glsl",
            ":/shaders/shadow_depth.vert.glsl",
        },
        {
            ":/shaders/shadow_depth.frag.glsl",
        },
        {
            "u_shadowViewMatrix",
            "u_shadowViewProjectionMatrix",
        });

    _geometryProgram.create(
        {
            ":/shaders/block_type.glsl",
            ":/shaders/block_face.glsl",
            ":/shaders/water_wave.glsl",
            ":/shaders/geometry.vert.glsl",
        },
        {
            ":/shaders/block_type.glsl",
            ":/shaders/water_wave.glsl",
            ":/shaders/geometry.frag.glsl",
        },
        {
            "u_time",
            "u_viewMatrix",
            "u_viewProjectionMatrix",
            "u_colorTexture",
            "u_normalTexture",
        });

    _lightingProgram.create(
        {
            ":/shaders/quad.vert.glsl",
        },
        {
            ":/shaders/block_type.glsl",
            ":/shaders/lighting.frag.glsl",
        },
        {
            "u_viewMatrix",
            "u_projectionMatrix",
            "u_projectionMatrixInverse",
            "u_cameraNear",
            "u_cameraFar",
            "u_shadowViewMatrices",
            "u_shadowViewProjectionMatrices",
            "u_shadowMapDepthBlurScales",
            "u_shadowDepthTexture",
            "u_opaqueNormalTexture",
            "u_opaqueAlbedoTexture",
            "u_opaqueDepthTexture",
            "u_translucentNormalTexture",
            "u_translucentAlbedoTexture",
            "u_translucentDepthTexture",
        });

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
    glGenVertexArrays(1, &_quadVAO);
    debugError();
}

void OpenGLWidget::paintGL()
{
    const auto time{static_cast<float>(QDateTime::currentMSecsSinceEpoch() - _startingMSecs)
                    / 1000.0f};

    ShadowMapCamera shadowMapCamera;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPosition;
    float cameraNear;
    float cameraFar;
    {
        const std::lock_guard lock{_scene.playerMutex()};
        const auto &camera{_scene.player().getSyncedCamera()};
        shadowMapCamera.update(glm::normalize(glm::vec3{1.5f, 1.0f, 2.0f}), camera);
        viewMatrix = camera.pose().viewMatrix();
        projectionMatrix = camera.projectionMatrix();
        cameraPosition = camera.pose().position();
        cameraNear = camera.near();
        cameraFar = camera.far();
    }

    glm::mat4 shadowViewMatrices[ShadowMapCamera::CascadeCount];
    glm::mat4 shadowViewProjectionMatrices[ShadowMapCamera::CascadeCount];
    glm::vec2 shadowMapDepthBlurScales[ShadowMapCamera::CascadeCount];
    for (const auto cascadeIndex : std::views::iota(0, ShadowMapCamera::CascadeCount)) {
        const auto &shadowViewMatrix{shadowMapCamera.viewMatrix(cascadeIndex)};
        const auto &shadowProjectionMatrix{shadowMapCamera.projectionMatrix(cascadeIndex)};
        shadowViewMatrices[cascadeIndex] = shadowViewMatrix;
        shadowViewProjectionMatrices[cascadeIndex] = shadowProjectionMatrix * shadowViewMatrix;
        shadowMapDepthBlurScales[cascadeIndex] = shadowMapCamera.getDepthBlurScale(cascadeIndex);
    }

    {
        const std::lock_guard lock{_scene.terrainMutex()};
        const auto updateResult{_terrainStreamer.update(cameraPosition)};

        _shadowDepthProgram.use();
        for (const auto cascadeIndex : std::views::iota(0, ShadowMapCamera::CascadeCount)) {
            _shadowDepthProgram.setUniform("u_shadowViewMatrix", shadowViewMatrices[cascadeIndex]);
            _shadowDepthProgram.setUniform("u_shadowViewProjectionMatrix",
                                           shadowViewProjectionMatrices[cascadeIndex]);

            _shadowMapFramebuffer.bind(cascadeIndex);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            debugError();
            for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
                chunk->drawOpaque();
            }
        }

        _geometryProgram.use();
        _geometryProgram.setUniform("u_time", time);
        _geometryProgram.setUniform("u_viewMatrix", viewMatrix);
        _geometryProgram.setUniform("u_viewProjectionMatrix", projectionMatrix * viewMatrix);

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

    glDisable(GL_DEPTH_TEST);
    debugError();

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
    _lightingProgram.setUniform("u_viewMatrix", viewMatrix);
    _lightingProgram.setUniform("u_projectionMatrix", projectionMatrix);
    _lightingProgram.setUniform("u_projectionMatrixInverse", glm::inverse(projectionMatrix));
    _lightingProgram.setUniform("u_cameraNear", cameraNear);
    _lightingProgram.setUniform("u_cameraFar", cameraFar);
    {
        // In the lighting pass, positions are transformed directly from the camera's view space to
        // the shadow map's view and clip spaces.
        const auto viewMatrixInverse{glm::inverse(viewMatrix)};
        for (auto &shadowViewMatrix : shadowViewMatrices) {
            shadowViewMatrix = shadowViewMatrix * viewMatrixInverse;
        }
        for (auto &shadowViewProjectionMatrix : shadowViewProjectionMatrices) {
            shadowViewProjectionMatrix = shadowViewProjectionMatrix * viewMatrixInverse;
        }
    }
    _lightingProgram.setUniforms("u_shadowViewMatrices",
                                 ShadowMapCamera::CascadeCount,
                                 shadowViewMatrices);
    _lightingProgram.setUniforms("u_shadowViewProjectionMatrices",
                                 ShadowMapCamera::CascadeCount,
                                 shadowViewProjectionMatrices);
    _lightingProgram.setUniforms("u_shadowMapDepthBlurScales",
                                 ShadowMapCamera::CascadeCount,
                                 shadowMapDepthBlurScales);

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    debugError();
    // Unnecessary, but set the viewport size for clarity.
    glViewport(0, 0, _opaqueGeometryFramebuffer.width(), _opaqueGeometryFramebuffer.height());
    debugError();
    glBindVertexArray(_quadVAO);
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

    glEnable(GL_DEPTH_TEST);
    debugError();
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
