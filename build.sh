#!/bin/sh
# Usage: ./build.sh desktop | android | release | aab | install | log | version
# release, aab: signed with GODOT_ANDROID_KEYSTORE_RELEASE_PATH / _USER / _PASSWORD
# aab: Play Store upload, Gradle build. The version comes from rust/Cargo.toml; bump it for every upload,
# then tag the release as v<version>. version/code is major*10000 + minor*100 + patch.
set -eu
cd "$(dirname "$0")"

BREW_NDK=/opt/homebrew/share/android-commandlinetools/ndk/29.0.14206865
if [ -z "${ANDROID_NDK_HOME:-}" ] && [ -d "$BREW_NDK" ]; then
    export ANDROID_NDK_HOME="$BREW_NDK"
fi
GODOT="${GODOT:-godot}"

[ -e firmware/.git ] || git submodule update --init

# Godot extension for the host OS. Windows needs the MinGW target: the firmware shim uses pthreads and C11 atomics.
build_desktop() {
    case "$(uname -s)" in
        Darwin) os=macos; target=; lib=libglo_godot.dylib ;;
        Linux) os=linux; target=; lib=libglo_godot.so ;;
        MINGW* | MSYS* | CYGWIN*) os=windows; target=x86_64-pc-windows-gnu; lib=glo_godot.dll ;;
        *) echo "unsupported OS: $(uname -s)" >&2; exit 1 ;;
    esac
    (cd rust && cargo build --release -p glo-godot ${target:+--target "$target"})
    mkdir -p "godot/bin/$os"
    cp "rust/target/${target:+$target/}release/$lib" "godot/bin/$os/"
}

version() {
    sed -n 's/^version = "\(.*\)"/\1/p' rust/Cargo.toml | head -n 1
}

# Writes the Cargo version into both Android export presets.
sync_version() {
    v=$(version)
    code=$(echo "$v" | awk -F. '{ print $1 * 10000 + $2 * 100 + $3 }')
    sed "s/^version\/name=.*/version\/name=\"$v\"/; s/^version\/code=.*/version\/code=$code/" \
        godot/export_presets.cfg > godot/export_presets.cfg.tmp
    mv godot/export_presets.cfg.tmp godot/export_presets.cfg
}

build_android() {
    sync_version
    (cd rust && cargo ndk -t arm64-v8a --platform 26 build --release -p glo-godot)
    mkdir -p godot/bin/android
    cp rust/target/aarch64-linux-android/release/libglo_godot.so godot/bin/android/
    mkdir -p build
    (cd godot && "$GODOT" --headless --import)
    if [ "${PRESET:-Android}" = "Android AAB" ] && [ ! -e godot/android/.build_version ]; then
        (cd godot && "$GODOT" --headless --install-android-build-template)
    fi
    (cd godot && "$GODOT" --headless "--${EXPORT:-export-debug}" "${PRESET:-Android}")
}

case "${1:-}" in
    desktop) build_desktop ;;
    android) build_android ;;
    release) EXPORT=export-release build_android ;;
    aab) EXPORT=export-release PRESET="Android AAB" build_android ;;
    install) build_android && adb install -r build/clau.apk ;;
    log) adb logcat -s glo ;;
    version) version ;;
    *) echo "usage: $0 desktop | android | release | aab | install | log | version" >&2; exit 1 ;;
esac
