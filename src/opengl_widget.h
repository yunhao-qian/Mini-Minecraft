#ifndef MINI_MINECRAFT_OPENGL_WIDGET_H
#define MINI_MINECRAFT_OPENGL_WIDGET_H

#include "array_texture_2d.h"
#include "framebuffer.h"
#include "opengl_context.h"
#include "player_controller.h"
#include "player_info_display_data.h"
#include "scene.h"
#include "shader_program.h"
#include "shadow_map_framebuffer.h"
#include "terrain_streamer.h"

#include <QOpenGLWidget>
#include <QTimer>

#include <utility>
#include <vector>

namespace minecraft {

class OpenGLWidget : public QOpenGLWidget, public OpenGLContext
{
    Q_OBJECT

public:
    OpenGLWidget(QWidget *const parent = nullptr);
    ~OpenGLWidget() override;

signals:
    void playerInfoChanged(const PlayerInfoDisplayData &data);

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

    ShaderProgram _shadowDepthProgram;
    ShaderProgram _geometryProgram;
    ShaderProgram _lightingProgram;
    ArrayTexture2D _colorTexture;
    ArrayTexture2D _normalTexture;
    ShadowMapFramebuffer _shadowMapFramebuffer;
    Framebuffer _opaqueGeometryFramebuffer;
    Framebuffer _translucentGeometryFramebuffer;

    GLuint _lightingVAO;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_OPENGL_WIDGET_H
