#pragma once
#include <cstdint>
#include "sim_state.hpp"

// FNV-1a 64-bit over an explicit byte stream.
inline uint64_t fnv1a(const uint8_t* data, size_t len, uint64_t h = 1469598103934665603ULL) {
    for (size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// Per-tick checksum, hashing only the logical fields of each player in a
// fixed, declared order. Deliberately field-wise rather than a raw
// sizeof(PlayerState) byte hash: a raw-byte hash would fold in compiler
// struct padding, whose content is unspecified whenever the state is not
// freshly zero-initialized, which would make the checksum not actually
// reproducible across allocations. Hashing named fields avoids that class
// of bug entirely rather than relying on every call site to zero-init.
inline uint64_t checksum_tick(const MatchState& m) {
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < kMaxPlayers; ++i) {
        const PlayerState& p = m.players[i];
        int64_t x = p.x.raw;
        int64_t y = p.y.raw;
        int32_t hp = p.health;
        uint8_t alive = p.alive;
        uint32_t id = p.id;
        h = fnv1a(reinterpret_cast<const uint8_t*>(&x), sizeof(x), h);
        h = fnv1a(reinterpret_cast<const uint8_t*>(&y), sizeof(y), h);
        h = fnv1a(reinterpret_cast<const uint8_t*>(&hp), sizeof(hp), h);
        h = fnv1a(reinterpret_cast<const uint8_t*>(&alive), sizeof(alive), h);
        h = fnv1a(reinterpret_cast<const uint8_t*>(&id), sizeof(id), h);
    }
    return h;
}
