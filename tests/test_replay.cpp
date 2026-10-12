#include <gtest/gtest.h>
#include "ability_vm.hpp"
#include "checksum.hpp"
#include "recording.hpp"
#include "replay.hpp"
#include "sim.hpp"

namespace {
matchrec::MatchRecording record_match(uint64_t seed, int ticks) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    MatchRecorder recorder(seed);
    for (int t = 0; t < ticks; ++t) {
        TickInputs inputs = gen_tick_inputs((uint64_t)t, seed, state);
        apply_tick(state, inputs, vm);
        recorder.append((uint64_t)t, inputs, checksum_tick(state));
    }
    return recorder.recording();
}
} // namespace

TEST(Replay, EveryInputAppendedToRecording) {
    auto rec = record_match(99, 100);
    EXPECT_EQ(rec.ticks_size(), 100);
    for (int i = 0; i < rec.ticks_size(); ++i) {
        EXPECT_EQ(rec.ticks(i).inputs_size(), kMaxPlayers);
    }
}

TEST(Replay, BitIdenticalChecksumOver40000PlusTicks) {
    const int kTicks = 45000;
    auto rec = record_match(2026, kTicks);
    ASSERT_EQ(rec.ticks_size(), kTicks);

    AbilityVM vm(LUA_DIR);
    ReplayResult result = replay_and_check(rec, vm);
    EXPECT_TRUE(result.bit_identical);
    EXPECT_EQ(result.ticks_checked, (uint64_t)kTicks);
    EXPECT_FALSE(result.first_divergent_tick.has_value());
}

TEST(Replay, SaveAndLoadRoundTripIsByteIdentical) {
    auto rec = record_match(5, 3000);
    MatchRecorder recorder(rec.seed());
    for (int i = 0; i < rec.ticks_size(); ++i) {
        TickInputs inputs = to_tick_inputs(rec.ticks(i));
        recorder.append(rec.ticks(i).tick(), inputs, rec.ticks(i).checksum());
    }
    std::string path = "test_recording_roundtrip.bin";
    ASSERT_TRUE(recorder.save(path));
    matchrec::MatchRecording loaded;
    ASSERT_TRUE(load_recording(path, loaded));
    ASSERT_EQ(loaded.ticks_size(), rec.ticks_size());
    for (int i = 0; i < rec.ticks_size(); ++i) {
        EXPECT_EQ(loaded.ticks(i).checksum(), rec.ticks(i).checksum());
    }
    std::remove(path.c_str());
}
