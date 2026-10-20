#!/system/bin/sh
# Switches the phone from bridge + separate /odm aodkeepalive to the single merged daemon.
# Payload in $STAGE: oplus-sensor-bridged, vendor.oplus-sensor-bridged.rc, sepolicy.cil,
# vendor_sepolicy.cil.stock (pristine vendor policy). Run as root.
set -e
STAGE=${STAGE:-/data/local/tmp/oplusmerge}
V=/vendor
VSEL=$V/etc/selinux
die() { echo "ERROR: $*"; exit 1; }

# --- 1. new vendor policy = stock + current bridge rules; must compile before anything changes ---
{ cat "$STAGE/vendor_sepolicy.cil.stock"; echo; cat "$STAGE/sepolicy.cil"; } > "$STAGE/vendor_sepolicy.cil"
VER=$(cat $VSEL/plat_sepolicy_vers.txt)
opt() { [ -f "$1" ] && echo "$1"; }
/system/bin/secilc /system/etc/selinux/plat_sepolicy.cil -m -M true -G -N \
    -c "$(cat /sys/fs/selinux/policyvers)" \
    /system/etc/selinux/mapping/$VER.cil $(opt /system/etc/selinux/mapping/$VER.compat.cil) \
    $(opt /system_ext/etc/selinux/system_ext_sepolicy.cil) $(opt /system_ext/etc/selinux/mapping/$VER.cil) \
    $(opt /product/etc/selinux/product_sepolicy.cil) $(opt /product/etc/selinux/mapping/$VER.cil) \
    $VSEL/plat_pub_versioned.cil "$STAGE/vendor_sepolicy.cil" $(opt /odm/etc/selinux/odm_sepolicy.cil) \
    -o "$STAGE/policy.test" -f /dev/null > "$STAGE/secilc.log" 2>&1 \
    || die "policy does not compile, nothing changed. See $STAGE/secilc.log"
echo "- SELinux policy compiles OK"

# --- 2. retire the separate aodkeepalive (its job moves into the bridge daemon) ---
mkdir -p "$STAGE/odm_backup"
for f in /odm/bin/aodkeepalive /odm/etc/init/aodkeepalive.rc; do
    [ -f "$f" ] && cp -p "$f" "$STAGE/odm_backup/"
done
killall aodkeepalive 2>/dev/null || true
if [ -f /odm/bin/aodkeepalive ] || [ -f /odm/etc/init/aodkeepalive.rc ]; then
    ODM_DM=$(readlink -f /dev/block/mapper/odm$(getprop ro.boot.slot_suffix))
    blockdev --setrw "$ODM_DM"
    mount -o remount,rw /odm
    rm -f /odm/bin/aodkeepalive /odm/etc/init/aodkeepalive.rc
    sync
    mount -o remount,ro /odm || true
    blockdev --setro "$ODM_DM" 2>/dev/null || true
    echo "- removed /odm aodkeepalive (backup in $STAGE/odm_backup)"
fi

# --- 3. update /vendor (stop the service first: a running binary can't be overwritten) ---
T=$V/bin/hw/oplus-sensor-bridged
DM=$(readlink -f /dev/block/mapper/vendor$(getprop ro.boot.slot_suffix))
stop vendor.oplus-sensor-bridged
blockdev --setrw "$DM"
mount -o remount,rw $V
cp "$STAGE/oplus-sensor-bridged" $T.new
chown root:shell $T.new; chmod 0755 $T.new; chcon u:object_r:hal_sensors_default_exec:s0 $T.new
mv -f $T.new $T
cat "$STAGE/vendor.oplus-sensor-bridged.rc" > $V/etc/init/vendor.oplus-sensor-bridged.rc
cat "$STAGE/vendor_sepolicy.cil" > $VSEL/vendor_sepolicy.cil
sync
mount -o remount,ro $V || true
blockdev --setro "$DM" 2>/dev/null || true
start vendor.oplus-sensor-bridged
echo "- vendor updated:"
ls -lZ $T $V/etc/init/vendor.oplus-sensor-bridged.rc
tail -3 $VSEL/vendor_sepolicy.cil
