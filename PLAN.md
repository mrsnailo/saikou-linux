# Architecture & MVP Plan

## Architecture

To satisfy the constraints of "native Plasma/GTK themes" and "lightweight" without a full rewrite of the parser logic, the application uses a split architecture:

1. **`saikou-core` (Kotlin Multiplatform)**
   - Houses the `ani.saikou.parsers` and `ani.saikou.connections`.
   - Replaces Android-specific networking (`NiceHttp`) with KMP `Ktor`.
   - Exports C headers for a shared library (`libsaikou_core.so`).
   - Uses `kotlinx.serialization` to share data objects over the C/Kotlin boundary as JSON strings, avoiding complex struct mappings.

2. **`saikou-ui` (Qt6 C++)**
   - Pure native presentation layer.
   - Embeds `libmpv` for video playback (replacing ExoPlayer).
   - Custom QGraphicsView / QImage pipeline for manga reading (replacing SubsamplingScaleImageView).
   - Asynchronous download manager in C++ (replacing WorkManager).

## Gaps Identified from Android Source
- **Android Coupling:** Need to rewrite networking in KMP and abstract preferences.
- **FFI Boundary:** Need an efficient JSON bridge between the Qt frontend and Kotlin core.
- **Media Engine:** Built from scratch on Qt/mpv.

## MVP (Minimum Viable Product) Deliverables

### Phase 1: Core Parsing via KMP
- Decouple `parsers` and `connections` into KMP.
- Rewrite HTTP calls using `Ktor`.
- Build the `libsaikou_core.so` Linux shared object and C header.

### Phase 2: Qt6 UI Foundation
- Scaffold Qt6 C++ project.
- Implement the FFI JSON bridge to `saikou-core`.
- Build Home, Search, and basic Details screens using system-native styles.

### Phase 3: Media & Reader
- Integrate `libmpv` in Qt6 for anime playback.
- Implement basic manga reader view.
