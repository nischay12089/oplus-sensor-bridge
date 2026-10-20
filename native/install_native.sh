#!/system/bin/sh
# Installs the Oplus sensor bridge natively into /vendor (run as root on the device).
# Expects in $STAGE: sensors.oplusbridge.so, oplus-sensor-bridged, vendor.oplus-sensor-bridged.rc,
# sepolicy.cil. Nothing on /vendor is modified unless the new SELinux policy compiles.
set -e
STAGE=${STAGE:-/data/local/tmp/oplusbridge}
ORIG=$STAGE/orig
WORK=$STAGE/work
V=/vendor
VSEL=$V/etc/selinux
FC_LINE='/(vendor|system/vendor)/bin/hw/oplus-sensor-bridged  u:object_r:hal_sensors_default_exec:s0'

die() { echo "ERROR: $*"; exit 1; }
mkdir -p "$ORIG" "$WORK"

# --- 1. keep pristine copies of every file we edit (first run only) ---
for f in $V/etc/sensors/hals.conf $VSEL/vendor_sepolicy.cil $VSEL/vendor_file_contexts; do
    [ -f "$ORIG/$(basename $f)" ] || cp -p "$f" "$ORIG/"
done

# --- 2. build candidates from the pristine copies ---
{ echo "sensors.oplusbridge.so"; grep -v "sensors.oplusbridge.so" "$ORIG/hals.conf"; } > "$WORK/hals.conf"
{ cat "$ORIG/vendor_sepolicy.cil"; echo; cat "$STAGE/sepolicy.cil"; } > "$WORK/vendor_sepolicy.cil"
{ cat "$ORIG/vendor_file_contexts"; echo; echo "$FC_LINE"; } > "$WORK/vendor_file_contexts"

# --- 3. compile the full policy the way init does (secilc, neverallows off) ---
VER=$(cat $VSEL/plat_sepolicy_vers.txt)
opt() { [ -f "$1" ] && echo "$1"; }
compile() {
    /system/bin/secilc /system/etc/selinux/plat_sepolicy.cil -m -M true -G -N \
        -c "$(cat /sys/fs/selinux/policyvers)" \
        /system/etc/selinux/mapping/$VER.cil $(opt /system/etc/selinux/mapping/$VER.compat.cil) \
        $(opt /system_ext/etc/selinux/system_ext_sepolicy.cil) \
        $(opt /system_ext/etc/selinux/mapping/$VER.cil) \
        $(opt /product/etc/selinux/product_sepolicy.cil) \
        $(opt /product/etc/selinux/mapping/$VER.cil) \
        $VSEL/plat_pub_versioned.cil "$1" $(opt /odm/etc/selinux/odm_sepolicy.cil) \
        -o "$2" -f /dev/null
}
echo "- compiling baseline policy (vendor $VER)"
compile "$ORIG/vendor_sepolicy.cil" "$WORK/policy.base" > "$WORK/secilc.base.log" 2>&1 \
    || die "baseline policy does not compile here, see $WORK/secilc.base.log"
echo "- compiling policy with bridge rules"
compile "$WORK/vendor_sepolicy.cil" "$WORK/policy.new" > "$WORK/secilc.new.log" 2>&1 \
    || die "new policy fails to compile, /vendor untouched. see $WORK/secilc.new.log"
echo "  ok: $(wc -c < "$WORK/policy.base") -> $(wc -c < "$WORK/policy.new") bytes"

# --- 4. write /vendor ---
DM=$(readlink -f /dev/block/mapper/vendor$(getprop ro.boot.slot_suffix))
blockdev --setrw "$DM"
mount -o remount,rw $V || die "remount rw failed"
label_of() { ls -Zd "$1" | awk '{print $1}'; }

install -m 0644 "$STAGE/sensors.oplusbridge.so" $V/lib64/sensors.oplusbridge.so
chown root:root $V/lib64/sensors.oplusbridge.so
chcon u:object_r:vendor_file:s0 $V/lib64/sensors.oplusbridge.so

install -m 0755 "$STAGE/oplus-sensor-bridged" $V/bin/hw/oplus-sensor-bridged
chown root:shell $V/bin/hw/oplus-sensor-bridged
chcon u:object_r:hal_sensors_default_exec:s0 $V/bin/hw/oplus-sensor-bridged

RC_LABEL=$(label_of $V/etc/init/$(ls $V/etc/init | grep '\.rc$' | head -1))
install -m 0644 "$STAGE/vendor.oplus-sensor-bridged.rc" $V/etc/init/vendor.oplus-sensor-bridged.rc
chown root:root $V/etc/init/vendor.oplus-sensor-bridged.rc
chcon "$RC_LABEL" $V/etc/init/vendor.oplus-sensor-bridged.rc

# Existing files: overwrite contents in place so owner/mode/label stay as shipped.
cat "$WORK/hals.conf" > $V/etc/sensors/hals.conf
cat "$WORK/vendor_sepolicy.cil" > $VSEL/vendor_sepolicy.cil
cat "$WORK/vendor_file_contexts" > $VSEL/vendor_file_contexts

sync
mount -o remount,ro $V || echo "warning: could not remount ro (harmless, reboot will)"
blockdev --setro "$DM" 2>/dev/null || true

echo "- installed:"
ls -laZ $V/lib64/sensors.oplusbridge.so $V/bin/hw/oplus-sensor-bridged $V/etc/init/vendor.oplus-sensor-bridged.rc
echo "- hals.conf:"; cat $V/etc/sensors/hals.conf
echo "- done, reboot to activate"
