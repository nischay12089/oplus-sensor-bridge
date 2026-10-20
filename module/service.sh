#!/system/bin/sh
# Starts the bridge daemon and restarts it if it ever exits.
MODDIR=${0%/*}

(
    # The daemon needs sensorservice; it also retries on its own, this just avoids log spam.
    until [ -n "$(service check sensorservice 2>/dev/null | grep -v 'not found')" ]; do
        sleep 2
    done
    while true; do
        "$MODDIR/bin/oplus-sensor-bridged"
        log -p w -t OplusSensorBridged "daemon exited ($?), restarting"
        sleep 2
    done
) &
