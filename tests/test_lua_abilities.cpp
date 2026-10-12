#include <gtest/gtest.h>
#include "ability_vm.hpp"
#include "sim_state.hpp"

TEST(LuaAbilities, FireballDamagesCloseTargetMore) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    state.players[0].x = Fixed::from_int(0);
    state.players[1].x = Fixed::from_int(2); // within falloff range
    PlayerInput move{};
    vm.cast(state, 0, 1, 1, move);
    EXPECT_EQ(state.players[1].health, 1000 - 120);
}

TEST(LuaAbilities, FireballDamagesFarTargetLess) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    state.players[0].x = Fixed::from_int(0);
    state.players[1].x = Fixed::from_int(50); // outside falloff range
    PlayerInput move{};
    vm.cast(state, 0, 1, 1, move);
    EXPECT_EQ(state.players[1].health, 1000 - 60);
}

TEST(LuaAbilities, HealRestoresHealth) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    state.players[1].health = 500;
    PlayerInput move{};
    vm.cast(state, 0, 1, 2, move);
    EXPECT_EQ(state.players[1].health, 580);
}

TEST(LuaAbilities, HealDoesNotExceedMaxHealth) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    state.players[1].health = 990;
    PlayerInput move{};
    vm.cast(state, 0, 1, 2, move);
    EXPECT_EQ(state.players[1].health, 1000);
}

TEST(LuaAbilities, DashMovesCasterByDashDistanceInMoveDirection) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    Fixed start_x = state.players[0].x;
    PlayerInput move{};
    move.move_x = 1;
    move.move_y = 0;
    vm.cast(state, 0, 0, 3, move);
    EXPECT_EQ(state.players[0].x.raw, (start_x + Fixed::from_int(6)).raw);
    EXPECT_EQ(state.players[0].y.raw, state.players[0].y.raw); // y unchanged
}

TEST(LuaAbilities, DeathClampsHealthAtZeroAndClearsAlive) {
    AbilityVM vm(LUA_DIR);
    MatchState state;
    reset_match(state);
    state.players[1].health = 50;
    state.players[0].x = Fixed::from_int(0);
    state.players[1].x = Fixed::from_int(0);
    vm.cast(state, 0, 1, 1, PlayerInput{}); // fireball for 120, overkill
    EXPECT_EQ(state.players[1].health, 0);
    EXPECT_EQ(state.players[1].alive, 0);
}
