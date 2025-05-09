#ifndef MINECRAFT_OPENGL_WIDGET_H
#define MINECRAFT_OPENGL_WIDGET_H

#include "array_texture_2d.h"
#include "geometry_framebuffer.h"
#include "opengl_context.h"
#include "opengl_object.h"
#include "player_controller.h"
#include "player_info_display_data.h"
#include "scene.h"
#include "scene_settings.h"
#include "shader_program.h"
#include "shadow_map_framebuffer.h"
#include "terrain_streamer.h"

#include <QOpenGLWidget>
#include <QTimer>

#include <cstdint>
#include <utility>
#include <vector>

namespace minecraft {

class OpenGLWidget : public QOpenGLWidget, public OpenGLContext
{
    Q_OBJECT

public:
    OpenGLWidget(QWidget *const parent = nullptr);
    ~OpenGLWidget() override;

    SceneSettings &sceneSettings() { return _sceneSettings; }

signals:
    void playerInfoChanged(const PlayerInfoDisplayData &displayData);

protected:
    void initializeGL() override;

    void paintGL() override;

    void resizeGL(const int width, const int height) override;

    void keyPressEvent(QKeyEvent *const event) override;

    void mousePressEvent(QMouseEvent *const event) override;

private slots:
    void tick();

private:
    void bindTextures(const std::vector<std::pair<GLenum, GLuint>> &textures);

    QTimer _timer;
    qint64 _startingMSecs;
    qint64 _lastTickMSecs;

    Scene _scene;
    TerrainStreamer _terrainStreamer;
    PlayerController _playerController;

    SceneSettings _sceneSettings;
    std::int32_t _sceneSettingsVersion;

    ShaderProgram _shadowDepthProgram;
    ShaderProgram _geometryProgram;
    ShaderProgram _lightingProgram;
    OpenGLObject _ubo;
    ArrayTexture2D _colorTexture;
    ArrayTexture2D _normalTexture;
    ShadowMapFramebuffer _shadowMapFramebuffer;
    GeometryFramebuffer _opaqueGeometryFramebuffer;
    GeometryFramebuffer _translucentGeometryFramebuffer;
    GeometryFramebuffer _reflectionGeometryFramebuffer;
    GeometryFramebuffer _refractionGeometryFramebuffer;

    OpenGLObject _quadVAO;
};

} // namespace minecraft

#endif // MINECRAFT_OPENGL_WIDGET_H
