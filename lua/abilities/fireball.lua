-- Fireball: flat damage to a target, plus fixed-point falloff by distance.
-- All values crossing the C++/Lua boundary are Lua integers that already
-- represent Q16.16 fixed-point (the raw int64), never Lua floats, so the
-- script cannot introduce a non-deterministic float path.
function cast(caster_id, target_id)
  local cx, cy = get_pos(caster_id)
  local tx, ty = get_pos(target_id)
  local dx = tx - cx
  local dy = ty - cy
  local dist2 = fx_mul(dx, dx) + fx_mul(dy, dy)
  local dist = fx_sqrt(dist2)
  local base_damage = 120
  local falloff_range = fx_from_int(20)
  local dmg = base_damage
  if dist > falloff_range then
    dmg = base_damage // 2
  end
  deal_damage(target_id, dmg)
end
