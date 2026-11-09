// sensors.oplusbridge.so - Multi-HAL 2.1 sub-HAL exposing the Oplus-convention sensors that
// OxygenOS SystemUI (AOD pickup) and com.oplus.gesture (raise to wake) look up by type:
//   type 22       "pick_up_motion" wake-up, special reporting: 0.0 = lifted, 1.0 = put down
//   type 33171026 "amd"            wake-up, on-change:         2.0 = moving, 1.0 = stationary
//   type 33171066 "gesture_prox"   wake-up, on-change:         0.0 = near,   1.0 = far
// Events are produced by oplus-sensor-bridged (root daemon) from the Xiaomi/QTI hub sensors and
// delivered over an abstract unix socket. Must be listed FIRST in hals.conf so its type-22 sensor
// has the lowest handle and wins SensorManager.getDefaultSensor(22, true) over QTI ccd_tilt.
#define LOG_TAG "OplusSensorBridge"

#include <V2_1/SubHal.h>

// Release builds are silent; build with -DBRIDGE_DEBUG to get logcat output.
#ifdef BRIDGE_DEBUG
#include <log/log.h>
#define BLOGI(...) ALOGI(__VA_ARGS__)
#define BLOGW(...) ALOGW(__VA_ARGS__)
#define BLOGE(...) ALOGE(__VA_ARGS__)
#else
#define BLOGI(...) do {} while (0)
#define BLOGW(...) do {} while (0)
#define BLOGE(...) do {} while (0)
#endif

#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "bridge_proto.h"

