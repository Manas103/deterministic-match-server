#pragma once
#include <array>
#include <cstdint>
#include "fixed.hpp"

constexpr int kMaxPlayers = 10;

struct PlayerState {
    Fixed x;
    Fixed y;
    int32_t health;
    uint8_t alive;
    uint32_t id;
};

struct MatchState {
    std::array<PlayerState, kMaxPlayers> players{};
    uint64_t tick = 0;
};

// A single player's command for one tick, decoded from the match recording.
struct PlayerInput {
    int8_t move_x;   // -1, 0, 1
    int8_t move_y;   // -1, 0, 1
    uint8_t ability; // 0 = none, 1 = fireball, 2 = heal, 3 = dash
    uint32_t target; // target player id, used by fireball/heal
};

inline void reset_match(MatchState& m) {
    m = MatchState{};
    for (int i = 0; i < kMaxPlayers; ++i) {
        m.players[i].x = Fixed::from_int((i - kMaxPlayers / 2) * 4);
        m.players[i].y = Fixed::from_int(0);
        m.players[i].health = 1000;
        m.players[i].alive = 1;
        m.players[i].id = (uint32_t)i;
    }
}
