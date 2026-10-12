#include <gtest/gtest.h>
#include "fixed.hpp"

TEST(Fixed, IntRoundTrip) {
    for (int v = -1000; v <= 1000; ++v) {
        EXPECT_EQ(Fixed::from_int(v).to_int(), v);
    }
}

TEST(Fixed, AddSubMulAreExact) {
    Fixed a = Fixed::from_int(3);
    Fixed b = Fixed::from_int(4);
    EXPECT_EQ((a + b).to_int(), 7);
    EXPECT_EQ((a - b).to_int(), -1);
    EXPECT_EQ((a * b).to_int(), 12);
}

TEST(Fixed, DivisionMatchesIntDivisionWhenExact) {
    Fixed a = Fixed::from_int(10);
    Fixed b = Fixed::from_int(5);
    EXPECT_EQ((a / b).to_int(), 2);
}

TEST(Fixed, DeterministicAcrossRepeatedComputation) {
    // The whole server leans on this: the same inputs, computed twice,
    // must produce the exact same bits, not just the same rounded value.
    Fixed acc1 = Fixed::from_int(0);
    Fixed acc2 = Fixed::from_int(0);
    for (int i = 0; i < 10000; ++i) {
        Fixed step = Fixed::from_raw((i * 37 + 11) % 65536);
        acc1 = acc1 + step * Fixed::from_int(3) - step / Fixed::from_int(2);
    }
    for (int i = 0; i < 10000; ++i) {
        Fixed step = Fixed::from_raw((i * 37 + 11) % 65536);
        acc2 = acc2 + step * Fixed::from_int(3) - step / Fixed::from_int(2);
    }
    EXPECT_EQ(acc1.raw, acc2.raw);
}

namespace {
// Reference oracle: a deliberately dumb, obviously-correct integer sqrt by
// binary search for the largest guess whose square does not exceed the
// target (the predicate "guess*guess <= target" is monotonic, which is the
// entire correctness argument), diffed exactly against the Newton's-method
// fixed_sqrt used by the real code. Slower than Newton's method, not
// algorithmically sophisticated, deliberately so: the point is a second,
// independent way to get the same answer.
Fixed slow_reference_sqrt(Fixed x) {
    if (x.raw <= 0) return Fixed::from_raw(0);
    __int128 target = (__int128)x.raw << Fixed::SHIFT;
    int64_t lo = 0, hi = x.raw <= (int64_t(1) << 32) ? (int64_t(1) << 32) : x.raw;
    while (lo < hi) {
        int64_t mid = lo + (hi - lo + 1) / 2;
        __int128 sq = (__int128)mid * mid;
        if (sq <= target) lo = mid; else hi = mid - 1;
    }
    return Fixed::from_raw(lo);
}
} // namespace

TEST(Fixed, SqrtMatchesSlowReferenceOracleExactly) {
    for (int v = 0; v <= 2000; v += 3) {
        Fixed x = Fixed::from_int(v);
        EXPECT_EQ(fixed_sqrt(x).raw, slow_reference_sqrt(x).raw) << "v=" << v;
    }
}
