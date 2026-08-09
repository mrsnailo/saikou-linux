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
    src/MediaGrid.cpp
    src/MediaGrid.h
    src/MpvWidget.cpp
    src/MpvWidget.h
    src/PlayerWindow.cpp
    src/PlayerWindow.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
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
