# Saikou Linux Native

Native Linux client for [Saikou](https://github.com/middlegear/Saikou), targeting Arch
Linux and KDE Plasma 6 (Wayland).

**Status: Phase 0.** The build, the core daemon, and the UI↔core transport work
end to end. No sources are ported yet — see [PLAN.md](PLAN.md).

**MVP scope is anime only.** Manga and novel support come after the anime path ships.

## Layout

| Path | What it is |
|---|---|
| `saikou-core/` | Kotlin/JVM scraping daemon. Speaks JSON-RPC over a Unix socket. |
| `saikou-core/vendor/android-src/` | The original Android sources, unported and uncompiled. Files move into `src/main/kotlin` as they are ported. |
| `saikou-ui/` | Qt 6 Widgets application. Presentation only, no scraping logic. |
| `packaging/` | `.desktop` entry and AppStream metadata. |
| `scripts/` | Developer and CI helpers. |

The two halves talk over `$XDG_RUNTIME_DIR/saikou/core.sock`. The UI spawns and
supervises the daemon, so a crashing parser cannot take the window down with it.

## Building

Requires JDK 21, CMake 3.21+, and Qt 6.4+.

```sh
# Core daemon
./gradlew :saikou-core:installDist

# UI
cmake -S saikou-ui -B saikou-ui/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build saikou-ui/build
```

On Arch: `pacman -S jdk21-openjdk cmake ninja qt6-base mpv`.

## Running

```sh
SAIKOU_CORE="$PWD/saikou-core/build/install/saikou-core/bin/saikou-core" \
    ./saikou-ui/build/saikou-ui
```

The UI starts the daemon itself. To debug against a daemon you control, run
`saikou-core --debug` first — the UI detects the running instance and attaches to it.

`./scripts/rpc-smoke-test.sh` checks that the daemon still boots and answers.
