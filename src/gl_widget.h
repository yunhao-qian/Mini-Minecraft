#ifndef MINI_MINECRAFT_GL_WIDGET_H
#define MINI_MINECRAFT_GL_WIDGET_H

#include "gl_context.h"
#include "player_controller.h"
#include "player_info_display_data.h"
#include "scene.h"
#include "shader_program.h"
#include "terrain_streamer.h"

#include <QOpenGLWidget>
#include <QString>
#include <QTimer>

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
    auto loadTexture(const QString &fileName) -> GLuint;

    QTimer _timer;
    qint64 _lastTickMilliseconds;

    Scene _scene;
    TerrainStreamer _terrainStreamer;
    PlayerController _playerController;
    ShaderProgram _program;
    GLuint _colorTexture;
    GLuint _normalTexture;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_GL_WIDGET_H
