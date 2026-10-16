#!/bin/sh
# Usage: ./build.sh desktop | android | install | log
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

build_android() {
    (cd rust && cargo ndk -t arm64-v8a --platform 26 build --release -p glo-godot)
    mkdir -p godot/bin/android
    cp rust/target/aarch64-linux-android/release/libglo_godot.so godot/bin/android/
    mkdir -p build
    (cd godot && "$GODOT" --headless --import && "$GODOT" --headless --export-debug Android)
}

case "${1:-}" in
    desktop) build_desktop ;;
    android) build_android ;;
    install) build_android && adb install -r build/glo-mobile.apk ;;
    log) adb logcat -s glo ;;
    *) echo "usage: $0 desktop | android | install | log" >&2; exit 1 ;;
esac
