#!/usr/bin/env bash
# Builds Saikou from this checkout and installs it into ~/.local for the current user.
#
# No root, no packaging step — the fastest way to get a working app on your own machine.
# For a system-wide install use packaging/PKGBUILD instead.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="${PREFIX:-$HOME/.local}"

say() { printf '\033[1;34m==>\033[0m %s\n' "$1"; }
die() { printf '\033[1;31merror:\033[0m %s\n' "$1" >&2; exit 1; }

say "Checking dependencies"
missing=()
command -v java    >/dev/null || missing+=("a Java runtime (jre-openjdk)")
command -v cmake   >/dev/null || missing+=("cmake")
command -v g++     >/dev/null || missing+=("gcc")
pkg-config --exists Qt6Widgets || missing+=("qt6-base")
pkg-config --exists mpv        || missing+=("mpv")

if (( ${#missing[@]} )); then
    printf 'Missing:\n'
    printf '  - %s\n' "${missing[@]}"
    printf '\nOn Arch:\n  sudo pacman -S --needed qt6-base mpv jre-openjdk jdk-openjdk cmake ninja base-devel\n'
    exit 1
fi

say "Building the core daemon"
"$root/gradlew" :saikou-core:installDist --no-daemon -q

say "Building the UI"
cmake -S "$root/saikou-ui" -B "$root/saikou-ui/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" >/dev/null
cmake --build "$root/saikou-ui/build" --parallel

say "Installing into $prefix"
install -d "$prefix/bin" "$prefix/lib/saikou" \
           "$prefix/share/applications" "$prefix/share/metainfo"

install -m755 "$root/saikou-ui/build/saikou-ui" "$prefix/bin/saikou-ui"

rm -rf "$prefix/lib/saikou/lib"
cp -r "$root/saikou-core/build/install/saikou-core/lib" "$prefix/lib/saikou/lib"

cat > "$prefix/bin/saikou-core" <<EOF
#!/bin/sh
exec java -cp "$prefix/lib/saikou/lib/*" ani.saikou.MainKt "\$@"
EOF
chmod +x "$prefix/bin/saikou-core"

install -m644 "$root/packaging/io.github.saikou.Saikou.desktop" \
    "$prefix/share/applications/io.github.saikou.Saikou.desktop"
install -m644 "$root/packaging/io.github.saikou.Saikou.metainfo.xml" \
    "$prefix/share/metainfo/io.github.saikou.Saikou.metainfo.xml"

# KDE reads the desktop database to register the saikou:// handler and show the launcher.
command -v update-desktop-database >/dev/null && \
    update-desktop-database "$prefix/share/applications" 2>/dev/null || true

say "Done"
echo
echo "Launch it from your application menu, or run: $prefix/bin/saikou-ui"
if ! printf '%s' ":$PATH:" | grep -q ":$prefix/bin:"; then
    echo
    echo "Note: $prefix/bin is not on your PATH. Add it with:"
    echo "  fish_add_path $prefix/bin        # fish"
    echo "  export PATH=\"$prefix/bin:\$PATH\"  # bash/zsh"
fi
