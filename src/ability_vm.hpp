#pragma once
#include <string>
#include <unordered_map>
#include "sim_state.hpp"

struct lua_State;

// Loads the three ability scripts once and runs cast(caster, target) against
// a MatchState. Every value that crosses the Lua boundary is the raw int64
// of a Fixed, passed as a Lua integer (Lua 5.4 has a native integer subtype,
// so there is no float conversion anywhere on this path).
class AbilityVM {
public:
    explicit AbilityVM(const std::string& lua_dir);
    ~AbilityVM();

    // Applies ability `ability_id` (1=fireball, 2=heal, 3=dash) cast by
    // `caster` against `target` on `state`, using `move` for the caster's
    // current-tick move input (dash reads this as "last move direction").
    void cast(MatchState& state, uint32_t caster, uint32_t target,
              uint8_t ability_id, const PlayerInput& move);

private:
    lua_State* L_;
};
