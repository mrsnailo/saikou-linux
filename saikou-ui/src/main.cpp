#include "MainWindow.h"
#include "theme/Theme.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("Saikou"));
    QApplication::setApplicationVersion(QStringLiteral(SAIKOU_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Saikou"));
    QApplication::setOrganizationDomain(QStringLiteral("github.io.saikou"));
    // Must match the .desktop file's basename, or Wayland cannot map the window to its
    // launcher entry (no icon in the task manager, no window rules).
    QApplication::setDesktopFileName(QStringLiteral("io.github.saikou.Saikou"));

    // Installed before the first widget exists: the theme records which QStyle Qt resolved
    // on its own (Kvantum, Breeze, QGtkTheme…) so "follow the system" can hand back to it,
    // and it must see that style untouched.
    Theme::instance()->install();

    MainWindow window;
    window.show();

    return QApplication::exec();
}
