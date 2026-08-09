# Saikou Linux — Architecture & MVP Plan

Target platform: Arch Linux + KDE Plasma 6 (Wayland first, X11 fallback). Everything below
is written against that target; other desktops are best-effort.

## 0. Current State (measured, not assumed)

- 88 Kotlin files copied verbatim from the Android app. No build files exist yet
  (`saikou-core` has no `build.gradle.kts`, `saikou-ui/CMakeLists.txt` is empty).
- 34 of 88 files import `android.*` / `androidx.*`.
- 12 files parse HTML with **Jsoup** (JVM-only).
- Networking is **OkHttp + NiceHttp** (JVM-only); `java.io` / `java.net` used in 26 files.
- 3 files require a **WebView** (`Network.kt` Cloudflare bypass, Discord login, MangaHub).

The files live under `commonMain/` but are not common code. Treat the current tree as
"vendored Android source pending port", not as a KMP module.

## 1. Architecture

Split into a scraping core and a native presentation layer.

### `saikou-core` — Kotlin
Houses `ani.saikou.parsers` and `ani.saikou.connections`.

**Decision required before Phase 1 (see §2): JVM core or Kotlin/Native core.**
The plan below assumes **JVM-first**, because it is the only option that keeps Jsoup and
OkHttp working unchanged, and it is reversible.

- Replace Android-specific glue (`Context`, `SharedPreferences`, `Log`, `Uri`) with small
  `expect`/interface seams implemented by the host.
- `kotlinx.serialization` for every value crossing the boundary — JSON strings, no struct
  mapping.
- Ships as a headless daemon process (`saikou-core`) speaking JSON-RPC over a Unix domain
  socket in `$XDG_RUNTIME_DIR/saikou/core.sock`.

### `saikou-ui` — Qt6 C++
- Presentation only. No scraping logic.
- `libmpv` via the **render API** (`mpv_render_context`) drawn into a `QOpenGLWidget` —
  **not** the `wid` embedding path, which does not work on Wayland.
- Post-MVP: manga reader on a custom `QGraphicsView` + tiled `QImage` pipeline.
- Download manager in C++ (`QNetworkAccessManager` + resumable ranged GETs).

### Boundary: JSON-RPC over Unix socket (replaces the C-FFI bridge)
Chosen over `libsaikou_core.so` + C headers because it removes the hardest unsolved
problems in one move: no C string ownership rules, no Kotlin/Native threading
restrictions, no ABI stability concerns, and a crashing parser cannot take the UI down.

Contract:
- Line-delimited JSON, JSON-RPC 2.0 framing.
- Every call is async; the UI never blocks its event loop.
- Server-initiated notifications for progress (download %, episode list streaming).
- Error envelope: `{code, message, source, retryable}` — the UI needs `retryable` to
  decide between a toast and a source-switch prompt.
- The daemon is supervised by the UI: spawn on launch, restart on crash, surface a
  status indicator when down.

If the core is later rebuilt as Kotlin/Native, the same JSON schema becomes the FFI
payload. The boundary shape is deliberately identical either way.

## 2. Decisions (settled — recorded 2026-08-09)

| # | Decision | Outcome |
|---|---|---|
| D1 | Core runtime | **Kotlin/JVM.** Keeps Jsoup and OkHttp working unchanged, so porting a parser means deleting Android imports, not rewriting its selector layer. Revisit only if idle RSS exceeds ~250 MB. |
| D2 | UI toolkit | **Qt Widgets.** Picks up Breeze on Plasma and QGtkTheme on GNOME with no styling work. Revisit for the player chrome only. |
| D3 | WebView-dependent sources | **Optional `saikou-webengine` package.** `core.capabilities` reports `webengine: false` by default; affected sources grey out with a tooltip instead of failing at click time. |
| D4 | Cloudflare bypass | Tied to D3. Per-source capability flags live in the source registry (Phase 2). |

**MVP scope: anime only.** Manga and novel parsers stay in `vendor/android-src` unported.
`core.capabilities` reports them as false so the UI never offers a dead path.

## 3. UX Requirements (first-class, not polish)

The app must feel like a Plasma application, not a ported Android app.

**Desktop integration**
- Follow the system color scheme live (`KColorScheme` / `QPalette` change events) — dark
  mode switches without restart.
- **MPRIS2** D-Bus interface so playback appears in the Plasma panel, media applet, and
  on media keys.
- Screensaver/idle inhibit during playback (`org.freedesktop.ScreenSaver`).
- Desktop notifications via the freedesktop Notification spec (new episode, download
  finished), respecting Do Not Disturb.
