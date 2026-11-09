// oplus-sensor-bridged - feeds sensors.oplusbridge.so with events translated from the Xiaomi/QTI
// sensor hub into the OnePlus conventions expected by OxygenOS:
//   xiaomi.sensor.pickup (33171036)         -> pick_up_motion (type 22): 0.0 lifted, 1.0 put down
//   motion_detect(30) / stationary_detect(29) -> amd (33171026):          2.0 moving, 1.0 still
// Sources are only enabled while the framework has the matching virtual sensor active.
//
// Also holds the AOD lit (merged from aodkeepalive): the OnePlus compositor never programs the
// Xiaomi panel's doze brightness, so AOD went dark right after its entry animation. And applies
// the AOD defaults once after the first boot of a fresh flash.
//
// No logging, and no polling: every wait sleeps on a property futex or a blocking read.
//
// Only the sensor/looper API subset shared by libandroid (NDK) and the vendor libsensorndkbridge
// is used, so the same source builds for the KernelSU module and for a native /vendor install.
#include <android/looper.h>
#include <android/sensor.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/system_properties.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <initializer_list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "bridge_proto.h"
#include "panel_command_guard.h"

#ifdef BRIDGE_PANEL_FEATURE
#include <aidl/vendor/oplus/hardware/displaypanelfeature/BnDisplayPanelFeature.h>

#include "binder_service.h"
#endif

using namespace oplusbridge;

