# Unported Android sources

`android-src/` holds the original Saikou Android sources verbatim. Nothing here is on any
compiled source set — Gradle does not see it.

Porting a file means moving it to `saikou-core/src/main/kotlin/` and:

1. Deleting the `android.*` / `androidx.*` imports and the Activity/Context plumbing.
2. Replacing `Log` with `ani.saikou.host.Log`, and `loadData`/`saveData` with
   `ani.saikou.host.Preferences`.
3. Replacing `NiceHttp` calls with the OkHttp wrapper (Phase 1).
4. Routing anything that needed a WebView through the `webengine` capability flag rather
   than calling it directly.

Jsoup and OkHttp stay — the core runs on the JVM (decision D1), so they work unchanged.

Anime parsers and extractors come first. Manga and novel sources stay here until the
anime MVP ships.
