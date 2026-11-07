# oplus-sensor-bridge

Native vendor-side fixes for the **OxygenOS 16 port on POCO F7 (onyx)**, without patching
SystemUI, com.oplus.gesture or any other app:

- **Raise to wake** and **AOD on pick-up**
- **AOD going dark** after its entry animation
- **Full-screen AOD turning off after 5 s** in all-day mode, and the **black flash** entering AOD
- **Full-screen AOD going dim, then brighter** after locking: AOD brightness matched to the fade
- **AOD clock styles dimming off**: fixed AOD level and brightness, restored if knocked off
- **AOD dimming after a fingerprint touch**, without ever writing the panel while a finger is down
- **AOD defaults** applied once on a fresh flash, `aod_media_support` on every boot

One vendor binary plus a Multi-HAL sub-HAL. No logging, no polling: every wait sleeps on a
property futex, `poll()` or a blocking read (measured 0 CPU ticks in idle AOD).

## Why it was broken

- OxygenOS looks up the pick-up sensor as Android type 22 (OnePlus `pick_up_motion`, `0.0` =
  lifted, `1.0` = put down) and smart-AOD motion as type 33171026. On the Xiaomi vendor, type 22 is
  QTI `ccd_tilt` (only reports `1.0`) and 33171026 does not exist; the hardware is
  `xiaomi.sensor.pickup` under a different type and value convention.
- The OnePlus compositor never programs the Xiaomi panel's AOD (doze) brightness.
- SystemUI reads the panel's AOD capabilities from `vendor.oplus.hardware.displaypanelfeature`
  (feature 217); the Xiaomi vendor has no such service, so full-screen AOD was treated as an
  energy-saving case.

## How it works

```
xiaomi.sensor.pickup ─┐
motion/stationary_detect ─► oplus-sensor-bridged ─(abstract socket)─► sensors.oplusbridge.so
                              (translates values)                    (Multi-HAL sub-HAL, first in
                              + AOD keep-alive                        hals.conf → owns type 22)
                              + displaypanelfeature service                  │
                                                  sensorservice ◄────────────┘
                                                  └► SystemUI AOD, com.oplus.gesture (unchanged)
```

**Sensor bridge** (`hal/`, `daemon/`). `sensors.oplusbridge.so` exposes `pick_up_motion` (type 22)
and `amd_oplus` (33171026) and must be the **first line of hals.conf** (sensors are ordered by
handle, the sub-HAL index is its top byte, so it wins `getDefaultSensor(22, true)` over
`ccd_tilt`). `oplus-sensor-bridged` (init service `class hal`, user system, domain
`hal_sensors_default`) reads the Xiaomi/QTI sensors through the vendor `libsensorndkbridge` and
forwards translated events. Hub state reports in the first 300 ms after enabling are ignored, so
locking while holding the phone doesn't re-wake it.

**AOD keep-alive** (only when build.prop has `ro.sys.nischay.bomb=1`). Sleeps on
`debug.tracing.screen_state` and sets the panel's AOD level through the mi_disp ioctl
`0x400c4402`: off when the screen is fully on, otherwise

- **full-screen AOD** (enters doze straight from ON, after its fade ramped the backlight to the
  ambient-light target): the AOD brightness is set to **where the fade ended**. The panel's AOD
  mode renders a DBV ~2.41× brighter than normal mode, so the bridge writes DBV = fade-end register
  ÷ 2.41 (clamped 16–808) through the driver's `mipi_rw` node (DCS `0x51`), after the kernel has put
  its own fixed AOD DBV in (~0.14 s after AOD entry; checked by reading DCS `0x52` back). The level
  used to enter AOD is the closer one: low (`2`) for a dark-room target, normal (`1`) otherwise.
  A return to AOD straight from ON only counts as a fade when the register is >= 40 (the darkest
  AOD target ends at ~68): a fingerprint wake that never fully turned the screen on comes back at
  the framework's fallback (~5), which otherwise gave DBV 16, nearly off. The register is checked
  again while the DBV settles: dropping below 40 (it still showed the screen-on value at entry)
  switches to the classic handling below;