namespace oplusbridge {

using ::android::sp;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::sensors::V1_0::MetaDataEventType;
using ::android::hardware::sensors::V1_0::OperationMode;
using ::android::hardware::sensors::V1_0::RateLevel;
using ::android::hardware::sensors::V1_0::Result;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::sensors::V1_0::SharedMemInfo;
using ::android::hardware::sensors::V2_1::Event;
using ::android::hardware::sensors::V2_1::SensorInfo;
using ::android::hardware::sensors::V2_1::SensorType;
using ::android::hardware::sensors::V2_1::implementation::IHalProxyCallback;
using ::android::hardware::sensors::V2_1::implementation::ISensorsSubHal;

static int64_t bootTimeNs() {
    timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static SensorInfo makeSensor(int32_t handle, const char* name, SensorType type,
                             const char* typeAsString, uint32_t flags) {
    SensorInfo s{};
    s.sensorHandle = handle;
    s.name = name;
    s.vendor = "Oplus Sensor Bridge";
    s.version = 1;
    s.type = type;
    s.typeAsString = typeAsString;
    s.maxRange = 2.0f;
    s.resolution = 1.0f;
    s.power = 0.1f;
    s.minDelay = 0;
    s.fifoReservedEventCount = 0;
    s.fifoMaxEventCount = 0;
    s.requiredPermission = "";
    s.maxDelay = 0;
    s.flags = flags;
    return s;
}

class OplusBridgeSubHal : public ISensorsSubHal {
  public:
    OplusBridgeSubHal() {
        const uint32_t wake = static_cast<uint32_t>(SensorFlagBits::WAKE_UP);
        mSensors.push_back(makeSensor(
                VS_PICKUP, "pick_up_motion  Wakeup", SensorType::TILT_DETECTOR,
                "android.sensor.tilt_detector",
                wake | static_cast<uint32_t>(SensorFlagBits::SPECIAL_REPORTING_MODE)));
        mSensors.push_back(makeSensor(VS_AMD, "amd_oplus  Wakeup",
                                      static_cast<SensorType>(kTypeOplusAmd), "qti.sensor.amd",
                                      wake | static_cast<uint32_t>(SensorFlagBits::ON_CHANGE_MODE)));
        mSensors.push_back(makeSensor(VS_GESTURE_PROX, "gesture_prox_oplus  Wakeup",
                                      static_cast<SensorType>(kTypeOplusGestureProx),
                                      "oplus.sensor.gesture_prox",
                                      wake | static_cast<uint32_t>(SensorFlagBits::ON_CHANGE_MODE)));
    }

    Return<void> getSensorsList_2_1(getSensorsList_2_1_cb _hidl_cb) override {
        _hidl_cb(hidl_vec<SensorInfo>(mSensors));
        return Void();
    }

    Return<Result> setOperationMode(OperationMode mode) override {
        return mode == OperationMode::NORMAL ? Result::OK : Result::BAD_VALUE;
    }

    Return<Result> activate(int32_t sensorHandle, bool enabled) override {
        if (!isValid(sensorHandle)) return Result::BAD_VALUE;
        std::lock_guard<std::mutex> lock(mLock);
        BLOGI("activate sensor=%d enabled=%d daemon=%s", sensorHandle, enabled,
              mClientFd >= 0 ? "connected" : "absent");
        if (mEnabled[sensorHandle] != enabled) {
            mEnabled[sensorHandle] = enabled;
            sendEnableLocked(sensorHandle, enabled);
        }
        return Result::OK;
    }

    Return<Result> batch(int32_t sensorHandle, int64_t, int64_t) override {
        return isValid(sensorHandle) ? Result::OK : Result::BAD_VALUE;
    }

    Return<Result> flush(int32_t sensorHandle) override {
        if (!isValid(sensorHandle)) return Result::BAD_VALUE;
        Event e{};
        e.timestamp = 0;
        e.sensorHandle = sensorHandle;
        e.sensorType = SensorType::META_DATA;
        e.u.meta.what = MetaDataEventType::META_DATA_FLUSH_COMPLETE;
        post(e);
        return Result::OK;
    }

    Return<Result> injectSensorData_2_1(const Event&) override {
        return Result::INVALID_OPERATION;
    }

    Return<void> registerDirectChannel(const SharedMemInfo&,
                                       registerDirectChannel_cb _hidl_cb) override {
        _hidl_cb(Result::INVALID_OPERATION, -1);
        return Void();
    }

    Return<Result> unregisterDirectChannel(int32_t) override { return Result::INVALID_OPERATION; }

    Return<void> configDirectReport(int32_t, int32_t, RateLevel,
                                    configDirectReport_cb _hidl_cb) override {
        _hidl_cb(Result::INVALID_OPERATION, 0);
        return Void();
    }

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>&) override {
        if (fd.getNativeHandle() == nullptr || fd->numFds < 1) return Void();
        std::lock_guard<std::mutex> lock(mLock);
        dprintf(fd->data[0],
                "OplusSensorBridge: daemon=%s pickup=%d amd=%d gesture_prox=%d in=%llu posted=%llu\n",
                mClientFd >= 0 ? "connected" : "absent", mEnabled[VS_PICKUP], mEnabled[VS_AMD],
                mEnabled[VS_GESTURE_PROX], (unsigned long long)mEventsIn.load(),
                (unsigned long long)mEventsPosted.load());
        return Void();
    }

    const std::string getName() override { return "OplusSensorBridge"; }

    Return<Result> initialize(const sp<IHalProxyCallback>& halProxyCallback) override {
        {
            // Called on every framework restart: start from a clean, all-disabled state.
            std::lock_guard<std::mutex> lock(mLock);
            mCallback = halProxyCallback;
            for (int32_t s = 1; s < VS_COUNT; s++) {
                if (mEnabled[s]) {
                    mEnabled[s] = false;
                    sendEnableLocked(s, false);
                }
            }
        }
        std::call_once(mServerStarted, [this] { std::thread([this] { serverLoop(); }).detach(); });
        return Result::OK;
    }

  private:
    static bool isValid(int32_t handle) { return handle > 0 && handle < VS_COUNT; }

    void sendEnableLocked(int32_t sensor, bool enabled) {
        if (mClientFd < 0) return;
        Msg m{MSG_ENABLE, sensor, enabled ? 1 : 0, 0.0f, 0};
        if (send(mClientFd, &m, sizeof(m), MSG_NOSIGNAL | MSG_DONTWAIT) != sizeof(m)) {
            BLOGW("send enable sensor=%d failed: %s", sensor, strerror(errno));
        }
    }

    void post(const Event& event) {
        sp<IHalProxyCallback> cb;
        {
            std::lock_guard<std::mutex> lock(mLock);
            cb = mCallback;
        }
        if (cb == nullptr) return;
        // Every sensor here is wake-up, and HalProxy asserts that any event on a wake-up handle
        // (flush-complete included) arrives with a locked wakelock.
        std::vector<Event> events{event};
        cb->postEvents(events, cb->createScopedWakelock(true));
        mEventsPosted++;
    }

    void serverLoop() {
        const size_t nameLen = strlen(kSocketName);
        for (;;) {
            int srv = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
            sockaddr_un addr{};
            addr.sun_family = AF_UNIX;
            memcpy(addr.sun_path + 1, kSocketName, nameLen);  // abstract namespace
            socklen_t len = offsetof(sockaddr_un, sun_path) + 1 + nameLen;
            if (srv < 0 || bind(srv, reinterpret_cast<sockaddr*>(&addr), len) != 0 ||
                listen(srv, 1) != 0) {
                BLOGE("socket setup failed: %s, retrying", strerror(errno));
                if (srv >= 0) close(srv);
                sleep(5);
                continue;
            }
            BLOGI("listening on @%s", kSocketName);
            for (;;) {
                int fd = accept4(srv, nullptr, nullptr, SOCK_CLOEXEC);
                if (fd < 0) {
                    if (errno != EINTR) sleep(1);
                    continue;
                }
                ucred cred{};
                socklen_t credLen = sizeof(cred);
                // Only root (KernelSU install) or the HAL's own uid (native init service,
                // user system) may feed events.
                if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &credLen) != 0 ||
                    (cred.uid != 0 && cred.uid != getuid())) {
                    BLOGW("rejecting peer uid=%d pid=%d", cred.uid, cred.pid);
                    close(fd);
                    continue;
                }
                serveClient(fd, cred.pid);
            }
        }
    }

    void serveClient(int fd, pid_t pid) {
        {
            std::lock_guard<std::mutex> lock(mLock);
            if (mClientFd >= 0) close(mClientFd);
            mClientFd = fd;
            BLOGI("daemon connected pid=%d", pid);
            // Resync: the daemon may have restarted while sensors were active.
            for (int32_t s = 1; s < VS_COUNT; s++) sendEnableLocked(s, mEnabled[s]);
        }
        Msg m;
        for (;;) {
            ssize_t n = recv(fd, &m, sizeof(m), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) break;
            if (n != sizeof(m) || m.type != MSG_EVENT || !isValid(m.sensor)) continue;
            mEventsIn++;
            {
                std::lock_guard<std::mutex> lock(mLock);
                if (!mEnabled[m.sensor]) continue;
            }
            Event e{};
            e.timestamp = m.timestampNs > 0 ? m.timestampNs : bootTimeNs();
            e.sensorHandle = m.sensor;
            e.sensorType = mSensors[m.sensor - 1].type;
            for (size_t k = 0; k < 16; k++) e.u.data[k] = 0.0f;
            e.u.data[0] = m.value;
            // SystemUI reads values[1] == 0.0 as "screen facing down"; a plain proximity sensor
            // (its fallback) has no values[1], which it treats as 1.0.
            if (m.sensor == VS_GESTURE_PROX) e.u.data[1] = 1.0f;
            BLOGI("event sensor=%d value=%.1f", m.sensor, m.value);
            post(e);
        }
        std::lock_guard<std::mutex> lock(mLock);
        if (mClientFd == fd) mClientFd = -1;
        close(fd);
        BLOGW("daemon disconnected");
    }

    std::vector<SensorInfo> mSensors;
    std::mutex mLock;
    sp<IHalProxyCallback> mCallback;
    bool mEnabled[VS_COUNT] = {};
    int mClientFd = -1;
    std::once_flag mServerStarted;
    std::atomic<uint64_t> mEventsIn{0};
    std::atomic<uint64_t> mEventsPosted{0};
};

}  // namespace oplusbridge

::android::hardware::sensors::V2_1::implementation::ISensorsSubHal* sensorsHalGetSubHal_2_1(
        uint32_t* version) {
    static oplusbridge::OplusBridgeSubHal subHal;
    *version = SUB_HAL_2_1_VERSION;
    return &subHal;
}
