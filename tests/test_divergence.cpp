#include <gtest/gtest.h>
#include "ability_vm.hpp"
#include "checksum.hpp"
#include "replay.hpp"
#include "sim.hpp"

// The sim core is pure fixed-point by construction, so there is no float
// path left to perturb honestly inside apply_tick. To test the localizer
// itself, this simulates what a seeded floating-point divergence would look
// like at the point it is observed: two per-tick checksum streams that
// agree up to some tick K and then disagree, because that is exactly the
// artifact a real float-vs-fixed divergence between two client builds would
// leave behind in the recorded vs. live checksum streams.
TEST(Divergence, LocalizesToExactFirstDisagreeingTick) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    std::vector<uint64_t> live, ticks;
    for (int t = 0; t < 5000; ++t) {
        TickInputs inputs = gen_tick_inputs((uint64_t)t, 314159, state);
        apply_tick(state, inputs, vm);
        live.push_back(checksum_tick(state));
        ticks.push_back((uint64_t)t);
    }

    std::vector<uint64_t> recorded = live;
    const size_t injected_tick_index = 3417;
    // Simulate a one-bit divergence first observable at this tick (e.g. a
    // seeded float path that rounded differently on one build).
    recorded[injected_tick_index] ^= 0x1ULL;
    for (size_t i = injected_tick_index + 1; i < recorded.size(); ++i) {
        recorded[i] ^= 0xDEADBEEFULL; // everything after a real divergence
                                      // also disagrees, since state is chained
    }

    auto found = first_mismatch(live, recorded, ticks);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found, ticks[injected_tick_index]);
}

TEST(Divergence, NoMismatchReportedWhenStreamsAgree) {
    std::vector<uint64_t> a = {1, 2, 3, 4, 5};
    std::vector<uint64_t> b = {1, 2, 3, 4, 5};
    std::vector<uint64_t> ticks = {10, 11, 12, 13, 14};
    EXPECT_FALSE(first_mismatch(a, b, ticks).has_value());
}
