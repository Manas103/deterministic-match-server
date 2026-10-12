#include "sim.hpp"

TickInputs gen_tick_inputs(uint64_t tick, uint64_t seed, const MatchState& state) {
    TickInputs inputs{};
    Xorshift64 rng(seed ^ (tick * 0x9E3779B97F4A7C15ULL + 1));
    for (int i = 0; i < kMaxPlayers; ++i) {
        inputs[i].move_x = (int8_t)rng.range(-1, 1);
        inputs[i].move_y = (int8_t)rng.range(-1, 1);
        inputs[i].ability = 0;
        inputs[i].target = 0;
    }
    if (tick % 7 == 0) {
        int caster = -1;
        for (int i = 0; i < kMaxPlayers; ++i) {
            if (state.players[i].alive) { caster = i; break; }
        }
        if (caster >= 0) {
            int target = (caster + 1 + rng.range(0, kMaxPlayers - 2)) % kMaxPlayers;
            inputs[caster].ability = (uint8_t)rng.range(1, 3);
            inputs[caster].target = (uint32_t)target;
        }
    }
    return inputs;
}

void apply_tick(MatchState& state, const TickInputs& inputs, AbilityVM& vm) {
    for (int i = 0; i < kMaxPlayers; ++i) {
        PlayerState& p = state.players[i];
        if (!p.alive) continue;
        const PlayerInput& in = inputs[i];
        Fixed step = Fixed::from_raw(Fixed::ONE / 8); // 1/8 unit per tick
        p.x = p.x + step * Fixed::from_int(in.move_x);
        p.y = p.y + step * Fixed::from_int(in.move_y);
    }
    for (int i = 0; i < kMaxPlayers; ++i) {
        const PlayerInput& in = inputs[i];
        if (in.ability == 0) continue;
        if (!state.players[i].alive) continue;
        if (in.target >= kMaxPlayers) continue;
        vm.cast(state, (uint32_t)i, in.target, in.ability, in);
    }
    state.tick += 1;
}
