#include "opengl_widget.h"

#include "shadow_map_camera.h"
#include "water_wave.h"

#include <glm/glm.hpp>

#include <QDateTime>
#include <QThreadPool>

#include <initializer_list>
#include <mutex>
#include <optional>
#include <ranges>

namespace minecraft {

OpenGLWidget::OpenGLWidget(QWidget *const parent)
    : QOpenGLWidget{parent}
    , _timer{}
    , _startingMSecs{QDateTime::currentMSecsSinceEpoch()}
    , _lastTickMSecs{-1}
    , _scene{}
    , _terrainStreamer{&_scene.terrain()}
    , _playerController{&_scene.player()}
    , _shadowDepthProgram{}
    , _geometryProgram{}
    , _lightingProgram{}
    , _colorTexture{}
    , _normalTexture{}
    , _shadowMapFramebuffer{}
    , _opaqueGeometryFramebuffer{}
    , _translucentGeometryFramebuffer{}
    , _reflectionGeometryFramebuffer{}
    , _refractionGeometryFramebuffer{}
    , _quadVAO{}
{
    // Allows the widget to accept focus for keyboard input.
    setFocusPolicy(Qt::StrongFocus);

    connect(&_timer, &QTimer::timeout, this, &OpenGLWidget::tick);
    _timer.start(33); // ~30 frames per second
}

OpenGLWidget::~OpenGLWidget()
{
    // Worker threads may be still writing to members of this class. Wait them to finish to avoid
    // corrupting the memory.
    const auto threadPool{QThreadPool::globalInstance()};
    threadPool->clear();
    threadPool->waitForDone();
}

void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    OpenGLContext::_instance = this;

    glEnable(GL_DEPTH_TEST);
    checkError();
    glDepthFunc(GL_LEQUAL);
    checkError();
    // Blending is disabled by default. We do not need it because we will composite the opaque and
    // translucent contents manually.

    // The clear color is only relevant for the depth textures used in the shadow map and geometry
    // passes, with the R channel encoding the linear depth and the G channel encoding its square.
    glClearColor(1e5f, 1e10f, 0.0f, 0.0f);
    checkError();

    _shadowDepthProgram.create(":/shaders/shadow_depth.vert.glsl",
                               ":/shaders/shadow_depth.frag.glsl");
    _geometryProgram.create(":/shaders/geometry.vert.glsl", ":/shaders/geometry.frag.glsl");
    _lightingProgram.create(":/shaders/quad.vert.glsl", ":/shaders/lighting.frag.glsl");

    _colorTexture.generate(":/textures/minecraft_textures_all.png", 16, 16);
    _normalTexture.generate(":/textures/minecraft_normals_all.png", 16, 16);

    // The shadow map framebuffer has a fixed size and does not resize with the viewport.
    _shadowMapFramebuffer.resizeViewport(4096, 4096);

    glActiveTexture(GL_TEXTURE0);
    checkError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _colorTexture.texture());
    checkError();
    glActiveTexture(GL_TEXTURE1);
    checkError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _normalTexture.texture());
    checkError();

    _geometryProgram.use();
    _geometryProgram.setUniform("u_colorTexture", 0);
    _geometryProgram.setUniform("u_normalTexture", 1);

    _lightingProgram.use();
    _lightingProgram.setUniform("u_shadowDepthTexture", 2);
    _lightingProgram.setUniform("u_opaqueDepthTexture", 3);
    _lightingProgram.setUniform("u_opaqueNormalTexture", 4);
    _lightingProgram.setUniform("u_opaqueAlbedoTexture", 5);
    _lightingProgram.setUniform("u_translucentDepthTexture", 6);
    _lightingProgram.setUniform("u_translucentNormalTexture", 7);
    _lightingProgram.setUniform("u_translucentAlbedoTexture", 8);
    _lightingProgram.setUniform("u_reflectionDepthTexture", 9);
    _lightingProgram.setUniform("u_reflectionNormalTexture", 10);
    _lightingProgram.setUniform("u_reflectionAlbedoTexture", 11);
    _lightingProgram.setUniform("u_refractionDepthTexture", 12);
    _lightingProgram.setUniform("u_refractionNormalTexture", 13);
    _lightingProgram.setUniform("u_refractionAlbedoTexture", 14);

    // The lighting pass does not need any vertex, index, or instance data, but we need a dummy VAO
    // for it.
    {
        GLuint vao{0u};
        glGenVertexArrays(1, &vao);
        checkError();
        _quadVAO = OpenGLObject{
            vao,
            [](OpenGLContext *const context, const GLuint vao) {
                context->glDeleteVertexArrays(1, &vao);
            },
        };
    }
}

