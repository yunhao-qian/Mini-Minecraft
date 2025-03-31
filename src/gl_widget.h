#ifndef MINI_MINECRAFT_GL_WIDGET_H
#define MINI_MINECRAFT_GL_WIDGET_H

#include "gl_context.h"
#include "shader_program.h"

#include <QOpenGLWidget>
#include <QTimer>

namespace minecraft {

class GLWidget : public QOpenGLWidget, public GLContext
{
    Q_OBJECT

public:
    GLWidget(QWidget *const parent = nullptr);

protected:
    auto initializeGL() -> void override;
    auto resizeGL(const int width, const int height) -> void override;
    auto paintGL() -> void override;

private slots:
    auto tick() -> void;

private:
    QTimer _timer;
    ShaderProgram _programFlat;
};

} // namespace minecraft

#endif // MINI_MINECRAFT_GL_WIDGET_H
