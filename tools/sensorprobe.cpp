// sensorprobe: subscribe to sensors by type and print their events.
// usage: sensorprobe <seconds> <type>[:w|:n] [<type>[:w|:n] ...]
//   :w = wake-up variant (default), :n = non-wake-up variant
#include <android/looper.h>
#include <android/sensor.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static int64_t nowMs() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <seconds> <type>[:w|:n] ...\n", argv[0]);
        return 1;
    }
    int seconds = atoi(argv[1]);
    ASensorManager* mgr = ASensorManager_getInstanceForPackage("oplus.sensorprobe");
    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ASensorEventQueue* q = ASensorManager_createEventQueue(mgr, looper, 1, nullptr, nullptr);

    for (int i = 2; i < argc; i++) {
        int type = atoi(argv[i]);
        bool wake = strstr(argv[i], ":n") == nullptr;
        const ASensor* s = ASensorManager_getDefaultSensorEx(mgr, type, wake);
        if (!s) {
            printf("type %d (%s): not found\n", type, wake ? "wake" : "non-wake");
            continue;
        }
        int mode = ASensor_getReportingMode(s);
        int32_t period = (mode == AREPORTING_MODE_CONTINUOUS) ? 100000 : 0;
        int rc = ASensorEventQueue_registerSensor(q, s, period, 0);
        printf("registered type=%d handle=0x%x name='%s' str='%s' mode=%d wake=%d rc=%d\n", type,
               ASensor_getHandle(s), ASensor_getName(s), ASensor_getStringType(s), mode,
               ASensor_isWakeUpSensor(s), rc);
    }
    fflush(stdout);

    int64_t end = nowMs() + seconds * 1000LL;
    while (nowMs() < end) {
        ALooper_pollOnce(500, nullptr, nullptr, nullptr);
        ASensorEvent ev[16];
        ssize_t n;
        while ((n = ASensorEventQueue_getEvents(q, ev, 16)) > 0) {
            for (ssize_t k = 0; k < n; k++) {
                printf("%lld ms  type=%d  v=[%.3f %.3f %.3f %.3f]\n", (long long)nowMs(),
                       ev[k].type, ev[k].data[0], ev[k].data[1], ev[k].data[2], ev[k].data[3]);
            }
            fflush(stdout);
        }
    }
    ASensorManager_destroyEventQueue(mgr, q);
    return 0;
}
