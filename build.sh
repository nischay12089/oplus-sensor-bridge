#!/bin/bash
# Builds sensors.oplusbridge.so (vendor sub-HAL) and oplus-sensor-bridged (root daemon),
# then packages the KernelSU module zip.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
AOSP="${AOSP:-$ROOT/aosp}"
NDK="${NDK:-/c/Users/Nischay_12089/AppData/Local/Android/Sdk/ndk/26.3.11579264}"
TC="$NDK/toolchains/llvm/prebuilt/windows-x86_64/bin"
CXX="$TC/clang++.exe"
TARGET="--target=aarch64-linux-android34"
OUT="$ROOT/out"
mkdir -p "$OUT"

GEN="$AOSP/v30/generated-headers"
INC=(
    -I"$ROOT/common"
    -I"$AOSP/multihal/include"
    -I"$GEN/hardware/interfaces/sensors/1.0/android.hardware.sensors@1.0_genc++_headers/gen"
    -I"$GEN/hardware/interfaces/sensors/2.0/android.hardware.sensors@2.0_genc++_headers/gen"
    -I"$GEN/hardware/interfaces/sensors/2.1/android.hardware.sensors@2.1_genc++_headers/gen"
    -I"$GEN/system/libhidl/transport/base/1.0/android.hidl.base@1.0_genc++_headers/gen"
    -I"$GEN/system/libhidl/transport/manager/1.0/android.hidl.manager@1.0_genc++_headers/gen"
    -I"$AOSP/v30/system/libhidl/base/include"
    -I"$AOSP/v30/system/libhidl/transport/include"
    -I"$AOSP/v30/system/libfmq/include"
    -I"$AOSP/v30/system/core/libutils/include"
    -I"$AOSP/v30/system/core/libcutils/include"
    -I"$AOSP/v30/system/core/liblog/include_vndk"
    -I"$AOSP/v30/system/core/libsystem/include"
    -I"$AOSP/v30/system/core/base/include"
)

# --- sub-HAL: must match the platform C++ ABI (std::__1, no RTTI) of the vendor HIDL libs ---
"$CXX" $TARGET -std=c++17 -O2 -fPIC -shared -fno-rtti -fno-exceptions \
    -fvisibility=hidden -Wall -Wno-unused-parameter -Wno-inconsistent-missing-override \
    -isystem "$ROOT/sysroot/libcxx_abi" \
    "${INC[@]}" \
    -nostdlib++ -L"$ROOT/sysroot/vendor_lib64" \
    -Wl,--allow-shlib-undefined -Wl,-soname,sensors.oplusbridge.so -Wl,--no-undefined \
    "$ROOT/hal/OplusBridgeSubHal.cpp" \
    -l:libc++.so -l:libutils.so -l:libhidlbase.so -l:libcutils.so -l:liblog.so \
    -l:android.hardware.sensors@1.0.so -l:android.hardware.sensors@2.0.so \
    -l:android.hardware.sensors@2.1.so -l:android.hardware.sensors@2.0-ScopedWakelock.so \
    -o "$OUT/sensors.oplusbridge.so"
echo "built $OUT/sensors.oplusbridge.so"

# --- daemon: ordinary NDK program (own static libc++, talks to sensorservice via libandroid) ---
"$CXX" $TARGET -std=c++17 -O2 -fPIE -pie -static-libstdc++ -Wall -Wno-unused-parameter \
    -I"$ROOT/common" \
    "$ROOT/daemon/oplus_sensor_bridged.cpp" -landroid \
    -o "$OUT/oplus-sensor-bridged"
echo "built $OUT/oplus-sensor-bridged"

# --- daemon, native /vendor flavour: vendor NDK sensor bridge + OnePlus display panel service ---
"$CXX" $TARGET -std=c++17 -O2 -fPIE -pie -static-libstdc++ -Wall -Wno-unused-parameter \
    -DBRIDGE_PANEL_FEATURE -I"$ROOT/common" -I"$ROOT/aidl" -I"$ROOT/aidl/compat" \
    -L"$ROOT/sysroot/vendor_lib64" -Wl,--allow-shlib-undefined -Wl,--as-needed \
    "$ROOT/daemon/oplus_sensor_bridged.cpp" \
    "$ROOT/aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.cpp" \
    -l:libsensorndkbridge.so -l:libbinder_ndk.so \
    -o "$OUT/oplus-sensor-bridged.vendor"
echo "built $OUT/oplus-sensor-bridged.vendor"

# --- KernelSU / Magisk module zip ---
STAGE="$OUT/module"
rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/system/vendor/lib64" "$STAGE/META-INF/com/google/android"
cp "$ROOT/module/"{module.prop,customize.sh,service.sh,sepolicy.rule} "$STAGE/"
cp "$OUT/sensors.oplusbridge.so" "$STAGE/system/vendor/lib64/"
cp "$OUT/oplus-sensor-bridged" "$STAGE/bin/"
cat > "$STAGE/META-INF/com/google/android/update-binary" << 'UB'
#!/sbin/sh
umask 022
ui_print() { echo "$1"; }
OUTFD=$2
ZIPFILE=$3
mount /data 2>/dev/null
[ -f /data/adb/magisk/util_functions.sh ] || { ui_print "! Magisk 20.4+ required"; exit 1; }
. /data/adb/magisk/util_functions.sh
install_module
exit 0
UB
echo "#MAGISK" > "$STAGE/META-INF/com/google/android/updater-script"
ZIP="$OUT/oplus_sensor_bridge.zip"
python - "$STAGE" "$ZIP" << 'PY'
import os, sys, zipfile
stage, out = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for root, _, files in os.walk(stage):
        for f in files:
            p = os.path.join(root, f)
            data = open(p, "rb").read()
            if f.endswith((".sh", ".prop", ".rule", "update-binary", "updater-script")):
                data = data.replace(b"\r\n", b"\n")  # keep scripts LF on Windows hosts
            z.writestr(os.path.relpath(p, stage).replace(os.sep, "/"), data)
PY
echo "packaged $ZIP"
