cmake_minimum_required(VERSION 3.21)

project(saikou-ui
    VERSION 0.1.0
    DESCRIPTION "Native Linux client for Saikou"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Qt Widgets, not QtQuick: decision D2. Widgets pick up Breeze on Plasma and QGtkTheme
# on GNOME for free, which is the whole point of the native-look requirement.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network DBus OpenGLWidgets)

# libmpv is the player, not an optional extra. Found through pkg-config so a packager
# gets a clear configure-time error rather than a link failure.
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

qt_standard_project_setup()

add_executable(saikou-ui
    src/main.cpp
    src/CoreClient.cpp
    src/CoreClient.h
    src/CoreProcess.cpp
    src/CoreProcess.h
    src/DetailsPage.cpp
    src/DetailsPage.h
    src/ImageLoader.cpp
    src/ImageLoader.h
    src/MainWindow.cpp
    src/MainWindow.h
    src/MpvWidget.cpp
    src/MpvWidget.h
    src/PlayerWindow.cpp
    src/PlayerWindow.h

    src/model/Media.cpp
    src/model/Media.h
    src/model/View.h

    src/theme/Icons.cpp
    src/theme/Icons.h
    src/theme/Motion.cpp
    src/theme/Motion.h
    src/theme/Theme.cpp
    src/theme/Theme.h
    src/theme/Type.cpp
    src/theme/Type.h

    src/widgets/Controls.cpp
    src/widgets/Controls.h
    src/widgets/DetailWidgets.cpp
    src/widgets/DetailWidgets.h
    src/widgets/FlowLayout.cpp
    src/widgets/FlowLayout.h
    src/widgets/HeroBanner.cpp
    src/widgets/HeroBanner.h
    src/widgets/MediaViews.cpp
    src/widgets/MediaViews.h
    src/widgets/PosterCard.cpp
    src/widgets/PosterCard.h
    src/widgets/SideBar.cpp
    src/widgets/SideBar.h
    src/widgets/StateView.cpp
    src/widgets/StateView.h
    src/widgets/ToastHost.cpp
    src/widgets/ToastHost.h
    src/widgets/TopBar.cpp
    src/widgets/TopBar.h

    src/pages/BrowsePage.cpp
    src/pages/BrowsePage.h
    src/pages/CalendarPage.cpp
    src/pages/CalendarPage.h
    src/pages/DownloadsPage.cpp
    src/pages/DownloadsPage.h
    src/pages/GenresPage.cpp
    src/pages/GenresPage.h
    src/pages/HomePage.cpp
    src/pages/HomePage.h
    src/pages/LibraryPage.cpp
    src/pages/LibraryPage.h
    src/pages/ScrollPage.cpp
    src/pages/ScrollPage.h
    src/pages/SettingsPage.cpp
    src/pages/SettingsPage.h
)

target_link_libraries(saikou-ui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::DBus
    Qt6::OpenGLWidgets
    PkgConfig::MPV
)

target_compile_definitions(saikou-ui PRIVATE
    SAIKOU_VERSION="${PROJECT_VERSION}"
)

target_compile_options(saikou-ui PRIVATE -Wall -Wextra -Wpedantic)

include(GNUInstallDirs)

install(TARGETS saikou-ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../packaging/io.github.saikou.Saikou.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../packaging/io.github.saikou.Saikou.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../packaging/io.github.saikou.Saikou.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
