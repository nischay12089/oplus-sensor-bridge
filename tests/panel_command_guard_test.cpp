#include "panel_command_guard.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace oplusbridge;
int main() {
    int calls = 0, waits = 0, state = 3;
    auto read = [&] { return state; };
    auto fail = [&](uint32_t) { ++calls; return false; };
    auto wake = [&] { ++waits; state = 2; };
    assert(!applyDozeLevelWhileCurrent(1, read, fail, wake));
    assert(calls == 1 && waits == 1); // failed AOD write never reaches awake lockscreen
    calls = waits = 0;
    assert(!applyDozeLevelWhileCurrent(1, read, fail, wake));
    assert(calls == 0 && waits == 0);
    auto doze = [&] { ++waits; state = 3; };
    assert(!applyDozeLevelWhileCurrent(0, read, fail, doze));
    assert(calls == 1 && waits == 1); // stale AOD-exit never turns off the next AOD
    calls = waits = 0;
    auto suspend = [&] { ++waits; state = 4; };
    auto success_after_retry = [&](uint32_t value) { assert(value == 2); return ++calls == 2; };
    assert(applyDozeLevelWhileCurrent(2, read, success_after_retry, suspend));
    assert(calls == 2 && waits == 1);
    calls = waits = 0;
    auto wait = [&] { ++waits; };
    assert(!applyDozeLevelWhileCurrent(1, read, fail, wait));
    assert(calls == 25 && waits == 24); // bounded failures, no permanent retry loop
    for (int invalid_state : {-1, 0, 1, 5}) {
        state = invalid_state; calls = waits = 0;
        assert(!applyDozeLevelWhileCurrent(1, read, fail, wait));
        assert(calls == 0 && waits == 0);
    }
    state = 3; calls = waits = 0;
    assert(!applyDozeLevelWhileCurrent(3, read, fail, wait));
    assert(calls == 0 && waits == 0);
    std::puts("PASS panel retry cancellation, wake/sleep reversal, doze-suspend, invalid states, bounded failures");
}