namespace {

constexpr int kTypeXiaomiPickup = 33171036;
constexpr int kTypeStationaryDetect = 29;
constexpr int kTypeMotionDetect = 30;
// Xiaomi pocket_mode: sensor-hub algorithm (its registry tunes a p-sensor threshold), cheap next to
// the ultrasound proximity. Non-wake-up only: while the AP sleeps, a change arrives on its next wake.
constexpr int kTypeXiaomiPocket = 33171095;
constexpr int kIdSensors = 1;

int64_t bootTimeNs() {
    timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Keeps the device awake long enough for an acked wake-up event to reach the HAL, which then
// holds its own wakelock until the framework consumes the event.
void holdWakelock() {
    int fd = open("/sys/power/wake_lock", O_WRONLY | O_CLOEXEC);
    if (fd < 0) return;
    const char kLock[] = "oplus_sensor_bridge 1000000000";  // auto-expires after 1 s
    write(fd, kLock, sizeof(kLock) - 1);
    close(fd);
}

// Translate one xiaomi.sensor.pickup event into the OnePlus pick_up_motion convention.
//   xiaomiValue   : 1.0 = picked up, 2.0 = put down / lying flat
//   msSinceEnable : time since the Xiaomi sensor was (re)enabled for this activation
//   lastSent      : last value forwarded during this activation, or -1 if none yet
// Write the Oplus value (0.0 = lifted, 1.0 = put down) to *out and return true to forward it,
// or return false to drop the event.
bool translatePickup(float xiaomiValue, int64_t msSinceEnable, float lastSent, float* out) {
    // The hub reports its current state ~100 ms after enable; at screen-off that is "held",
    // which must not be mistaken for a lift or the screen would turn straight back on.
    if (msSinceEnable < 300) return false;
    if (xiaomiValue == 1.0f) {
        *out = 0.0f;  // picked up -> lifted
    } else if (xiaomiValue == 2.0f) {
        *out = 1.0f;  // put down
    } else {
        return false;
    }
    // Repeated lifts are kept (the hub can send 1.0 twice with no 2.0 in between, and each may
    // be a real pickup); repeated put-downs carry no new information.
    return !(*out == 1.0f && lastSent == 1.0f);
}

// Translate one xiaomi.sensor.oem_pocket event into the Oplus gesture-proximity convention:
// write 0.0 (near) or 1.0 (far) to *out and return true to forward it, false to drop it.
// Measured: v[0] is 0.0 in a pocket and 5.0 out of it (a palm over the sensor changes nothing).
// Changes carry v[2] = 0. The report sent on enable carries v[2] = -1 and can be stale: after a
// missed "out" change it read 0.0 twice with the phone on a desk. It is dropped; every activation
// starts as far instead (setWantedLocked).
bool translatePocket(const float* v, float* out) {
    if (v[2] != 0.0f) return false;
    if (v[0] == 0.0f) {
        *out = 0.0f;
    } else if (v[0] == 5.0f) {
        *out = 1.0f;
    } else {
        return false;
    }
    return true;
}

// libsensorndkbridge has no ASensor_isWakeUpSensor / getDefaultSensorEx, so the wake-up variant
// is picked by name ("pickup  Wakeup", "motion_detect_wakeup" vs "... Non-wakeup").
bool isWakeUpName(const char* name) {
    std::string n(name ? name : "");
    for (char& c : n) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return n.find("wakeup") != std::string::npos && n.find("non-wakeup") == std::string::npos &&
           n.find("non_wakeup") == std::string::npos;
}

const ASensor* findWakeUpSensor(ASensorManager* mgr, int type) {
    ASensorList list;
    int count = ASensorManager_getSensorList(mgr, &list);
    for (int i = 0; i < count; i++) {
        if (ASensor_getType(list[i]) == type && isWakeUpName(ASensor_getName(list[i]))) {
            return list[i];
        }
    }
    return nullptr;
}

// --- System properties: futex waits, no polling -----------------------------------------------
// Blocks until the property exists: sleeps on the global property serial, which changes whenever
// any property is added or set.
const prop_info* waitForProp(const char* name) {
    uint32_t serial = 0;
    __system_property_wait(nullptr, 0, &serial, nullptr);  // returns at once with the current serial
    const prop_info* pi;
    while ((pi = __system_property_find(name)) == nullptr) {
        __system_property_wait(nullptr, serial, &serial, nullptr);
    }
    return pi;
}

// --- AOD keep-alive ---------------------------------------------------------------------------
// While the screen is not fully on, force the panel's doze brightness on through the Xiaomi
// mi_disp ioctl; clear it when the screen turns fully on. Gated by the ROM's read-only build.prop
// switch, so it does nothing unless the ROM opts in.
//
// Full-screen AOD: OnePlus picks the AOD brightness from ambient light and its transition fades the
// backlight to it before entering doze. The panel's AOD mode then takes over with one of two fixed
// levels, which never matched where the fade ended (it got dim, then jumped brighter). So the AOD
// brightness (DBV) is set to the end of the fade, converted for AOD mode, and the closer of the two
// fixed levels is only used as the mode to enter AOD with.
constexpr char kGateProp[] = "ro.sys.nischay.bomb";
constexpr char kScreenStateProp[] = "debug.tracing.screen_state";  // 1 off 2 on 3 doze 4 doze-suspend
constexpr char kDispFeatureDev[] = "/dev/mi_display/disp_feature";
constexpr char kBacklightNode[] = "/sys/class/backlight/panel0-backlight/brightness";
constexpr char kDozeNode[] = "/sys/devices/virtual/mi_display/disp_feature/disp-DSI-0/doze_brightness";
constexpr char kMipiNode[] = "/sys/devices/virtual/mi_display/disp_feature/disp-DSI-0/mipi_rw";
// AOD mode renders a DBV ~2.4x brighter than normal mode: a fade ending at register 1945 matches the
// panel's normal AOD level, DBV 808.
constexpr float kAodGain = 1945.0f / 808.0f;
constexpr int kAodDbvMin = 16;
constexpr int kAodDbvMax = 808;  // never brighter than the panel's own normal AOD level
constexpr unsigned long kMiDispSetDozeBrightness = 0x400c4402;  // MI_DISP_IOCTL_SET_DOZE_BRIGHTNESS
constexpr uint32_t kDozeOff = 0;
constexpr uint32_t kDozeHigh = 1;  // panel's normal AOD level
constexpr uint32_t kDozeLow = 2;   // panel's dim AOD level
// AOD mode to enter with (the kernel also restores its DBV when leaving AOD, so the closer one avoids
// a flash on wake). With the stock brightness table SystemUI's dark-room target (rank 0) ends the
// fade at register ~68 and every other ambient rank at >= ~529.
constexpr int kLowAodMaxRegister = 200;
// A fade never ends below SystemUI's darkest AOD target (register ~68); lower registers are the
// framework's fallback (~5) or 0 and must not be matched (they gave DBV 16, nearly off).
constexpr int kMinFadeRegister = 40;
struct DozeReq {
    uint32_t flag;
    uint32_t dispId;
    uint32_t value;  // kDozeOff / kDozeHigh / kDozeLow
};

bool gateEnabled() {
    char v[PROP_VALUE_MAX] = {};
    return __system_property_get(kGateProp, v) > 0 && strcmp(v, "1") == 0;
}

int liveScreenState() {
    char value[PROP_VALUE_MAX] = {};
    return __system_property_get(kScreenStateProp, value) > 0 ? atoi(value) : -1;
}

class DozeKeeper {
  public:
    void run() {
        if (!gateEnabled()) return;  // ro.* is fixed at boot: off now means off for good
        const prop_info* pi = waitForProp(kScreenStateProp);
        uint32_t serial = 0;
        int prev = -1;
        for (;;) {
            int state = liveScreenState();
            const bool inAod = state == 3 || state == 4;
            bool dbvPending = false;  // the kernel may have put its own fixed DBV in
            bool applyLate = false;   // set the AOD level shortly before the panel enters AOD
            if (inAod) {
                const bool entered = prev != 3 && prev != 4;
                // Only the full-screen AOD enters doze straight from ON after a fade that ramped
                // the backlight to the ambient-light target; every other clock style goes
                // ON -> OFF -> doze with no meaningful register, so it gets the fixed level. A
                // fingerprint wake that never fully turned the screen on also returns straight
                // from ON, but at the framework's fallback register (~5): not a fade.
                const int reg = readInt(kBacklightNode);
                if (prev == 2) mFollowFade = reg >= kMinFadeRegister;
                else if (prev == 1) mFollowFade = false;
                int dbv = mDbv;
                if (mFollowFade) {
                    // Re-read on every doze <-> doze-suspend change too: SystemUI updates the AOD
                    // brightness when the ambient light changes. A register below any AOD target
                    // (0 while energy-saving AOD hides itself, or the fallback) is not one.
                    if (reg >= kMinFadeRegister) {
                        mLevel = reg < kLowAodMaxRegister ? kDozeLow : kDozeHigh;
                        dbv = std::clamp(static_cast<int>(reg / kAodGain + 0.5f), kAodDbvMin, kAodDbvMax);
                    }
                } else {
                    // These styles pass no AOD brightness, so the framework's fallback backlight
                    // (~5) reaches the panel even in AOD (DBV ~15, nearly off): hold the panel's
                    // own normal AOD level instead.
                    mLevel = kDozeHigh;
                    dbv = kAodDbvMax;
                }
                const bool dbvChanged = dbv != mDbv;
                mDbv = dbv;
                // Entering AOD, or a new level, makes the kernel put its fixed DBV in. Doze <->
                // doze-suspend leaves level and DBV alone (measured), so the ioctl is skipped then
                // and a new target (ambient light) is simply written.
                if (entered || readInt(kDozeNode) != static_cast<int>(mLevel)) {
                    // Straight from ON the panel stays lit in normal mode until it enters AOD
                    // (116-134 ms after the state flips, measured), and the level's command set
                    // starts with a normal-mode DBV write (51 03 28 = 808): sent at once, it
                    // snapped the screen to 808 for ~0.13 s mid screen-off. Sent late, the snap
                    // is a few frames, and the panel still enters AOD with its level set.
                    if (entered && prev == 2) applyLate = true;
                    else apply(mLevel);
                    dbvPending = true;
                } else if (dbvChanged) {
                    writeAodDbv();
                }
            } else if (state == 1) {
                // No level here: the panel is still lit for ~0.1 s after the state flips to OFF,
                // so the level's DBV write dimmed the screen-off (or re-lit the panel after the
                // framework blanked it), and the kernel re-sends the level anyway when the panel
                // next powers on into AOD.
                mFollowFade = false;
            } else if (state == 2 && prev != 2) {
                mFollowFade = false;
                apply(kDozeOff);
            }
            prev = state;
            serial = waitForChange(pi, serial, inAod, dbvPending, applyLate);
        }
    }

  private:
    // Sleeps on the screen-state futex (0% CPU) until it changes. Timed re-checks follow a change
    // only while something can still change (the driver gives no notification for either):
    //  - the kernel puts its fixed DBV in ~0.14 s after the panel enters AOD or changes AOD level:
    //    the DBV is checked densely until it reads back right, then not again;
    //  - with clock styles that pass no AOD brightness, the framework's fallback update can switch
    //    the AOD level off ~0.3-0.8 s after a doze <-> doze-suspend change and rewrite the DBV: the
    //    level is re-checked (a plain sysfs read) at 0.3/0.8/1.5/3 s and the DBV at 0.3 s.
    // Full-screen AOD going between doze and doze-suspend needs neither (measured): no wake-ups.
    //
    // The screen-state property flips ~0.13 s before the panel really enters AOD, and the
    // framework's fallback write (DBV 15) comes ~10-60 ms after that, so a right read-back only
    // counts as settled from kSettleMs on: earlier it may still be the kernel's preset in normal
    // mode. Likewise an entry straight from ON only proves to be a full-screen fade if the register
    // stays at an AOD target; dropping to the fallback means it was not one.
    //
    // applyLate: an entry straight from ON sets the AOD level at kApplyLateMs, just before the
    // panel enters AOD, not at the flip (it is still lit in normal mode until then); a change of
    // state before that skips it.
    uint32_t waitForChange(const prop_info* pi, uint32_t serial, bool inAod, bool dbvPending,
                           bool applyLate) {
        static constexpr int kApplyLateMs = 100;
        static constexpr int kAtMs[] = {kApplyLateMs, 120, 160, 200, 240, 280, 300, 340, 440, 600,
                                        800, 1500, 3000};
        static constexpr int kSettleMs = 400;
        uint32_t next = serial;
        int elapsed = 0;
        for (int at : kAtMs) {
            const bool lateAt = at == kApplyLateMs;
            const bool classic = inAod && !mFollowFade;
            const bool levelAt = !lateAt && classic && (at == 300 || at == 800 || at == 1500 || at == 3000);
            bool dbvAt = !lateAt && inAod && (dbvPending || (classic && at == 300));
            if (lateAt ? !applyLate : !levelAt && !dbvAt) continue;
            const int gap = at - elapsed;
            elapsed = at;
            timespec ts{gap / 1000, (gap % 1000) * 1000000L};
            if (__system_property_wait(pi, serial, &next, &ts)) return next;
            if (lateAt) {
                apply(mLevel);
                continue;
            }
            if (dbvPending && mFollowFade) {
                const int reg = readInt(kBacklightNode);
                if (reg >= 0 && reg < kMinFadeRegister) {  // no fade: other clock style
                    mFollowFade = false;
                    mDbv = kAodDbvMax;
                    if (mLevel != kDozeHigh) {
                        mLevel = kDozeHigh;
                        apply(mLevel);
                    }
                }
            }
            if (levelAt && readInt(kDozeNode) != static_cast<int>(mLevel)) {
                apply(mLevel);
                dbvPending = dbvAt = true;
            }
            if (dbvAt && syncDbv() && at >= kSettleMs) dbvPending = false;
        }
        __system_property_wait(pi, serial, &next, nullptr);
        return next;
    }

    // Makes the panel show the AOD DBV (fade-matched, or the normal AOD level for the other clock
    // styles); returns true once it reads back right. Not while the DBV still equals the
    // normal-mode backlight register: the panel has not entered AOD yet, and writing then would
    // dim the screen before AOD starts.
    bool syncDbv() {
        if (!isDozeScreenState(liveScreenState())) return true;
        int dbv = readDbv();
        if (dbv == mDbv) return true;
        if (dbv >= 0 && dbv != readInt(kBacklightNode)) writeDbv(mDbv);
        return false;
    }

    void writeAodDbv() {
        if (!isDozeScreenState(liveScreenState())) return;
        writeDbv(mDbv);
    }

    // The panel's brightness register (DCS 0x51/0x52, 2 bytes big-endian) through the display
    // driver's MIPI command node.
    static bool writeMipi(const char* cmd) {
        int fd = open(kMipiNode, O_WRONLY | O_CLOEXEC);
        if (fd < 0) return false;
        bool ok = write(fd, cmd, strlen(cmd)) > 0;
        close(fd);
        return ok;
    }

    static void writeDbv(int dbv) {
        if (!isDozeScreenState(liveScreenState())) return;
        char cmd[48];
        snprintf(cmd, sizeof(cmd), "00 00 00 39 00 00 00 00 00 03 51 %02X %02X", (dbv >> 8) & 0xff,
                 dbv & 0xff);
        writeMipi(cmd);
    }

    // Returns -1 if the read-back has no "0xHH,0xLL" pair (another reader got there first).
    static int readDbv() {
        if (!writeMipi("01 02 00 06 01 00 00 00 00 01 52")) return -1;
        int fd = open(kMipiNode, O_RDONLY | O_CLOEXEC);
        if (fd < 0) return -1;
        char buf[128] = {};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        unsigned hi, lo;
        const char* p = n > 0 ? strstr(buf, "0x") : nullptr;
        return p != nullptr && sscanf(p, "0x%x ,0x%x", &hi, &lo) == 2 ? static_cast<int>(hi << 8 | lo) : -1;
    }

    static int readInt(const char* path) {
        int fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) return -1;
        char buf[16] = {};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        return n > 0 ? atoi(buf) : -1;
    }

    bool applyOnce(uint32_t value) {
        if (!permitsDozeLevel(liveScreenState(), value)) return false;
        if (mFd < 0) mFd = open(kDispFeatureDev, O_RDWR | O_CLOEXEC);
        if (mFd < 0) return false;
        DozeReq req{0, 0, value};
        return ioctl(mFd, kMiDispSetDozeBrightness, &req) == 0;
    }

    // A transition can reject the first ioctl. Cancel retries as soon as their
    // screen state ends, so an old AOD command cannot dim the awake lockscreen.
    void apply(uint32_t value) {
        applyDozeLevelWhileCurrent(value, liveScreenState,
            [this](uint32_t level) { return applyOnce(level); },
            [] { usleep(150 * 1000); });
    }

    int mFd = -1;
    uint32_t mLevel = kDozeHigh;
    // Accessed only by the doze worker; no fingerprint monitoring or release writes.
    bool mFollowFade = false;
    int mDbv = kAodDbvMax;
};

// --- First-boot AOD defaults ----------------------------------------------------------------------
// Applied once after the first boot of a fresh flash, then recorded with a marker setting so later
// changes made in Settings are never overwritten. Runs as a short-lived second instance of this
// binary (`--aod-defaults`) that init starts as uid shell once boot has completed and setup has
// finished (the setup wizard initialises AOD itself and would overwrite them): `cmd settings put`
// only accepts shell or root callers. On later boots it only re-applies kEveryBoot and reads the
// marker, then exits.
struct SecureSetting {
    const char* key;
    const char* value;
};
constexpr SecureSetting kAodDefaults[] = {
        // First: the AOD app (com.oplus.aod) resets always-on/timer/energy-saving to 0 the first
        // time it starts unless this is set, and then switches AOD to energy saving. It starts
        // lazily, often well after this has run, so without it the mode below did not stick.
        {"Setting_AodOldDataHasBeenReset", "1"},
        {"Setting_AodEnable", "1"},
        {"Setting_AodSwitchEnable", "1"},
        {"Setting_AodEnableImmediate", "1"},
        {"Setting_AodUserSetTime", "0"},
        {"Setting_AodUserEnergySavingSet", "0"},
        {"Setting_AodAdditionalTime", "1"},
        {"Setting_AodAdditionalNotification", "1"},
        {"Setting_AodAdditionalBattery", "1"},
        {"Setting_AodAdditionalDate", "1"},
        {"Setting_AodAdditionalLunar", "1"},
        {"Setting_AodAdditionalUV", "1"},
        {"Setting_AodSceneInfoSwitchEnable", "1"},
        {"Setting_AodSceneMusicSwitchEnable", "1"},
        {"Setting_Sub_AodSwitchEnable", "1"},
        {"Setting_Sub_AodEnableImmediate", "1"},
        {"Setting_Sub_AodFullScreen", "1"},
        {"Setting_Sub_AodUserEnergySavingSet", "0"},
        {"oplus_keyguard_panoramic_aod_support", "1"},
        {"oplus_keyguard_panoramic_aod_all_day_support", "1"},
        {"oplus_keyguard_workshop_aod_support", "1"},
        {"aod_media_support", "1"},
        {"full_screen_aod", "0"},
        {"Setting_AodStyleInfo", R"({"type":2,"uuid":""})"},
        {"panoramic_aod_enable", "1"},
        {"panoramic_aod_rate", "2000"},
        {"sub_display_aod_on_to_doze_switch", "1"},
};
// v2: v1 lacked Setting_AodOldDataHasBeenReset, so devices that ran it apply the list once more.
constexpr char kAodDefaultsMarker[] = "oplus_bridge_aod_defaults_v2";

// Applied on every boot, not once: SystemUI rewrites these each time it starts.
constexpr SecureSetting kEveryBoot[] = {
        // SystemUI sets this from the OnePlus touch firmware's AOD-media gesture bit (touch node 39,
        // 0x10000), which this phone's touch driver does not report, so AOD media went off at every
        // boot. It is the "feature available" switch; the user's own AOD music toggle is separate.
        // Measured: SystemUI's start-up check lands ~2.5 s before boot completed, so this runs after.
        {"aod_media_support", "1"},
};
constexpr unsigned kEveryBootRecheckS = 10;  // one more look, in case SystemUI's check ran late

// Runs /system/bin/cmd with `args` and waits for it. Returns true if it exited with status 0;
// its stdout, trailing whitespace trimmed, goes to *out.
bool runCmd(std::initializer_list<const char*> args, std::string* out = nullptr) {
    std::vector<char*> argv{const_cast<char*>("cmd")};
    for (const char* a : args) argv.push_back(const_cast<char*>(a));
    argv.push_back(nullptr);
    int pipeFd[2];
    if (pipe2(pipeFd, O_CLOEXEC) != 0) return false;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, pipeFd[1], STDOUT_FILENO);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    pid_t pid;
    int rc = posix_spawn(&pid, "/system/bin/cmd", &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(pipeFd[1]);
    std::string text;
    char buf[256];
    ssize_t n;
    while (rc == 0 && ((n = read(pipeFd[0], buf, sizeof(buf))) > 0 || (n < 0 && errno == EINTR))) {
        if (n > 0) text.append(buf, n);
    }
    close(pipeFd[0]);
    if (rc != 0) return false;
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    while (!text.empty() && isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
    if (out != nullptr) *out = text;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// Puts each kEveryBoot value; with onlyIfReset, only those that read back different.
void applyEveryBoot(bool onlyIfReset) {
    for (const SecureSetting& s : kEveryBoot) {
        std::string now;
        if (onlyIfReset && (!runCmd({"settings", "get", "secure", s.key}, &now) || now == s.value)) {
            continue;
        }
        runCmd({"settings", "put", "secure", s.key, s.value});
    }
}

void applyAodDefaults() {
    applyEveryBoot(false);
    // Only a marker read back as unset applies them: a failed read must never overwrite settings.
    std::string marker;
    if (runCmd({"settings", "get", "secure", kAodDefaultsMarker}, &marker) && marker == "null") {
        bool ok = true;
        for (const SecureSetting& s : kAodDefaults) {
            ok &= runCmd({"settings", "put", "secure", s.key, s.value});
        }
        if (ok) runCmd({"settings", "put", "secure", kAodDefaultsMarker, "1"});  // else next boot
    }
    sleep(kEveryBootRecheckS);
    applyEveryBoot(true);
}

#ifdef BRIDGE_PANEL_FEATURE
// --- OnePlus display panel capabilities ----------------------------------------------------------
// OnePlus SystemUI asks vendor.oplus.hardware.displaypanelfeature for feature 217 to learn what the
// panel supports for AOD. The Xiaomi vendor has no such service, so every query failed and SystemUI
// treated full-screen (panoramic) AOD as an energy-saving case: hidden 5 s after it appeared, with
// no power block over the screen-off transition. Only the capabilities whose behaviour SystemUI
// implements itself are reported; the smooth-transition bit (2) is not, because it hands AOD entry
// to a panel command (set 217 [3]) this DDIC cannot execute.
constexpr int32_t kFeatureAodPanelCaps = 217;
constexpr int32_t kCapPanoramic = 4;
constexpr int32_t kCapPanoramicAllDay = 8;
constexpr int32_t kCapSmoothTransition = 2;  // SystemUI animates screen-off instead of cutting to black
constexpr int32_t kCapDozeDirect = 1;        // gotoDozeWithOff=false: panel goes ON->DOZE, no OFF gap
// Feature 211 = udfps type. Without bit 16 SystemUI takes the "optical, no local HBM" sleep path
// (AodType 5001): a black mask over the lockscreen until the panel is in DOZE, then the AOD fades in.
// With it, the lockscreen morphs into the panoramic AOD while the backlight ramps (AodType 4001).
// Bit 16 is the truth here: mi_disp lights the sensor itself (LHBM_ON_WHITE_1000NIT on finger down),
// so SystemUI also drops its own FP dim layer/highlight. system_server's FP path reads only bits
// 4/1024, so it behaves as with no answer.
constexpr int32_t kFeatureUdfpsType = 211;
constexpr int32_t kUdfpsLocalHbm = 16;
constexpr char kPanelFeatureService[] =
        "vendor.oplus.hardware.displaypanelfeature.IDisplayPanelFeature/default";

// Every other feature must look exactly as it did with no service at all, so callers keep their own
// defaults: they read values[0] whatever `ret` says, and a 0 there is a real answer (feature 211 = 0
// turned on SystemUI's fingerprint dim layer and system_server's "optical without local HBM" mode:
// bright fingerprint icon over a dimmed lockscreen; 211 is now answered with 16, see above).
// "Unknown transaction" reaches Java callers as a RemoteException, the same fallback path as a
// missing service.
class PanelFeature : public aidl::vendor::oplus::hardware::displaypanelfeature::BnDisplayPanelFeature {
  public:
    // `values` arrives pre-sized to the caller's array length and must keep that length.
    ndk::ScopedAStatus getDisplayPanelFeatureValue(int32_t id, std::vector<int32_t>* values,
                                                   int32_t* ret) override {
        if (values == nullptr || values->empty()) return unsupported();
        if (id == kFeatureAodPanelCaps) {
            (*values)[0] = kCapPanoramic | kCapPanoramicAllDay | kCapSmoothTransition | kCapDozeDirect;
        } else if (id == kFeatureUdfpsType) {
            (*values)[0] = kUdfpsLocalHbm;
        } else {
            return unsupported();
        }
        *ret = 0;
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus setDisplayPanelFeatureValue(int32_t id, const std::vector<int32_t>&,
                                                   int32_t* ret) override {
        if (id != kFeatureAodPanelCaps) return unsupported();
        *ret = 0;
        return ndk::ScopedAStatus::ok();
    }

    // `out List<String>`: the request carries only the id (an `out String[]` stub also read an array
    // length the framework never sends, so its call failed with NOT_ENOUGH_DATA, which system_server
    // does not catch). Unknown transaction reaches callers as RemoteException, which they handle.
    ndk::ScopedAStatus getDisplayPanelInfo(int32_t, std::vector<std::string>*, int32_t*) override {
        return unsupported();
    }

  private:
    static ndk::ScopedAStatus unsupported() {
        return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
    }
};

// Registered before anything else so SystemUI finds it when it reads the panel caps at startup.
void startPanelFeatureService() {
    static std::shared_ptr<PanelFeature> service = ndk::SharedRefBase::make<PanelFeature>();
    if (!ABinderProcess_isThreadPoolStarted()) {
        ABinderProcess_setThreadPoolMaxThreadCount(2);
        ABinderProcess_startThreadPool();
    }
    AServiceManager_addService(service->asBinder().get(), kPanelFeatureService);
}
#endif  // BRIDGE_PANEL_FEATURE

}  // namespace

class Bridge {
  public:
    bool init() {
        mMgr = ASensorManager_getInstanceForPackage("oplus.sensorbridge");
        if (mMgr == nullptr) return false;
        mLooper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        mQueue = ASensorManager_createEventQueue(mMgr, mLooper, kIdSensors, nullptr, nullptr);
        mPickup = findWakeUpSensor(mMgr, kTypeXiaomiPickup);
        mMotion = findWakeUpSensor(mMgr, kTypeMotionDetect);
        mStationary = findWakeUpSensor(mMgr, kTypeStationaryDetect);
        mPocket = ASensorManager_getDefaultSensor(mMgr, kTypeXiaomiPocket);
        return mQueue != nullptr && mPickup != nullptr;
    }

    [[noreturn]] void run() {
        std::thread([this] { socketLoop(); }).detach();
        for (;;) {
            ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
            drainSensors();
        }
    }

  private:
    static int connectHal() {
        int fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
        if (fd < 0) return -1;
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        const size_t nameLen = strlen(kSocketName);
        memcpy(addr.sun_path + 1, kSocketName, nameLen);
        socklen_t len = offsetof(sockaddr_un, sun_path) + 1 + nameLen;
        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), len) != 0) {
            close(fd);
            return -1;
        }
        return fd;
    }

