#!/bin/bash
# Copies the vendor libraries build.sh links against from a connected onyx phone (adb) into
# sysroot/vendor_lib64/. They are device binaries, so they are not kept in the repository.
set -euo pipefail
export MSYS_NO_PATHCONV=1
DIR="$(cd "$(dirname "$0")" && pwd)/vendor_lib64"
mkdir -p "$DIR"
for lib in android.hardware.sensors@1.0.so android.hardware.sensors@2.0.so \
           android.hardware.sensors@2.1.so android.hardware.sensors@2.0-ScopedWakelock.so \
           libbase.so libbinder_ndk.so libc++.so libcutils.so libhidlbase.so liblog.so \
           libsensorndkbridge.so libutils.so; do
    src=/vendor/lib64/$lib
    adb shell "[ -f $src ]" || src=/system/lib64/$lib  # VNDK/system copies on some builds
    out="$DIR/$lib"
    command -v cygpath >/dev/null && out="$(cygpath -w "$out")"
    adb pull "$src" "$out" >/dev/null
    echo "pulled $src"
done
