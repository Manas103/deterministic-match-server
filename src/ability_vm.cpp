#include "ability_vm.hpp"
#include "fixed.hpp"
#include <lua.hpp>
#include <stdexcept>

// Single-threaded sim: one AbilityVM is live at a time, so a thread-local
// raw pointer to the state being mutated is safe and avoids plumbing a
// context object through every lua_CFunction trampoline.
namespace {
MatchState* g_state = nullptr;
PlayerInput g_move{};

PlayerState& player(uint32_t id) { return g_state->players[id]; }

int l_get_pos(lua_State* L) {
    uint32_t id = (uint32_t)lua_tointeger(L, 1);
    lua_pushinteger(L, player(id).x.raw);
    lua_pushinteger(L, player(id).y.raw);
    return 2;
}

int l_set_pos(lua_State* L) {
    uint32_t id = (uint32_t)lua_tointeger(L, 1);
    int64_t x = lua_tointeger(L, 2);
    int64_t y = lua_tointeger(L, 3);
    player(id).x = Fixed::from_raw(x);
    player(id).y = Fixed::from_raw(y);
    return 0;
}

int l_get_move_dir(lua_State* L) {
    lua_pushinteger(L, Fixed::from_int(g_move.move_x).raw);
    lua_pushinteger(L, Fixed::from_int(g_move.move_y).raw);
    return 2;
}

int l_deal_damage(lua_State* L) {
    uint32_t id = (uint32_t)lua_tointeger(L, 1);
    int64_t amount = lua_tointeger(L, 2);
    PlayerState& p = player(id);
    p.health -= (int32_t)amount;
    if (p.health <= 0) { p.health = 0; p.alive = 0; }
    if (p.health > 1000) p.health = 1000;
    return 0;
}

int l_fx_mul(lua_State* L) {
    Fixed a = Fixed::from_raw(lua_tointeger(L, 1));
    Fixed b = Fixed::from_raw(lua_tointeger(L, 2));
    lua_pushinteger(L, (a * b).raw);
    return 1;
}

int l_fx_sqrt(lua_State* L) {
    Fixed a = Fixed::from_raw(lua_tointeger(L, 1));
    lua_pushinteger(L, fixed_sqrt(a).raw);
    return 1;
}

int l_fx_from_int(lua_State* L) {
    int64_t v = lua_tointeger(L, 1);
    lua_pushinteger(L, Fixed::from_int(v).raw);
    return 1;
}

const char* ability_file(uint8_t id) {
    switch (id) {
        case 1: return "fireball";
        case 2: return "heal";
        case 3: return "dash";
        default: throw std::runtime_error("unknown ability id");
    }
}
} // namespace

AbilityVM::AbilityVM(const std::string& lua_dir) {
    L_ = luaL_newstate();
    luaL_openlibs(L_);
    lua_register(L_, "get_pos", l_get_pos);
    lua_register(L_, "set_pos", l_set_pos);
    lua_register(L_, "get_move_dir", l_get_move_dir);
    lua_register(L_, "deal_damage", l_deal_damage);
    lua_register(L_, "fx_mul", l_fx_mul);
    lua_register(L_, "fx_sqrt", l_fx_sqrt);
    lua_register(L_, "fx_from_int", l_fx_from_int);

    for (const char* name : {"fireball", "heal", "dash"}) {
        std::string path = lua_dir + "/" + name + ".lua";
        // Each script is loaded with its own fresh environment table so
        // fireball's `cast` global does not clobber heal's; the resulting
        // function is stashed under a per-ability global name for exact
        // dispatch in cast() below.
        std::string boot =
            "local env = setmetatable({}, {__index = _G})\n"
            "local fn = assert(loadfile([[" + path + "]], 't', env))\n"
            "fn()\n"
            "_G['__cast_" + std::string(name) + "'] = env.cast\n";
        if (luaL_dostring(L_, boot.c_str()) != LUA_OK) {
            std::string err = lua_tostring(L_, -1);
            lua_close(L_);
            throw std::runtime_error("lua load failed: " + err);
        }
    }
}

AbilityVM::~AbilityVM() { if (L_) lua_close(L_); }

void AbilityVM::cast(MatchState& state, uint32_t caster, uint32_t target,
                      uint8_t ability_id, const PlayerInput& move) {
    g_state = &state;
    g_move = move;
    std::string fn_name = std::string("__cast_") + ability_file(ability_id);
    lua_getglobal(L_, fn_name.c_str());
    lua_pushinteger(L_, caster);
    lua_pushinteger(L_, target);
    if (lua_pcall(L_, 2, 0, 0) != LUA_OK) {
        std::string err = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        throw std::runtime_error("lua cast failed: " + err);
    }
    g_state = nullptr;
}