void OpenGLWidget::paintGL()
{
    const auto waterElevation{138.0f + getAverageWaterWaveOffset()};
    const auto time{static_cast<float>(QDateTime::currentMSecsSinceEpoch() - _startingMSecs)
                    / 1000.0f};

    std::optional<Camera> camera;
    {
        const std::lock_guard lock{_scene.playerMutex()};
        camera = _scene.player().getSyncedCamera();
    }
    ShadowMapCamera shadowMapCamera;
    shadowMapCamera.update(glm::normalize(glm::vec3{1.5f, 1.0f, 2.0f}), *camera);
    const auto reflectionCamera{camera->createReflectionCamera(waterElevation)};
    const auto refractionCamera{camera->createRefractionCamera(waterElevation, 1.1f)};

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
        const auto &cameraPosition{camera->pose().position()};

        const std::lock_guard lock{_scene.terrainMutex()};
        const auto updateResult{_terrainStreamer.update(cameraPosition)};

        _shadowDepthProgram.use();
        for (const auto cascadeIndex : std::views::iota(0, ShadowMapCamera::CascadeCount)) {
            _shadowDepthProgram.setUniform("u_shadowViewMatrix", shadowViewMatrices[cascadeIndex]);
            _shadowDepthProgram.setUniform("u_shadowViewProjectionMatrix",
                                           shadowViewProjectionMatrices[cascadeIndex]);

            _shadowMapFramebuffer.bind(cascadeIndex);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            checkError();
            for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
                chunk->drawOpaque();
            }
        }

        _geometryProgram.use();
        _geometryProgram.setUniform("u_time", time);
        _geometryProgram.setUniform("u_viewMatrix", camera->viewMatrix());
        _geometryProgram.setUniform("u_viewProjectionMatrix", camera->viewProjectionMatrix());
        _geometryProgram.setUniform("u_isAboveWaterOnly", 0);
        _geometryProgram.setUniform("u_isUnderWaterOnly", 0);

        _opaqueGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
            if (camera->isInViewFrustum(chunk->boundingBox())) {
                chunk->drawOpaque();
            }
        }

        _translucentGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        for (const auto chunk : updateResult.chunksWithTranslucentFaces) {
            if (camera->isInViewFrustum(chunk->boundingBox())) {
                chunk->drawTranslucent();
            }
        }

        const auto isAboveWater{
            cameraPosition.y
            >= 138.0f + getWaterWaveOffset(glm::vec2{cameraPosition.x, cameraPosition.z}, time)};

        _geometryProgram.setUniform("u_viewMatrix", reflectionCamera.viewMatrix());
        _geometryProgram.setUniform("u_viewProjectionMatrix",
                                    reflectionCamera.viewProjectionMatrix());
        _geometryProgram.setUniform("u_isAboveWaterOnly", isAboveWater ? 1 : 0);
        _geometryProgram.setUniform("u_isUnderWaterOnly", isAboveWater ? 0 : 1);

        _reflectionGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
            if (reflectionCamera.isInViewFrustum(chunk->boundingBox())) {
                chunk->drawOpaque();
            }
        }

        _geometryProgram.setUniform("u_viewMatrix", refractionCamera.viewMatrix());
        _geometryProgram.setUniform("u_viewProjectionMatrix",
                                    refractionCamera.viewProjectionMatrix());
        _geometryProgram.setUniform("u_isAboveWaterOnly", isAboveWater ? 0 : 1);
        _geometryProgram.setUniform("u_isUnderWaterOnly", isAboveWater ? 1 : 0);

        _refractionGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        for (const auto chunk : updateResult.chunksWithOpaqueFaces) {
            if (refractionCamera.isInViewFrustum(chunk->boundingBox())) {
                chunk->drawOpaque();
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    checkError();

    glActiveTexture(GL_TEXTURE2);
    checkError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, _shadowMapFramebuffer.depthTexture());
    checkError();

    bindTextures({
        {GL_TEXTURE3, _opaqueGeometryFramebuffer.depthTexture()},
        {GL_TEXTURE4, _opaqueGeometryFramebuffer.normalTexture()},
        {GL_TEXTURE5, _opaqueGeometryFramebuffer.albedoTexture()},
        {GL_TEXTURE6, _translucentGeometryFramebuffer.depthTexture()},
        {GL_TEXTURE7, _translucentGeometryFramebuffer.normalTexture()},
        {GL_TEXTURE8, _translucentGeometryFramebuffer.albedoTexture()},
        {GL_TEXTURE9, _reflectionGeometryFramebuffer.depthTexture()},
        {GL_TEXTURE10, _reflectionGeometryFramebuffer.normalTexture()},
        {GL_TEXTURE11, _reflectionGeometryFramebuffer.albedoTexture()},
        {GL_TEXTURE12, _refractionGeometryFramebuffer.depthTexture()},
        {GL_TEXTURE13, _refractionGeometryFramebuffer.normalTexture()},
        {GL_TEXTURE14, _refractionGeometryFramebuffer.albedoTexture()},
    });

    const auto viewMatrixInverse{glm::inverse(camera->viewMatrix())};

    _lightingProgram.use();
    _lightingProgram.setUniform("u_viewMatrix", camera->viewMatrix());
    _lightingProgram.setUniform("u_projectionMatrixInverse",
                                glm::inverse(camera->projectionMatrix()));
    _lightingProgram.setUniform("u_reflectionProjectionMatrix", reflectionCamera.projectionMatrix());
    _lightingProgram.setUniform("u_originalToReflectionViewMatrix",
                                reflectionCamera.viewMatrix() * viewMatrixInverse);
    _lightingProgram.setUniform("u_reflectionToOriginalViewMatrix",
                                camera->viewMatrix() * glm::inverse(reflectionCamera.viewMatrix()));
    _lightingProgram.setUniform("u_refractionProjectionMatrix", refractionCamera.projectionMatrix());
    _lightingProgram.setUniform("u_originalToRefractionViewMatrix",
                                refractionCamera.viewMatrix() * viewMatrixInverse);
    _lightingProgram.setUniform("u_refractionToOriginalViewMatrix",
                                camera->viewMatrix() * glm::inverse(refractionCamera.viewMatrix()));
    _lightingProgram.setUniform("u_cameraNear", camera->near());
    _lightingProgram.setUniform("u_cameraFar", camera->far());
    {
        // In the lighting pass, positions are transformed directly from the camera's view space to
        // the shadow map's view and clip spaces.
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
    checkError();
    // Unnecessary, but set the viewport size for clarity.
    glViewport(0, 0, _opaqueGeometryFramebuffer.width(), _opaqueGeometryFramebuffer.height());
    checkError();
    glBindVertexArray(_quadVAO.get());
    checkError();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkError();

    // These textures are written to in the shadow depth and geometry passes, so they should be
    // unbound after the lighting pass to avoid potential conflicts.
    glActiveTexture(GL_TEXTURE2);
    checkError();
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0u);
    checkError();

    bindTextures({
        {GL_TEXTURE3, 0u},
        {GL_TEXTURE4, 0u},
        {GL_TEXTURE5, 0u},
        {GL_TEXTURE6, 0u},
        {GL_TEXTURE7, 0u},
        {GL_TEXTURE8, 0u},
        {GL_TEXTURE9, 0u},
        {GL_TEXTURE10, 0u},
        {GL_TEXTURE11, 0u},
        {GL_TEXTURE12, 0u},
        {GL_TEXTURE13, 0u},
        {GL_TEXTURE14, 0u},
    });

    glEnable(GL_DEPTH_TEST);
    checkError();
}

void OpenGLWidget::resizeGL([[maybe_unused]] const int width, [[maybe_unused]] const int height)
{
    // This is how QOpenGLWidget::recreateFbos() computes the frame buffer size in its source code.
    const auto deviceSize{size() * devicePixelRatio()};
    for (const auto framebuffer : {
             &_opaqueGeometryFramebuffer,
             &_translucentGeometryFramebuffer,
             &_reflectionGeometryFramebuffer,
             &_refractionGeometryFramebuffer,
         }) {
        framebuffer->resizeViewport(deviceSize.width(), deviceSize.height());
    }
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
        checkError();
        glBindTexture(GL_TEXTURE_2D, textureID);
        checkError();
    }
}

} // namespace minecraft