- **every other clock style** (enters doze via OFF): the fixed normal level (`1`) and its DBV (808).
  These styles pass no AOD brightness, so the framework's fallback backlight (~5) otherwise reaches
  the panel even in AOD (DBV ~15, nearly off).

The DBV is only written once the panel is really in AOD: every attempt reads the live screen state
first, so a stale command never survives a wake or a later sleep. Finger presses are left to the
stock fingerprint service and the kernel's local HBM; the bridge does no fingerprint monitoring.
Re-checks after a screen-state change run only when something can still
change: the DBV on AOD entry or a level change (densely from 0.12 s until it reads back right,
which only counts from 0.4 s on: the panel enters AOD ~0.13 s after the screen state changes, and
the framework's fallback DBV 15 lands 10-60 ms after that, so an earlier right read can be undone);
for classic styles also the level at 0.3/0.8/1.5/3 s (the framework's fallback can switch AOD off)
and the DBV once at 0.3 s. The level is never sent while the panel is still lit in normal mode: its
command set starts with a normal-mode DBV write (`51 03 28` = 808), which snapped the screen to 808
mid screen-off. So it is not sent on OFF at all (the kernel re-sends it when the panel powers on
into AOD), and on an entry straight from ON it is sent 0.1 s after the state change, just before
the panel enters AOD (measured 116-134 ms). Full-screen AOD going between doze and doze-suspend leaves level and DBV
alone (measured), so it causes no timed wake-ups.

**Display panel capabilities** (`vendor.oplus.hardware.displaypanelfeature.IDisplayPanelFeature/default`).
Answers feature 217 with **15** = doze direct (1) + smooth transition (2) + panoramic (4) +
panoramic all-day (8): full-screen AOD stays on all day and the panel goes ON→DOZE with no OFF gap.
Answers feature 211 (udfps type) with **16 = local HBM**, which is true for this panel: mi_disp lights
the sensor itself on finger down (`LHBM_ON_WHITE_1000NIT`). Without it SystemUI takes its "optical
without local HBM" power-off path (AodType 5001): a black mask over the lockscreen until the panel is
in DOZE, then the AOD fades in. With it the lockscreen or app morphs into the AOD while the backlight
ramps (AodType 4001), and SystemUI drops its own fingerprint dim layer and highlight. system_server's
fingerprint path reads only bits 4/1024 of 211, so it is unchanged. Every other feature id and
method answers **"unknown transaction"**, exactly like no service. Callers read `values[0]` whatever
the return code says, so even "unsupported + 0" is an answer: feature 211 = 0 once switched OxygenOS
into "optical fingerprint without local HBM" (panel raised to ~8272 under a dark overlay: bright
fingerprint icon over a dim lockscreen).
The stubs are generated from `aidl/IDisplayPanelFeature.aidl`, which must keep the framework's
stable-AIDL V1 layout (hash `39afca22ac77253421ad681eeb2bae0c6ff62c13`): `getDisplayPanelInfo` takes
`out List<String>`, so its request is only the id. Declared as `out String[]`, the stub also read an
array length the framework never sends, and the call failed with `NOT_ENOUGH_DATA`, which
system_server does not catch (the CPH2723 port calls it at every boot and crashed every ~15 s).

**AOD defaults** (same binary, `oplus-sensor-bridged --aod-defaults`, one-shot init service
`vendor.oplus-aod-defaults`). Init starts it as uid shell once `sys.boot_completed=1` and
`persist.sys.device_provisioned=1` (`cmd settings put` only accepts shell or root callers).
- Every boot: `aod_media_support=1`. SystemUI resets it at each start from the OnePlus touch
  firmware's AOD-media gesture bit (touch node 39, `0x10000`), which this touch driver lacks; its
  check lands ~2.5 s before boot completed, and one recheck 10 s later covers a late one.
- Once per flash: 26 AOD settings (full-screen style, all day, …), led by
  `Setting_AodOldDataHasBeenReset=1` (otherwise the AOD app resets the mode on its first start),
  then the marker `oplus_bridge_aod_defaults_v2`. Applied only if the marker reads back as unset,
  so changes made in Settings are never overwritten.

## Layout

| Path | What |
|---|---|
| `hal/OplusBridgeSubHal.cpp` | Multi-HAL 2.1 sub-HAL (`sensors.oplusbridge.so`) |
| `daemon/oplus_sensor_bridged.cpp` | Sensor bridge, AOD keep-alive, panel service, AOD defaults |
| `common/bridge_proto.h` | Socket protocol between the two |
| `aidl/` | `IDisplayPanelFeature.aidl` and its generated NDK stubs (+ small compat headers) |
| `native/` | Init `.rc`, SELinux rules, on-device dev installers |
| `patches/` | SELinux appends, `repack_attributes.txt` (fs_config + file_contexts) |
| `module/` | Older KernelSU/Magisk module variant (sub-HAL + NDK daemon, no panel service) |
| `aosp/` | AOSP headers to build against (HIDL sensors 1.0/2.0/2.1, libhidl, multihal) |
| `sysroot/` | `libcxx_abi/__config_site`; `vendor_lib64/` is filled by `pull_vendor_libs.sh` |
| `tools/` | Small on-device probes used during development |

## Build

Git Bash on Windows (or any shell), Android NDK r26:

```
bash sysroot/pull_vendor_libs.sh        # phone connected: copies the vendor libs to link against
NDK=/path/to/ndk/26.x bash build.sh
```

Outputs in `out/`: `sensors.oplusbridge.so`, `oplus-sensor-bridged.vendor` (the native daemon:
install as `vendor/bin/hw/oplus-sensor-bridged`), `oplus-sensor-bridged` (NDK flavour for the
module) and `oplus_sensor_bridge.zip` (module). The daemon embeds one source path (from the AIDL
stub), so its sha256 depends on where the tree is checked out; the code is otherwise identical.

## Add to a ROM

| File | Owner / mode | Label |
|---|---|---|
| `vendor/bin/hw/oplus-sensor-bridged` | `0 2000 0755` | `hal_sensors_default_exec` |
| `vendor/lib64/sensors.oplusbridge.so` | `0 0 0644` | `vendor_file` |
| `vendor/etc/init/vendor.oplus-sensor-bridged.rc` | `0 0 0644` | `vendor_configs_file` |

1. Prepend `sensors.oplusbridge.so` to `vendor/etc/sensors/hals.conf`.
2. **Append** `patches/vendor_sepolicy.cil.append`, `patches/vendor_file_contexts.append` and
   `patches/vendor_service_contexts.append` to the matching files in `vendor/etc/selinux/`. Never
   replace those files: a ROM that adds its own vendor policy would lose it while its contexts
   still reference it (bootloop risk).
3. Enforcing builds: also append `patches/aod_defaults_enforcing.cil` (lets the one-shot run
   `cmd settings`), or the AOD defaults are simply not applied.
4. build.prop: `ro.sys.nischay.bomb=1`. Optional, faster full-screen AOD lock/wake fades:
   `ro.oplus.display.panoramic_aod_force_update_rate=true`, `debug.oplus.display.fast_rate_scale=4.0`.

## Verify

```
adb shell getprop init.svc.vendor.oplus-sensor-bridged         # running
adb shell dumpsys sensorservice | grep "Oplus Sensor Bridge"    # handles 0x1 and 0x2
adb shell service check vendor.oplus.hardware.displaypanelfeature.IDisplayPanelFeature/default  # found
adb shell getprop init.svc.vendor.oplus-aod-defaults            # stopped (ran, exited)
adb shell settings get secure aod_media_support                 # 1
adb shell cat /sys/devices/virtual/mi_display/disp_feature/disp-DSI-0/doze_brightness  # 1 or 2 in AOD, 0 on
```

## Known limits

- Full-screen AOD is as bright as OnePlus's fade ends, which on this panel's stock brightness table
  is dimmer than the panel's own fixed AOD levels (very dim in dark rooms). The 2.41× factor
  (`kAodGain`) comes from one observation; tune it if the AOD still steps at the end of the fade.
- The kernel's fixed AOD DBV shows for a frame or two on each AOD entry before it is replaced.
- Choosing a classic clock style in AOD settings makes the AOD app switch to energy saving and
  SystemUI turn full-screen AOD off; the bridge follows whatever style is picked.
