# KernelSU/Magisk install script
SKIPUNZIP=0

HALS=/vendor/etc/sensors/hals.conf
[ -f "$HALS" ] || abort "! $HALS not found - this device does not use the sensors Multi-HAL"
grep -q "sensors.qsh.so" "$HALS" || ui_print "! warning: sensors.qsh.so not listed in $HALS"

# The bridge must be the FIRST sub-HAL: the framework orders sensors by handle and the
# sub-HAL index is the handle's top byte, so index 0 makes our type-22 sensor win
# getDefaultSensor(22, true) over QTI ccd_tilt. Generated from the live file so any
# other vendor sub-HALs are preserved in their original order.
mkdir -p "$MODPATH/system/vendor/etc/sensors"
{ echo "sensors.oplusbridge.so"; grep -v "sensors.oplusbridge.so" "$HALS"; } \
    > "$MODPATH/system/vendor/etc/sensors/hals.conf"
ui_print "- hals.conf:"
while read -r l; do ui_print "    $l"; done < "$MODPATH/system/vendor/etc/sensors/hals.conf"

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/system/vendor/lib64/sensors.oplusbridge.so" 0 0 0644 u:object_r:vendor_file:s0
set_perm "$MODPATH/system/vendor/etc/sensors/hals.conf" 0 0 0644 u:object_r:vendor_configs_file:s0
set_perm "$MODPATH/bin/oplus-sensor-bridged" 0 0 0755
