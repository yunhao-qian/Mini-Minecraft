#include "opengl_widget.h"

#include "constants.h"
#include "shadow_map_camera.h"
#include "terrain_chunk.h"
#include "uniform_buffer_data.h"
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
    , _ubo{}
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

    // Generate and bind the uniform buffer object. As it is globally unique, we only need to do it
    // once.
    {
        GLuint ubo{0u};
        glGenBuffers(1, &ubo);
        checkError();
        _ubo = OpenGLObject{
            ubo,
            [](OpenGLContext *const context, const GLuint ubo) {
                context->glDeleteBuffers(1, &ubo);
            },
        };
    }
    glBindBuffer(GL_UNIFORM_BUFFER, _ubo.get());
    checkError();
    glBufferData(GL_UNIFORM_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(UniformBufferData)),
                 nullptr,
                 GL_STREAM_DRAW);
    checkError();
    for (const auto program : {
             &_shadowDepthProgram,
             &_geometryProgram,
             &_lightingProgram,
         }) {
        program->bindUniformBlock("UniformBufferData", 0);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _ubo.get());
    checkError();

    _colorTexture.generate(":/textures/minecraft_textures_all.png", 16, 16);
    _normalTexture.generate(":/textures/minecraft_normals_all.png", 16, 16);

    // The shadow map framebuffer has a fixed size and does not resize with the viewport.
    _shadowMapFramebuffer.resize(4096, 4096);

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
    const auto averageWaterLevel{static_cast<float>(WaterLevel) + getAverageWaterWaveOffset()};
    const auto time{static_cast<float>(QDateTime::currentMSecsSinceEpoch() - _startingMSecs)
                    / 1000.0f};

    std::optional<Camera> camera;
    {
        const std::lock_guard lock{_scene.playerMutex()};
        camera = _scene.player().getSyncedCamera();
    }
    ShadowMapCamera shadowMapCamera;
    shadowMapCamera.update(glm::normalize(glm::vec3{1.5f, 1.0f, 2.0f}), *camera);
    const auto reflectionCamera{camera->createReflectionCamera(averageWaterLevel)};
    const auto refractionCamera{camera->createRefractionCamera(averageWaterLevel, 1.1f)};

    // Update the UBO data.
    {
        const auto viewMatrix{camera->viewMatrix()};
        const auto viewMatrixInverse{glm::inverse(viewMatrix)};
        UniformBufferData uboData{
            .time = time,
            .cameraNear = camera->near(),
            .cameraFar = camera->far(),
            .viewMatrices{viewMatrix, reflectionCamera.viewMatrix(), refractionCamera.viewMatrix()},
            .viewProjectionMatrices{
                camera->viewProjectionMatrix(),
                reflectionCamera.viewProjectionMatrix(),
                refractionCamera.viewProjectionMatrix(),
            },
            .shadowViewMatrices{},
            .shadowViewProjectionMatrices{},
            .mainToShadowViewMatrices{},
            .mainToShadowViewProjectionMatrices{},
            .shadowMapDepthBlurScales{},
            .viewMatrixInverse{viewMatrixInverse},
            .projectionMatrixInverse{glm::inverse(camera->projectionMatrix())},
            .reflectionProjectionMatrix{reflectionCamera.projectionMatrix()},
            .mainToReflectionViewMatrix{reflectionCamera.viewMatrix() * viewMatrixInverse},
            .reflectionToMainViewMatrix{viewMatrix * glm::inverse(reflectionCamera.viewMatrix())},
            .refractionProjectionMatrix{refractionCamera.projectionMatrix()},
            .mainToRefractionViewMatrix{refractionCamera.viewMatrix() * viewMatrixInverse},
            .refractionToMainViewMatrix{viewMatrix * glm::inverse(refractionCamera.viewMatrix())},
        };
        for (const auto cascadeIndex : std::views::iota(0, ShadowMapCascadeCount)) {
            const auto &shadowViewMatrix{shadowMapCamera.viewMatrix(cascadeIndex)};
            const auto shadowViewProjectionMatrix{
                shadowMapCamera.projectionMatrix(cascadeIndex) * shadowViewMatrix,
            };
            uboData.shadowViewMatrices[cascadeIndex] = shadowViewMatrix;
            uboData.shadowViewProjectionMatrices[cascadeIndex] = shadowViewProjectionMatrix;
            uboData.mainToShadowViewMatrices[cascadeIndex] = shadowViewMatrix * viewMatrixInverse;
            uboData.mainToShadowViewProjectionMatrices[cascadeIndex] = shadowViewProjectionMatrix
                                                                       * viewMatrixInverse;
            uboData.shadowMapDepthBlurScales[cascadeIndex]
                = glm::vec4{shadowMapCamera.getDepthBlurScale(cascadeIndex), 0.0f, 0.0f};
        }
        glBindBuffer(GL_UNIFORM_BUFFER, _ubo.get());
        checkError();
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformBufferData), &uboData);
        checkError();
    }

    {
        const auto &cameraPosition{camera->pose().position()};

        const std::lock_guard lock{_scene.terrainMutex()};
        const auto visibleChunks{_terrainStreamer.update(cameraPosition)};

        _shadowDepthProgram.use();
        for (const auto cascadeIndex : std::views::iota(0, ShadowMapCascadeCount)) {
            _shadowDepthProgram.setUniform("u_cascadeIndex", cascadeIndex);

            _shadowMapFramebuffer.bind(cascadeIndex);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            checkError();
            for (const auto chunk : visibleChunks) {
                chunk->draw(BlockFaceGroup::Opaque);
            }
        }

        const auto drawBlockFaceGroup{
            [&visibleChunks](const BlockFaceGroup group, const Camera &camera) {
                for (const auto chunk : visibleChunks) {
                    const auto boundingBox{chunk->rendererBoundingBox(group)};
                    if (!boundingBox.isEmpty() && camera.isInViewFrustum(boundingBox)) {
                        chunk->draw(group);
                    }
                }
            }};

        _geometryProgram.use();
        _geometryProgram.setUniform("u_cameraIndex", 0);
        _geometryProgram.setUniform("u_isAboveWaterOnly", 0);
        _geometryProgram.setUniform("u_isUnderWaterOnly", 0);

        _opaqueGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        drawBlockFaceGroup(BlockFaceGroup::Opaque, *camera);

        _translucentGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        drawBlockFaceGroup(BlockFaceGroup::Translucent, *camera);

        const auto waterLevel{
            static_cast<float>(WaterLevel)
            + getWaterWaveOffset(glm::vec2{cameraPosition.x, cameraPosition.z}, time)};
        const auto isAboveWater{cameraPosition.y >= waterLevel};

        _geometryProgram.setUniform("u_cameraIndex", 1);
        _geometryProgram.setUniform("u_isAboveWaterOnly", isAboveWater ? 1 : 0);
        _geometryProgram.setUniform("u_isUnderWaterOnly", isAboveWater ? 0 : 1);

        _reflectionGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        drawBlockFaceGroup(isAboveWater ? BlockFaceGroup::AboveWater : BlockFaceGroup::UnderWater,
                           reflectionCamera);

        _geometryProgram.setUniform("u_cameraIndex", 2);
        _geometryProgram.setUniform("u_isAboveWaterOnly", isAboveWater ? 0 : 1);
        _geometryProgram.setUniform("u_isUnderWaterOnly", isAboveWater ? 1 : 0);

        _refractionGeometryFramebuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkError();
        drawBlockFaceGroup(isAboveWater ? BlockFaceGroup::UnderWater : BlockFaceGroup::AboveWater,
                           refractionCamera);
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

    _lightingProgram.use();

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
        framebuffer->resize(deviceSize.width(), deviceSize.height());
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
