#pragma once
#include <cstdint>

// xorshift64*, used only to generate synthetic test/benchmark traffic.
// Integer-only and seeded, so two runs from the same seed produce the exact
// same input stream; this is not gameplay randomness, it stands in for real
// client input the same way the other repos use a fixed-seed synthetic feed.
struct Xorshift64 {
    uint64_t state;
    explicit Xorshift64(uint64_t seed) : state(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
    uint64_t next() {
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x * 0x2545F4914F6CDD1DULL;
    }
    int range(int lo, int hi) { // inclusive
        uint64_t span = (uint64_t)(hi - lo + 1);
        return lo + (int)(next() % span);
    }
};
