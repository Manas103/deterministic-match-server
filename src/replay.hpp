#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>
#include "checksum.hpp"
#include "match_recording.pb.h"
#include "recording.hpp"

struct ReplayResult {
    bool bit_identical;
    uint64_t ticks_checked;
    std::optional<uint64_t> first_divergent_tick;
};

// Re-runs every tick in `rec` from a fresh MatchState and compares the
// recomputed checksum against the one stored at recording time. Stops at
// the first mismatch, which is the exact tick the two runs first disagreed
// (the whole point of per-tick checksums: divergence does not have to be
// bisected after the fact, it is caught at the tick it happens).
inline ReplayResult replay_and_check(const matchrec::MatchRecording& rec,
                                      AbilityVM& vm) {
    MatchState state;
    reset_match(state);
    ReplayResult result{true, 0, std::nullopt};
    for (int i = 0; i < rec.ticks_size(); ++i) {
        const matchrec::TickRecord& tr = rec.ticks(i);
        TickInputs inputs = to_tick_inputs(tr);
        apply_tick(state, inputs, vm);
        uint64_t recomputed = checksum_tick(state);
        result.ticks_checked = (uint64_t)(i + 1);
        if (recomputed != tr.checksum()) {
            result.bit_identical = false;
            result.first_divergent_tick = tr.tick();
            return result;
        }
    }
    return result;
}

// Compares two already-computed checksum streams (used by the divergence
// test, which injects a perturbation into a copy of a known-good stream
// rather than re-deriving float nondeterminism, since the sim core is pure
// fixed-point by construction and has no float path to perturb honestly).
inline std::optional<uint64_t> first_mismatch(const std::vector<uint64_t>& a,
                                                const std::vector<uint64_t>& b,
                                                const std::vector<uint64_t>& ticks) {
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return ticks[i];
    }
    return std::nullopt;
}