    // HAL link runs on its own thread: the vendor looper has no ALooper_addFd.
    [[noreturn]] void socketLoop() {
        for (;;) {
            int fd = connectHal();
            if (fd < 0) {
                sleep(1);
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(mLock);
                mFd = fd;
            }
            Msg m;
            for (;;) {
                ssize_t n = recv(fd, &m, sizeof(m), 0);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) break;
                if (n == sizeof(m) && m.type == MSG_ENABLE && m.sensor > 0 && m.sensor < VS_COUNT) {
                    std::lock_guard<std::mutex> lock(mLock);
                    setWantedLocked(m.sensor, m.enabled != 0);
                }
            }
            {
                std::lock_guard<std::mutex> lock(mLock);
                mFd = -1;
                for (int32_t s = 1; s < VS_COUNT; s++) setWantedLocked(s, false);
            }
            close(fd);
        }
    }

    void setWantedLocked(int32_t sensor, bool on) {
        if (mWanted[sensor] == on) return;
        mWanted[sensor] = on;
        if (sensor == VS_PICKUP) {
            if (on) {
                mPickupEnabledAtNs = bootTimeNs();
                mLastPickupSent = -1.0f;
                ASensorEventQueue_registerSensor(mQueue, mPickup, 0, 0);
            } else {
                ASensorEventQueue_disableSensor(mQueue, mPickup);
            }
        } else if (sensor == VS_AMD && mMotion && mStationary) {
            disarm(mMotion);
            disarm(mStationary);
            if (on) arm(mMotion);
        } else if (sensor == VS_GESTURE_PROX) {
            if (mPocket) on ? arm(mPocket) : disarm(mPocket);
            if (on) {
                // A stale "near" would hide the AOD and block double tap on a desk: start far and
                // follow real changes only. Without pocket_mode it simply stays far.
                mLastProxSent = 1.0f;
                sendEventLocked(VS_GESTURE_PROX, 1.0f, bootTimeNs());
            }
        }
    }

    // motion/stationary detect are one-shot: they auto-disable after firing and must be re-armed.
    void arm(const ASensor* s) { ASensorEventQueue_registerSensor(mQueue, s, 0, 0); }
    void disarm(const ASensor* s) { ASensorEventQueue_disableSensor(mQueue, s); }

    void drainSensors() {
        holdWakelock();  // before reading: getEvents acks (and releases) the service wakelock
        ASensorEvent ev[8];
        ssize_t n;
        while ((n = ASensorEventQueue_getEvents(mQueue, ev, 8)) > 0) {
            std::lock_guard<std::mutex> lock(mLock);
            for (ssize_t i = 0; i < n; i++) handleEventLocked(ev[i]);
        }
    }

    void handleEventLocked(const ASensorEvent& e) {
        switch (e.type) {
            case kTypeXiaomiPickup: {
                if (!mWanted[VS_PICKUP]) return;
                float out;
                int64_t ms = (bootTimeNs() - mPickupEnabledAtNs) / 1000000;
                if (translatePickup(e.data[0], ms, mLastPickupSent, &out)) {
                    mLastPickupSent = out;
                    sendEventLocked(VS_PICKUP, out, e.timestamp);
                }
                break;
            }
            case kTypeMotionDetect:
                if (!mWanted[VS_AMD]) return;
                sendEventLocked(VS_AMD, 2.0f, e.timestamp);
                disarm(mMotion);
                arm(mStationary);
                break;
            case kTypeStationaryDetect:
                if (!mWanted[VS_AMD]) return;
                sendEventLocked(VS_AMD, 1.0f, e.timestamp);
                disarm(mStationary);
                arm(mMotion);
                break;
            case kTypeXiaomiPocket: {
                if (!mWanted[VS_GESTURE_PROX]) return;
                float out;
                if (translatePocket(e.data, &out) && out != mLastProxSent) {
                    mLastProxSent = out;
                    sendEventLocked(VS_GESTURE_PROX, out, e.timestamp);
                }
                break;
            }
        }
    }

    void sendEventLocked(int32_t sensor, float value, int64_t ts) {
        if (mFd < 0) return;
        Msg m{MSG_EVENT, sensor, 0, value, ts};
        send(mFd, &m, sizeof(m), MSG_NOSIGNAL);
    }

    ASensorManager* mMgr = nullptr;
    ALooper* mLooper = nullptr;
    ASensorEventQueue* mQueue = nullptr;
    const ASensor* mPickup = nullptr;
    const ASensor* mMotion = nullptr;
    const ASensor* mStationary = nullptr;
    const ASensor* mPocket = nullptr;
    std::mutex mLock;  // guards everything below
    int mFd = -1;
    bool mWanted[VS_COUNT] = {};
    int64_t mPickupEnabledAtNs = 0;
    float mLastPickupSent = -1.0f;
    float mLastProxSent = -1.0f;
};

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "--aod-defaults") == 0) {
        applyAodDefaults();
        return 0;
    }
    // Init latch for the SurfaceFlinger reset in the rc: "running" means the reset was done.
    if (argc > 1 && strcmp(argv[1], "--latch") == 0) {
        for (;;) pause();
    }
#ifdef BRIDGE_PANEL_FEATURE
    startPanelFeatureService();
#endif
#ifndef BRIDGE_NO_PANEL_WRITES
    // Independent of the sensor bridge: start even if sensorservice is not up yet.
    std::thread([] { DozeKeeper().run(); }).detach();
#endif

    Bridge bridge;
    // The framework sensor service may not be up yet when started early in boot.
    while (!bridge.init()) sleep(3);
    bridge.run();
}
