# Saikou Linux Native

Native Linux anime client with AniList tracking, targeting Arch Linux and KDE Plasma 6.
Qt 6 Widgets front end, mpv for playback, a Kotlin/JVM daemon for scraping.

**MVP scope is anime only.** Manga and novel support come later.

## Status

Working today:

- Home screen with Trending, This season, and Continue watching from AniList
- AniList search, title details, sign-in, and automatic progress sync while you watch
- mpv playback through the render API (works on Wayland), with subtitles and referer-locked streams
- The full source pipeline: search → episodes → servers → stream resolution

**Not working: video sources.** All seven ported sources (Anikoto, AniBD, Anizone,
AnimeHeaven, AniDB, AllAnime, AnimePahe) proxy through a single private backend API. The
Android app injected its address and key at build time as `BuildConfig.SERVER_URL` and
`BuildConfig.MY_CUSTOM_API_KEY`; neither value is in this repository. Until you supply
them the sources report themselves unavailable and the app explains why. See
[Anime sources](#anime-sources).

## Installing

### From a release (recommended on Arch)

Download the `.pkg.tar.zst` from the [latest release](https://github.com/mrsnailo/saikou-linux/releases)
and install it:

```sh
sudo pacman -U saikou-linux-*-x86_64.pkg.tar.zst
```

pacman pulls in `qt6-base`, `mpv` and a JRE. Saikou then appears in your application
launcher and KRunner.

### From source

```sh
sudo pacman -S --needed qt6-base mpv jdk21-openjdk cmake ninja base-devel
./scripts/install-local.sh
```

That installs into `~/.local` without root. To build a package instead, use
`packaging/release/PKGBUILD` with a source tarball, or `packaging/PKGBUILD` for an
AUR-style build straight from git.

## Connecting AniList

Saikou signs in with your own AniList API client, so no shared secret ships in the binary.

1. Open <https://anilist.co/settings/developer> and create a new client.
2. Set its redirect url to exactly `http://localhost:8998/callback`.
3. In Saikou, open **Settings**, paste the client id and secret, and press **Save client**.
4. Press **Sign in to AniList**. Your browser opens; approve, and the tab closes itself.

Progress syncs automatically once you pass 85% of an episode.

## Anime sources

Set the backend in **Settings → Anime backend**, or with environment variables:

```sh
SAIKOU_API_HOST=https://your-backend SAIKOU_API_KEY=your-key saikou-ui
```

Both are also stored in `~/.config/saikou/settings.json` once saved. Supplying them
enables all seven sources at once.

If you do not have that backend, the alternative is a new parser written against a public
site. `saikou-core/vendor/README.md` describes the porting contract.

## Layout

| Path | What it is |
|---|---|
| `saikou-core/` | Kotlin/JVM scraping daemon. JSON-RPC over a Unix socket. |
| `saikou-core/vendor/android-src/` | Original Android sources, unported and uncompiled. |
| `saikou-ui/` | Qt 6 Widgets application and the mpv player. |
| `packaging/` | PKGBUILD, desktop entry, AppStream metadata. |
| `scripts/` | Local installer and the RPC smoke test. |

The UI spawns and supervises the daemon, so a crashing parser cannot take the window down.

## Development

```sh
./gradlew :saikou-core:installDist
cmake -S saikou-ui -B saikou-ui/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build saikou-ui/build

SAIKOU_CORE="$PWD/saikou-core/build/install/saikou-core/bin/saikou-core" ./saikou-ui/build/saikou-ui
```

`./scripts/rpc-smoke-test.sh` checks that the daemon boots and answers. Run
`saikou-core --debug` by hand and the UI will attach to that instance instead of spawning
its own.

## Keyboard

| Key | Action |
|---|---|
| `/` | Focus search |
| `Enter` | Open the selected title / play the selected episode |
| `Esc` | Back |
| `Space` / `K` | Play or pause |
| `←` `→` | Seek 10 seconds |
| `S` | Skip the opening |
| `F` | Fullscreen |
