// Wire protocol between sensors.oplusbridge.so (Multi-HAL sub-HAL, server)
// and oplus-sensor-bridged (root daemon, client).
//
// Transport: abstract-namespace AF_UNIX SOCK_SEQPACKET socket, one Msg per packet.
//   HAL -> daemon : MSG_ENABLE  (framework activated/deactivated a virtual sensor)
//   daemon -> HAL : MSG_EVENT   (value already translated to the Oplus convention)
#pragma once

#include <stdint.h>

namespace oplusbridge {

// Abstract socket name (sun_path[0] == '\0').
constexpr char kSocketName[] = "oplus_sensor_bridge";

// Virtual sensors exposed by the sub-HAL. The value doubles as the sub-HAL local handle.
enum VirtualSensor : int32_t {
    VS_PICKUP = 1,  // Android type 22 (TILT_DETECTOR), OnePlus "pick_up_motion": 0.0 lift, 1.0 put down
    VS_AMD = 2,     // Oplus TYPE_AMD 33171026: 2.0 moving, 1.0 stationary
    VS_COUNT = 3,
};

constexpr int32_t kTypeOplusAmd = 33171026;

enum MsgType : int32_t {
    MSG_ENABLE = 1,
    MSG_EVENT = 2,
};

struct Msg {
    int32_t type;         // MsgType
    int32_t sensor;       // VirtualSensor
    int32_t enabled;      // MSG_ENABLE only
    float value;          // MSG_EVENT only
    int64_t timestampNs;  // MSG_EVENT only, CLOCK_BOOTTIME (sensor event time base)
};

static_assert(sizeof(Msg) == 24, "Msg layout must stay stable between HAL and daemon");

}  // namespace oplusbridge
