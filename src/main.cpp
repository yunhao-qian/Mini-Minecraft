#include "main_window.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char **const argv)
{
    using minecraft::MainWindow;

    const QApplication app{argc, argv};

    {
        QSurfaceFormat format;
        format.setVersion(4, 1);
        format.setOption(QSurfaceFormat::DeprecatedFunctions, false);
        format.setProfile(QSurfaceFormat::CoreProfile);
        // TODO: Multisampling is disabled by default. May consider enabling it in the future for
        // better anti-aliasing.
        QSurfaceFormat::setDefaultFormat(format);
    }

    MainWindow window;
    window.setVisible(true);

    return app.exec();
}
