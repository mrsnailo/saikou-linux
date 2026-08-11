# Saikou Linux Native

Native Linux anime client with AniList tracking, targeting Arch Linux and KDE Plasma 6.
Qt 6 Widgets front end, mpv for playback, a Kotlin/JVM daemon for scraping.

**MVP scope is anime only.** Manga and novel support come later.

## Status

Working today:

- The full desktop shell: sidebar navigation, search bar, hero, poster rails, and a
  detail page with metadata, genres, recommendations and an episode list
- Home, Browse (filter by sort / format / genre), Library, Genres and an airing Calendar
- AniList search, title details, sign-in, and automatic progress sync while you watch
- mpv playback through the render API (works on Wayland), with subtitles and referer-locked streams
- The full source pipeline: search → episodes → servers → stream resolution

**Sources:** AniBD, KickAssAnime and AnimeHeaven work out of the box — all three scrape
their site directly and need no configuration. The remaining four (Anikoto, Anizone, AniDB,
AnimePahe) proxy through a single private backend API. The Android app injected its address
and key at build time as `BuildConfig.SERVER_URL` and `BuildConfig.MY_CUSTOM_API_KEY`;
neither value is in this repository, so those four report themselves unavailable until you
supply them. See [Anime sources](#anime-sources).

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

`jdk21-openjdk`, not `jdk-openjdk`: the core targets Java 21, and Arch's unversioned JDK
tracks whatever release is newest — often ahead of what Gradle supports, which fails the
build before it reaches a task. If 21 is not installed, Gradle provisions it itself
through the toolchain resolver, at the cost of a download.

That installs into `~/.local` without root. To build a package instead, use
`packaging/release/PKGBUILD` with a source tarball, or `packaging/PKGBUILD` for an
AUR-style build straight from git.

## Connecting AniList

**Settings → Account → Sign in with AniList.** Your browser opens, you approve, and the
tab hands the token back. Nothing to create, nothing to paste.

That needs the app to hold an AniList client id **and** secret. AniList rejects
`response_type=token` with `unsupported_grant_type` and answers `invalid_client` for a
token request without a secret, so the authorization-code grant is the only one available
and PKCE is not an alternative.

The client is therefore **injected at build time and never committed**:

```sh
SAIKOU_ANILIST_CLIENT_ID=… SAIKOU_ANILIST_CLIENT_SECRET=… ./scripts/install-local.sh
```

Gradle writes it into a generated `BundledClient.kt`; the equivalent Gradle properties are
`saikou.anilist.clientId` and `saikou.anilist.clientSecret`. Release builds get it from the
`ANILIST_CLIENT_ID` / `ANILIST_CLIENT_SECRET` repository secrets. The daemon also reads the
same two environment variables at runtime.

A build without them still works — sign-in then asks for a client of your own. **Use my own
AniList client** in the same panel takes an id and secret; register the client at
<https://anilist.co/settings/developer> with the redirect url `http://localhost:8998/callback`.

Progress syncs automatically once you pass 85% of an episode.

## Anime sources

Three sources need nothing at all:

| Source | How it works | What you get |
|---|---|---|
| **AniBD** | The same open JSON API its own site calls. Episodes are indexed by **AniList id**, so there is no title guessing at all. | HLS up to 1080p, Blu-ray rips. The default. |
| **KickAssAnime** | The JSON API its own web player uses. `kaa.to` only redirects — the live mirror rotates, so the parser resolves it once per run. | HLS, several resolutions, real subtitle tracks. |
| **AnimeHeaven** | Plain HTML scrape. An episode is selected by a cookie rather than a url, so the "server" carries that cookie. | Direct MP4 across three CDN mirrors. No subtitle tracks — the video is hardsubbed. |

AniBD reached the Android app through the private backend, at `/api/anibd/…`, which is why
it looked like it needed one. It does not: `anibd.app` is a front end over three open
endpoints that want no key. Because it keys on the AniList id, opening a title goes
straight to its episode list instead of searching for a title that matches.

**AllAnime** also needs no backend, but its endpoint sits behind Cloudflare, which lets
some residential connections through and challenges datacentre and VPN addresses; when
that happens the source says so rather than failing silently. Anything it plays,
KickAssAnime plays too, so it is not the default.

The remaining four proxy through the private backend API. Set it in **Settings → Sources**,
or with environment variables:

```sh
SAIKOU_API_HOST=https://your-backend SAIKOU_API_KEY=your-key saikou-ui
```

Both are also stored in `~/.config/saikou/settings.json` once saved. Supplying them
enables all eight sources at once. The three above keep working either way.

If you do not have that backend, the alternative is a new parser written against a public
site. `saikou-core/vendor/README.md` describes the porting contract.

## Theming

**Settings → Appearance** offers two themes:

| Theme | What it does |
|---|---|
| Saikou Dark | The designed look: black surfaces, pink accent, periwinkle labels. Default. |
| Follow system | Hands every stock control back to the platform Qt style. |

There is deliberately no light brand theme. The design is drawn against black surfaces and
the light palette did not hold up against it. A light interface is still available, and a
better one, through "Follow system" with a light platform style.

"Follow system" is the Kvantum option. Saikou installs **no application stylesheet** in
that mode, because any stylesheet puts `QStyleSheetStyle` in front of the platform style
and Kvantum's SVG rendering degrades behind it. Buttons, inputs, scrollbars and menus are
then drawn by Kvantum (or Breeze, or QGtkTheme) exactly as in any other Qt app.

Saikou's own painted surfaces — poster cards, rails, the sidebar, the hero — cannot be
drawn by a QStyle, so they take their colours from the active theme instead: the palette
first, and, when Kvantum is the active style, the `[GeneralColors]` block of the selected
Kvantum theme's `.kvconfig` on top of it. That is where the accent, base, alt-base and
link colours come from, so a Kvantum theme's real accent reaches the score pills and
active-nav markers rather than only its palette approximation.

Saikou never forces a style, so the usual mechanisms all work unchanged:

```sh
QT_STYLE_OVERRIDE=kvantum saikou-ui
QT_QPA_PLATFORMTHEME=qt6ct saikou-ui   # with Kvantum selected in qt6ct
```

The type scale is set in [Outfit](https://fonts.google.com/specimen/Outfit); if it is not
installed the stack falls back through Inter, Cantarell and Noto Sans. `ttf-outfit` from
the AUR gets the intended typography.

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
| `1` `2` `3` `4` | Home / Browse / Library / Downloads |
| `Ctrl ,` | Settings |
| `/` or `Ctrl K` | Focus search |
| `Enter` | Open the selected title / play the selected episode |
| `Esc` | Back |
| `Space` / `K` | Play or pause |
| `←` `→` | Seek 10 seconds |
| `S` | Skip the opening |
| `F` | Fullscreen |
