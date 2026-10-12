// Measures sustained headless ticks/sec for a 10-player match: no sleeping,
// no pacing, just how fast this machine can run tick after tick. This is
// not a claim about network latency, only about the authoritative
// simulation step itself, which is the part the 128 ticks/sec budget
// (7.8125ms per tick) has to fit inside.
#include <chrono>
#include <cstdio>
#include "ability_vm.hpp"
#include "checksum.hpp"
#include "sim.hpp"

int main(int argc, char** argv) {
    int ticks = argc > 1 ? atoi(argv[1]) : 200000;
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    uint64_t sink = 0;

    auto start = std::chrono::steady_clock::now();
    for (int t = 0; t < ticks; ++t) {
        TickInputs inputs = gen_tick_inputs((uint64_t)t, 777, state);
        apply_tick(state, inputs, vm);
        sink ^= checksum_tick(state);
    }
    auto end = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(end - start).count();
    double ticks_per_sec = ticks / secs;
    double ms_per_tick = (secs * 1000.0) / ticks;

    printf("ticks=%d elapsed_s=%.4f ticks_per_sec=%.1f ms_per_tick=%.5f budget_ms_per_tick=7.81250 sink=%llu\n",
           ticks, secs, ticks_per_sec, ms_per_tick, (unsigned long long)sink);
    return 0;
}