- `.desktop` entry with actions, AppStream metainfo, and a scalable icon so KRunner and
  Discover show the app properly.
- `saikou://` URI handler registered via the `.desktop` `MimeType` — needed for the
  AniList/MAL/Discord OAuth redirect. Fallback: transient `localhost` listener.
- HiDPI and fractional scaling correct on Wayland.

**Interaction**
- Keyboard-first: `/` focuses search, arrows navigate the grid, `Space` plays, `Esc`
  goes back. Every action reachable without a mouse.
- Nothing blocks the UI thread. Grids show skeleton placeholders, never a frozen window.
- Errors are recoverable in place: a failed source offers "retry" and "try another
  source" inline, not a dead end.
- Session restore: reopens on the last screen, resumes playback position.
- Player: remembers volume/speed/subtitle track per series, supports drag-to-seek
  preview thumbnails, and passes through mpv keybinds users already know.
- Episode list: watched state visible at a glance, resume from the exact position, and a
  clear marker for the next unwatched episode.

**Non-goals for MVP** — state them so scope stays honest: **no manga**, no novel sources,
no Discord RPC, no download manager UI (background downloads only), no multi-profile
support.

## 4. Phased Delivery

Each phase ends with something runnable. No phase ports "all parsers" before any UI
exists.

### Phase 0 — Scaffolding ✅ done
- Gradle build for `saikou-core` (JVM 21, wrapper pinned to 8.14.3), CMake build for
  `saikou-ui` (Qt 6.4+).
- The 88 Android source files moved to `saikou-core/vendor/android-src`, outside every
  compiled source set. Files move into `src/main/kotlin` as they are ported, which makes
  the remaining work countable.
- Host seams replacing the Android APIs: `host/Paths` (XDG dirs), `host/Log`,
  `host/Preferences` (atomic JSON in `$XDG_CONFIG_HOME/saikou`).
- JSON-RPC daemon: Unix socket, line-delimited, concurrent per-request dispatch,
  structured errors carrying `retryable`, single-instance file lock.
- Qt shell: window, search field, live core-status indicator, `/` and `Esc` shortcuts,
  and `CoreProcess` supervising the daemon with restart backoff.
- `scripts/rpc-smoke-test.sh` and a GitHub Actions job building both halves on Arch.

### Phase 1 — Vertical slice, one anime source
- Port **AllAnime only** end-to-end: strip Android imports, route it through the host
  seams, and replace NiceHttp with a thin OkHttp wrapper keeping the same call surface
  (`client.get(...).document` / `.parsed<T>()`).
- RPC methods: `anime.search`, `anime.details`, `anime.episodes`, `anime.servers`,
  `anime.streams`.
- Qt: result grid, details page, episode list, mpv playback via the render API.
- Exit criterion: search a title, pick an episode, watch it, on Wayland.

### Phase 2 — Breadth of anime sources
- Port the remaining **anime** parsers and extractors behind the same interfaces.
  Manga and novel parsers stay in `vendor/` until after the MVP ships.
- Source registry with per-source health status and the D3/D4 capability flags.
- **Parser smoke-test harness**: a headless CLI that runs every source's search+load
  against the live site and reports pass/fail. Scrapers rot constantly; this is the only
  way to know what is broken before users do. Run it on a schedule in CI.

### Phase 3 — AniList/MAL and library
- OAuth via the `saikou://` handler, token stored in the **KDE Wallet / Secret Service**
  API — not a plaintext file.
- Home screen, watch lists, progress sync.

### Phase 4 — Downloads and desktop integration
- Background download manager with resume.
- MPRIS2, notifications, idle inhibit, session restore.

### Phase 5 — Packaging for Arch
- `PKGBUILD` targeting AUR: depends `qt6-base qt6-svg mpv`, plus a JRE if D1 stays JVM.
- Optional `saikou-webengine` split package.
- `.desktop`, AppStream metainfo, icons, and a `makepkg`-clean build from a fresh chroot.

## 5. Risks

- **Scraper rot** — sources break weekly. Mitigated by the Phase 2 test harness and
  per-source health flags, not by hoping.
- **Legal/distribution** — AUR hosts the build recipe, not the content; keep it that way
  and ship no bundled source lists that imply otherwise.
- **mpv on Wayland** — the render-API path is mandatory; verify early in Phase 1 rather
  than discovering it at Phase 4.
- **JVM footprint** — if RSS at idle exceeds ~250 MB, revisit D1.
