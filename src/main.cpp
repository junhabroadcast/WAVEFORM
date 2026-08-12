#include "ui/MainWindow.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char* argv[])
{
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(0);
    fmt.setStencilBufferSize(0);
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("WfmMonitor"));
    QApplication::setOrganizationName(QStringLiteral("WFM"));

    MainWindow w;
    w.show();
    return app.exec();
}
