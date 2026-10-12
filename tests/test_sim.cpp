#include <gtest/gtest.h>
#include "ability_vm.hpp"
#include "checksum.hpp"
#include "sim.hpp"

namespace {
std::vector<uint64_t> run_match(uint64_t seed, int ticks) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    std::vector<uint64_t> checksums;
    checksums.reserve(ticks);
    for (int t = 0; t < ticks; ++t) {
        TickInputs inputs = gen_tick_inputs((uint64_t)t, seed, state);
        apply_tick(state, inputs, vm);
        checksums.push_back(checksum_tick(state));
    }
    return checksums;
}
} // namespace

TEST(Sim, TenPlayersSimulated) {
    MatchState state;
    reset_match(state);
    int alive = 0;
    for (int i = 0; i < kMaxPlayers; ++i) alive += state.players[i].alive;
    EXPECT_EQ(alive, 10);
    EXPECT_EQ(kMaxPlayers, 10);
}

TEST(Sim, SameSeedProducesBitIdenticalChecksumStream) {
    auto a = run_match(42, 2000);
    auto b = run_match(42, 2000);
    ASSERT_EQ(a.size(), b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        EXPECT_EQ(a[i], b[i]) << "tick " << i;
    }
}

TEST(Sim, DifferentSeedProducesDifferentChecksumStream) {
    auto a = run_match(1, 500);
    auto b = run_match(2, 500);
    int differences = 0;
    for (size_t i = 0; i < a.size(); ++i) if (a[i] != b[i]) ++differences;
    EXPECT_GT(differences, 0);
}

TEST(Sim, AbilitiesEventuallyReduceSomePlayerHealth) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    int start_total = 0;
    for (int i = 0; i < kMaxPlayers; ++i) start_total += state.players[i].health;
    for (int t = 0; t < 500; ++t) {
        TickInputs inputs = gen_tick_inputs((uint64_t)t, 7, state);
        apply_tick(state, inputs, vm);
    }
    int end_total = 0;
    for (int i = 0; i < kMaxPlayers; ++i) end_total += state.players[i].health;
    EXPECT_NE(start_total, end_total);
}
