#ifndef MINI_MINECRAFT_GL_WIDGET_H
#define MINI_MINECRAFT_GL_WIDGET_H

#include "framebuffer.h"
#include "gl_context.h"
#include "player_controller.h"
#include "player_info_display_data.h"
#include "scene.h"
#include "shader_program.h"
#include "terrain_streamer.h"
#include "vertex.h"
#include "vertex_array_helper.h"

#include <QOpenGLWidget>
#include <QString>
#include <QTimer>

#include <memory>

namespace minecraft {

class GLWidget : public QOpenGLWidget, public GLContext
{
    Q_OBJECT

public:
    GLWidget(QWidget *const parent = nullptr);

signals:
    auto playerInfoChanged(const minecraft::PlayerInfoDisplayData &data) -> void;

protected:
    auto initializeGL() -> void override;
    auto resizeGL(const int width, const int height) -> void override;
    auto paintGL() -> void override;

    auto keyPressEvent(QKeyEvent *const event) -> void override;

    auto mousePressEvent(QMouseEvent *const event) -> void override;

private slots:
    auto tick() -> void;

private:
    auto loadTextureArray(const QString &filename) -> GLuint;

    QTimer _timer;
    qint64 _startingMSecs;
    qint64 _lastTickMSecs;

    Scene _scene;
    TerrainStreamer _terrainStreamer;
    PlayerController _playerController;
    ShaderProgram _lambertProgram;
    ShaderProgram _postProcessingProgram;
    Framebuffer _solidBlocksFramebuffer;
    Framebuffer _liquidBlocksFramebuffer;
    std::unique_ptr<VertexArrayHelper<EmptyVertex>> _postProcessingHelper;
    GLuint _colorTextureArray;
    GLuint _normalTextureArray;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_GL_WIDGET_H
