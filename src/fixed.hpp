#pragma once
#include <cstdint>
#include <cstdlib>

// Deterministic Q16.16 fixed-point. Every arithmetic op is pure integer so the
// result is bit-identical across platforms and optimization levels, which is
// the property the whole server leans on: no float ever touches player state.
struct Fixed {
    int64_t raw = 0;

    static constexpr int SHIFT = 16;
    static constexpr int64_t ONE = int64_t(1) << SHIFT;

    // Left-shifting a negative signed integer is undefined behavior before
    // C++20 (UBSan caught this on the very first test run, from_int(-1000)
    // in Fixed.IntRoundTrip). Shifting the unsigned reinterpretation and
    // casting back is well defined and produces the identical bit pattern
    // on every two's-complement machine this runs on.
    static Fixed from_int(int64_t v) {
        Fixed f; f.raw = (int64_t)((uint64_t)v << SHIFT); return f;
    }
    static Fixed from_raw(int64_t r) { Fixed f; f.raw = r; return f; }

    int64_t to_int() const { return raw >> SHIFT; }

    Fixed operator+(Fixed o) const { return from_raw(raw + o.raw); }
    Fixed operator-(Fixed o) const { return from_raw(raw - o.raw); }
    Fixed operator-() const { return from_raw(-raw); }

    Fixed operator*(Fixed o) const {
        // widen to __int128 so the intermediate product never overflows
        __int128 wide = (__int128)raw * (__int128)o.raw;
        return from_raw((int64_t)(wide >> SHIFT));
    }

    Fixed operator/(Fixed o) const {
        __int128 wide = ((__int128)raw << SHIFT);
        return from_raw((int64_t)(wide / o.raw));
    }

    bool operator==(Fixed o) const { return raw == o.raw; }
    bool operator!=(Fixed o) const { return raw != o.raw; }
    bool operator<(Fixed o) const { return raw < o.raw; }
    bool operator<=(Fixed o) const { return raw <= o.raw; }
    bool operator>(Fixed o) const { return raw > o.raw; }
    bool operator>=(Fixed o) const { return raw >= o.raw; }
};

// Integer Newton's-method sqrt on the raw fixed-point value. Deterministic:
// pure integer iteration, no float, no platform-dependent rounding.
inline Fixed fixed_sqrt(Fixed x) {
    if (x.raw <= 0) return Fixed::from_raw(0);
    __int128 target = (__int128)x.raw << Fixed::SHIFT;
    int64_t guess = x.raw > 0 ? x.raw : 1;
    for (int i = 0; i < 40; ++i) {
        if (guess == 0) { guess = 1; }
        __int128 next = (guess + (int64_t)(target / guess)) / 2;
        if (next == guess) break;
        guess = (int64_t)next;
    }
    return Fixed::from_raw(guess);
}
