#pragma once
#include <cstdint>

namespace oplusbridge {
constexpr bool isDozeScreenState(int state) { return state == 3 || state == 4; }

constexpr bool permitsDozeLevel(int state, uint32_t level) {
    return level == 0 ? state == 2 : (level == 1 || level == 2) && isDozeScreenState(state);
}

// A rejected transition command must not survive a wake or a later sleep.
// Read the live screen state on every attempt, not the worker's cached state.
template <class ReadState, class Apply, class Wait>
bool applyDozeLevelWhileCurrent(uint32_t level, ReadState readState, Apply apply, Wait wait) {
    for (int attempt = 0; attempt < 25; ++attempt) {
        if (!permitsDozeLevel(readState(), level)) return false;
        if (apply(level)) return true;
        if (attempt < 24) wait();
    }
    return false;
}
}
