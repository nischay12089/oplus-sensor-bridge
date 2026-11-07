#!/system/bin/sh
# Run with su: restore the original EROFS executable by removing this test bind.
# This does not restore or rewrite fingerprint enrollment data.
set -e
stop vendor.oplus-sensor-bridged
if grep -q ' /vendor/bin/hw/oplus-sensor-bridged ' /proc/mounts; then
    if ! umount /vendor/bin/hw/oplus-sensor-bridged; then
        start vendor.oplus-sensor-bridged
        exit 1
    fi
fi
start vendor.oplus-sensor-bridged
