#!/system/bin/sh
# Adds the display-panel capabilities service + first-boot AOD defaults (built into the daemon) to
# an existing bridge install. Payload in $STAGE: oplus-sensor-bridged, vendor.oplus-sensor-bridged.rc,
# sepolicy.cil, vendor_service_contexts.append, vendor_sepolicy.cil.stock.
# Run as root. /vendor is untouched unless the policy compiles.
set -e
STAGE=${STAGE:-/data/local/tmp/oplusbridge}
V=/vendor
VSEL=$V/etc/selinux
die() { echo "ERROR: $*"; exit 1; }

# --- candidates ---
{ cat "$STAGE/vendor_sepolicy.cil.stock"; echo; cat "$STAGE/sepolicy.cil"; } > "$STAGE/vendor_sepolicy.cil"
cp $VSEL/vendor_service_contexts "$STAGE/vendor_service_contexts"
SC_LINE=$(cat "$STAGE/vendor_service_contexts.append")
SC_NAME=${SC_LINE%% *}
grep -q "^$SC_NAME " "$STAGE/vendor_service_contexts" || echo "$SC_LINE" >> "$STAGE/vendor_service_contexts"
[ "$(grep -c "^$SC_NAME " "$STAGE/vendor_service_contexts")" = "1" ] || die "service_contexts entry not unique"

# --- policy must compile the way init compiles it ---
VER=$(cat $VSEL/plat_sepolicy_vers.txt)
opt() { [ -f "$1" ] && echo "$1"; }
/system/bin/secilc /system/etc/selinux/plat_sepolicy.cil -m -M true -G -N \
    -c "$(cat /sys/fs/selinux/policyvers)" \
    /system/etc/selinux/mapping/$VER.cil $(opt /system/etc/selinux/mapping/$VER.compat.cil) \
    $(opt /system_ext/etc/selinux/system_ext_sepolicy.cil) $(opt /system_ext/etc/selinux/mapping/$VER.cil) \
    $(opt /product/etc/selinux/product_sepolicy.cil) $(opt /product/etc/selinux/mapping/$VER.cil) \
    $VSEL/plat_pub_versioned.cil "$STAGE/vendor_sepolicy.cil" $(opt /odm/etc/selinux/odm_sepolicy.cil) \
    -o "$STAGE/policy.test" -f /dev/null > "$STAGE/secilc.log" 2>&1 \
    || die "policy does not compile, /vendor untouched. See $STAGE/secilc.log"
echo "- SELinux policy compiles OK"

# --- write /vendor (daemon stopped first: a running binary can't be overwritten) ---
mkdir -p "$STAGE/orig"
cp -p $VSEL/vendor_service_contexts "$STAGE/orig/"
DM=$(readlink -f /dev/block/mapper/vendor$(getprop ro.boot.slot_suffix))
stop vendor.oplus-sensor-bridged
blockdev --setrw "$DM"
mount -o remount,rw $V

T=$V/bin/hw/oplus-sensor-bridged
cp "$STAGE/oplus-sensor-bridged" $T.new
chown root:shell $T.new; chmod 0755 $T.new; chcon u:object_r:hal_sensors_default_exec:s0 $T.new
mv -f $T.new $T
rm -f $V/etc/oplus_aod_defaults.sh  # older installs: the defaults now live in the daemon

# existing files: rewrite contents in place so owner/mode/label stay as shipped
cat "$STAGE/vendor.oplus-sensor-bridged.rc" > $V/etc/init/vendor.oplus-sensor-bridged.rc
cat "$STAGE/vendor_sepolicy.cil" > $VSEL/vendor_sepolicy.cil
cat "$STAGE/vendor_service_contexts" > $VSEL/vendor_service_contexts

sync
mount -o remount,ro $V || true
blockdev --setro "$DM" 2>/dev/null || true
echo "- installed:"
ls -lZ $T $V/etc/init/vendor.oplus-sensor-bridged.rc
grep "^$SC_NAME " $VSEL/vendor_service_contexts
echo "- reboot to activate (policy, service label and class hal start are boot-time)"
