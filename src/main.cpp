#include "main_window.h"

#include <QApplication>
#include <QSurfaceFormat>

auto main(int argc, char **const argv) -> int
{
    const QApplication app{argc, argv};

    {
        QSurfaceFormat format;
        format.setVersion(4, 1);
        format.setOption(QSurfaceFormat::DeprecatedFunctions, false);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSamples(4);
        QSurfaceFormat::setDefaultFormat(format);
    }
    qDebug() << "Default surface format:" << QSurfaceFormat::defaultFormat();

    minecraft::MainWindow window;
    window.show();

    return app.exec();
}
