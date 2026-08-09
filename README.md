# Saikou Linux Native

Native Linux client for [Saikou](https://github.com/middlegear/Saikou).

This project splits the application into:
1. **Core Layer (`saikou-core`)**: A Kotlin Multiplatform (KMP) shared library that extracts the original Kotlin scraping logic (parsers and AniList connections). Compiles down to a Linux-native `libsaikou_core.so`.
2. **UI & Presentation Layer (`saikou-ui`)**: A C++ application using **Qt6** to provide a lightweight, native GUI that respects local platform styles (e.g., KDE Plasma/Breeze, GNOME/QGtkTheme).
3. **Media Engine**: Leverages `libmpv` embedded in Qt to replace ExoPlayer for high-performance, lightweight media playback on Linux.

See [PLAN.md](PLAN.md) for MVP requirements and the architecture breakdown.
