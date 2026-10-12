// CLI used both to produce the committed sample recording and to replay it.
//   replay_tool record <path> <seed> <ticks>
//   replay_tool replay <path>
#include <cstdio>
#include <cstring>
#include <string>
#include "ability_vm.hpp"
#include "checksum.hpp"
#include "recording.hpp"
#include "replay.hpp"
#include "sim.hpp"

static int do_record(const std::string& path, uint64_t seed, int ticks) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    MatchRecorder recorder(seed);
    for (int t = 0; t < ticks; ++t) {
        TickInputs inputs = gen_tick_inputs((uint64_t)t, seed, state);
        apply_tick(state, inputs, vm);
        recorder.append((uint64_t)t, inputs, checksum_tick(state));
    }
    if (!recorder.save(path)) {
        fprintf(stderr, "failed to write %s\n", path.c_str());
        return 1;
    }
    printf("recorded %d ticks (seed=%llu) to %s\n", ticks, (unsigned long long)seed, path.c_str());
    return 0;
}

static int do_replay(const std::string& path) {
    matchrec::MatchRecording rec;
    if (!load_recording(path, rec)) {
        fprintf(stderr, "failed to read %s\n", path.c_str());
        return 1;
    }
    AbilityVM vm(LUA_DIR);
    ReplayResult result = replay_and_check(rec, vm);
    printf("replayed %llu ticks, bit_identical=%s\n",
           (unsigned long long)result.ticks_checked,
           result.bit_identical ? "true" : "false");
    if (!result.bit_identical) {
        printf("first divergent tick: %llu\n",
               (unsigned long long)*result.first_divergent_tick);
        return 2;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s record <path> <seed> <ticks> | replay <path>\n", argv[0]);
        return 1;
    }
    std::string mode = argv[1];
    std::string path = argv[2];
    if (mode == "record") {
        if (argc < 5) { fprintf(stderr, "record needs <seed> <ticks>\n"); return 1; }
        return do_record(path, (uint64_t)strtoull(argv[3], nullptr, 10), atoi(argv[4]));
    } else if (mode == "replay") {
        return do_replay(path);
    }
    fprintf(stderr, "unknown mode %s\n", mode.c_str());
    return 1;
}
