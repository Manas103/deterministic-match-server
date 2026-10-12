#pragma once
#include <array>
#include "ability_vm.hpp"
#include "prng.hpp"
#include "sim_state.hpp"

using TickInputs = std::array<PlayerInput, kMaxPlayers>;

// Generates one tick's worth of synthetic input: every player moves one
// fixed-point step in a seed-derived direction, and every 7th tick one
// living player casts an ability on another living player. Pure function of
// (tick, seed): no wall-clock, no hidden state, which is what lets replay
// reproduce it exactly from the recorded inputs alone.
TickInputs gen_tick_inputs(uint64_t tick, uint64_t seed, const MatchState& state);

// Applies movement then abilities, in player-id order, to `state` in place.
// Deterministic: the only inputs are `state`, `inputs` and the Lua VM, all
// of which are themselves deterministic.
void apply_tick(MatchState& state, const TickInputs& inputs, AbilityVM& vm);
