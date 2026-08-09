#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("Saikou"));
    QApplication::setApplicationVersion(QStringLiteral(SAIKOU_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Saikou"));
    // Must match the .desktop file's basename, or Wayland cannot map the window to its
    // launcher entry (no icon in the task manager, no window rules).
    QApplication::setDesktopFileName(QStringLiteral("io.github.saikou.Saikou"));

    MainWindow window;
    window.show();

    return QApplication::exec();
}
